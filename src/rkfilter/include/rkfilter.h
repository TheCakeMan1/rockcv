// rkfilter.h
#ifndef _RKFILTER_H
#define _RKFILTER_H

#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include <gbm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static struct gbm_device *gbm = NULL;
static EGLDisplay egl_dpy;
static EGLContext egl_ctx;
static EGLSurface egl_surf;

static void init_egl_gbm()
{
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0)
    {
        perror("drm open");
        exit(1);
    }

    gbm = gbm_create_device(drm_fd);

    egl_dpy = eglGetDisplay((EGLNativeDisplayType)gbm);
    if (egl_dpy == EGL_NO_DISPLAY)
    {
        printf("EGL no display\n");
        exit(1);
    }

    eglInitialize(egl_dpy, NULL, NULL);

    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint cfg_attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_NONE};

    EGLConfig cfg;
    EGLint ncfg;
    eglChooseConfig(egl_dpy, cfg_attribs, &cfg, 1, &ncfg);

    EGLint pb_attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    egl_surf = eglCreatePbufferSurface(egl_dpy, cfg, pb_attribs);

    EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE};

    egl_ctx = eglCreateContext(egl_dpy, cfg, EGL_NO_CONTEXT, ctx_attribs);
    eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx);

    printf("EGL OK\n");
}

