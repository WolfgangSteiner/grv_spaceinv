#include "grvgm.h"
#include "grv/grv_log.h"
#include "grv/grv_fs.h"
#include "grv_gfx/grv_bitmap_font.h"
#include "grv_gfx/grv_framebuffer.h"
#include "grv_gfx/grv_img8.h"
#include "grv_gfx/grv_spritesheet8.h"
#include <SDL2/SDL.h>

typedef void (*grvgm_on_init_func)(void**, size_t*);
typedef void (*grvgm_on_update_func)(void*, f32);
typedef void (*grvgm_on_draw_func)(void*);

typedef struct {
    grv_window_t* window;
    grv_framebuffer_t* framebuffer;
    grv_bitmap_font_t* font;
    grv_spritesheet8_t spritesheet;
    u64 spritesheet_mod_time;
    u64 spritesheet_timestamp;
    u64 timestamp;
    void* lib_handle;
    u64 game_time_ms;
    u64 frame_index;
    grvgm_on_init_func on_init;
    grvgm_on_update_func on_update;
    grvgm_on_draw_func on_draw;
    void* game_state;
    size_t game_state_size;
    struct {
        size_t capacity;
        size_t size;
        u8* data;
    } game_state_store;
    struct {
        bool show_frame_time;
    } options;
} grvgm_state_t;

typedef struct {
    u64 frame_index;
    u64 game_time_ms;
} grvgm_frame_info_t;

static grvgm_state_t _grvgm_state = {0};
char* _grv_spritesheet_file_path = "assets/spritesheet.bmp";
static u8* _grvgm_previous_keyboard_state = NULL;
static u8* _grvgm_current_keyboard_state = NULL;


grv_framebuffer_t* _grvgm_framebuffer(void) {
    return &_grvgm_state.window->framebuffer;
}

//==============================================================================
// hot-loading of game code
//==============================================================================
int _grvgm_load_game_code(char* path) {
    if (_grvgm_state.lib_handle) {
        _grvgm_state.on_init = NULL;
        _grvgm_state.on_update = NULL;
        _grvgm_state.on_draw = NULL;
        SDL_UnloadObject(_grvgm_state.lib_handle);
        _grvgm_state.lib_handle = NULL;
    }

    _grvgm_state.lib_handle = SDL_LoadObject(path);
    if (_grvgm_state.lib_handle == NULL) {
        printf("[ERROR] Could not open dynamic library %s.\n", path);
        exit(1);
    }

    _grvgm_state.on_init = (grvgm_on_init_func)SDL_LoadFunction(_grvgm_state.lib_handle, "on_init");
    _grvgm_state.on_update = (grvgm_on_update_func)SDL_LoadFunction(_grvgm_state.lib_handle, "on_update");
    _grvgm_state.on_draw = (grvgm_on_draw_func)SDL_LoadFunction(_grvgm_state.lib_handle, "on_draw");
    
    grv_assert(_grvgm_state.on_init);
    grv_assert(_grvgm_state.on_update);
    grv_assert(_grvgm_state.on_draw);
    return 0;
}



//==============================================================================
// Keyboard and button state
//=============================================================ago=================
bool _grvgm_is_sdl_key_down(int scancode) {
    if (_grvgm_current_keyboard_state == NULL) return false;
    return _grvgm_current_keyboard_state[scancode] != 0;
}

