find_package(PkgConfig REQUIRED)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVUTIL REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVCODEC REQUIRED libswscale)

add_library(file SHARED src/rtsp.cpp) #scr/img_op.cpp

# Подключаем заголовочные файлы FFmpeg и GUI
target_include_directories(file PUBLIC ${CMAKE_SOURCE_DIR}/modules/file/include)

target_link_libraries(file PUBLIC ${AVUTIL_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${SWSCALE_LIBRARIES})
# Устанавливаем библиотеку
install(TARGETS file DESTINATION lib)