#include "lib/grv/grvbld.h"

int main(int argc, char** argv) {
    GRV_CHECK_AND_REBUILD();    
    grvbld_config_t* config =  grvbld_config_new(argc, argv);
    //config->cc = "tcc";
    //config->use_ccache = false;
    grvbld_strarr_push(&config->warnings, "-Wextra -Wpedantic");
    grvbld_strarr_push(&config->warnings, "-Wno-error=strict-prototypes");
    grvbld_config_add_include_directories(config, ".", "lib/grv/include", NULL);
    grvbld_config_add_library_directory(config, "lib/grv/build");

    grvbld_target_t* spaceinv = grvbld_target_create_executable("spaceinv");
    grvbld_target_add_src(spaceinv, "src/main.c");
    grvbld_target_add_src(spaceinv, "src/grv/grv_gfx/grv_spritesheet8.c");
    grvbld_target_add_src(spaceinv, "src/grvgm.c");
    grvbld_target_link_libraries(spaceinv, "grv", "grvgfx", "SDL2", NULL);
    spaceinv->run_after_build = true;
    grvbld_build_target(config, spaceinv);

    return 0;
}

