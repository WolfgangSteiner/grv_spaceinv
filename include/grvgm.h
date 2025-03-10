#ifndef GRVGM_H
#define GRVGM_H

#include "grv_gfx/grv_window.h"
#include "grv/grv_vec2_fixed32.h"
#include "grv/grv_rect_fixed32.h"
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

int grvgm_main(int argc, char** argv);
bool grvgm_is_button_down(grvgm_button_code_t button_code);

extern void on_init(void);
extern void on_update(f32);
extern void on_draw(void);

typedef struct {
    grv_vec2_fixed32_t pos;
    i32 index;
    bool flip_x, flip_y;
    grv_spritesheet8_t* spritesheet;
} grvgm_sprite_t;


void grvgm_clear_screen(u8 color);
void grvgm_draw_sprite(grvgm_sprite_t sprite);
void grvgm_draw_pixel(grv_vec2_fixed32_t pos, u8 color);
void grvgm_draw_rect(grv_rect_fixed32_t rect, u8 color);
grv_vec2_fixed32_t grvgm_screen_size(void);
grv_fixed32_t grvgm_time(void);

#endif
