#include "grvgm.h"
#include "grv/grv_log.h"
#include "grv_gfx/grv_bitmap_font.h"
#include "grv_gfx/grv_frame_buffer.h"
#include "grv_gfx/grv_img8.h"
#include "grv_gfx/grv_spritesheet8.h"
#include <SDL2/SDL.h>
typedef struct {
    grv_window_t* window;
    grv_frame_buffer_t* framebuffer;
    grv_bitmap_font_t* font;
    grv_spritesheet8_t spritesheet;
    u64 timestamp;
    u64 game_time;
} grvgm_state_t;
static grvgm_state_t _grvgm_state = {0};

static u8* _grvgm_previous_keyboard_state = NULL;
static u8* _grvgm_current_keyboard_state = NULL;

//==============================================================================
// Keyboard and button state
//==============================================================================
bool _grvgm_is_key_down(int scancode) {
    if (_grvgm_current_keyboard_state == NULL) return false;
    return _grvgm_current_keyboard_state[scancode] != 0;
}

bool grvgm_is_button_down(grvgm_button_code_t button_code) {
    const u8* keyboard_state = SDL_GetKeyboardState(NULL);
    switch (button_code) {
        case GRVGM_BUTTON_CODE_LEFT:
            return _grvgm_is_key_down(SDL_SCANCODE_LEFT) || _grvgm_is_key_down(SDL_SCANCODE_H);
        case GRVGM_BUTTON_CODE_RIGHT:
            return _grvgm_is_key_down(SDL_SCANCODE_RIGHT) || _grvgm_is_key_down(SDL_SCANCODE_L);
        case GRVGM_BUTTON_CODE_UP:
            return _grvgm_is_key_down(SDL_SCANCODE_UP) || _grvgm_is_key_down(SDL_SCANCODE_K);
        case GRVGM_BUTTON_CODE_DOWN:
            return _grvgm_is_key_down(SDL_SCANCODE_DOWN) || _grvgm_is_key_down(SDL_SCANCODE_J);
        case GRVGM_BUTTON_CODE_A:
            return _grvgm_is_key_down(SDL_SCANCODE_F);
        case GRVGM_BUTTON_CODE_B:
            return _grvgm_is_key_down(SDL_SCANCODE_D);
        case GRVGM_BUTTON_CODE_X:
            return _grvgm_is_key_down(SDL_SCANCODE_S);
        case GRVGM_BUTTON_CODE_Y:
            return _grvgm_is_key_down(SDL_SCANCODE_A);
        default:
            return false;
    }
}

void grvgm_poll_keyboard() {
    int num_keys = 0;
    const u8* keyboard_state = SDL_GetKeyboardState(&num_keys);
    if (_grvgm_previous_keyboard_state == NULL) {
        _grvgm_previous_keyboard_state = grv_alloc_zeros(num_keys);
        _grvgm_current_keyboard_state = grv_alloc_zeros(num_keys);

        memcpy(_grvgm_previous_keyboard_state, keyboard_state, num_keys);
        memcpy(_grvgm_current_keyboard_state, keyboard_state, num_keys);
    } else {
        memcpy(_grvgm_previous_keyboard_state, _grvgm_current_keyboard_state, num_keys); 
        memcpy(_grvgm_current_keyboard_state, keyboard_state, num_keys);
    }
}

int _grvgm_char_to_sdl_scancode(char key) {
    switch(key) {
        case 'n': return SDL_SCANCODE_N;
        case 'p': return SDL_SCANCODE_P;
        default: return -1;
    }
}

bool grvgm_was_key_pressed(char key) {
    int scancode = _grvgm_char_to_sdl_scancode(key);
    if (scancode < 0) return false;
    return _grvgm_previous_keyboard_state[scancode] == 0 && _grvgm_current_keyboard_state[scancode] != 0;
}


//==============================================================================
// debug ui
//==============================================================================
char* _grvgm_button_sprite_data = 
"07777700""07777700""07777700""07777700"
"77707770""77707770""77707770""77707770"
"77007770""77700770""77000770""77707770"
"70000070""70000070""70000070""70000070"
"77007770""77700770""77707770""77000770"
"77707770""77707770""77707770""77707770"
"07777700""07777700""07777700""07777700"
"00000000""00000000""00000000""00000000"

"03333300""03333300""03333300""03333300"
"33303330""33303330""33303330""33303330"
"33003330""33300330""33000330""33303330"
"30000030""30000030""30000030""30000030"
"33003330""33300330""33303330""33000330"
"33303330""33303330""33303330""33303330"
"03333300""03333300""03333300""03333300"
"00000000""00000000""00000000""00000000";

typedef grv_frame_buffer_t grv_framebuffer_t;

