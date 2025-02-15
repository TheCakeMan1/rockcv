add_library(gui STATIC src/gui.cpp)
target_include_directories(gui PUBLIC ${CMAKE_SOURCE_DIR}/modules/gui/include)
install(TARGETS gui DESTINATION lib)


# find_package(OpenCL REQUIRED)
# target_link_libraries(gui
#     PUBLIC
#         profile
#         ${RGA_LIBRARIES}
# )
# target_include_directories(gui PRIVATE ${OpenCL_INCLUDE_DIRS})
# target_link_libraries(gui PRIVATE ${OpenCL_LIBRARIES})
