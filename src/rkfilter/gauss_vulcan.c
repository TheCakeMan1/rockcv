// rkfilter.c
#include "rkfilter.h"
#include <libavutil/hwcontext.h>

// === утилиты ===
uint32_t find_type(VkPhysicalDevice phys, uint32_t bits, VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if (bits & (1u << i))
            return i;
    fprintf(stderr, "mem type not found\n");
    exit(1);
}

void make_gauss(float *w, int r, float s)
{
    float sum = 0.f;
    for (int i = 0; i <= r; i++)
    {
        w[i] = expf(-(i * i) / (2.f * s * s));
        sum += i ? 2.f * w[i] : w[i];
    }
    for (int i = 0; i <= r; i++)
        w[i] /= sum;
}

void init_vk(VkCtx *c)
{
    VkApplicationInfo ai = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "rkcv",
        .apiVersion = VK_API_VERSION_1_1};
    VkInstanceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &ai};
    VK_CHECK(vkCreateInstance(&ci, 0, &c->instance));

    uint32_t dev_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(c->instance, &dev_count, NULL));
    if (!dev_count)
    {
        fprintf(stderr, "No Vulkan devices found\n");
        exit(1);
    }
    VK_CHECK(vkEnumeratePhysicalDevices(c->instance, &dev_count, &c->phys));

    uint32_t qcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(c->phys, &qcount, NULL);
    VkQueueFamilyProperties *qp = calloc(qcount, sizeof(*qp));
    vkGetPhysicalDeviceQueueFamilyProperties(c->phys, &qcount, qp);
    for (uint32_t i = 0; i < qcount; i++)
    {
        if (qp[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            c->qfam = i;
            break;
        }
    }
    free(qp);

    // --- фильтруем расширения ---
    const char *wanted[] = {
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
        VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME};
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(c->phys, NULL, &ext_count, NULL);
    VkExtensionProperties *exts = malloc(sizeof(*exts) * ext_count);
    vkEnumerateDeviceExtensionProperties(c->phys, NULL, &ext_count, exts);

    const char *enabled[8];
    uint32_t enabled_count = 0;
    for (size_t i = 0; i < sizeof(wanted) / sizeof(wanted[0]); i++)
    {
        for (uint32_t j = 0; j < ext_count; j++)
        {
            if (!strcmp(wanted[i], exts[j].extensionName))
            {
                enabled[enabled_count++] = wanted[i];
                break;
            }
        }
    }
    free(exts);

    printf("Enabled Vulkan extensions:\n");
    for (uint32_t i = 0; i < enabled_count; i++)
        printf("  %s\n", enabled[i]);

    float pr = 1.0f;
    VkDeviceQueueCreateInfo qci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = c->qfam,
        .queueCount = 1,
        .pQueuePriorities = &pr};
    VkDeviceCreateInfo di = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qci,
        .enabledExtensionCount = enabled_count,
        .ppEnabledExtensionNames = enabled};

    VK_CHECK(vkCreateDevice(c->phys, &di, 0, &c->device));
    vkGetDeviceQueue(c->device, c->qfam, 0, &c->queue);
}

static VkShaderModule load_spv(VkDevice dev, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror(path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    rewind(f);
    uint32_t *buf = (uint32_t *)malloc(sz);
    fread(buf, 1, sz, f);
    fclose(f);
    VkShaderModuleCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sz, .pCode = buf};
    VkShaderModule m;
    VK_CHECK(vkCreateShaderModule(dev, &ci, 0, &m));
    free(buf);
    return m;
}

void build_pipelines(VkCtx *c, const char *spv_y, const char *spv_uv)
{
    // 1. Загружаем шейдеры
    VkShaderModule mY = load_spv(c->device, spv_y);
    VkShaderModule mUV = load_spv(c->device, spv_uv);

    // 2. Описываем layout для дескрипторов (src и dst image)
    VkDescriptorSetLayoutBinding binds[2] = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL}};
    VkDescriptorSetLayoutCreateInfo dsci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = binds};
    VK_CHECK(vkCreateDescriptorSetLayout(c->device, &dsci, NULL, &c->desc_layout));

    // 3. Push constants (радиус, invW, invH, weights)
    VkPushConstantRange pcr = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(int) + sizeof(float) * (3 + 16)};

    // 4. Pipeline layout
    VkPipelineLayoutCreateInfo lci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &c->desc_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pcr};
    VK_CHECK(vkCreatePipelineLayout(c->device, &lci, NULL, &c->layout));

    // 5. Создаём pipeline для Y
    VkPipelineShaderStageCreateInfo st = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = mY,
        .pName = "main"};
    VkComputePipelineCreateInfo pi = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = st,
        .layout = c->layout};
    printf("device=%p layout=%p module=%p\n", c->device, c->layout, mY);
    fflush(stdout);
    VK_CHECK(vkCreateComputePipelines(c->device, VK_NULL_HANDLE, 1, &pi, NULL, &c->pipeY));

    // 6. Pipeline для UV
    st.module = mUV;
    pi.stage = st;
    VK_CHECK(vkCreateComputePipelines(c->device, VK_NULL_HANDLE, 1, &pi, NULL, &c->pipeUV));

    // 7. Освобождаем модули
    vkDestroyShaderModule(c->device, mY, NULL);
    vkDestroyShaderModule(c->device, mUV, NULL);
}

