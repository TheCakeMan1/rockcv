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

# --- RGA ---
# Если RGA установлен в /usr/local/lib и /usr/local/include
# то можно напрямую указать:
find_path(RGA_INCLUDE_DIR NAMES rga/rga.h rga/im2d.h)
find_library(RGA_LIBRARY NAMES rga)
if (RGA_INCLUDE_DIR AND RGA_LIBRARY)
    message(STATUS "Found RGA: ${RGA_LIBRARY}")
    target_include_directories(rkvideo PUBLIC ${RGA_INCLUDE_DIR})
    target_link_libraries(rkvideo PUBLIC ${RGA_LIBRARY})
else()
    message(FATAL_ERROR "Rockchip RGA not found. Please install librga-dev or set RGA_INCLUDE_DIR/RGA_LIBRARY")
endif()

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
    PRIVATE
        freetype
        ${FFMPEG_LIBRARIES}
        # ${RGA_LIB}     # <--- вот это важно
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
