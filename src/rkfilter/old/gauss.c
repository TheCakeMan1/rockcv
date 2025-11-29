#include "rkfilter.h"

int gauss(rkcv_shot_t *restrict shot, int sigma, int sizes)
{
    float **kernel = make_gaussian_kernel(RK_GAUSS_5x5, sigma);
    if (!shot || !shot->frame)
        return -1;

    const AVDRMFrameDescriptor *desc = (const AVDRMFrameDescriptor *)shot->frame->data[0];
    if (!desc)
        return -1;

    int fd = desc->objects[0].fd;
    if (fd < 0)
        return -1;

    int width = shot->frame->width;
    int height = shot->frame->height;
    int stride = desc->layers[0].planes[0].pitch;
    size_t size = stride * height;

    uint8_t *map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        perror("mmap failed");
        return -1;
    }

    uint8_t *tmp = aligned_alloc(16, size);
    if (!tmp)
    {
        munmap(map, size);
        return -1;
    }

    // коэффициенты ядра в виде NEON-векторов (сумма весов = 16)
    const uint8x16_t k1 = vdupq_n_u8(1);
    const uint8x16_t k2 = vdupq_n_u8(2);
    const uint8x16_t k4 = vdupq_n_u8(4);

    for (int y = 1; y < height - 1; y++)
    {
        uint8_t *dst = tmp + y * stride;
        for (int x = 1; x < width - 16; x += 16)
        {
            // загружаем три строки по 16 байт каждая
            uint8x16_t r0_prev = vld1q_u8(&map[(y - 1) * stride + (x - 1)]);
            uint8x16_t r0_curr = vld1q_u8(&map[(y - 1) * stride + x]);
            uint8x16_t r0_next = vld1q_u8(&map[(y - 1) * stride + (x + 1)]);

            uint8x16_t r1_prev = vld1q_u8(&map[y * stride + (x - 1)]);
            uint8x16_t r1_curr = vld1q_u8(&map[y * stride + x]);
            uint8x16_t r1_next = vld1q_u8(&map[y * stride + (x + 1)]);

            uint8x16_t r2_prev = vld1q_u8(&map[(y + 1) * stride + (x - 1)]);
            uint8x16_t r2_curr = vld1q_u8(&map[(y + 1) * stride + x]);
            uint8x16_t r2_next = vld1q_u8(&map[(y + 1) * stride + (x + 1)]);

            // расширяем до 16-бит
            uint16x8_t s0_lo = vmovl_u8(vget_low_u8(r0_prev));
            uint16x8_t s0_mi = vmovl_u8(vget_low_u8(r0_curr));
            uint16x8_t s0_hi = vmovl_u8(vget_low_u8(r0_next));

            uint16x8_t s1_lo = vmovl_u8(vget_low_u8(r1_prev));
            uint16x8_t s1_mi = vmovl_u8(vget_low_u8(r1_curr));
            uint16x8_t s1_hi = vmovl_u8(vget_low_u8(r1_next));

            uint16x8_t s2_lo = vmovl_u8(vget_low_u8(r2_prev));
            uint16x8_t s2_mi = vmovl_u8(vget_low_u8(r2_curr));
            uint16x8_t s2_hi = vmovl_u8(vget_low_u8(r2_next));

            // применяем веса для первой половины (8 пикселей)
            uint16x8_t sum_lo =
                vmulq_n_u16(s0_lo, 1) + vmulq_n_u16(s0_mi, 2) + vmulq_n_u16(s0_hi, 1) +
                vmulq_n_u16(s1_lo, 2) + vmulq_n_u16(s1_mi, 4) + vmulq_n_u16(s1_hi, 2) +
                vmulq_n_u16(s2_lo, 1) + vmulq_n_u16(s2_mi, 2) + vmulq_n_u16(s2_hi, 1);

            // то же для второй половины (старшие 8)
            s0_lo = vmovl_u8(vget_high_u8(r0_prev));
            s0_mi = vmovl_u8(vget_high_u8(r0_curr));
            s0_hi = vmovl_u8(vget_high_u8(r0_next));

            s1_lo = vmovl_u8(vget_high_u8(r1_prev));
            s1_mi = vmovl_u8(vget_high_u8(r1_curr));
            s1_hi = vmovl_u8(vget_high_u8(r1_next));

            s2_lo = vmovl_u8(vget_high_u8(r2_prev));
            s2_mi = vmovl_u8(vget_high_u8(r2_curr));
            s2_hi = vmovl_u8(vget_high_u8(r2_next));

            uint16x8_t sum_hi =
                vmulq_n_u16(s0_lo, 1) + vmulq_n_u16(s0_mi, 2) + vmulq_n_u16(s0_hi, 1) +
                vmulq_n_u16(s1_lo, 2) + vmulq_n_u16(s1_mi, 4) + vmulq_n_u16(s1_hi, 2) +
                vmulq_n_u16(s2_lo, 1) + vmulq_n_u16(s2_mi, 2) + vmulq_n_u16(s2_hi, 1);

            // делим на 16 (сдвиг вправо на 4)
            sum_lo = vshrq_n_u16(sum_lo, 4);
            sum_hi = vshrq_n_u16(sum_hi, 4);

            // объединяем обратно в 8-бит
            uint8x16_t result = vcombine_u8(vqmovn_u16(sum_lo), vqmovn_u16(sum_hi));

            vst1q_u8(&dst[x], result);
        }
    }

    memcpy(map, tmp, size);
    free(tmp);
    munmap(map, size);

    return 0;
}

