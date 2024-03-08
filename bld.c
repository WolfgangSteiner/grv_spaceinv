#include "lib/grv/grvbld.h"

int main(int argc, char** argv) {
    GRV_CHECK_AND_REBUILD();    
    grvbld_config_t* config =  grvbld_config_new(argc, argv);
    //config->cc = "tcc";
    //config->use_ccache = false;
    grvbld_strarr_push(&config->warnings, "-Wextra -Wpedantic");
    grvbld_config_add_include_directories(config, ".", "lib/grv/include", NULL);
    grvbld_config_add_library_directory(config, "lib/grv/build");

    grvbld_target_t* spaceinv = grvbld_target_create("spaceinv", GRVBLD_EXECUTABLE);
    grvbld_target_add_src(spaceinv, "src/main.c");
    grvbld_target_link_libraries(spaceinv, "grv", "grvgfx", "SDL2", NULL);
    grvbld_build_target(config, spaceinv);

    return 0;
}

