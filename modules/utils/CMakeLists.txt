find_package(PkgConfig REQUIRED)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVUTIL REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVCODEC REQUIRED libswscale)

set(CMAKE_ASM_COMPILER "aarch64-linux-gnu-gcc")

enable_language(ASM)

add_library(utils SHARED src/func.S)

set_target_properties(utils PROPERTIES LINKER_LANGUAGE C)

# Подключаем заголовочные файлы FFmpeg и GUI
target_include_directories(utils PUBLIC ${AVUTIL_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS})
target_include_directories(utils PUBLIC ${CMAKE_SOURCE_DIR}/modules/utils/include)

set(CMAKE_ASM_FLAGS "-march=armv8-a+simd")

# Линкуем библиотеки FFmpeg
target_link_libraries(utils PUBLIC ${AVUTIL_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${SWSCALE_LIBRARIES})

# Устанавливаем библиотеку
install(TARGETS utils DESTINATION lib)