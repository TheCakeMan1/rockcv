find_package(PkgConfig REQUIRED)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(SWSCALE REQUIRED libswscale)

enable_language(ASM)
set(CMAKE_ASM_COMPILER "/usr/bin/as")

add_library(frame_asm OBJECT src/calculate.asm src/codec.asm src/frame_operation.asm)

add_library(frame SHARED
    src/transform.cpp
    src/codec.cpp
    src/type_drm.cpp
    src/frame_operation.cpp
    $<TARGET_OBJECTS:frame_asm> 
)

# Подключаем заголовочные файлы FFmpeg и GUI
target_include_directories(frame PUBLIC ${AVUTIL_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} /usr/include/drm /usr/include/dma)
target_include_directories(frame PUBLIC ${CMAKE_SOURCE_DIR}/modules/frame/include)

include_directories(${PROJECT_SOURCE_DIR}/modules/allocator/include)
include_directories(${PROJECT_SOURCE_DIR}/modules/utils/include)
include_directories(${PROJECT_SOURCE_DIR}/modules/file/include)

# Линкуем библиотеки FFmpeg
target_link_libraries(frame PUBLIC ${AVUTIL_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${SWSCALE_LIBRARIES})

# Устанавливаем библиотеку
install(TARGETS frame DESTINATION lib)
