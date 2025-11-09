#include "rkvideo.h"
#include "logs.h"
#include "rkimage.h"
// #include "rkfilter.h"
#include "rkframe.h"
#include "rkfilter.h"

int main()
{
    // VkCtx vk = {0};
    // init_vk(&vk);
    // build_pipelines(&vk, "gauss_vulcan_nv12_uv.spv", "gauss_vulcan_nv12_y.spv");
    info_frame_t temp = {
        .fmt = RK_PIX_FMT_YCbCr_420_SP,
        .width = 1920,
        .height = 1080};
    rkcv_shot_t *c = nshot(&temp);
    s_text_f text = {
        .x = 20,
        .y = 20,
        .text = "Hello",
        .fontsize = 100};

    // rkcv_t *a = openv("rtsp://192.168.6.102:554/live/main");
    rkcv_t *a = openv("rtsp://192.168.6.53:554/user=admin_password=1UfX6Hen_channel=1_stream=0&protocol=unicast.sdp?real_stream");
    // info_frame_t temp = {
    //     .fmt = RK_PIX_FMT_YCbCr_420_SP,
    //     .width = 1920,
    //     .height = 1080};
    rkcv_t *o = video(a, "test.mp4", RKCodec_HEVC, &temp);
    printf_streams(a);
    int k = 0;
    while (readf(a))
    {
        printf("I read frame\n");
        // nv12_gaussian_blur_frames(&vk, a->shot->frame, c->frame, /*radius=*/7, /*sigma=*/3.0f);
        imw(o, a->shot, &text);
        k++;
        if (k > 1000)
        {
            break;
        }
        // break;
    }
    // free_shot(c);
    realese(o);
}