void grvgm_draw_button_state(grv_framebuffer_t* fb) {
    static grv_spritesheet8_t sprite_sheet = {0};
    if (sprite_sheet.img.pixel_data == NULL) {
        sprite_sheet = grv_spritesheet8_from_str(grv_str_ref(_grvgm_button_sprite_data), 32, 16, 8, 8);
    }

    const i32 left_row_idx = grvgm_is_button_down(GRVGM_BUTTON_CODE_LEFT) ? 1 : 0;
    const i32 right_row_idx = grvgm_is_button_down(GRVGM_BUTTON_CODE_RIGHT) ? 1 : 0;
    const i32 up_row_idx = grvgm_is_button_down(GRVGM_BUTTON_CODE_UP) ? 1 : 0;
    const i32 down_row_idx = grvgm_is_button_down(GRVGM_BUTTON_CODE_DOWN) ? 1 : 0;
    grv_img8_t spr_left = grv_spritesheet8_get_img8(&sprite_sheet, left_row_idx, 0);
    grv_img8_t spr_right = grv_spritesheet8_get_img8(&sprite_sheet, right_row_idx, 1);
    grv_img8_t spr_up = grv_spritesheet8_get_img8(&sprite_sheet, up_row_idx, 2);
    grv_img8_t spr_down = grv_spritesheet8_get_img8(&sprite_sheet, down_row_idx, 3);

    i32 x = 0;
    i32 y = 0;
    i32 w = 8;
    grv_frame_buffer_fill_rect_u8(fb, (recti_t){x,y,4*w,w}, 0);
    grv_framebuffer_blit_img8(fb, &spr_left, x, y);
    grv_framebuffer_blit_img8(fb, &spr_right, x + w, y);
    grv_framebuffer_blit_img8(fb, &spr_up, x + 2*w, y);
    grv_framebuffer_blit_img8(fb, &spr_down, x + 3*w, y);
}
    
//==============================================================================
// drawing api
//==============================================================================
void grvgm_cls(u8 color) {
    grv_frame_buffer_fill_u8(_grvgm_state.framebuffer, color);
} 

void grvgm_spr(i32 number, f32 x, f32 y) {
    grv_img8_t img = grv_spritesheet8_get_img8_by_number(&_grvgm_state.spritesheet, number);
    grv_framebuffer_blit_img8(&_grvgm_state.window->frame_buffer, &img, grv_round_f32(x), grv_round_f32(y));   
}

void grvgm_pset(f32 x, f32 y, u8 color) {
    grv_frame_buffer_set_pixel_u8(&_grvgm_state.window->frame_buffer, vec2i_make(grv_round_f32(x), grv_round_f32(y)), color);
}

//==============================================================================
// main loop
//==============================================================================
void _grvgm_init() {
    _grvgm_state.window = grv_window_new(128,128, 8.0f, grv_str_ref(""));
    _grvgm_state.framebuffer = &_grvgm_state.window->frame_buffer;
    grv_window_t* w = _grvgm_state.window;
    grv_color_palette_init_with_type(&w->frame_buffer.palette, GRV_COLOR_PALETTE_PICO8);
    w->borderless = true;
    grv_window_show(w);
    _grvgm_state.font = grv_get_cozette_font();
    grv_log_info(grv_str_ref("Loading sprite sheet..."));
    _grvgm_state.spritesheet.spr_w = 8;
    _grvgm_state.spritesheet.spr_h = 8;
    grv_error_t err;
    bool success = grv_spritesheet8_load_from_bmp(grv_str_ref("assets/spritesheet.bmp"), &_grvgm_state.spritesheet, &err);
    if (success == false) {
        grv_abort(err);
    }
} 

int grvgm_main(int argc, char** argv) {
    _grvgm_init();

    on_init();

    u64 last_timestamp = SDL_GetTicks64();
    f32 delta_time = 0.0f;
    bool pause = true;
    bool show_debug_ui = false;

    grv_window_t* w = _grvgm_state.window;
    grv_frame_buffer_t* fb = &w->frame_buffer;

    bool first_iteration = true;

    while (true) {
        grv_window_poll_events();
        grvgm_poll_keyboard();

        if (grvgm_was_key_pressed('p')) {
            pause = !pause;
        }

        if (_grvgm_state.window->should_close) {
            break;
        }

        const u64 current_timestamp = SDL_GetTicks64();

        if (first_iteration) {
            _grvgm_state.game_time = 0;
            first_iteration = false;
        } else if (pause == false || grvgm_was_key_pressed('n')) {
            const u64 delta_timestamp = current_timestamp - _grvgm_state.timestamp;
            _grvgm_state.game_time += delta_timestamp;
            delta_time = (f32)delta_timestamp / 1000.0f; 
            on_update(delta_time);
        }

        _grvgm_state.timestamp = current_timestamp;

        on_draw();

        if (show_debug_ui) {
            grvgm_draw_button_state(fb);
        }

        grv_window_present(w);
        grv_sleep(0.03);
    }

    return 0;
}

