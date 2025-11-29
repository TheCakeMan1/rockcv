cmake_minimum_required(VERSION 3.15)
project(rkcv C CXX)

enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Подключаем модули
add_subdirectory(src/rkutils)
add_subdirectory(src/rkdrm)
add_subdirectory(src/rkframe)
add_subdirectory(src/rkvideo)
add_subdirectory(src/rkimage)
add_subdirectory(src/rkfilter)
add_subdirectory(src/rkdraw)
if(ENABLE_TEST)
    add_subdirectory(tests)
endif()
# Исполняемый файл
add_executable(rkcv main.c)

# target_compile_definitions(MyProject PRIVATE RKLOG_ENABLE)
option(RKLOG_ENABLE "Enable logging support" ON)

if(RKLOG_ENABLE)
    add_compile_definitions(RKLOG_ENABLE)
endif()

option(RKMPP_ENABLE "Enable mpp support" ON)

if(RKLOG_ENABLE)
    add_compile_definitions(RKLOG_ENABLE)
endif()

if(RKMPP_ENABLE)
    add_compile_definitions(RKMPP_ENABLE)
endif()

add_subdirectory(src/rkrgalib)
add_compile_definitions(RKRGALIB_ENABLE)
if(RKRGALIB_ENABLE)
endif()

# ---- тип сборки ----
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "🔧 Development build (Debug)")
    add_compile_definitions(BUILD_DEV)
else()
    message(STATUS "🚀 Production build (Release)")
    add_compile_definitions(BUILD_PROD)
endif()

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
link_libraries(${FREETYPE_LIBRARIES})

# Линкуем всё
target_link_libraries(rkcv
    PRIVATE rkutils
    PRIVATE rkdrm
    PRIVATE rkframe
    PRIVATE rkvideo
    PRIVATE rkimage
    PRIVATE rkfilter
    PRIVATE rkdraw
)
target_link_libraries(rkcv
    PRIVATE rkrgalib
)
if(RKRGALIB_ENABLE)

endif()
# Оптимизация под ARM Cortex-A76
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -pipe -march=native -mtune=cortex-a76 -flto -funroll-loops")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-semantic-interposition -fno-plt -fomit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -falign-functions=32 -falign-loops=32 -falign-jumps=16")

# Для C++
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pipe -march=native -mtune=cortex-a76 -flto -funroll-loops")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-semantic-interposition -fno-plt -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -falign-functions=32 -falign-loops=32 -falign-jumps=16")

# Обязательно для LTO (линковка)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")


