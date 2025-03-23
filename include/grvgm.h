#ifndef GRVGM_H
#define GRVGM_H

#include "grv_gfx/grv_window.h"
#include "grv/grv_vec2_fixed32.h"
#include "grv/grv_rect_fixed32.h"
#include "grv_gfx/grv_spritesheet8.h"
#include <dlfcn.h>

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

typedef enum {
    GRVGM_KEYMOD_SHIFT_LEFT = 1,
    GRVGM_KEYMOD_SHIFT_RIGHT = 2,
    GRVGM_KEYMOD_SHIFT = GRVGM_KEYMOD_SHIFT_LEFT | GRVGM_KEYMOD_SHIFT_RIGHT,
    GRVGM_KEYMOD_CTRL_LEFT = 4,
    GRVGM_KEYMOD_CTRL_RIGHT = 8,
    GRVGM_KEYMOD_CTRL = GRVGM_KEYMOD_CTRL_LEFT | GRVGM_KEYMOD_CTRL_RIGHT,
    GRVGM_KEYMOD_ALT_LEFT = 16,
    GRVGM_KEYMOD_ALT_RIGHT = 32,
    GRVGM_KEYMOD_ALT = GRVGM_KEYMOD_ALT_LEFT | GRVGM_KEYMOD_ALT_RIGHT,
} grvgm_keymod_t;


int grvgm_main(int argc, char** argv);

#if 0
extern void on_init(void);
extern void on_update(f32);
extern void on_draw(void);
#endif

typedef struct {
    grv_vec2_fixed32_t pos;
    i32 index;
    bool flip_x, flip_y;
    grv_spritesheet8_t* spritesheet;
} grvgm_sprite_t;

bool grvgm_is_button_down(grvgm_button_code_t button_code);
void grvgm_clear_screen(u8 color);
void grvgm_draw_sprite(grvgm_sprite_t sprite);
void grvgm_draw_pixel(grv_vec2_fixed32_t pos, u8 color);
void grvgm_draw_rect(grv_rect_fixed32_t rect, u8 color);
void grvgm_draw_circle(grv_vec2_fixed32_t pos, grv_fixed32_t r, u8 color);
void grvgm_fill_circle(grv_vec2_fixed32_t pos, grv_fixed32_t r, u8 color);
grv_vec2_fixed32_t grvgm_screen_size(void);

// get the current game time
grv_fixed32_t grvgm_time(void);

// compute time difference between a timestamp and the current time
grv_fixed32_t grvgm_timediff(grv_fixed32_t timestamp);
#endif
