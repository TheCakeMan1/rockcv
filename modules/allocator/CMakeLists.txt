find_package(PkgConfig REQUIRED)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(SWSCALE REQUIRED libswscale)

add_library(allocator SHARED
    src/dma_alloc.cpp
    src/drm_alloc.cpp
)

# Подключаем заголовочные файлы FFmpeg и GUI
target_include_directories(allocator PUBLIC ${AVUTIL_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} /usr/include/drm /usr/include/dma)
target_include_directories(allocator PUBLIC ${CMAKE_SOURCE_DIR}/modules/allocator/include)

# include_directories(${PROJECT_SOURCE_DIR}/modules/allocator/include)

# Линкуем библиотеки FFmpeg
target_link_libraries(allocator PUBLIC ${AVUTIL_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${SWSCALE_LIBRARIES})

# Устанавливаем библиотеку
install(TARGETS allocator DESTINATION lib)