bool grvgm_is_button_down(grvgm_button_code_t button_code) {
    switch (button_code) {
        case GRVGM_BUTTON_CODE_LEFT:
            return _grvgm_is_sdl_key_down(SDL_SCANCODE_LEFT) || _grvgm_is_sdl_key_down(SDL_SCANCODE_H);
        case GRVGM_BUTTON_CODE_RIGHT:
            return _grvgm_is_sdl_key_down(SDL_SCANCODE_RIGHT) || _grvgm_is_sdl_key_down(SDL_SCANCODE_L);
        case GRVGM_BUTTON_CODE_UP:
            return _grvgm_is_sdl_key_down(SDL_SCANCODE_UP) || _grvgm_is_sdl_key_down(SDL_SCANCODE_K);
        case GRVGM_BUTTON_CODE_DOWN:
            return _grvgm_is_sdl_key_down(SDL_SCANCODE_DOWN) || _grvgm_is_sdl_key_down(SDL_SCANCODE_J);
        case GRVGM_BUTTON_CODE_A:
            return _grvgm_is_sdl_key_down(SDL_SCANCODE_F) || _grvgm_is_sdl_key_down(SDL_SCANCODE_SPACE);
        case GRVGM_BUTTON_CODE_B:
            return _grvgm_is_sdl_key_down(SDL_SCANCODE_D);
        case GRVGM_BUTTON_CODE_X:
            return _grvgm_is_sdl_key_down(SDL_SCANCODE_S);
        case GRVGM_BUTTON_CODE_Y:
            return _grvgm_is_sdl_key_down(SDL_SCANCODE_A);
        default:
            return false;
    }
}

void grvgm_poll_keyboard(void) {
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
    if (key >= 'a' && key <= 'z') return SDL_SCANCODE_A + (key - 'a');
    switch(key) {
        default: return -1;
    }
}

bool grvgm_was_key_pressed(char key) {
    int scancode = _grvgm_char_to_sdl_scancode(key);
    if (scancode < 0) return false;
    return _grvgm_previous_keyboard_state[scancode] == 0 && _grvgm_current_keyboard_state[scancode] != 0;
}

bool grvgm_is_key_down(char key) {
    int scancode = _grvgm_char_to_sdl_scancode(key);
    if (scancode < 0) return false;
    return _grvgm_current_keyboard_state[scancode] != 0;
}

bool grvgm_is_keymod_down(grvgm_keymod_t keymod) {
    if (keymod & GRVGM_KEYMOD_SHIFT_LEFT) {
        if (_grvgm_current_keyboard_state[SDL_SCANCODE_LSHIFT]) return true;
    }
    if (keymod & GRVGM_KEYMOD_SHIFT_RIGHT) {
        if (_grvgm_current_keyboard_state[SDL_SCANCODE_RSHIFT]) return true;
    }
    if (keymod & GRVGM_KEYMOD_CTRL_LEFT) {
        if (_grvgm_current_keyboard_state[SDL_SCANCODE_LCTRL]) return true;
    }
    if (keymod & GRVGM_KEYMOD_CTRL_RIGHT) {
        if (_grvgm_current_keyboard_state[SDL_SCANCODE_RCTRL]) return true;
    }
    if (keymod & GRVGM_KEYMOD_ALT_LEFT) {
        if (_grvgm_current_keyboard_state[SDL_SCANCODE_LALT]) return true;
    }
    if (keymod & GRVGM_KEYMOD_ALT_RIGHT) {
        if (_grvgm_current_keyboard_state[SDL_SCANCODE_RALT]) return true;
    }
    return false;
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
    grv_framebuffer_fill_rect_u8(fb, (recti_t){x,y,4*w,w}, 0);
    grv_framebuffer_blit_img8(fb, &spr_left, x, y);
    grv_framebuffer_blit_img8(fb, &spr_right, x + w, y);
    grv_framebuffer_blit_img8(fb, &spr_up, x + 2*w, y);
    grv_framebuffer_blit_img8(fb, &spr_down, x + 3*w, y);
}
    
//==============================================================================
// api
//==============================================================================
void grvgm_clear_screen(u8 color) {
    grv_framebuffer_fill_u8(_grvgm_state.framebuffer, color);
} 

void grvgm_draw_sprite(grvgm_sprite_t sprite) {
    grv_spritesheet8_t* spritesheet = sprite.spritesheet ? sprite.spritesheet : &_grvgm_state.spritesheet;
    grv_img8_t img = grv_spritesheet8_get_img8_by_index(spritesheet, sprite.index);
    grv_framebuffer_blit_img8(
        &_grvgm_state.window->framebuffer,
        &img,
        grv_fixed32_round(sprite.pos.x),
        grv_fixed32_round(sprite.pos.y)
    );
}

