find_package(PkgConfig REQUIRED)

add_library(log SHARED src/log.cpp) #scr/img_op.cpp

# Подключаем заголовочные файлы FFmpeg и GUI
target_include_directories(log PUBLIC ${CMAKE_SOURCE_DIR}/modules/log/include)
# Устанавливаем библиотеку
install(TARGETS log DESTINATION lib)