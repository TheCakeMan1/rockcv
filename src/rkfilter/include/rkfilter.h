// rkfilter.h
#ifndef _RKFILTER_H
#define _RKFILTER_H

#include <vulkan/vulkan.h>
#include <libavutil/frame.h>
#include <libavutil/hwcontext_drm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "logs.h"

#define VK_CHECK(x)                                    \
    do                                                 \
    {                                                  \
        VkResult err = (x);                            \
        if (err != VK_SUCCESS)                         \
        {                                              \
            fprintf(stderr, "Vulkan error %d\n", err); \
            exit(1);                                   \
        }                                              \
    } while (0)

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice phys;
    VkDevice device;
    uint32_t qfam;
    VkQueue queue;
    VkCommandPool pool;
    VkSampler sampler;

    // Layouts и pipeline
    VkDescriptorSetLayout desc_layout;
    VkPipelineLayout layout;
    VkPipeline pipeY;
    VkPipeline pipeUV;

    // Descriptor pool + set
    VkDescriptorPool dpool;
    VkDescriptorSet dset;
} VkCtx;

typedef struct
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView viewY;
    VkImageView viewUV;
    int width, height;
} NV12Image;

typedef struct
{
    VkImage yA, yB;
    VkDeviceMemory mY[2];
    VkImageView yViewA, yViewB;
    VkImage uvA, uvB;
    VkDeviceMemory mUV[2];
    VkImageView uvViewA, uvViewB;
} Scratch;

// утилиты
uint32_t find_type(VkPhysicalDevice phys, uint32_t bits, VkMemoryPropertyFlags props);
void make_gauss(float *w, int r, float s);

// инициализация/уничтожение
void init_vk(VkCtx *c);
void build_pipelines(VkCtx *c, const char *spv_y, const char *spv_uv);
void destroy_vk(VkCtx *c);

// основное
void nv12_gaussian_blur_frames(VkCtx *vk, AVFrame *in, AVFrame *out, int radius, float sigma);

#endif