void grvgm_draw_pixel(grv_vec2_fixed32_t pos, u8 color) {
    grv_framebuffer_set_pixel_u8(
        &_grvgm_state.window->framebuffer,
        vec2i_make(grv_fixed32_round(pos.x), grv_fixed32_round(pos.y)),
        color
    );
}

void grvgm_draw_rect(grv_rect_fixed32_t rect, u8 color) {
    recti_t r = {
        .x=grv_fixed32_round(rect.x),
        .y=grv_fixed32_round(rect.y),
        .w=grv_fixed32_round(rect.w),
        .h=grv_fixed32_round(rect.h)
    };
    grv_framebuffer_draw_rect_u8(&_grvgm_state.window->framebuffer, r, color);
}

void grvgm_draw_circle(grv_vec2_fixed32_t pos, grv_fixed32_t r, u8 color) {
    grv_framebuffer_draw_circle_u8(
        &_grvgm_state.window->framebuffer, 
        grv_fixed32_round(pos.x),
        grv_fixed32_round(pos.y),
        grv_fixed32_round(r),
        color);
}

void grvgm_fill_circle(grv_vec2_fixed32_t pos, grv_fixed32_t r, u8 color) {
    grv_framebuffer_fill_circle_u8(
        &_grvgm_state.window->framebuffer, 
        grv_fixed32_round(pos.x),
        grv_fixed32_round(pos.y),
        grv_fixed32_round(r),
        color);
}


grv_vec2_fixed32_t grvgm_screen_size(void) {
    i32 w = _grvgm_state.window->framebuffer.width;
    i32 h = _grvgm_state.window->framebuffer.height;
    return grv_vec2_fixed32_from_i32(w, h);
}

grv_fixed32_t grvgm_time(void) {
    return (grv_fixed32_t){.val = (i32)(_grvgm_state.game_time_ms * 1024 / 1000)};
}

grv_fixed32_t grvgm_timediff(grv_fixed32_t timestamp) {
    return grv_fixed32_sub(grvgm_time(), timestamp);
}

void grvgm_draw_text(grv_str_t text, grv_vec2_fixed32_t pos, u8 color) {
    grv_put_text_u8(
        _grvgm_framebuffer(),
        text,
        grv_vec2_fixed32_round(pos),
        _grvgm_state.font,
        color
    );
}

//==============================================================================
// spritesheet hot loading
//==============================================================================
u64 _grvgm_spritesheet_mod_time(void) {
    grv_u64_result_t result = grv_fs_file_mod_time(grv_str_ref(_grv_spritesheet_file_path));
    if (!result.valid) {
        grv_abort(result.error);
    }
    return result.value;
}

void _grvgm_load_spritesheet(void) {
    grv_log_info(grv_str_ref("Loading sprite sheet."));
    _grvgm_state.spritesheet.spr_w = 8;
    _grvgm_state.spritesheet.spr_h = 8;
    grv_error_t err;
    bool success = grv_spritesheet8_load_from_bmp(grv_str_ref(_grv_spritesheet_file_path), &_grvgm_state.spritesheet, &err);
    if (success == false) {
        grv_abort(err);
    }
    _grvgm_state.spritesheet_mod_time = _grvgm_spritesheet_mod_time();
    _grvgm_state.spritesheet_timestamp = SDL_GetTicks64();
}

void _grvgm_check_reload_spritesheet(void) {
    u64 timestamp = SDL_GetTicks64();
    if (timestamp - _grvgm_state.spritesheet_timestamp > 1000) {
        u64 mod_time = _grvgm_spritesheet_mod_time();
        if (mod_time > _grvgm_state.spritesheet_mod_time) {
            _grvgm_load_spritesheet();
        }
        _grvgm_state.spritesheet_timestamp = timestamp;
    }
}

