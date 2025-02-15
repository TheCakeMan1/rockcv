cmake_minimum_required(VERSION 3.10)
project(RockCV)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Включаем модули
add_subdirectory(modules/img)
add_subdirectory(modules/gui)

# Определяем библиотеку RockCV
add_library(RockCV STATIC rockcv.cpp)

# Добавляем заголовочные файлы (PUBLIC, чтобы были видны пользователям)
target_include_directories(RockCV PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Линкуем модули img и gui в RockCV
target_link_libraries(RockCV PRIVATE img gui)

# Опции компиляции (оптимизация для ARM NEON, Rockchip)
target_compile_options(RockCV PRIVATE -march=armv8-a -O2)

# Устанавливаем библиотеку и заголовки
install(TARGETS RockCV DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
