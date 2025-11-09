add_library(rkimage image_save.c)

target_include_directories(rkimage
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

# Инклуды
target_include_directories(rkimage PRIVATE ${FFMPEG_INCLUDE_DIRS})

# Линкуем FFmpeg + rkutils
target_link_libraries(rkimage
    PRIVATE
        ${FFMPEG_LIBRARIES}
        rkutils
        rkvideo
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
