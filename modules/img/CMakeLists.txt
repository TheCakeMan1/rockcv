find_package(PkgConfig REQUIRED)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVUTIL REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVCODEC REQUIRED libswscale)

add_library(img SHARED src/transform.cpp src/codec.cpp src/type_drm.cpp src/frame_operation.cpp) #scr/img_op.cpp

# Подключаем заголовочные файлы FFmpeg и GUI
target_include_directories(img PUBLIC ${AVUTIL_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} /usr/include/drm)
target_include_directories(img PUBLIC ${CMAKE_SOURCE_DIR}/modules/img/include)

# Линкуем библиотеки FFmpeg
target_link_libraries(img PUBLIC ${AVUTIL_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${SWSCALE_LIBRARIES})

# Устанавливаем библиотеку
install(TARGETS img DESTINATION lib)