add_library(img SHARED
    scr/format.cpp
)

target_include_directories(img
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)