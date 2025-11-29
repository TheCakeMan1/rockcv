// rkfilter.c
#include "rkfilter.h"
#include <libavutil/hwcontext.h>

static vk_ctx_t G = {0};

// ---- утилиты ----
static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    abort();
}

static uint32_t read_file(const char *path, uint8_t **data)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        fprintf(stderr, "open %s: %s\n", path, strerror(errno));
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0)
    {
        fclose(f);
        return 0;
    }
    *data = (uint8_t *)malloc(sz);
    if (!*data)
    {
        fclose(f);
        return 0;
    }
    if (fread(*data, 1, sz, f) != (size_t)sz)
    {
        fclose(f);
        free(*data);
        *data = NULL;
        return 0;
    }
    fclose(f);
    return (uint32_t)sz;
}

static VkShaderModule load_spirv(VkDevice dev, const char *path)
{
    uint8_t *bin = NULL;
    uint32_t sz = read_file(path, &bin);
    if (!sz)
        die("Failed to read SPIR-V");
    VkShaderModuleCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sz,
        .pCode = (const uint32_t *)bin};
    VkShaderModule mod;
    VkResult r = vkCreateShaderModule(dev, &ci, NULL, &mod);
    free(bin);
    if (r)
        die("vkCreateShaderModule failed");
    return mod;
}

static uint32_t pick_compute_queue(VkPhysicalDevice pd)
{
    uint32_t n = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &n, NULL);
    VkQueueFamilyProperties *qfp = (VkQueueFamilyProperties *)calloc(n, sizeof(*qfp));
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &n, qfp);
    for (uint32_t i = 0; i < n; i++)
    {
        if (qfp[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            free(qfp);
            return i;
        }
    }
    free(qfp);
    return UINT32_MAX;
}

static int has_ext(const char *const *list, uint32_t count, const char *name)
{
    for (uint32_t i = 0; i < count; i++)
        if (strcmp(list[i], name) == 0)
            return 1;
    return 0;
}

static void vk_init_once(void)
{
    if (G.ready)
        return;

    // 1) Instance
    const char *inst_exts[] = {
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_external_memory_capabilities"};
    VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "gauss",
        .apiVersion = VK_API_VERSION_1_1};
    VkInstanceCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledExtensionCount = (uint32_t)(sizeof(inst_exts) / sizeof(inst_exts[0])),
        .ppEnabledExtensionNames = inst_exts};
    if (vkCreateInstance(&ici, NULL, &G.instance) != VK_SUCCESS)
        die("vkCreateInstance");

    // 2) Physical device
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(G.instance, &gpuCount, NULL);
    if (!gpuCount)
        die("No physical device");
    printf("Found %u Vulkan devices\n", gpuCount);

    VkPhysicalDevice pds[16];
    if (gpuCount > 16)
        gpuCount = 16;
    vkEnumeratePhysicalDevices(G.instance, &gpuCount, pds);

    for (uint32_t i = 0; i < gpuCount; i++)
    {
        uint32_t qf = pick_compute_queue(pds[i]);
        if (qf == UINT32_MAX)
            continue;

        uint32_t ec = 0;
        vkEnumerateDeviceExtensionProperties(pds[i], NULL, &ec, NULL);
        VkExtensionProperties exts[128];
        if (ec > 128)
            ec = 128;
        vkEnumerateDeviceExtensionProperties(pds[i], NULL, &ec, exts);

        int have_ext_mem = 0, have_ext_mem_fd = 0;
        for (uint32_t j = 0; j < ec; j++)
        {
            if (!strcmp(exts[j].extensionName, "VK_KHR_external_memory"))
                have_ext_mem = 1;
            if (!strcmp(exts[j].extensionName, "VK_KHR_external_memory_fd"))
                have_ext_mem_fd = 1;
        }
        if (!(have_ext_mem && have_ext_mem_fd))
            continue;

        G.phys = pds[i];
        G.computeQF = qf;
        break;
    }
    if (G.phys == VK_NULL_HANDLE)
        die("No suitable GPU");

    // 3) Device + queue
    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = G.computeQF,
        .queueCount = 1,
        .pQueuePriorities = &prio};
    const char *dev_exts[] = {
        "VK_KHR_external_memory",
        "VK_KHR_external_memory_fd"
        // "VK_KHR_sampler_ycbcr_conversion" // не обязательно для storage image
    };
    VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qci,
        .enabledExtensionCount = (uint32_t)(sizeof(dev_exts) / sizeof(dev_exts[0])),
        .ppEnabledExtensionNames = dev_exts};
    if (vkCreateDevice(G.phys, &dci, NULL, &G.device) != VK_SUCCESS)
        die("vkCreateDevice");
    vkGetDeviceQueue(G.device, G.computeQF, 0, &G.queue);

    // 4) Cmd pool
    VkCommandPoolCreateInfo cpci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = G.computeQF,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
    if (vkCreateCommandPool(G.device, &cpci, NULL, &G.cmdPool) != VK_SUCCESS)
        die("cmdPool");

    // 5) Descriptor set layout (Y in/out, UV in/out)
    VkDescriptorSetLayoutBinding b[4] = {0};
    for (int i = 0; i < 4; i++)
    {
        b[i].binding = i;
        b[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        b[i].descriptorCount = 1;
        b[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo dslci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 4,
        .pBindings = b};
    if (vkCreateDescriptorSetLayout(G.device, &dslci, NULL, &G.dsl) != VK_SUCCESS)
        die("dsl");

    // 6) Pipeline layout
    VkPipelineLayoutCreateInfo plci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &G.dsl};
    if (vkCreatePipelineLayout(G.device, &plci, NULL, &G.ppl) != VK_SUCCESS)
        die("ppl");

    // 7) Compute pipeline
    VkShaderModule sm = load_spirv(G.device, "gauss3x3_nv12.spv");
    VkPipelineShaderStageCreateInfo sst = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = sm,
        .pName = "main"};
    VkComputePipelineCreateInfo pi = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = sst,
        .layout = G.ppl};
    if (vkCreateComputePipelines(G.device, VK_NULL_HANDLE, 1, &pi, NULL, &G.pipeline) != VK_SUCCESS)
        die("pipeline");
    vkDestroyShaderModule(G.device, sm, NULL);

    // 8) Descriptor pool
    VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 * 16};
    VkDescriptorPoolCreateInfo dpci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 16,
        .poolSizeCount = 1,
        .pPoolSizes = &ps};
    if (vkCreateDescriptorPool(G.device, &dpci, NULL, &G.dpool) != VK_SUCCESS)
        die("dpool");

    G.ready = 1;
}

