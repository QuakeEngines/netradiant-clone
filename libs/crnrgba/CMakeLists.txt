add_library(crnrgba
        crn_rgba.h
        crn_rgba.cpp
        ../crunch/inc/crn_decomp.h
        ../crunch/inc/crnlib.h
        )

set_target_properties(crnrgba PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(crnrgba PRIVATE ddslib)
target_compile_options(crnrgba PRIVATE -fexceptions)
