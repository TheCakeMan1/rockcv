#include <unistd.h>
#include <stdio.h>
#include "rkutils.h"

static int rkcv_hw_supports_drm(void)
{
    return (access("/dev/dri/renderD128", F_OK) == 0 ||
            access("/dev/dri/card0", F_OK) == 0);
}

static int rkcv_hw_supports_vpu(void)
{
    return access("/dev/mpp_service", F_OK) == 0;
}

static int rkcv_hw_supports_rga(void)
{
    return access("/dev/rga", F_OK) == 0;
}

int main(void)
{
    int drm = rkcv_hw_supports_drm();
    int vpu = rkcv_hw_supports_vpu();
    int rga = rkcv_hw_supports_rga();

    RKX_D("test_platform", "Missing devices:\n"
                           "  DRM: %s\n"
                           "  VPU: %s\n"
                           "  RGA: %s\n",
          drm ? "OK" : "missing",
          vpu ? "OK" : "missing",
          rga ? "OK" : "missing");

    if (drm && vpu && rga)
        return EXIT_SUCCESS;

    return EXIT_FAILURE;
}
