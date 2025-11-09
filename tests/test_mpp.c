#include <stdio.h>
#include "rockchip/rk_mpi.h"
#include <string.h>

static const char *mpp_coding_name(MppCodingType type)
{
    switch (type)
    {
    case MPP_VIDEO_CodingMPEG2:
        return "MPEG-2";
    case MPP_VIDEO_CodingH263:
        return "H.263";
    case MPP_VIDEO_CodingMPEG4:
        return "MPEG-4";
    case MPP_VIDEO_CodingAVC:
        return "H.264/AVC";
    case MPP_VIDEO_CodingHEVC:
        return "H.265/HEVC";
    case MPP_VIDEO_CodingMJPEG:
        return "MJPEG";
    case MPP_VIDEO_CodingVP8:
        return "VP8";
    case MPP_VIDEO_CodingVP9:
        return "VP9";
    case MPP_VIDEO_CodingAV1:
        return "AV1";
    default:
        return "Unknown";
    }
}

int main(void)
{
    MppCtx ctx = NULL;
    MppCodingType type;

    printf("Decoder support:\n");
    for (int t = MPP_VIDEO_CodingUnused; t < MPP_VIDEO_CodingMax; t++)
    {
        if (!mpp_check_support_format(MPP_CTX_DEC, t))
            continue;

        const char *name = mpp_coding_name(t);
        if (strcmp(name, "Unknown") != 0)
            printf("  %s\n", name);
    }
    printf("Encoder support:\n");
    for (type = MPP_VIDEO_CodingUnused; type < MPP_VIDEO_CodingMax; type++)
    {
        if (!mpp_check_support_format(MPP_CTX_ENC, type))
            continue;

        const char *name = mpp_coding_name(type);
        if (strcmp(name, "Unknown") != 0)
            printf("  %s\n", name);
    }

    return 0;
}
