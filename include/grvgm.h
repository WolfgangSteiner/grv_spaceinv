#ifndef GRVGM_H
#define GRVGM_H

#include "grv_gfx/grv_window.h"
#include "grv/vec2_fx32.h"
#include "grv_gfx/rect_fx32.h"
#include "grv_gfx/grv_spritesheet8.h"

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
    vec2_fx32 pos;
    i32 index;
    bool flip_x, flip_y;
    grv_spritesheet8_t* spritesheet;
} grvgm_sprite_t;

bool grvgm_is_button_down(grvgm_button_code_t button_code);
void grvgm_clear_screen(u8 color);
void grvgm_draw_sprite(grvgm_sprite_t sprite);
void grvgm_draw_pixel(vec2_fx32 pos, u8 color);
void grvgm_draw_rect(rect_fx32 rect, u8 color);
void grvgm_draw_circle(vec2_fx32 pos, fx32 r, u8 color);
void grvgm_fill_circle(vec2_fx32 pos, fx32 r, u8 color);
void grvgm_draw_text(grv_str_t text, vec2_fx32 pos, u8 color);
vec2_fx32 grvgm_screen_size(void);
rect_fx32 grvgm_screen_rect(void);

// get the current game time
fx32 grvgm_time(void);

// compute time difference between a timestamp and the current time
fx32 grvgm_timediff(fx32 timestamp);
#endif