void destroy_vk(VkCtx *c)
{
    vkDestroyPipeline(c->device, c->pipeY, 0);
    vkDestroyPipeline(c->device, c->pipeUV, 0);
    vkDestroyPipelineLayout(c->device, c->layout, 0);
    vkDestroyDescriptorSetLayout(c->device, c->desc_layout, 0);
    vkDestroyDescriptorPool(c->device, c->dpool, 0);
    vkDestroySampler(c->device, c->sampler, 0);
    vkDestroyCommandPool(c->device, c->pool, 0);
    vkDestroyDevice(c->device, 0);
    vkDestroyInstance(c->instance, 0);
}

// === cmd utils ===
static VkCommandBuffer begin_cmd(VkCtx *c)
{
    VkCommandBufferAllocateInfo ai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = c->pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1};
    VkCommandBuffer cb;
    VK_CHECK(vkAllocateCommandBuffers(c->device, &ai, &cb));
    VkCommandBufferBeginInfo bi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CHECK(vkBeginCommandBuffer(cb, &bi));
    return cb;
}
static void end_submit(VkCtx *c, VkCommandBuffer cb)
{
    VK_CHECK(vkEndCommandBuffer(cb));
    VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb};
    VK_CHECK(vkQueueSubmit(c->queue, 1, &si, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(c->queue));
    vkFreeCommandBuffers(c->device, c->pool, 1, &cb);
}
static void barrier(VkCommandBuffer cb, VkImage img, VkImageLayout oldL, VkImageLayout newL, VkImageAspectFlags aspect)
{
    VkImageMemoryBarrier b = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                              .srcAccessMask = 0,
                              .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                              .oldLayout = oldL,
                              .newLayout = newL,
                              .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                              .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                              .image = img,
                              .subresourceRange = {aspect, 0, 1, 0, 1}};
    vkCmdPipelineBarrier(
        cb,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, NULL,
        0, NULL,
        1, &b);
}

// === NV12 импорт из AVDRMFrameDescriptor ===
static void create_nv12_from_dmabuf(VkCtx *vk, const AVDRMFrameDescriptor *dsc, int w, int h, NV12Image *out)
{
    // предполагаем 1 общий объект (fd) и plane[0]=Y, plane[1]=UV
    int objY = dsc->layers[0].planes[0].object_index;
    int objUV = dsc->layers[0].planes[1].object_index;
    int fd = dsc->objects[objY].fd; // часто одинаковый для UV
    uint64_t mod = dsc->objects[0].format_modifier;

    int strideY = dsc->layers[0].planes[0].pitch;
    int offY = dsc->layers[0].planes[0].offset;
    int strideUV = dsc->layers[0].planes[1].pitch;
    int offUV = dsc->layers[0].planes[1].offset;

    VkSubresourceLayout planes[2] = {0};
    planes[0].offset = offY;
    planes[0].rowPitch = strideY;
    planes[1].offset = offUV;
    planes[1].rowPitch = strideUV;

    VkImageDrmFormatModifierExplicitCreateInfoEXT drmInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,
                                                             .drmFormatModifier = mod,
                                                             .drmFormatModifierPlaneCount = 2,
                                                             .pPlaneLayouts = planes};

    VkExternalMemoryImageCreateInfo extImg = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
                                              .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
    drmInfo.pNext = &extImg;

    VkFormat viewFormats[2] = {VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM};
    VkImageFormatListCreateInfo fmtList = {.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO, .viewFormatCount = 2, .pViewFormats = viewFormats, .pNext = &drmInfo};

    VkImageCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = &fmtList, .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, .extent = {(uint32_t)w, (uint32_t)h, 1}, .mipLevels = 1, .arrayLayers = 1, .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, .sharingMode = VK_SHARING_MODE_EXCLUSIVE, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    VK_CHECK(vkCreateImage(vk->device, &ci, 0, &out->image));

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(vk->device, out->image, &req);

    VkImportMemoryFdInfoKHR imp = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
                                   .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
                                   .fd = fd};

    VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &imp, .allocationSize = req.size, .memoryTypeIndex = find_type(vk->phys, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    VK_CHECK(vkAllocateMemory(vk->device, &mai, 0, &out->memory));
    VK_CHECK(vkBindImageMemory(vk->device, out->image, out->memory, 0));

    VkImageViewCreateInfo vci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = out->image, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = VK_FORMAT_R8_UNORM, .subresourceRange = {VK_IMAGE_ASPECT_PLANE_0_BIT, 0, 1, 0, 1}};
    VK_CHECK(vkCreateImageView(vk->device, &vci, 0, &out->viewY));
    vci.format = VK_FORMAT_R8G8_UNORM;
    vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    VK_CHECK(vkCreateImageView(vk->device, &vci, 0, &out->viewUV));

    out->width = w;
    out->height = h;
}

