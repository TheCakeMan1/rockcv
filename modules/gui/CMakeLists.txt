add_library(gui SHARED
    src/gui.cpp
)

target_include_directories(gui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# find_package(OpenCL REQUIRED)
# target_link_libraries(gui
#     PUBLIC
#         profile
#         ${RGA_LIBRARIES}
# )
# target_include_directories(gui PRIVATE ${OpenCL_INCLUDE_DIRS})
# target_link_libraries(gui PRIVATE ${OpenCL_LIBRARIES})
