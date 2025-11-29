#include <EGL/egl.h>
#include <GLES3/gl32.h>

#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define CHECK_EGL(step)                                        \
    do                                                         \
    {                                                          \
        EGLint err = eglGetError();                            \
        if (err != EGL_SUCCESS)                                \
        {                                                      \
            printf("EGL error after %s: 0x%04x\n", step, err); \
            std::exit(1);                                      \
        }                                                      \
    } while (0)

#define CHECK_GL(step)                                        \
    do                                                        \
    {                                                         \
        GLenum err = glGetError();                            \
        if (err != GL_NO_ERROR)                               \
        {                                                     \
            printf("GL error after %s: 0x%04x\n", step, err); \
            std::exit(1);                                     \
        }                                                     \
    } while (0)

struct EGLState
{
    EGLDisplay dpy = EGL_NO_DISPLAY;
    EGLContext ctx = EGL_NO_CONTEXT;
    EGLSurface surf = EGL_NO_SURFACE;
};

EGLState init_egl()
{
    EGLState st{};

    st.dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (st.dpy == EGL_NO_DISPLAY)
    {
        printf("eglGetDisplay failed\n");
        std::exit(1);
    }
    eglInitialize(st.dpy, nullptr, nullptr);
    CHECK_EGL("eglInitialize");

    EGLint cfg_attrs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE};

    EGLConfig cfg;
    EGLint ncfg = 0;
    if (!eglChooseConfig(st.dpy, cfg_attrs, &cfg, 1, &ncfg) || ncfg == 0)
    {
        printf("eglChooseConfig failed\n");
        CHECK_EGL("eglChooseConfig");
    }

    EGLint pb_attr[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE};
    st.surf = eglCreatePbufferSurface(st.dpy, cfg, pb_attr);
    if (st.surf == EGL_NO_SURFACE)
    {
        printf("eglCreatePbufferSurface failed\n");
        CHECK_EGL("eglCreatePbufferSurface");
    }

    EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3, // ES 3.x (compute = 3.1+)
        EGL_NONE};

    eglBindAPI(EGL_OPENGL_ES_API);
    CHECK_EGL("eglBindAPI");

    st.ctx = eglCreateContext(st.dpy, cfg, EGL_NO_CONTEXT, ctx_attrs);
    if (st.ctx == EGL_NO_CONTEXT)
    {
        printf("eglCreateContext failed\n");
        CHECK_EGL("eglCreateContext");
    }

    if (!eglMakeCurrent(st.dpy, st.surf, st.surf, st.ctx))
    {
        printf("eglMakeCurrent failed\n");
        CHECK_EGL("eglMakeCurrent");
    }

    // Информация о контексте
    const GLubyte *ver = glGetString(GL_VERSION);
    const GLubyte *rend = glGetString(GL_RENDERER);
    const GLubyte *ven = glGetString(GL_VENDOR);
    const GLubyte *sl = glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GL_VERSION  : %s\n", ver ? (const char *)ver : "null");
    printf("GL_RENDERER : %s\n", rend ? (const char *)rend : "null");
    printf("GL_VENDOR   : %s\n", ven ? (const char *)ven : "null");
    printf("GL_SL       : %s\n", sl ? (const char *)sl : "null");

    CHECK_GL("after context creation");

    return st;
}

GLuint create_compute_program(const char *src)
{
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    CHECK_GL("glCreateShader");

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        log[len] = 0;
        printf("Compute shader compile FAILED, log:\n%s\n", log);
        std::exit(1);
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, shader);
    glLinkProgram(prog);

    GLint linkOK = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linkOK);
    if (!linkOK)
    {
        char log[2048];
        GLsizei len = 0;
        glGetProgramInfoLog(prog, sizeof(log), &len, log);
        log[len] = 0;
        printf("Program link FAILED, log:\n%s\n", log);
        std::exit(1);
    }

    glDeleteShader(shader);
    CHECK_GL("create_compute_program");
    return prog;
}

GLuint create_ssbo(const std::vector<float> &data)
{
    GLuint ssbo = 0;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(float),
                 data.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    CHECK_GL("create_ssbo");
    return ssbo;
}

void read_ssbo(GLuint ssbo, std::vector<float> &out)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    CHECK_GL("bind ssbo for read");

    float *ptr = (float *)glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER,
        0, out.size() * sizeof(float),
        GL_MAP_READ_BIT);
    if (!ptr)
    {
        printf("glMapBufferRange returned NULL\n");
        CHECK_GL("glMapBufferRange");
        std::exit(1);
    }

    std::memcpy(out.data(), ptr, out.size() * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    CHECK_GL("read_ssbo");
}

int main()
{
    EGLState egl = init_egl();

    // Проверка, что есть compute шейдеры
    GLint workgroupSize = 0;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workgroupSize);
    GLenum glErr = glGetError();
    if (glErr == GL_INVALID_ENUM)
    {
        printf("Compute shaders NOT supported in this context (no GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS)\n");
        return 1;
    }
    printf("GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS = %d\n", workgroupSize);

    std::vector<float> data = {1, 2, 3, 4};
    GLuint ssbo = create_ssbo(data);

    const char *shader_src = R"(
        #version 310 es
        layout(local_size_x = 64) in;

        layout(std430, binding = 0) buffer Data {
            float values[];
        };

        void main() {
            uint id = gl_GlobalInvocationID.x;
            if (id >= 4u) return;
            values[id] = values[id] * 2.0;
        }
    )";

    GLuint prog = create_compute_program(shader_src);

    glUseProgram(prog);
    CHECK_GL("glUseProgram");

    GLuint groups = 1; // нам нужно только 4 элемента, 1 группа по 64 потока
    glDispatchCompute(groups, 1, 1);
    CHECK_GL("glDispatchCompute");

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    CHECK_GL("glMemoryBarrier");

    read_ssbo(ssbo, data);

    printf("Result:\n");
    for (float v : data)
        printf("%f\n", v);

    return 0;
}
