#ifndef GRVGM_H
#define GRVGM_H

#include "grv_gfx/grv_window.h"
#include "grv/vec2_fx32.h"
#include "grv_gfx/rect_fx32.h"
#include "grv_gfx/grv_spritesheet8.h"
#include "grv/grv_arena.h"

#define GRVGM_SAMPLE_RATE 48000

typedef enum {
    GRVGM_BUTTON_CODE_LEFT  = 0,
    GRVGM_BUTTON_CODE_RIGHT = 1,
    GRVGM_BUTTON_CODE_UP    = 2,
    GRVGM_BUTTON_CODE_DOWN  = 3,
    GRVGM_BUTTON_CODE_A     = 4,
    GRVGM_BUTTON_CODE_B     = 5,
    GRVGM_BUTTON_CODE_X     = 6,
    GRVGM_BUTTON_CODE_Y     = 7,
	GRVGM_BUTTON_MOUSE_LEFT = 1,
	GRVGM_BUTTON_MOUSE_RIGHT = 2,
	GRVGM_BUTTON_MOUSE_MIDDLE = 3,
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
    i32 index;
    i32 w,h;
    bool flip_x, flip_y;
    grv_spritesheet8_t* spritesheet;
} grvgm_sprite_t;

//==============================================================================
// config
//==============================================================================
void grvgm_set_screen_size(i32 w, i32 h);
void grvgm_set_sprite_size(i32 w);
void grvgm_set_fps(i32 fps);
void grvgm_set_use_game_state_store(bool flag);

//==============================================================================
// controls
//==============================================================================
bool grvgm_is_button_down(grvgm_button_code_t button_code);
bool grvgm_was_button_pressed(grvgm_button_code_t button_code);
bool grvgm_key_was_pressed(char key);
bool grvgm_key_was_pressed_with_mod(char key, u32 mod);
bool grvgm_is_keymod_down(u32 keymod);
//==============================================================================
// drawing api
//==============================================================================
void grvgm_clear_screen(u8 color);

void grvgm_draw_sprite(vec2_i32 pos, grvgm_sprite_t sprite);
void grvgm_draw_pixel(vec2_i32 pos, u8 color);
void grvgm_draw_line(vec2_i32 p1, vec2_i32 p2, u8 color);
void grvgm_draw_rect(rect_i32 rect, u8 color);
void grvgm_fill_rect(rect_i32 rect, u8 color);
void grvgm_draw_rect_chamfered(rect_i32 rect, u8 color);
void grvgm_fill_rect_chamfered(rect_i32 rect, u8 color);
void grvgm_draw_circle(vec2_i32 pos, i32 r, u8 color);
void grvgm_fill_circle(vec2_i32 pos, i32 r, u8 color);
void grvgm_draw_text(vec2_i32 pos, grv_str_t text, u8 color);
void grvgm_draw_text_floating(vec2_i32 pos, grv_str_t text, u8 color);
void grvgm_draw_text_aligned(rect_i32 rect, grv_str_t text, grv_alignment_t alignment, u8 color);

void grvgm_draw_sprite_fx32(vec2_fx32 pos, grvgm_sprite_t sprite);
void grvgm_draw_pixel_fx32(vec2_fx32 pos, u8 color);
void grvgm_draw_line_fx32(vec2_fx32 p1, vec2_fx32 p2, u8 color);
void grvgm_draw_rect_fx32(rect_fx32 rect, u8 color);
void grvgm_fill_rect_fx32(rect_fx32 rect, u8 color);
void grvgm_draw_rect_chamfered_fx32(rect_fx32 rect, u8 color);
void grvgm_fill_rect_chamfered_fx32(rect_fx32 rect, u8 color);
void grvgm_draw_circle_fx32(vec2_fx32 pos, fx32 r, u8 color);
void grvgm_fill_circle_fx32(vec2_fx32 pos, fx32 r, u8 color);
void grvgm_draw_text_fx32(vec2_fx32 pos, grv_str_t text, u8 color);
void grvgm_draw_text_aligned_fx32(rect_fx32 retct, grv_str_t text, grv_alignment_t alignment, u8 color);

void* grvgm_draw_arena_alloc(size_t size);
grv_arena_t* grvgm_draw_arena(void);
void grvgm_defer(void(*callback)(void*), void* data);
//==============================================================================
// math
//==============================================================================
fx32 grvgm_sin(fx32 x);
fx32 grvgm_cos(fx32 x);

//==============================================================================
// text
//==============================================================================
rect_i32 grvgm_text_rect(grv_str_t str);

//==============================================================================
// mouse
//==============================================================================
vec2_i32 grvgm_mouse_position(void);
vec2_fx32 grvgm_mouse_position_fx32(void);
vec2_i32 grvgm_initial_drag_position(void);
vec2_fx32 grvgm_initial_drag_position_fx32(void);

bool grvgm_mouse_in_rect(rect_i32 rect);
bool grvgm_mouse_click_in_rect(rect_i32 rect, i32 button_id);
bool grvgm_mouse_click_in_rect_with_id(u64 id, rect_i32 rect, i32 button_id);

bool grvgm_mouse_drag_started_in_rect(rect_i32 rect);

//==============================================================================
// random numbers
//==============================================================================
f32 grvgm_random_f32(void);

//==============================================================================
// misc
//==============================================================================
vec2_fx32 grvgm_screen_size_fx32(void);
rect_fx32 grvgm_screen_rect_fx32(void);

vec2_i32 grvgm_screen_size(void);
rect_i32 grvgm_screen_rect(void);

// get the current game time
fx32 grvgm_time(void);
f32 grvgm_time_f32(void);

// compute time difference between a timestamp and the current time
fx32 grvgm_timediff(fx32 timestamp);

// ticks since start of game
u64 grvgm_ticks(void);
#endif