//==============================================================================
// main loop
//==============================================================================
void _grvgm_init(void) {
    _grvgm_load_game_code("libspaceinv.so");
    _grvgm_load_spritesheet();
    grv_window_t* w = grv_window_new(128,128, 2.0f, grv_str_ref(""));
    _grvgm_state.window = w;
    w->horizontal_align = GRV_WINDOW_HORIZONTAL_ALIGN_RIGHT;
    w->vertical_align = GRV_WINDOW_VERTICAL_ALIGN_TOP;
    _grvgm_state.framebuffer = &w->framebuffer;
    grv_color_palette_init_with_type(&w->framebuffer.palette, GRV_COLOR_PALETTE_PICO8);
    w->borderless = true;
    w->resizable = true;
    grv_window_show(w);
    _grvgm_state.font = grv_get_cozette_font();
    _grvgm_state.game_state_store.capacity = 1024ull*1024ull*1024ull;
    _grvgm_state.game_state_store.size = 0;
    _grvgm_state.game_state_store.data = grv_alloc(_grvgm_state.game_state_store.capacity);
} 

void _grvgm_push_game_state(void) {
    size_t element_size = _grvgm_state.game_state_size + sizeof(grvgm_frame_info_t);
    size_t current_size = _grvgm_state.game_state_store.size * element_size;
    size_t new_size = current_size + element_size;
    if (new_size > _grvgm_state.game_state_store.capacity) {
        _grvgm_state.game_state_store.capacity *= 2;
        _grvgm_state.game_state_store.data = grv_realloc(
            _grvgm_state.game_state_store.data,
            _grvgm_state.game_state_store.capacity);
    }
    u8* dst = _grvgm_state.game_state_store.data + current_size;
    grvgm_frame_info_t frame_info = {
        .frame_index=_grvgm_state.frame_index,
        .game_time_ms=_grvgm_state.game_time_ms
    };
    *(grvgm_frame_info_t*)dst = frame_info;
    dst += sizeof(grvgm_frame_info_t);
    grv_memcpy(dst, _grvgm_state.game_state, _grvgm_state.game_state_size);
    _grvgm_state.game_state_store.size++;
}

void _grvgm_pop_game_state(u64 count) {
    size_t num_states = _grvgm_state.game_state_store.size;
    if (num_states == 0) return;
    u64 new_frame_index = num_states < count ? 0 : num_states - count;
    size_t element_size = _grvgm_state.game_state_size + sizeof(grvgm_frame_info_t);
    u8* src = _grvgm_state.game_state_store.data + new_frame_index * element_size;
    grvgm_frame_info_t frame_info = *(grvgm_frame_info_t*)src;
    src += sizeof(grvgm_frame_info_t);
    grv_memcpy(_grvgm_state.game_state, src, _grvgm_state.game_state_size);
    _grvgm_state.frame_index = frame_info.frame_index;
    _grvgm_state.game_time_ms = frame_info.game_time_ms;
    _grvgm_state.game_state_store.size = new_frame_index + 1;
}

void _grvgm_reset_game_state_store(void) {
    _grvgm_state.game_state_store.size = 0;
}


void _grvgm_draw_frame_time(i32 frame_time_ms) {
    char fps_string[16];
    snprintf(fps_string, 16, "%2d", (int)frame_time_ms);
    grvgm_draw_text(grv_str_ref(fps_string), grv_vec2_fixed32_from_i32(114,1), 7);
}

void _grvgm_parse_command_line(int argc, char** argv) {
    grv_strarr_t args = grv_strarr_new_from_cstrarr(argv, argc);
    for (i32 i = 1; i < args.size; i++) {
        grv_str_t arg = *grv_strarr_at(args, i);
        if (grv_str_eq_cstr(arg, "--show-frame-time")) {
            _grvgm_state.options.show_frame_time = true;
        } else {
            grv_str_t error_msg = grv_str_format(grv_str_ref("Unknown option {str}"), arg);
            grv_exit(error_msg);
        }
    }
}

