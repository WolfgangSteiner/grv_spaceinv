#ifndef GRVGM_H
#define GRVGM_H

#include "grv_gfx/grv_window.h"

typedef enum {
    GRVGM_BUTTON_CODE_LEFT  = 0,
    GRVGM_BUTTON_CODE_RIGHT = 1,
    GRVGM_BUTTON_CODE_UP    = 2,
    GRVGM_BUTTON_CODE_DOWN  = 3,
    GRVGM_BUTTON_CODE_A     = 4,
    GRVGM_BUTTON_CODE_B     = 5,
    GRVGM_BUTTON_CODE_X     = 6,
    GRVGM_BUTTON_CODE_Y     = 7,
} grvgm_button_code_t;

int grvgm_main(int argc, char** argv);
bool grvgm_is_button_down(grvgm_button_code_t button_code);

extern void on_init();
extern void on_update(f32);
extern void on_draw();

void grvgm_cls(u8 color);
void grvgm_spr(i32 number, f32 x, f32 y);
void grvgm_pset(f32 x, f32 y, u8 color);

#endif