static uint32_t find_mem_type(uint32_t typeBits)
{
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(G.phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if (typeBits & (1u << i))
            return i;
    return 0;
}

static void cmd_barrier_image(VkCommandBuffer cmd, VkImage img, VkImageAspectFlags aspect,
                              VkImageLayout oldL, VkImageLayout newL,
                              VkAccessFlags src, VkAccessFlags dst,
                              VkPipelineStageFlags sst, VkPipelineStageFlags dstst)
{
    VkImageMemoryBarrier b = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = src,
        .dstAccessMask = dst,
        .oldLayout = oldL,
        .newLayout = newL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = img,
        .subresourceRange.aspectMask = aspect,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1};
    vkCmdPipelineBarrier(cmd, sst, dstst, 0, 0, NULL, 0, NULL, 1, &b);
}

static Nv12Image make_nv12_from_dmabuf(const AVDRMFrameDescriptor *desc, int width, int height)
{
    Nv12Image out = (Nv12Image){0};
    const int obj = 0;
    int fd_main = desc->objects[obj].fd;
    if (fd_main < 0)
        die("bad fd in AVDRMFrameDescriptor");

    const AVDRMPlaneDescriptor *pY = &desc->layers[0].planes[0];
    const AVDRMPlaneDescriptor *pUV = &desc->layers[0].planes[1];

    // 1) Создаём DISJOINT линейный мультипланарный image
    VkExternalMemoryImageCreateInfo extImg = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
    VkImageCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = &extImg,
        .flags = VK_IMAGE_CREATE_DISJOINT_BIT, // важно!
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, // NV12
        .extent = {(uint32_t)width, (uint32_t)height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR, // без модификаторов
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT};
    if (vkCreateImage(G.device, &ici, NULL, &out.image) != VK_SUCCESS)
        die("vkCreateImage NV12");

    // 2) MR для каждой плоскости
    VkImagePlaneMemoryRequirementsInfo pmriY = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO,
        .planeAspect = VK_IMAGE_ASPECT_PLANE_0_BIT};
    VkImagePlaneMemoryRequirementsInfo pmriUV = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO,
        .planeAspect = VK_IMAGE_ASPECT_PLANE_1_BIT};
    VkImageMemoryRequirementsInfo2 imri = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
        .image = out.image};
    VkMemoryRequirements2 mr2Y = {.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    VkMemoryRequirements2 mr2UV = {.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};

    imri.pNext = &pmriY;
    vkGetImageMemoryRequirements2(G.device, &imri, &mr2Y);
    imri.pNext = &pmriUV;
    vkGetImageMemoryRequirements2(G.device, &imri, &mr2UV);

    // 3) Импорт памяти для каждой плоскости (один и тот же fd, но импортирован дважды; оффсеты при bind)
    int fdY = dup(fd_main);
    if (fdY < 0)
        die("dup fdY");
    int fdUV = dup(fd_main);
    if (fdUV < 0)
        die("dup fdUV");

    VkImportMemoryFdInfoKHR importY = {
        .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
        .fd = fdY};

    VkMemoryAllocateInfo maiY = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &importY,
        .allocationSize = mr2Y.memoryRequirements.size + pY->offset, // учитываем смещение
        .memoryTypeIndex = find_mem_type(mr2Y.memoryRequirements.memoryTypeBits)};

    VkDeviceMemory memY = VK_NULL_HANDLE, memUV = VK_NULL_HANDLE;
    if (vkAllocateMemory(G.device, &maiY, NULL, &memY) != VK_SUCCESS)
        die("alloc memY");

    // 4) Привязка плоскостей с разными memoryOffset
    VkBindImagePlaneMemoryInfo bindPlaneY = {
        .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO,
        .planeAspect = VK_IMAGE_ASPECT_PLANE_0_BIT};

    VkBindImageMemoryInfo binds[2] = {0};
    binds[0].sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
    binds[0].pNext = &bindPlaneY;
    binds[0].image = out.image;
    binds[0].memory = memY;
    binds[0].memoryOffset = pY->offset; // ключ!

    if (vkBindImageMemory2(G.device, 2, binds) != VK_SUCCESS)
        die("vkBindImageMemory2");

    out.mem = VK_NULL_HANDLE; // не используется как единая память
    out.w = width;
    out.h = height;

    // 5) Plane views
    VkImageViewCreateInfo vci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = out.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM};
    vci.subresourceRange = (VkImageSubresourceRange){
        .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT, .levelCount = 1, .layerCount = 1};
    if (vkCreateImageView(G.device, &vci, NULL, &out.yView) != VK_SUCCESS)
        die("yView");
    vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    if (vkCreateImageView(G.device, &vci, NULL, &out.uvView) != VK_SUCCESS)
        die("uvView");

    // сохраним обе памяти в структуре, чтобы корректно освободить
    out.memY = memY;
    out.memUV = memUV;
    return out;
}