int grvgm_main(int argc, char** argv) {
    GRV_UNUSED(argc);
    GRV_UNUSED(argv);

    _grvgm_parse_command_line(argc, argv);

    _grvgm_init();
    _grvgm_state.on_init(&_grvgm_state.game_state, &_grvgm_state.game_state_size);
    printf("[INFO] game_state_size: %d (%.2fk/s)\n",
           (int)_grvgm_state.game_state_size,
           (f32)_grvgm_state.game_state_size*32.0f/1024.0f);
    //u64 last_timestamp = SDL_GetTicks64();
    bool pause = true;
    bool show_debug_ui = false;

    grv_window_t* w = _grvgm_state.window;
    grv_framebuffer_t* fb = &w->framebuffer;

    bool first_iteration = true;
    bool pause_has_been_activated = false;

    while (true) {
        u64 frame_start_time_ms = SDL_GetTicks64();
        grv_window_poll_events();
        grvgm_poll_keyboard();
        _grvgm_check_reload_spritesheet();

        if (grvgm_was_key_pressed('r') && grvgm_is_keymod_down(GRVGM_KEYMOD_CTRL)) {
            _grvgm_load_game_code("libspaceinv.so");
            printf("[INFO] Reloaded game code.\n");
            if (grvgm_is_keymod_down(GRVGM_KEYMOD_SHIFT)) {
                printf("[INFO] Resetting game state.\n");
                if (_grvgm_state.game_state) {
                    grv_free(_grvgm_state.game_state);
                }
                _grvgm_state.on_init(&_grvgm_state.game_state, &_grvgm_state.game_state_size);
                _grvgm_reset_game_state_store();
            }
        }

        if (grvgm_was_key_pressed('p') && grvgm_is_keymod_down(GRVGM_KEYMOD_CTRL)) {
            pause = !pause;
            if (pause) pause_has_been_activated = true;
        } else if (!grvgm_is_key_down('p') && pause_has_been_activated) {
            pause_has_been_activated = false;
        }

        if (_grvgm_state.window->should_close) {
            break;
        }

        if (first_iteration) {
            first_iteration = false;
            _grvgm_state.game_time_ms = 0;
            _grvgm_state.on_update(_grvgm_state.game_state, 0.0f);
            _grvgm_push_game_state();
        } else if (pause == false || grvgm_was_key_pressed('n')) {
            const u64 delta_timestamp_ms = _grvgm_state.frame_index % 4 == 0 ? 32 : 31; 
            _grvgm_state.game_time_ms += delta_timestamp_ms;
            _grvgm_state.frame_index++;
            f32 delta_time = (f32)delta_timestamp_ms / 1000.0f; 
            _grvgm_state.on_update(_grvgm_state.game_state, delta_time);
            _grvgm_push_game_state();
        } else if (pause == true && !pause_has_been_activated && grvgm_is_key_down('p')) {
            i32 frames_to_rewind = grvgm_is_keymod_down(GRVGM_KEYMOD_SHIFT) ? 4 : 2;
            _grvgm_pop_game_state(frames_to_rewind);
        } else if (pause == true && grvgm_was_key_pressed('s')) {
            FILE* file = fopen("/tmp/game_state.dat", "wb");
            fwrite(
                _grvgm_state.game_state_store.data,
                _grvgm_state.game_state_size + sizeof(grvgm_frame_info_t),
                _grvgm_state.game_state_store.size,
                file
            );
            fclose(file);
            printf("[INFO] Game state has been saved.\n");
        }

        _grvgm_state.on_draw(_grvgm_state.game_state);

        if (show_debug_ui) {
            grvgm_draw_button_state(fb);
        }

        u64 frame_end_time_ms = SDL_GetTicks64();
        u64 frame_time_ms = frame_end_time_ms - frame_start_time_ms;
        if (_grvgm_state.options.show_frame_time) _grvgm_draw_frame_time(frame_time_ms);

        grv_window_present(w);
        
        u64 frame_target_time_ms = 31;
        if (frame_time_ms < frame_target_time_ms) {
            SDL_Delay(frame_target_time_ms - frame_time_ms);
        }
    }

    return 0;
}
