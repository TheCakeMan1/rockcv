#include "rkfilter.h"

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

// ВМЕСТО твоей make_avframe_from_fd_nv12_dupfd(...)
static AVFrame *make_avframe_from_fd_nv12_dupfd(const Nv12Layout *ly,
                                                int orig_fd,
                                                size_t object_size,
                                                uint64_t modifier)
{
    AVFrame *frame = av_frame_alloc();
    if (!frame)
        return NULL;

    AVDRMFrameDescriptor *d = av_mallocz(sizeof(*d));
    d->nb_objects = 1;
    d->objects[0].fd = dup(orig_fd);          // FFmpeg сам закроет
    d->objects[0].size = object_size;         // ВАЖНО: точный size из FFmpeg
    d->objects[0].format_modifier = modifier; // сохраняем модификатор (0 для LINEAR)

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

// ВМЕСТО твоей import_dmabuf_to_cl(...)
typedef cl_mem(CL_API_CALL *PFN_clImportMemoryARM)(
    cl_context context, cl_mem_flags flags,
    const cl_import_properties_arm *properties,
    void *memory, size_t size, cl_int *errcode_ret);

#ifndef CL_IMPORT_TYPE_ARM
#define CL_IMPORT_TYPE_ARM 0x40B2
#endif
#ifndef CL_IMPORT_TYPE_DMA_BUF_ARM
#define CL_IMPORT_TYPE_DMA_BUF_ARM 0x40B6
#endif

static cl_mem import_dmabuf_to_cl(OclEnv *oe, int dma_fd, size_t exact_size, cl_int *out_err)
{
    cl_int err = CL_INVALID_VALUE;

#ifdef CL_IMPORT_MEMORY_ARM
    if (oe->has_arm_import)
    {
        PFN_clImportMemoryARM pImport = (PFN_clImportMemoryARM)
            clGetExtensionFunctionAddressForPlatform(oe->platform, "clImportMemoryARM");
        if (!pImport)
        {
            if (out_err)
                *out_err = CL_INVALID_OPERATION;
            fprintf(stderr, "clImportMemoryARM not found\n");
            return NULL;
        }

        // dup(fd): чтобы жизненный цикл корректно управлялся
        int fd2 = dup(dma_fd);
        if (fd2 < 0)
        {
            if (out_err)
                *out_err = CL_INVALID_VALUE;
            perror("dup(fd)");
            return NULL;
        }

        const cl_import_properties_arm props[] = {
            CL_IMPORT_TYPE_ARM, CL_IMPORT_TYPE_DMA_BUF_ARM,
            0};

        cl_mem m = pImport(oe->context, CL_MEM_READ_WRITE, props,
                           (void *)(uintptr_t)fd2, exact_size, &err);
        if (out_err)
            *out_err = err;
        fprintf(stderr, "clImportMemoryARM(fd=%d,size=%zu)->%d\n", fd2, exact_size, err);

        if (err != CL_SUCCESS)
        {
            // если драйвер не взял владение — закрываем наш dup
            close(fd2);
            return NULL;
        }
        return m;
    }
#endif

    // У тебя khr_extmem=0, так что сюда не зайдём; оставляю на всякий случай.
#ifdef CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR
#ifndef CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR
#define CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR 0x2067
#endif
    if (oe->has_extmem_core && oe->has_extmem_dma_buf)
    {
        const cl_mem_properties props[] = {
            CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR, (cl_mem_properties)dma_fd,
            0};
        cl_mem m = clCreateBufferWithProperties(oe->context, props,
                                                CL_MEM_READ_WRITE, exact_size, NULL, &err);
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

// ВМЕСТО твоей process_drmprime_frame_with_opencl(...)
AVFrame *process_drmprime_frame_with_opencl(AVFrame *in_frame)
{
    CHECK_FF(in_frame && in_frame->format == AV_PIX_FMT_DRM_PRIME, "Input not DRM_PRIME");
    AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)in_frame->data[0];
    CHECK_FF(desc && desc->nb_objects >= 1, "Invalid AVDRMFrameDescriptor");

    Nv12Layout ly;
    int r = compute_layout_from_desc(desc, in_frame->width, in_frame->height, &ly);
    CHECK_FF(r == 0, "Unsupported layout (expect NV12 2-plane in one object)");

    const int fd = desc->objects[0].fd;
    size_t obj_size = (size_t)desc->objects[0].size;            // ВАЖНО
    const uint64_t modifier = desc->objects[0].format_modifier; // может быть 0 (LINEAR) или AFBC

    if (obj_size == 0)
        obj_size = (size_t)ly.total_size; // fallback, если драйвер не заполнил
    void *p = mmap(NULL, obj_size, PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED)
        perror("mmap");
    else
        munmap(p, obj_size);

    fprintf(stderr, "modifier=0x%llx\n",
            (unsigned long long)desc->objects[0].format_modifier);

    if (modifier != 0)
    {
        fprintf(stderr, "WARNING: format_modifier=0x%llx (не LINEAR). ARM import может не принять.\n",
                (unsigned long long)modifier);
    }

    // Инициализируем OpenCL
    OclEnv oe;
    ocl_init(&oe);

    // Импортируем dma-buf с ТОЧНЫМ size (функция внутри сама делает dup(fd))
    cl_int err = 0;
    cl_mem ext_mem = import_dmabuf_to_cl(&oe, fd, obj_size, &err);
    CHECK_CL(err, "import_dmabuf_to_cl");
    CHECK_FF(ext_mem != NULL, "Failed to import dma-buf");

    // Работаем по Y-плоскости через sub-buffer
    cl_buffer_region reg = {.origin = (size_t)ly.y_offset, .size = (size_t)ly.y_size};
    cl_mem y_sub = clCreateSubBuffer(ext_mem, CL_MEM_READ_WRITE,
                                     CL_BUFFER_CREATE_TYPE_REGION, &reg, &err);
    CHECK_CL(err, "clCreateSubBuffer(Y)");

    run_kernel_brighten(&oe, y_sub, (size_t)ly.y_size);
    clReleaseMemObject(y_sub);
    clReleaseMemObject(ext_mem);
    clFinish(oe.queue);

#if CL_TARGET_OPENCL_VERSION >= 200
    clReleaseCommandQueue(oe.queue);
#else
    clReleaseCommandQueue(oe.queue);
#endif
    clReleaseContext(oe.context);

    // Собираем новый AVFrame: тот же fd, но с точным size и исходным modifier
    AVFrame *out = make_avframe_from_fd_nv12_dupfd(&ly, fd, obj_size, modifier);
    CHECK_FF(out != NULL, "make_avframe_from_fd failed");
    return out;
}