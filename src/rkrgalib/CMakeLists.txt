add_library(rkrgalib
    NormalRga.c
    NormalRgaApi.c
)

target_include_directories(rkrgalib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)