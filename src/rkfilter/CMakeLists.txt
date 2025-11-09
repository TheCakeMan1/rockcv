add_library(rkfilter  gauss_vulcan.c)# gauss.c

target_include_directories(rkfilter
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

target_include_directories(rkfilter PRIVATE ${FFMPEG_INCLUDE_DIRS})

find_package(OpenCL REQUIRED)

# Подключаем заголовки OpenCL
target_include_directories(rkfilter PRIVATE ${OpenCL_INCLUDE_DIRS})

# Линкуем библиотеку OpenCL
target_link_libraries(rkfilter PRIVATE ${OpenCL_LIBRARIES})
target_link_libraries(rkfilter
    PRIVATE
        ${FFMPEG_LIBRARIES}
        rkutils
        rkframe
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