static GLuint import_dmabuf_to_texture(
    int fd, int width, int height, int stride, int fourcc)
{
    EGLint attr[] = {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_LINUX_DRM_FOURCC_EXT, fourcc,

        EGL_DMA_BUF_PLANE0_FD_EXT, fd,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, stride,

        EGL_NONE};

    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR =
        (void *)eglGetProcAddress("eglCreateImageKHR");

    EGLImageKHR img = eglCreateImageKHR(
        egl_dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);

    if (img == EGL_NO_IMAGE_KHR)
    {
        printf("EGLImage import failed\n");
        exit(1);
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
        (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, img);

    return tex;
}

static struct gbm_bo *create_output_bo(int w, int h, int *out_fd)
{
    struct gbm_bo *bo = gbm_bo_create(
        gbm, w, h, DRM_FORMAT_ARGB8888,
        GBM_BO_USE_LINEAR | GBM_BO_USE_RENDERING);

    if (!bo)
    {
        printf("gbm_bo_create failed\n");
        exit(1);
    }

    *out_fd = gbm_bo_get_fd(bo);
    return bo;
}

static GLuint import_output_image(int fd, int w, int h)
{
    EGLint attr[] = {
        EGL_WIDTH, w,
        EGL_HEIGHT, h,
        EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_ARGB8888,
        EGL_DMA_BUF_PLANE0_FD_EXT, fd,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, w * 4,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
        EGL_NONE};

    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR =
        (void *)eglGetProcAddress("eglCreateImageKHR");

    EGLImageKHR img = eglCreateImageKHR(
        egl_dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);

    GLuint out_tex;
    glGenTextures(1, &out_tex);

    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
        (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");

    glBindImageTexture(1, out_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    return out_tex;
}

const char *gauss_cs =
    "#version 310 es\n"
    "layout(local_size_x=16, local_size_y=16) in;\n"
    "layout(binding=0) uniform sampler2D inputTex;\n"
    "layout(binding=1, rgba8) writeonly uniform highp image2D outImg;\n"
    "void main() {\n"
    "    ivec2 p = ivec2(gl_GlobalInvocationID.xy);\n"
    "    vec3 s = vec3(0.0);\n"
    "    float k[3][3] = float[3][3](\n"
    "        float[3](1,2,1),\n"
    "        float[3](2,4,2),\n"
    "        float[3](1,2,1)\n"
    "    );\n"
    "    float norm = 1.0/16.0;\n"
    "    for(int y=-1;y<=1;y++)\n"
    "    for(int x=-1;x<=1;x++)\n"
    "        s += k[y+1][x+1]*texture(inputTex,p+ivec2(x,y)).rgb;\n"
    "    s*=norm;\n"
    "    imageStore(outImg,p,vec4(s,1.0));\n"
    "}\n";

static GLuint compile_compute(const char *src)
{
    GLuint s = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2000];
        glGetShaderInfoLog(s, 2000, NULL, log);
        printf("shader error:\n%s\n", log);
        exit(1);
    }

    GLuint p = glCreateProgram();
    glAttachShader(p, s);
    glLinkProgram(p);
    glDeleteShader(s);
    return p;
}

void run_gauss(GLuint prog, int w, int h)
{
    glUseProgram(prog);
    glDispatchCompute((w + 15) / 16, (h + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

static int run_gauss_proc(int input_fd, int input_stride, int w, int h, int out_fd)
{
    init_egl_gbm();

    int input_fourcc = DRM_FORMAT_NV12;

    // Импорт входного
    GLuint input_tex = import_dmabuf_to_texture(
        input_fd, w, h, input_stride, input_fourcc);

    struct gbm_bo *out_bo = create_output_bo(w, h, &out_fd);

    GLuint out_tex = import_output_image(out_fd, w, h);

    GLuint prog = compile_compute(gauss_cs);

    run_gauss(prog, w, h);

    // out_fd — обработанный кадр, отправляешь обратно в FFmpeg
    printf("Output FD: %d ready\n", out_fd);

    return 0;
}

#endif

// // #include <vulkan/vulkan.h>
// #include <libavutil/frame.h>
// #include <libavutil/hwcontext_drm.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>
// #include <unistd.h>
// #include "rkutils.h"
// #include "rkframe.h"
// // #include "rkfilter_kernel.h"
// // #include <arm_neon.h>
// // #include "rkfilter_kernel.h"

// int gauss(rkcv_shot_t *restrict shot, int sigma, int sizes);
// typedef struct vk_ctx_s
// {
//     // VkInstance instance;
//     // VkPhysicalDevice phys;
//     // uint32_t computeQF;
//     // VkDevice device;
//     // VkQueue queue;

//     // VkCommandPool cmdPool;

//     // VkDescriptorSetLayout dsl;
//     // VkPipelineLayout ppl;
//     // VkPipeline pipeline;
//     // VkDescriptorPool dpool;

//     int ready;
// } vk_ctx_t;
// typedef struct Nv12Image
// {
//     VkImage image;
//     VkDeviceMemory mem;   // не используется для DISJOINT, оставлен для совместимости
//     VkDeviceMemory memY;  // новая
//     VkDeviceMemory memUV; // новая
//     VkImageView yView;
//     VkImageView uvView;
//     int w, h;
// } Nv12Image;

// int gauss_v(rkcv_shot_t *restrict shot, int sigma, int size);

// // #define VK_CHECK(x)                                    \
// //     do                                                 \
// //     {                                                  \
// //         VkResult err = (x);                            \
// //         if (err != VK_SUCCESS)                         \
// //         {                                              \
// //             fprintf(stderr, "Vulkan error %d\n", err); \
// //             exit(1);                                   \
// //         }                                              \
// //     } while (0)

// // typedef struct
// // {
// //     VkInstance instance;
// //     VkPhysicalDevice phys;
// //     VkDevice device;
// //     uint32_t qfam;
// //     VkQueue queue;
// //     VkCommandPool pool;
// //     VkSampler sampler;

// //     // Layouts и pipeline
// //     VkDescriptorSetLayout desc_layout;
// //     VkPipelineLayout layout;
// //     VkPipeline pipeY;
// //     VkPipeline pipeUV;

// //     // Descriptor pool + set
// //     VkDescriptorPool dpool;
// //     VkDescriptorSet dset;
// // } VkCtx;

// // typedef struct
// // {
// //     VkImage image;
// //     VkDeviceMemory memory;
// //     VkImageView viewY;
// //     VkImageView viewUV;
// //     int width, height;
// // } NV12Image;

// // typedef struct
// // {
// //     VkImage yA, yB;
// //     VkDeviceMemory mY[2];
// //     VkImageView yViewA, yViewB;
// //     VkImage uvA, uvB;
// //     VkDeviceMemory mUV[2];
// //     VkImageView uvViewA, uvViewB;
// // } Scratch;

// // // утилиты
// // uint32_t find_type(VkPhysicalDevice phys, uint32_t bits, VkMemoryPropertyFlags props);
// // void make_gauss(float *w, int r, float s);

// // // инициализация/уничтожение
// // void init_vk(VkCtx *c);
// // void build_pipelines(VkCtx *c, const char *spv_y, const char *spv_uv);
// // void destroy_vk(VkCtx *c);

// // // основное
// // void nv12_gaussian_blur_frames(VkCtx *vk, AVFrame *in, AVFrame *out, int radius, float sigma);
