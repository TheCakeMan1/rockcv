cmake_minimum_required(VERSION 3.15)
project(rkutils C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Поиск библиотеки zlog
find_path(ZLOG_INCLUDE_DIR zlog.h
    PATHS /usr/local/include /usr/include
)
find_library(ZLOG_LIBRARY zlog
    PATHS /usr/local/lib /usr/lib
)

if (NOT ZLOG_INCLUDE_DIR OR NOT ZLOG_LIBRARY)
    message(FATAL_ERROR "zlog не найден. Убедись, что он установлен (make install).")
endif()

add_library(rkutils logs.c)

# Добавляем путь к заголовкам текущей папки (для logs.h)
find_package(Freetype REQUIRED)
target_link_libraries(rkutils PRIVATE ${FREETYPE_LIBRARIES})
target_include_directories(rkutils PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_include_directories(rkutils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${ZLOG_INCLUDE_DIR})

# Линкуем
target_link_libraries(rkutils PRIVATE ${ZLOG_LIBRARY} pthread)


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
