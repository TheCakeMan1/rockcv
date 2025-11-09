add_library(rkframe shot.c)

target_include_directories(rkframe
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
)

find_path(RGA_INCLUDE_DIR NAMES rga/rga.h rga/im2d.h)
find_library(RGA_LIBRARY NAMES rga)
if (RGA_INCLUDE_DIR AND RGA_LIBRARY)
    target_include_directories(rkframe PUBLIC ${RGA_INCLUDE_DIR})
    target_link_libraries(rkframe PUBLIC ${RGA_LIBRARY})
else()
    message(FATAL_ERROR "Rockchip RGA not found. Please install librga-dev or set RGA_INCLUDE_DIR/RGA_LIBRARY")
endif()

# Инклуды
target_include_directories(rkframe PRIVATE ${FFMPEG_INCLUDE_DIRS})

find_package(Vulkan REQUIRED)

target_link_libraries(rkframe
    PUBLIC
        rkutils
        rkdrm
    PRIVATE
        ${Vulkan_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        m
        stdc++
)

option(RKLOG_ENABLE "Enable logging support" ON)

if(RKLOG_ENABLE)
    add_compile_definitions(RKLOG_ENABLE)
endif()

option(RKMPP_ENABLE "Enable mpp support" ON)

if(RKLOG_ENABLE)
    add_compile_definitions(RKMPP_ENABLE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(BUILD_DEV)
else()
    add_compile_definitions(BUILD_PROD)
endif()