// создаём временное NV12-изображение под результат (обычная память устройства)
static Nv12Image make_scratch_nv12(int width, int height)
{
    Nv12Image out = {0};
    VkImageCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        .extent = {(uint32_t)width, (uint32_t)height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT};
    if (vkCreateImage(G.device, &ici, NULL, &out.image) != VK_SUCCESS)
        die("scratch image");

    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(G.device, out.image, &mr);
    VkMemoryAllocateInfo mai = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mr.size,
        .memoryTypeIndex = find_mem_type(mr.memoryTypeBits)};
    if (vkAllocateMemory(G.device, &mai, NULL, &out.mem) != VK_SUCCESS)
        die("scratch mem");
    if (vkBindImageMemory(G.device, out.image, out.mem, 0) != VK_SUCCESS)
        die("scratch bind");

    VkImageViewCreateInfo vci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = out.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM};
    vci.subresourceRange = (VkImageSubresourceRange){
        .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT, .levelCount = 1, .layerCount = 1};
    if (vkCreateImageView(G.device, &vci, NULL, &out.yView) != VK_SUCCESS)
        die("scratch yView");
    vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    if (vkCreateImageView(G.device, &vci, NULL, &out.uvView) != VK_SUCCESS)
        die("scratch uvView");

    out.w = width;
    out.h = height;
    return out;
}

