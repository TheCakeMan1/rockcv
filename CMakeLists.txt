cmake_minimum_required(VERSION 3.10)
project(RockCV)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Включаем модули
add_subdirectory(modules/frame)
add_subdirectory(modules/gui)
add_subdirectory(modules/log)
add_subdirectory(modules/file)

# Найти OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Пути к Rockchip RGA
set(RGA_INCLUDE_DIR "/usr/include/rga") # Проверь, где у тебя лежат заголовочные файлы
set(RGA_LIBS "/usr/lib/librga.so")      # Проверь путь к библиотеке RGA

include_directories(${RGA_INCLUDE_DIR})

# Определяем библиотеку RockCV
add_library(RockCV SHARED rockcv.cpp)

set_target_properties(RockCV PROPERTIES
    CMAKE_INSTALL_RPATH "\$ORIGIN/../lib"
    CMAKE_BUILD_WITH_INSTALL_RPATH TRUE
)

# Подключаем заголовочные файлы
target_include_directories(RockCV PUBLIC ${CMAKE_SOURCE_DIR}/modules/gui/include)
target_include_directories(RockCV PUBLIC ${CMAKE_SOURCE_DIR}/modules/frame/include)
target_include_directories(RockCV PUBLIC ${CMAKE_SOURCE_DIR}/modules/log/include)
target_include_directories(RockCV PUBLIC ${CMAKE_SOURCE_DIR}/modules/file/include)

# Определяем макросы для использования OpenCV и RGA
target_compile_definitions(RockCV PUBLIC WITH_OPENCV=1 WITH_RGA=1)

# Опции компиляции (оптимизация для ARM NEON, Rockchip)
target_compile_options(RockCV PRIVATE -march=armv8-a -O2)

# Линкуем OpenCV и RGA
target_link_libraries(RockCV PRIVATE frame gui log)
target_link_libraries(RockCV PRIVATE ${OpenCV_LIBS} ${RGA_LIBS} ${DRM_LIBS})


# Устанавливаем библиотеку и заголовки
install(TARGETS RockCV DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/modules/frame/include/ DESTINATION include/rockcv)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/modules/gui/include/ DESTINATION include/rockcv)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/modules/log/include/ DESTINATION include/rockcv)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/modules/file/include/ DESTINATION include/rockcv)
