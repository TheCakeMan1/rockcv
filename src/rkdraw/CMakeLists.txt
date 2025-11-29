add_library(rkdraw draw_text.c font32.c)

find_path(RGA_INCLUDE_DIR NAMES rga.h im2d.h)
find_library(RGA_LIBRARY NAMES rga)
if (RGA_INCLUDE_DIR AND RGA_LIBRARY)
    target_include_directories(rkdraw PUBLIC ${RGA_INCLUDE_DIR})
    target_link_libraries(rkdraw PUBLIC ${RGA_LIBRARY})
    message(${RGA_INCLUDE_DIR})
else()
    message(FATAL_ERROR "Rockchip RGA not found. Please install librga-dev or set RGA_INCLUDE_DIR/RGA_LIBRARY")
endif()

target_include_directories(rkdraw
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(rkdraw
    PUBLIC
        rkutils
        rkframe
)