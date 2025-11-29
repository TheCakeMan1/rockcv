add_library(rkvideo video.c)

target_include_directories(rkvideo
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Ищем FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
    libavfilter
)

# Инклуды
target_include_directories(rkvideo PRIVATE ${FFMPEG_INCLUDE_DIRS})

find_package(Freetype REQUIRED)
target_include_directories(rkvideo PRIVATE ${FREETYPE_INCLUDE_DIRS})

# Линкуем библиотеку OpenCL
target_link_libraries(rkvideo PRIVATE ${OpenCL_LIBRARIES})
target_link_libraries(rkvideo
    PUBLIC
        rkutils
        rkdrm
        rkframe
        rkdraw
    PRIVATE
        freetype
        ${FFMPEG_LIBRARIES}
        m
        stdc++
)

# target_link_libraries(rkvideo
#     PUBLIC 
#         rkrgalib
# )
# if(RKRGALIB_ENABLE)
# )
# else()
# find_path(RGA_INCLUDE_DIR NAMES rga/rga.h rga/im2d.h)
# find_library(RGA_LIBRARY NAMES rga)
# if (RGA_INCLUDE_DIR AND RGA_LIBRARY)
#     message(STATUS "Found RGA: ${RGA_LIBRARY}")
#     target_include_directories(rkvideo PUBLIC ${RGA_INCLUDE_DIR})
#     target_link_libraries(rkvideo PUBLIC ${RGA_LIBRARY})
# else()
#     message(FATAL_ERROR "Rockchip RGA not found. Please install librga-dev or set RGA_INCLUDE_DIR/RGA_LIBRARY")
# endif()
# endif()


find_path(RGA_INCLUDE_DIR NAMES rga.h im2d.h)
find_library(RGA_LIBRARY NAMES rga)
if (RGA_INCLUDE_DIR AND RGA_LIBRARY)
    target_include_directories(rkvideo PUBLIC ${RGA_INCLUDE_DIR})
    target_link_libraries(rkvideo PUBLIC ${RGA_LIBRARY})
    message(${RGA_INCLUDE_DIR})
else()
    message(FATAL_ERROR "Rockchip RGA not found. Please install librga-dev or set RGA_INCLUDE_DIR/RGA_LIBRARY")
endif()

option(RKLOG_ENABLE "Enable logging support" ON)

if(RKLOG_ENABLE)
    add_compile_definitions(RKLOG_ENABLE)
endif()

option(RKMPP_ENABLE "Enable mpp support" OFF)

if(RKMPP_ENABLE)
    execute_process(
        COMMAND ffmpeg -decoders
        OUTPUT_VARIABLE FF_DECODERS
    )

    # Поиск
    string(FIND "${FF_DECODERS}" "h264_rkmpp" HAS_H264_D_RKMPP)
    string(FIND "${FF_DECODERS}" "hevc_rkmpp" HAS_HEVC_D_RKMPP)
    string(FIND "${FF_DECODERS}" "h263_rkmpp" HAS_H263_D_RKMPP)
    string(FIND "${FF_DECODERS}" "av1_rkmpp"  HAS_AV1_D_RKMPP)
    string(FIND "${FF_DECODERS}" "mjpeg_rkmpp" HAS_MJPEG_D_RKMPP)

    message("decoders:")
    if(HAS_H264_D_RKMPP EQUAL -1)
        message(WARNING "   * decoder h264_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_H264_D_RKMPP=1)
        message("   * decoder h264_rkmpp found in FFmpeg")
    endif()

    if(HAS_HEVC_D_RKMPP EQUAL -1)
        message(WARNING "   * decoder hevc_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_HEVC_D_RKMPP=1)
        message("   * decoder hevc_rkmpp found in FFmpeg")
    endif()
    
    if(HAS_H263_D_RKMPP EQUAL -1)
        message(WARNING "   * decoder h263_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_H263_D_RKMPP=1)
        message("   * decoder h263_rkmpp found in FFmpeg")
    endif()
    
    if(HAS_AV1_D_RKMPP EQUAL -1) 
        message(WARNING "   * decoder av1_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_AV1_D_RKMPP=1)   
        message("   * decoder av1_rkmpp found in FFmpeg")
    endif()
    
    if(HAS_MJPEG_D_RKMPP EQUAL -1)
        message(WARNING "   * decoder mjpeg_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_MJPEG_D_RKMPP=1)
        message("   * decoder mjpeg_rkmpp found in FFmpeg")
    endif()

    execute_process(
        COMMAND ffmpeg -encoders
        OUTPUT_VARIABLE FF_ENCODECS
    )

    string(FIND "${FF_ENCODECS}" "h264_rkmpp" HAS_H264_E_RKMPP)
    string(FIND "${FF_ENCODECS}" "hevc_rkmpp" HAS_HEVC_E_RKMPP)
    string(FIND "${FF_ENCODECS}" "h263_rkmpp" HAS_H263_E_RKMPP)
    string(FIND "${FF_ENCODECS}" "av1_rkmpp" HAS_AV1_E_RKMPP)
    string(FIND "${FF_ENCODECS}" "mjpeg_rkmpp" HAS_MJPEG_E_RKMPP)

    message("encoders:")
    if(HAS_H264_E_RKMPP EQUAL -1)
        message(WARNING "   * encoder h264_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_H264_E_RKMPP=1)
        message("   * encoder h264_rkmpp found in FFmpeg")
    endif()

    if(HAS_HEVC_E_RKMPP EQUAL -1)
        message(WARNING "   * encoder hevc_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_HEVC_E_RKMPP=1)
        message("   * encoder hevc_rkmpp found in FFmpeg")
    endif()
    
    if(HAS_H263_E_RKMPP EQUAL -1)
        message(WARNING "   * encoder h263_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_H263_E_RKMPP=1)
        message("   * encoder h263_rkmpp found in FFmpeg")
    endif()
    
    if(HAS_AV1_E_RKMPP EQUAL -1)
        message(WARNING "   * encoder av1_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_AV1_E_RKMPP=1)
        message("   * encoder av1_rkmpp found in FFmpeg")
    endif()
    
    if(HAS_MJPEG_E_RKMPP EQUAL -1)
        message(WARNING "   * encoder mjpeg_rkmpp not found in FFmpeg")
    else()
        add_compile_definitions(HAS_MJPEG_E_RKMPP=1)
        message("   * encoder mjpeg_rkmpp found in FFmpeg")
    endif()

    add_compile_definitions(RKMPP_ENABLE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(BUILD_DEV)
else()
    add_compile_definitions(BUILD_PROD)
endif()
    
if(USE_DRM)

endif()

if(USE_CPU_DRAW_TEXT)
    add_compile_definitions(USE_CPU_DRAW_TEXT)
endif()

if(USE_DRM_BUFFER)
    add_compile_definitions(USE_DRM_BUFFER)
endif()