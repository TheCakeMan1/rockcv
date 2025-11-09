add_executable(test_ffmpeg test_ffmpeg.c)
target_link_libraries(test_ffmpeg PRIVATE rkvideo rkutils)
add_test(NAME test_ffmpeg COMMAND test_ffmpeg)

add_executable(test_platform test_platform.c)
target_link_libraries(test_platform PRIVATE rkutils)
add_test(NAME test_platform COMMAND test_platform)

include_directories(/usr/include/rockchip/)
link_directories(/usr/lib/aarch64-linux-gnu/)

add_executable(test_mpp test_mpp.c)
target_link_libraries(test_mpp PRIVATE rockchip_mpp)
add_test(NAME test_mpp COMMAND test_mpp)
