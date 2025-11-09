
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <CL/cl.h>
#include <CL/cl_ext.h> // для cl_arm_import_memory и external memory KHR

#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
#include <libavutil/pixfmt.h>
#include <libavutil/buffer.h>

// -------------------- Утилиты --------------------
#define CHECK_CL(err, msg)                                                                       \
    do                                                                                           \
    {                                                                                            \
        if ((err) != CL_SUCCESS)                                                                 \
        {                                                                                        \
            fprintf(stderr, "OpenCL error %d at %s:%d: %s\n", (err), __FILE__, __LINE__, (msg)); \
            exit(1);                                                                             \
        }                                                                                        \
    } while (0)

#define CHECK_FF(cond, msg)                                                            \
    do                                                                                 \
    {                                                                                  \
        if (!(cond))                                                                   \
        {                                                                              \
            fprintf(stderr, "FFmpeg error at %s:%d: %s\n", __FILE__, __LINE__, (msg)); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

static bool str_has_token(const char *s, const char *token)
{
    if (!s || !token)
        return false;
    const char *p = s;
    size_t n = strlen(token);
    while ((p = strstr(p, token)))
    {
        // разделители — пробелы/концы строк
        bool left = (p == s) || (p[-1] == ' ' || p[-1] == '\t' || p[-1] == '\n');
        bool right = (p[n] == 0) || (p[n] == ' ' || p[n] == '\t' || p[n] == '\n');
        if (left && right)
            return true;
        p += n;
    }
    return false;
}

// -------------------- OpenCL Kernel (простой пример) --------------------
// Небольшой кернел, который увеличивает яркость Y-плоскости NV12 на +10 (с насыщением).
static const char *KERNEL_SRC =
    "__kernel void brighten_y(__global uchar* yplane, int y_size) {\n"
    "  int gid = get_global_id(0);\n"
    "  if (gid < y_size) {\n"
    "    uint v = (uint)yplane[gid] + 10u;\n"
    "    yplane[gid] = (uchar)(v > 255u ? 255u : v);\n"
    "  }\n"
    "}\n";

// -------------------- Поиск устройства Mali и создание контекста --------------------
typedef struct
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    int has_arm_import;
    int has_extmem_dma_buf;
    int has_extmem_core;
    int has_extmem_acqrel;
} OclEnv;

static void ocl_init(OclEnv *oe)
{
    memset(oe, 0, sizeof(*oe));
    cl_int err;

    cl_uint num_platforms = 0;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    CHECK_CL(err, "clGetPlatformIDs count");
    CHECK_FF(num_platforms > 0, "No OpenCL platforms");

    cl_platform_id *plats = (cl_platform_id *)calloc(num_platforms, sizeof(*plats));
    err = clGetPlatformIDs(num_platforms, plats, NULL);
    CHECK_CL(err, "clGetPlatformIDs list");

    // Выбираем платформу/устройство: предпочтительно ARM/Mali.
    for (cl_uint i = 0; i < num_platforms && !oe->device; ++i)
    {
        cl_platform_id pid = plats[i];
        size_t sz = 0;
        clGetPlatformInfo(pid, CL_PLATFORM_NAME, 0, NULL, &sz);
        char *pname = (char *)calloc(sz + 1, 1);
        clGetPlatformInfo(pid, CL_PLATFORM_NAME, sz, pname, NULL);

        cl_uint nd = 0;
        clGetDeviceIDs(pid, CL_DEVICE_TYPE_GPU, 0, NULL, &nd);
        if (nd == 0)
        {
            free(pname);
            continue;
        }

        cl_device_id *devs = (cl_device_id *)calloc(nd, sizeof(*devs));
        clGetDeviceIDs(pid, CL_DEVICE_TYPE_GPU, nd, devs, NULL);

        for (cl_uint d = 0; d < nd && !oe->device; ++d)
        {
            cl_device_id did = devs[d];
            size_t sz2 = 0;
            clGetDeviceInfo(did, CL_DEVICE_NAME, 0, NULL, &sz2);
            char *dname = (char *)calloc(sz2 + 1, 1);
            clGetDeviceInfo(did, CL_DEVICE_NAME, sz2, dname, NULL);
            size_t sz3 = 0;
            clGetDeviceInfo(did, CL_DEVICE_EXTENSIONS, 0, NULL, &sz3);
            char *exts = (char *)calloc(sz3 + 1, 1);
            clGetDeviceInfo(did, CL_DEVICE_EXTENSIONS, sz3, exts, NULL);

            // Предпочтение Mali, но если нет — берём первый GPU с нужными расширениями
            int pick = 0;
            if (strstr(dname, "Mali"))
                pick = 1;
            if (!pick)
            {
                // если не Mali — тоже можно, но главное расширения
                if (str_has_token(exts, "cl_arm_import_memory") ||
                    str_has_token(exts, "cl_khr_external_memory_dma_buf"))
                {
                    pick = 1;
                }
            }

            if (pick)
            {
                oe->platform = pid;
                oe->device = did;

                oe->has_arm_import = str_has_token(exts, "cl_arm_import_memory");
                oe->has_extmem_core = str_has_token(exts, "cl_khr_external_memory");
                oe->has_extmem_dma_buf = str_has_token(exts, "cl_khr_external_memory_dma_buf");
                // для acquire/release:
                oe->has_extmem_acqrel = (str_has_token(exts, "cl_khr_external_memory") &&
                                         str_has_token(exts, "cl_khr_external_memory_dma_buf"));

                free(dname);
                free(exts);
                break;
            }
            free(dname);
            free(exts);
        }
        free(devs);
        free(pname);
    }
    free(plats);

    CHECK_FF(oe->device != NULL, "No suitable OpenCL GPU device");

    cl_context_properties cps[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)oe->platform, 0};
    oe->context = clCreateContext(cps, 1, &oe->device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
#if CL_TARGET_OPENCL_VERSION >= 200
    oe->queue = clCreateCommandQueueWithProperties(oe->context, oe->device, NULL, &err);
#else
    oe->queue = clCreateCommandQueue(oe->context, oe->device, 0, &err);
#endif
    CHECK_CL(err, "clCreateCommandQueue");

    fprintf(stderr, "OpenCL: arm_import=%d, khr_extmem=%d, khr_dma_buf=%d\n",
            oe->has_arm_import, oe->has_extmem_core, oe->has_extmem_dma_buf);
}

// -------------------- Работа с AVDRMFrameDescriptor --------------------
typedef struct
{
    int width, height;
    int fourcc; // DRM_FORMAT_*
    int y_offset, y_pitch, y_size;
    int uv_offset, uv_pitch, uv_size; // для NV12
    int total_size;                   // максимальный охват по всем плоскостям
    int fd;
} Nv12Layout;

static int compute_layout_from_desc(const AVDRMFrameDescriptor *desc, int width, int height, Nv12Layout *out)
{
    if (!desc || desc->nb_objects < 1 || desc->nb_layers < 1)
        return -1;
    // Предположим NV12: один объект (objects[0]), 2 плоскости в слое 0
    const AVDRMLayerDescriptor *L = &desc->layers[0];
    if (L->nb_planes < 1)
        return -2;

    out->width = width;
    out->height = height;
    out->fourcc = L->format; // ожидаем DRM_FORMAT_NV12

    // Plane 0: Y
    out->y_offset = L->planes[0].offset;
    out->y_pitch = L->planes[0].pitch;
    out->y_size = out->y_pitch * height;

    // Plane 1: UV (если есть)
    if (L->nb_planes > 1)
    {
        out->uv_offset = L->planes[1].offset;
        out->uv_pitch = L->planes[1].pitch;
        out->uv_size = out->uv_pitch * (height / 2);
    }
    else
    {
        out->uv_offset = out->uv_pitch = out->uv_size = 0;
    }

    // total size: берём максимум охваченного диапазона
    int end_y = out->y_offset + out->y_size;
    int end_uv = out->uv_offset + out->uv_size;
    out->total_size = (end_y > end_uv ? end_y : end_uv);
    // fd:
    out->fd = desc->objects[0].fd;

    return 0;
}

// Создать AVFrame с DRM_PRIME, указывая существующий fd и layout (NV12)
static AVFrame *make_avframe_from_fd_nv12_dupfd(const Nv12Layout *ly, int orig_fd)
{
    AVFrame *frame = av_frame_alloc();
    if (!frame)
        return NULL;

    AVDRMFrameDescriptor *d = av_mallocz(sizeof(*d));
    d->nb_objects = 1;
    d->objects[0].fd = dup(orig_fd); // FFmpeg освободит сам
    d->objects[0].format_modifier = 0;
    d->objects[0].size = ly->total_size; // общий охват
    d->nb_layers = 1;
    d->layers[0].format = ly->fourcc;
    d->layers[0].nb_planes = 2;

    d->layers[0].planes[0].object_index = 0;
    d->layers[0].planes[0].offset = ly->y_offset;
    d->layers[0].planes[0].pitch = ly->y_pitch;

    d->layers[0].planes[1].object_index = 0;
    d->layers[0].planes[1].offset = ly->uv_offset;
    d->layers[0].planes[1].pitch = ly->uv_pitch;

    frame->format = AV_PIX_FMT_DRM_PRIME;
    frame->width = ly->width;
    frame->height = ly->height;

    frame->data[0] = (uint8_t *)d;
    frame->buf[0] = av_buffer_create((uint8_t *)d, sizeof(*d),
                                     (void (*)(void *, uint8_t *))av_free, NULL, 0);
    return frame;
}

// -------------------- Импорт dma-buf в OpenCL --------------------
typedef cl_mem(CL_API_CALL *PFN_clImportMemoryARM)(
    cl_context context, cl_mem_flags flags,
    const cl_import_properties_arm *properties,
    void *memory, size_t size, cl_int *errcode_ret);

static cl_mem import_dmabuf_to_cl(OclEnv *oe, int dma_fd, size_t size, cl_int *out_err)
{
    cl_int err = CL_INVALID_VALUE;

    // Вариант 1: ARM extension
#ifdef CL_IMPORT_MEMORY_ARM
    if (oe->has_arm_import)
    {
        PFN_clImportMemoryARM pImport = (PFN_clImportMemoryARM)
            clGetExtensionFunctionAddressForPlatform(oe->platform, "clImportMemoryARM");
        if (pImport)
        {
            // свойства импорта DMA-BUF (ARM)
#ifndef CL_IMPORT_TYPE_ARM
#define CL_IMPORT_TYPE_ARM 0x40B2
#endif
#ifndef CL_IMPORT_TYPE_DMA_BUF_ARM
#define CL_IMPORT_TYPE_DMA_BUF_ARM 0x40B6
#endif
            cl_import_properties_arm props[] = {
                CL_IMPORT_TYPE_ARM, CL_IMPORT_TYPE_DMA_BUF_ARM,
                0};
            cl_mem m = pImport(oe->context, CL_MEM_READ_WRITE, props,
                               (void *)(uintptr_t)dma_fd, size, &err);
            if (out_err)
                *out_err = err;
            if (err == CL_SUCCESS)
                return m;
        }
    }
#endif

    // Вариант 2: KHR external memory dma-buf
#ifdef CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR
    if (oe->has_extmem_core && oe->has_extmem_dma_buf)
    {
        // clCreateBufferWithProperties + свойство handle
#ifndef CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR
#define CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR 0x2067
#endif
        const cl_mem_properties props[] = {
            CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR, (cl_mem_properties)dma_fd,
            0};
        cl_mem m = clCreateBufferWithProperties(oe->context, props,
                                                CL_MEM_READ_WRITE, size, NULL, &err);
        if (out_err)
            *out_err = err;
        if (err == CL_SUCCESS)
            return m;
    }
#endif

    if (out_err)
        *out_err = err;
    return NULL;
}

// -------------------- Выполнение кернела над Y-плоскостью --------------------
static void run_kernel_brighten(OclEnv *oe, cl_mem buf, size_t y_size)
{
    cl_int err;
    cl_program prog = clCreateProgramWithSource(oe->context, 1, &KERNEL_SRC, NULL, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    err = clBuildProgram(prog, 1, &oe->device, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        // напечатаем лог компиляции
        size_t logsz = 0;
        clGetProgramBuildInfo(prog, oe->device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logsz);
        char *log = (char *)calloc(logsz + 1, 1);
        clGetProgramBuildInfo(prog, oe->device, CL_PROGRAM_BUILD_LOG, logsz, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
    }
    CHECK_CL(err, "clBuildProgram");

    cl_kernel krn = clCreateKernel(prog, "brighten_y", &err);
    CHECK_CL(err, "clCreateKernel");

    err = clSetKernelArg(krn, 0, sizeof(cl_mem), &buf);
    CHECK_CL(err, "set arg0");
    err = clSetKernelArg(krn, 1, sizeof(cl_int), &y_size);
    CHECK_CL(err, "set arg1");

    size_t gsz = y_size;
    err = clEnqueueNDRangeKernel(oe->queue, krn, 1, NULL, &gsz, NULL, 0, NULL, NULL);
    CHECK_CL(err, "enqueue kernel");
    clFinish(oe->queue);

    clReleaseKernel(krn);
    clReleaseProgram(prog);
}

// -------------------- Основной пример потока --------------------
// В реальном коде вы получите AVFrame* frame из декодера (DRM_PRIME).
// Здесь — каркас функции, куда вы передаёте готовый frame и получаете новый AVFrame* с тем же fd.
AVFrame *process_drmprime_frame_with_opencl(AVFrame *in_frame)
{
    CHECK_FF(in_frame && in_frame->format == AV_PIX_FMT_DRM_PRIME, "Input not DRM_PRIME");
    AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)in_frame->data[0];
    CHECK_FF(desc && desc->nb_objects >= 1, "Invalid AVDRMFrameDescriptor");

    // Размер/раскладка NV12
    Nv12Layout ly;
    int r = compute_layout_from_desc(desc, in_frame->width, in_frame->height, &ly);
    CHECK_FF(r == 0, "Unsupported layout (expect NV12 2-plane in one object)");
    int fd = desc->objects[0].fd;

    // Инициализируем OpenCL
    OclEnv oe;
    ocl_init(&oe);

    // Импортируем dma-buf как cl_mem (используем весь диапазон — Y+UV)
    cl_int err = 0;
    cl_mem ext_mem = import_dmabuf_to_cl(&oe, fd, (size_t)ly.total_size, &err);
    CHECK_CL(err, "import_dmabuf_to_cl");
    CHECK_FF(ext_mem != NULL, "Failed to import dma-buf");

    // Если поддерживается KHR acquire/release external memory — корректно захватываем
#ifdef CL_VERSION_1_2
    // KHR API может быть доступен как функции с суффиксом KHR
    cl_int(CL_API_CALL * pAcquireKHR)(cl_command_queue, cl_uint, const cl_mem *, cl_uint, const cl_event *, cl_event *) =
        (void *)clGetExtensionFunctionAddressForPlatform(oe.platform, "clEnqueueAcquireExternalMemObjectsKHR");
    cl_int(CL_API_CALL * pReleaseKHR)(cl_command_queue, cl_uint, const cl_mem *, cl_uint, const cl_event *, cl_event *) =
        (void *)clGetExtensionFunctionAddressForPlatform(oe.platform, "clEnqueueReleaseExternalMemObjectsKHR");
    if (oe.has_extmem_acqrel && pAcquireKHR && pReleaseKHR)
    {
        CHECK_CL(pAcquireKHR(oe.queue, 1, &ext_mem, 0, NULL, NULL), "acquire extmem");
        // запустим кернел только по Y-плоскости: передаём базу + смещение
        // Для простоты создадим alias-буфер на поддиапазон Y (через sub-buffer)
#ifdef CL_VERSION_1_1
        cl_buffer_region reg = {.origin = (size_t)ly.y_offset, .size = (size_t)ly.y_size};
        cl_mem y_sub = clCreateSubBuffer(ext_mem, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION, &reg, &err);
        CHECK_CL(err, "clCreateSubBuffer(Y)");
        run_kernel_brighten(&oe, y_sub, (size_t)ly.y_size);
        clReleaseMemObject(y_sub);
#else
        // Если нет sub-buffer — можно передать смещение в кернел и обрабатывать через yplane[gid + y_offset]
        run_kernel_brighten(&oe, ext_mem /* в кернеле учитывайте смещение */, (size_t)(ly.y_size)); // упрощённо
#endif
        CHECK_CL(pReleaseKHR(oe.queue, 1, &ext_mem, 0, NULL, NULL), "release extmem");
    }
    else
    {
        // ARM import обычно не требует явного acquire/release в OpenCL.
#ifdef CL_VERSION_1_1
        cl_buffer_region reg = {.origin = (size_t)ly.y_offset, .size = (size_t)ly.y_size};
        cl_mem y_sub = clCreateSubBuffer(ext_mem, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION, &reg, &err);
        CHECK_CL(err, "clCreateSubBuffer(Y)");
        run_kernel_brighten(&oe, y_sub, (size_t)ly.y_size);
        clReleaseMemObject(y_sub);
#else
        run_kernel_brighten(&oe, ext_mem, (size_t)(ly.y_size)); // смещение учитывайте в кернеле, если нужно
#endif
    }
#endif

    clReleaseMemObject(ext_mem);
    clFinish(oe.queue);

#if CL_TARGET_OPENCL_VERSION >= 200
    clReleaseCommandQueue(oe.queue);
#else
    clReleaseCommandQueue(oe.queue);
#endif
    clReleaseContext(oe.context);

    // Теперь собираем новый AVFrame, указывая ТЕ ЖЕ fd и раскладку.
    AVFrame *out = make_avframe_from_fd_nv12_dupfd(&ly, fd);
    CHECK_FF(out != NULL, "make_avframe_from_fd failed");
    return out;
}