// int gauss_v(rkcv_shot_t *restrict shot, int sigma, int size)
// {
//     if (!shot || !shot->frame)
//         return -1;
//     const AVDRMFrameDescriptor *desc = (const AVDRMFrameDescriptor *)shot->frame->data[0];
//     if (!desc)
//         return -1;

//     int fd = desc->objects[0].fd;
//     if (fd < 0)
//         return -1;

//     int width = shot->frame->width;
//     int height = shot->frame->height;

//     // --- 1. Vulkan instance + device (1 раз за весь процесс) ---
//     static VkInstance instance = VK_NULL_HANDLE;
//     static VkDevice device = VK_NULL_HANDLE;
//     static VkQueue queue;
//     static VkCommandPool pool;
//     static VkPipeline pipeline;
//     static VkPipelineLayout layout;
//     static VkDescriptorSetLayout descLayout;
//     static VkDescriptorPool descPool;
//     static VkDescriptorSet descSet;

//     if (instance == VK_NULL_HANDLE)
//     {
//         VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
//                                  .pApplicationName = "gauss",
//                                  .apiVersion = VK_API_VERSION_1_1};
//         VkInstanceCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
//                                    .pApplicationInfo = &app};
//         vkCreateInstance(&ci, 0, &instance);
//         // выбрать физическое устройство, создать логическое (опущено для краткости)
//         // создать compute queue, command pool и pipeline (см. ниже)
//     }

//     // --- 2. Импорт fd как VkImage ---
//     VkExternalMemoryImageCreateInfo extImg = {
//         .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
//         .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};

//     VkImageCreateInfo imgInfo = {
//         .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
//         .pNext = &extImg,
//         .imageType = VK_IMAGE_TYPE_2D,
//         .format = VK_FORMAT_R8_UNORM,
//         .extent = {width, height, 1},
//         .mipLevels = 1,
//         .arrayLayers = 1,
//         .samples = VK_SAMPLE_COUNT_1_BIT,
//         .tiling = VK_IMAGE_TILING_LINEAR,
//         .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT};

//     VkImage vkImage;
//     vkCreateImage(device, &imgInfo, NULL, &vkImage);

//     VkMemoryRequirements req;
//     vkGetImageMemoryRequirements(device, vkImage, &req);

//     VkImportMemoryFdInfoKHR importInfo = {
//         .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
//         .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
//         .fd = fd};

//     VkMemoryAllocateInfo allocInfo = {
//         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
//         .pNext = &importInfo,
//         .allocationSize = req.size,
//         .memoryTypeIndex = 0}; // подбери через vkGetPhysicalDeviceMemoryProperties

//     VkDeviceMemory memory;
//     vkAllocateMemory(device, &allocInfo, NULL, &memory);
//     vkBindImageMemory(device, vkImage, memory, 0);

//     // --- 3. Создание image view ---
//     VkImageViewCreateInfo viewInfo = {
//         .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
//         .image = vkImage,
//         .viewType = VK_IMAGE_VIEW_TYPE_2D,
//         .format = VK_FORMAT_R8_UNORM,
//         .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//         .subresourceRange.levelCount = 1,
//         .subresourceRange.layerCount = 1};
//     VkImageView view;
//     vkCreateImageView(device, &viewInfo, NULL, &view);

//     // --- 4. Дескрипторы ---
//     // (inY = view, outY = view, можно одно и то же, если in-place)
//     // пропускаю код инициализации VkDescriptorSet и VkPipeline
//     // см. https://github.com/SaschaWillems/Vulkan/tree/master/examples/computeshader

//     // --- 5. Команда dispatch ---
//     VkCommandBuffer cmd;
//     VkCommandBufferAllocateInfo a = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//                                      .commandPool = pool,
//                                      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//                                      .commandBufferCount = 1};
//     vkAllocateCommandBuffers(device, &a, &cmd);

//     VkCommandBufferBeginInfo bi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
//     vkBeginCommandBuffer(cmd, &bi);

//     vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
//     vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &descSet, 0, NULL);

//     vkCmdDispatch(cmd, (width + 15) / 16, (height + 15) / 16, 1);

//     vkEndCommandBuffer(cmd);

//     VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//                        .commandBufferCount = 1,
//                        .pCommandBuffers = &cmd};
//     vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
//     vkQueueWaitIdle(queue);

//     // результат уже в том же fd (in-place)

//     vkDestroyImageView(device, view, NULL);
//     vkDestroyImage(device, vkImage, NULL);
//     vkFreeMemory(device, memory, NULL);

//     return 0;
// }
