cmake_minimum_required(VERSION 3.10)
project(gles_compute_demo)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)

pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLESv2 REQUIRED glesv2)

include_directories(
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

link_directories(
    ${EGL_LIBRARY_DIRS}
    ${GLESv2_LIBRARY_DIRS}
)

add_executable(main main.cpp)

target_link_libraries(main
    ${EGL_LIBRARIES}
    ${GLESv2_LIBRARIES}
)