static void write_desc(VkDescriptorSet ds,
                       VkImageView inY, VkImageView outY,
                       VkImageView inUV, VkImageView outUV)
{
    VkDescriptorImageInfo yi = {.imageView = inY, .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
    VkDescriptorImageInfo yo = {.imageView = outY, .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
    VkDescriptorImageInfo ui = {.imageView = inUV, .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
    VkDescriptorImageInfo uo = {.imageView = outUV, .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

    VkWriteDescriptorSet w[4];
    memset(w, 0, sizeof(w));
    for (int i = 0; i < 4; i++)
    {
        w[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w[i].dstSet = ds;
        w[i].descriptorCount = 1;
        w[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }
    w[0].dstBinding = 0;
    w[0].pImageInfo = &yi;
    w[1].dstBinding = 1;
    w[1].pImageInfo = &yo;
    w[2].dstBinding = 2;
    w[2].pImageInfo = &ui;
    w[3].dstBinding = 3;
    w[3].pImageInfo = &uo;

    vkUpdateDescriptorSets(G.device, 4, w, 0, NULL);
}

static void destroy_nv12(Nv12Image *im)
{
    if (!im || !im->image)
        return;
    vkDestroyImageView(G.device, im->yView, NULL);
    vkDestroyImageView(G.device, im->uvView, NULL);
    vkDestroyImage(G.device, im->image, NULL);
    if (im->memY)
        vkFreeMemory(G.device, im->memY, NULL);
    if (im->memUV)
        vkFreeMemory(G.device, im->memUV, NULL);
    memset(im, 0, sizeof(*im));
}
// ---- ПУБЛИЧНАЯ ФУНКЦИЯ ----
// Применяет 3×3 Гаусс по NV12; сейчас по умолчанию фильтруется только Y (яркость).
// Для UV включи define BLUR_UV в шейдере (см. .comp), либо оставь passthrough.
int gauss_v(rkcv_shot_t *restrict shot, int sigma, int size)
{
    (void)sigma;
    (void)size; // ядро фиксированное 3x3 в шейдере
    if (!shot || !shot->frame)
        return -1;
    const AVDRMFrameDescriptor *desc = (const AVDRMFrameDescriptor *)shot->frame->data[0];
    if (!desc)
        return -1;

    int width = shot->frame->width;
    int height = shot->frame->height;

    vk_init_once();

    // 1) Создаём импортированное NV12-изображение из fd
    Nv12Image in = make_nv12_from_dmabuf(desc, width, height);

    // 2) Временное NV12 для результата
    Nv12Image out = make_scratch_nv12(width, height);

    // 3) Выделим дескрипторный набор
    VkDescriptorSetAllocateInfo dsai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = G.dpool,
        .descriptorSetCount = 1,
        .pSetLayouts = &G.dsl};
    VkDescriptorSet ds;
    if (vkAllocateDescriptorSets(G.device, &dsai, &ds) != VK_SUCCESS)
        die("alloc DS");
    write_desc(ds, in.yView, out.yView, in.uvView, out.uvView);

    // 4) Командный буфер
    VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = G.cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1};
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(G.device, &cbai, &cmd);

    VkCommandBufferBeginInfo bi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd, &bi);

    // 5) Layouts -> GENERAL
    cmd_barrier_image(cmd, in.image, VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT,
                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                      0, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    cmd_barrier_image(cmd, out.image, VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT,
                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                      0, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    // 6) Dispatch compute
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, G.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, G.ppl, 0, 1, &ds, 0, NULL);

    // Для Y: полный размер; для UV: внутри шейдера берём половинный
    uint32_t gx = (width + 15) / 16;
    uint32_t gy = (height + 15) / 16;
    vkCmdDispatch(cmd, gx, gy, 1);

    // 7) Барьер перед копированием результатов в импортированный image
    cmd_barrier_image(cmd, out.image,
                      VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT,
                      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    cmd_barrier_image(cmd, in.image,
                      VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT,
                      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 8) Копируем PLANE0 (Y) и PLANE1 (UV)
    VkImageCopy regions[2];
    memset(regions, 0, sizeof(regions));
    regions[0].srcSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
    regions[0].srcSubresource.layerCount = 1;
    regions[0].dstSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
    regions[0].dstSubresource.layerCount = 1;
    regions[0].extent.width = width;
    regions[0].extent.height = height;
    regions[0].extent.depth = 1;

    regions[1].srcSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    regions[1].srcSubresource.layerCount = 1;
    regions[1].dstSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    regions[1].dstSubresource.layerCount = 1;
    regions[1].extent.width = width;
    regions[1].extent.height = height / 2; // для NV12 в Vulkan копируем по «высоте субресурса»

    vkCmdCopyImage(cmd,
                   out.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   in.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   2, regions);

    // 9) Возвращаем импортированный image в GENERAL (по желанию)
    cmd_barrier_image(cmd, in.image,
                      VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                      VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                       .commandBufferCount = 1,
                       .pCommandBuffers = &cmd};
    vkQueueSubmit(G.queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(G.queue);

    // 10) Cleanup per-call
    vkFreeCommandBuffers(G.device, G.cmdPool, 1, &cmd);
    vkDestroyDescriptorPool(G.device, G.dpool, NULL); // освободим и создадим заново — проще
    // пересоздадим dpool для следующего кадра
    VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 * 16};
    VkDescriptorPoolCreateInfo dpci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                       .maxSets = 16,
                                       .poolSizeCount = 1,
                                       .pPoolSizes = &ps};
    if (vkCreateDescriptorPool(G.device, &dpci, NULL, &G.dpool) != VK_SUCCESS)
        die("dpool re");

    destroy_nv12(&out);
    destroy_nv12(&in);

    return 0;
}
