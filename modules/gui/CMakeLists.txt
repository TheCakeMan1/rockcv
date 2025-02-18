find_package(PkgConfig REQUIRED)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVUTIL REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVCODEC REQUIRED libswscale)

add_library(gui SHARED src/gui.cpp)

# Подключаем заголовочные файлы FFmpeg и GUI
target_include_directories(gui PUBLIC ${AVUTIL_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS})
target_include_directories(gui PUBLIC ${CMAKE_SOURCE_DIR}/modules/gui/include)

# Линкуем библиотеки FFmpeg
target_link_libraries(gui PUBLIC ${AVUTIL_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${SWSCALE_LIBRARIES})

find_package(Qt5 COMPONENTS Widgets REQUIRED)
set(QT_VERSION Qt5)

include_directories(${PROJECT_SOURCE_DIR}/modules/frame/include)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
target_link_libraries(gui PUBLIC Qt5::Widgets)

# Устанавливаем библиотеку
install(TARGETS gui DESTINATION lib)


# find_package(OpenCL REQUIRED)
# target_link_libraries(gui
#     PUBLIC
#         profile
#         ${RGA_LIBRARIES}
# )
# target_include_directories(gui PRIVATE ${OpenCL_INCLUDE_DIRS})
# target_link_libraries(gui PRIVATE ${OpenCL_LIBRARIES})