static void destroy_nv12(VkCtx *vk, NV12Image *img)
{
    vkDestroyImageView(vk->device, img->viewY, 0);
    vkDestroyImageView(vk->device, img->viewUV, 0);
    vkDestroyImage(vk->device, img->image, 0);
    vkFreeMemory(vk->device, img->memory, 0);
}

// scratch (промежуточные картинки)
static void make_image(VkCtx *vk, VkFormat fmt, uint32_t w, uint32_t h, VkImage *img, VkDeviceMemory *mem, VkImageView *view)
{
    VkImageCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .imageType = VK_IMAGE_TYPE_2D, .format = fmt, .extent = {w, h, 1}, .mipLevels = 1, .arrayLayers = 1, .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL, .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, .sharingMode = VK_SHARING_MODE_EXCLUSIVE, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    VK_CHECK(vkCreateImage(vk->device, &ci, 0, img));
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(vk->device, *img, &req);
    VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = req.size, .memoryTypeIndex = find_type(vk->phys, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    VK_CHECK(vkAllocateMemory(vk->device, &mai, 0, mem));
    VK_CHECK(vkBindImageMemory(vk->device, *img, *mem, 0));
    VkImageViewCreateInfo vci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = *img, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = fmt, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VK_CHECK(vkCreateImageView(vk->device, &vci, 0, view));
}
static void create_scratch(VkCtx *vk, int w, int h, Scratch *s)
{
    make_image(vk, VK_FORMAT_R8_UNORM, w, h, &s->yA, &s->mY[0], &s->yViewA);
    make_image(vk, VK_FORMAT_R8_UNORM, w, h, &s->yB, &s->mY[1], &s->yViewB);
    make_image(vk, VK_FORMAT_R8G8_UNORM, w, h / 2, &s->uvA, &s->mUV[0], &s->uvViewA);
    make_image(vk, VK_FORMAT_R8G8_UNORM, w, h / 2, &s->uvB, &s->mUV[1], &s->uvViewB);
}
static void destroy_scratch(VkCtx *vk, Scratch *s)
{
    vkDestroyImageView(vk->device, s->yViewA, 0);
    vkDestroyImageView(vk->device, s->yViewB, 0);
    vkDestroyImage(vk->device, s->yA, 0);
    vkDestroyImage(vk->device, s->yB, 0);
    vkFreeMemory(vk->device, s->mY[0], 0);
    vkFreeMemory(vk->device, s->mY[1], 0);

    vkDestroyImageView(vk->device, s->uvViewA, 0);
    vkDestroyImageView(vk->device, s->uvViewB, 0);
    vkDestroyImage(vk->device, s->uvA, 0);
    vkDestroyImage(vk->device, s->uvB, 0);
    vkFreeMemory(vk->device, s->mUV[0], 0);
    vkFreeMemory(vk->device, s->mUV[1], 0);
}

// === один проход (любой: горизонт/вертикаль) ===
typedef struct
{
    int radius;
    float invW;
    float invH;
    float dir[2];
    float weights[32];
} PCData;

static void dispatch_pass(VkCtx *vk, VkPipeline pipe,
                          VkImageView src, VkImageView dst,
                          uint32_t w, uint32_t h,
                          const PCData *pc)
{
    VkDescriptorImageInfo samp = {.sampler = vk->sampler, .imageView = src, .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
    VkDescriptorImageInfo outi = {.sampler = VK_NULL_HANDLE, .imageView = dst, .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
    VkWriteDescriptorSet wr[2] = {0};
    wr[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wr[0].dstSet = vk->dset;
    wr[0].dstBinding = 0;
    wr[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    wr[0].descriptorCount = 1;
    wr[0].pImageInfo = &samp;
    wr[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wr[1].dstSet = vk->dset;
    wr[1].dstBinding = 1;
    wr[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    wr[1].descriptorCount = 1;
    wr[1].pImageInfo = &outi;
    vkUpdateDescriptorSets(vk->device, 2, wr, 0, 0);

    VkCommandBuffer cb = begin_cmd(vk);
    // макеты (layout) GENERAL для простоты
    barrier(cb, VK_NULL_HANDLE /*ignored in barrier below*/, 0, 0, 0); // заглушка чтобы линтер не ругался

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, vk->layout, 0, 1, &vk->dset, 0, 0);
    vkCmdPushConstants(cb, vk->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PCData), pc);

    uint32_t gx = (w + 15) / 16, gy = (h + 15) / 16;
    vkCmdDispatch(cb, gx, gy, 1);

    end_submit(vk, cb);
}

// === главный API: вход/выход AVFrame (NV12/DRM) ===
void nv12_gaussian_blur_frames(VkCtx *vk, AVFrame *in, AVFrame *out, int radius, float sigma)
{
    out->width = in->width;
    out->height = in->height;
    out->format = AV_PIX_FMT_DRM_PRIME; // формат обязателен
    out->pts = in->pts;
    out->time_base = in->time_base;

    AVBufferRef *hw_dev = NULL;

    if (av_hwdevice_ctx_create(&hw_dev, AV_HWDEVICE_TYPE_DRM, NULL, NULL, 0) < 0)
    {
        log_fatal("av_hwdevice_ctx_create(DRM)");
        return;
    }

    out->hw_frames_ctx = av_hwframe_ctx_alloc(hw_dev);
    if (!out->hw_frames_ctx)
    {
        log_fatal("av_hwframe_ctx_alloc failed");
        av_buffer_unref(&hw_dev);
        return;
    }

    AVHWFramesContext *frames_ctx = (AVHWFramesContext *)out->hw_frames_ctx->data;
    frames_ctx->format = AV_PIX_FMT_DRM_PRIME;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width = out->width;
    frames_ctx->height = out->height;

    if (av_hwframe_ctx_init(out->hw_frames_ctx) < 0)
    {
        log_fatal("av_hwframe_ctx_init failed");
        av_buffer_unref(&out->hw_frames_ctx);
        av_buffer_unref(&hw_dev);
        return;
    }
    const AVDRMFrameDescriptor *din = (const AVDRMFrameDescriptor *)in->data[0];
    const AVDRMFrameDescriptor *dout = (const AVDRMFrameDescriptor *)out->data[0];
    if (!din || !dout)
    {
        fprintf(stderr, "no DRM frames\n");
        return;
    }

    int w = in->width, h = in->height;

    NV12Image inImg, outImg;
    create_nv12_from_dmabuf(vk, din, w, h, &inImg);
    create_nv12_from_dmabuf(vk, dout, w, h, &outImg);

    Scratch sc;
    create_scratch(vk, w, h, &sc);

    // Перевод в GENERAL
    VkCommandBuffer cb = begin_cmd(vk);
    barrier(cb, inImg.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(cb, outImg.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(cb, sc.yA, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(cb, sc.yB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(cb, sc.uvA, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    barrier(cb, sc.uvB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    end_submit(vk, cb);

    // коэффициенты
    float wts[32];
    if (radius > 31)
        radius = 31;
    make_gauss(wts, radius, sigma);

    // Y: горизонт -> вертикаль
    PCData pcYh = {.radius = radius, .invW = 1.f / w, .invH = 1.f / h, .dir = {1.f, 0.f}};
    memcpy(pcYh.weights, wts, sizeof(float) * (radius + 1));
    PCData pcYv = pcYh;
    pcYv.dir[0] = 0.f;
    pcYv.dir[1] = 1.f;

    dispatch_pass(vk, vk->pipeY, inImg.viewY, sc.yViewA, w, h, &pcYh);
    dispatch_pass(vk, vk->pipeY, sc.yViewA, outImg.viewY, w, h, &pcYv);

    // UV: размер h/2
    int hv = h / 2;
    PCData pcUVh = {.radius = radius, .invW = 1.f / w, .invH = 1.f / hv, .dir = {1.f, 0.f}};
    memcpy(pcUVh.weights, wts, sizeof(float) * (radius + 1));
    PCData pcUVv = pcUVh;
    pcUVv.dir[0] = 0.f;
    pcUVv.dir[1] = 1.f;

    dispatch_pass(vk, vk->pipeUV, inImg.viewUV, sc.uvViewA, w, hv, &pcUVh);
    dispatch_pass(vk, vk->pipeUV, sc.uvViewA, outImg.viewUV, w, hv, &pcUVv);

    // уборка
    destroy_scratch(vk, &sc);
    destroy_nv12(vk, &inImg);
    destroy_nv12(vk, &outImg);
}
