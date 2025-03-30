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

    #if 0
    grvbld_target_t* lib_grv = grvbld_target_create_dynamic_library("grv");
    grvbld_target_add_src(lib_grv, "lib/grv/src/grv.c");
    grvbld_build_target(config, lib_grv);

    grvbld_target_t* lib_grvgfx = grvbld_target_create_dynamic_library("grvgfx");
    grvbld_target_add_src(lib_grvgfx, "lib/grv/src/grv_gfx/grv_gfx.c");
    grvbld_target_add_src(lib_grvgfx, "src/grv/grv_gfx/grv_spritesheet8.c");
    grvbld_target_add_data_file(lib_grvgfx, "lib/grv/src/grv_gfx/cozette.psf");
    grvbld_build_target(config, lib_grvgfx);
    #endif

    grvbld_target_t* lib_grvgm = grvbld_target_create_dynamic_library("grvgm");
    grvbld_target_add_src(lib_grvgm, "src/grvgm.c");
    grvbld_target_add_src(lib_grvgm, "src/grv/grv_gfx/grv_spritesheet8.c");
    grvbld_target_add_src(lib_grvgm, "src/grvgm_small_font.c");
    //grvbld_target_link_libraries(lib_grvgm, "grv", "grvgfx");
    grvbld_build_target(config, lib_grvgm);

    grvbld_target_t* lib_spaceinv = grvbld_target_create_dynamic_library("spaceinv");
    grvbld_target_add_src(lib_spaceinv, "src/spaceinv.c");
    //grvbld_target_link_libraries(lib_spaceinv, "grv", "grvgfx", "grvgm");
    grvbld_build_target(config, lib_spaceinv);

    grvbld_target_t* spaceinv = grvbld_target_create_executable("spaceinv");
    grvbld_target_add_src(spaceinv, "src/grvgm_main.c");
    grvbld_target_add_link_option(spaceinv, "-Wl,-rpath=\\$ORIGIN/");
    //grvbld_target_add_link_option(spaceinv, "-Wl,-z,origin");
    grvbld_target_link_libraries(spaceinv, "grv", "grvgfx", "grvgm", "SDL2", NULL);
    //spaceinv->run_after_build = true;
    grvbld_build_target(config, spaceinv);

    return 0;
}

