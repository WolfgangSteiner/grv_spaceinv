#include "grvgm.h"
#include "grv/grv_log.h"
#include "grv/grv_fs.h"
#include "grv/grv_path.h"
#include "grv_gfx/grv_bitmap_font.h"
#include "grv_gfx/grv_framebuffer.h"
#include "grv_gfx/grv_img8.h"
#include "grv_gfx/grv_spritesheet8.h"
#include "grvgm_small_font.h"
#include "grv/grv_arena.h"
#include <SDL2/SDL.h>
#include <zstd.h>
#include <stdatomic.h>

typedef void (*grvgm_on_init_func)(void**, size_t*);
typedef void (*grvgm_on_update_func)(void*, f32);
typedef void (*grvgm_on_draw_func)(void*);
typedef void (*grvgm_on_audio_func)(void*, i16*, i32);

typedef struct {
	u64 game_time_ms;
	i64 frame_index;
    i64 offset;
    i64 size;
} grvgm_frame_info_t;

typedef struct {
	i64 current_frame_index;
    struct {
        i64 capacity;
        i64 initial_capacity;
        i64 size;
        u8* data;
    } frame_data;
    struct {
        i64 capacity;
        i64 initial_capacity;
        i64 size;
        grvgm_frame_info_t* arr;
    } frame_info_data;
} grvgm_game_state_store_t;

typedef struct grvgm_callback_s { 
	void(*func)(void*);
	void* data;
	struct grvgm_callback_s* next;
} grvgm_callback_t;

typedef struct {
	void* handle;
	grvgm_on_init_func on_init;
	grvgm_on_update_func on_update;
	grvgm_on_draw_func on_draw;
	grvgm_on_audio_func on_audio;
	u64 mod_time;
	u64 timestamp;
} grvgm_dylib_t;

typedef struct {
	grvgm_dylib_t dylib;
	grv_window_t* window;
	grv_framebuffer_t* framebuffer;
	grv_bitmap_font_t* font;
	u64 timestamp;
	u64 game_time_ms;
	u64 frame_index;
	void* game_state;
	size_t game_state_size;
    grvgm_game_state_store_t game_state_store;
	grv_str_t executable_path;
	char* dynamic_library_name;
	grv_spritesheet8_t spritesheet;
	u64 spritesheet_mod_time;
	u64 spritesheet_timestamp;
	grv_str_t spritesheet_path;
	struct {
		i32 fps;
		i32 sprite_width;
		i32 screen_width;
		i32 screen_height;
		bool pause_enabled;
		bool show_frame_time;
		bool use_game_state_store;
	} options;
	SDL_AudioDeviceID sdl_audio_device;
	f64 audio_load;
	grv_arena_t* draw_arena;
	struct {
		grvgm_callback_t* root;
		grvgm_callback_t* head;
	} end_of_frame_callback_queue;
} grvgm_state_t;

static grvgm_state_t _grvgm_state = {
	.options = {
		.screen_width=128,
		.screen_height=128,
		.sprite_width=8,
		.fps=60
	}
};
static u8* _grvgm_previous_keyboard_state = NULL;
static u8* _grvgm_current_keyboard_state = NULL;
static u8* _grvgm_block_keyboard_state = NULL;

grv_framebuffer_t* _grvgm_framebuffer(void) {
	return &_grvgm_state.window->framebuffer;
}

grv_window_t* _grvgm_window(void) {
	return _grvgm_state.window;
}

grv_spritesheet8_t* _grvgm_spritesheet(void) {
	return &_grvgm_state.spritesheet;
}

grv_bitmap_font_t* _grvgm_font(void) {
	return _grvgm_state.font;
}

u64 _grvgm_game_time_ms(void) {
	return _grvgm_state.game_time_ms;
}
//==============================================================================
// api
//==============================================================================
#include "grvgm_api.c"

//==============================================================================
// hot-loading of game code
//==============================================================================
grv_u64_result_t _grvgm_dylib_mod_time(void) {
	return grv_fs_file_mod_time(grv_str_ref(_grvgm_state.dynamic_library_name));
}

grvgm_dylib_t _grvgm_dylib_load(void) {
	grvgm_dylib_t lib = {0};
	lib.handle = SDL_LoadObject(_grvgm_state.dynamic_library_name);
	if (lib.handle == NULL) {
		printf("[ERROR] Could not open dynamic library %s.\n", _grvgm_state.dynamic_library_name);
		printf("        %s\n", SDL_GetError());
		return lib;
	}
	grv_u64_result_t mod_time = _grvgm_dylib_mod_time();
	grv_assert(mod_time.valid);
	lib.mod_time = mod_time.value;
	lib.timestamp = grvgm_ticks();

	lib.on_init = (grvgm_on_init_func)SDL_LoadFunction(lib.handle, "on_init");
	lib.on_update = (grvgm_on_update_func)SDL_LoadFunction(lib.handle, "on_update");
	lib.on_draw = (grvgm_on_draw_func)SDL_LoadFunction(lib.handle, "on_draw");
	lib.on_audio = (grvgm_on_audio_func)SDL_LoadFunction(lib.handle, "on_audio");
	return lib;
}

void _grvgm_dylib_unload(grvgm_dylib_t* lib) {
	SDL_UnloadObject(lib->handle);
	*lib = (grvgm_dylib_t) {0};
}


void _grvgm_load_game_code(void) {
	grvgm_dylib_t lib = _grvgm_dylib_load();
	if (lib.handle == NULL) {
		printf("[ERROR] Could not open dynamic library %s.\n", _grvgm_state.dynamic_library_name);
		printf("        %s\n", SDL_GetError());
		exit(1);
	}
	_grvgm_state.dylib = lib;
}

bool _grvgm_dylib_needs_reload(void) {
	grvgm_dylib_t* dylib = &_grvgm_state.dylib;
	u64 ticks = grvgm_ticks();
	if (ticks - dylib->timestamp < 500) return false;
	dylib->timestamp = ticks;
	grv_u64_result_t mod_time = _grvgm_dylib_mod_time();
	if (!mod_time.valid || mod_time.value == dylib->mod_time) return false;
	return true;
}

void _grvgm_check_reload_game_code(void) {
	if (_grvgm_dylib_needs_reload()) {
		SDL_PauseAudioDevice(_grvgm_state.sdl_audio_device, 1);
		_grvgm_dylib_unload(&_grvgm_state.dylib);
		grvgm_dylib_t lib = _grvgm_dylib_load();
		_grvgm_state.dylib = lib;
		grv_assert(_grvgm_state.dylib.handle != NULL);
		SDL_PauseAudioDevice(_grvgm_state.sdl_audio_device, 0);
	}
}

//==============================================================================
// Keyboard and button state
//=============================================================ago=================
bool _grvgm_is_sdl_key_down(int scancode) {
	if (_grvgm_current_keyboard_state == NULL) return false;
	return _grvgm_current_keyboard_state[scancode] != 0;
}

bool _grvgm_was_sdl_key_pressed(int scancode) {
	if (_grvgm_current_keyboard_state == NULL) return false;
	bool was_pressed = _grvgm_current_keyboard_state[scancode] != 0 && _grvgm_previous_keyboard_state[scancode] == 0;
	if (was_pressed) {
		_grvgm_block_keyboard_state[scancode] = 1;
	}
	return was_pressed;
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

bool grvgm_was_button_pressed(grvgm_button_code_t button_code) {
	switch (button_code) {
		case GRVGM_BUTTON_CODE_LEFT:
			return _grvgm_was_sdl_key_pressed(SDL_SCANCODE_LEFT) || _grvgm_was_sdl_key_pressed(SDL_SCANCODE_H);
		case GRVGM_BUTTON_CODE_RIGHT:
			return _grvgm_was_sdl_key_pressed(SDL_SCANCODE_RIGHT) || _grvgm_was_sdl_key_pressed(SDL_SCANCODE_L);
		case GRVGM_BUTTON_CODE_UP:
			return _grvgm_was_sdl_key_pressed(SDL_SCANCODE_UP) || _grvgm_was_sdl_key_pressed(SDL_SCANCODE_K);
		case GRVGM_BUTTON_CODE_DOWN:
			return _grvgm_was_sdl_key_pressed(SDL_SCANCODE_DOWN) || _grvgm_was_sdl_key_pressed(SDL_SCANCODE_J);
		case GRVGM_BUTTON_CODE_A:
			return _grvgm_was_sdl_key_pressed(SDL_SCANCODE_F) || _grvgm_was_sdl_key_pressed(SDL_SCANCODE_SPACE);
		case GRVGM_BUTTON_CODE_B:
			return _grvgm_was_sdl_key_pressed(SDL_SCANCODE_D);
		case GRVGM_BUTTON_CODE_X:
			return _grvgm_was_sdl_key_pressed(SDL_SCANCODE_S);
		case GRVGM_BUTTON_CODE_Y:
			return _grvgm_was_sdl_key_pressed(SDL_SCANCODE_A);
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
		_grvgm_block_keyboard_state = grv_alloc_zeros(num_keys);

		memcpy(_grvgm_previous_keyboard_state, keyboard_state, num_keys);
		memcpy(_grvgm_current_keyboard_state, keyboard_state, num_keys);
	} else {
		memcpy(_grvgm_previous_keyboard_state, _grvgm_current_keyboard_state, num_keys); 
		memcpy(_grvgm_current_keyboard_state, keyboard_state, num_keys);
	}

	for (i32 i = 0; i < num_keys; i++) {
		bool was_blocked = _grvgm_block_keyboard_state[i];
		if (was_blocked && _grvgm_current_keyboard_state[i] == 0) {
			_grvgm_block_keyboard_state[i] = 0;
		} else if (was_blocked && _grvgm_current_keyboard_state[i] != 0) {
			_grvgm_current_keyboard_state[i] = 0;
		}
	}
}

int _grvgm_char_to_sdl_scancode(char key) {
	if (key >= 'a' && key <= 'z') return SDL_SCANCODE_A + (key - 'a');
	switch(key) {
		case ' '  : return SDL_SCANCODE_SPACE;
		case '\t' : return SDL_SCANCODE_TAB;
		default: return -1;
	}
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

bool grvgm_key_was_pressed(char key) {
	int scancode = _grvgm_char_to_sdl_scancode(key);
	if (scancode < 0) return false;
	return _grvgm_was_sdl_key_pressed(scancode);
}

bool grvgm_key_was_pressed_with_mod(char key, u32 mod) {
	return grvgm_is_keymod_down(mod) && grvgm_key_was_pressed(key);
}

bool grvgm_key_is_down(char key) {
	int scancode = _grvgm_char_to_sdl_scancode(key);
	if (scancode < 0) return false;
	return _grvgm_current_keyboard_state[scancode] != 0;
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
	grv_img8_t spr_left = grv_spritesheet8_get_img8(&sprite_sheet, left_row_idx, 0, 1, 1);
	grv_img8_t spr_right = grv_spritesheet8_get_img8(&sprite_sheet, right_row_idx, 1, 1, 1);
	grv_img8_t spr_up = grv_spritesheet8_get_img8(&sprite_sheet, up_row_idx, 2, 1, 1);
	grv_img8_t spr_down = grv_spritesheet8_get_img8(&sprite_sheet, down_row_idx, 3, 1, 1);

	i32 x = 0;
	i32 y = 0;
	i32 w = 8;
	grv_framebuffer_fill_rect_u8(fb, (rect_i32){x,y,4*w,w}, 0);
	grv_framebuffer_blit_img8(fb, &spr_left, x, y);
	grv_framebuffer_blit_img8(fb, &spr_right, x + w, y);
	grv_framebuffer_blit_img8(fb, &spr_up, x + 2*w, y);
	grv_framebuffer_blit_img8(fb, &spr_down, x + 3*w, y);
}
	
//==============================================================================
// spritesheet hot loading
//==============================================================================
u64 _grvgm_spritesheet_mod_time(void) {
	grv_u64_result_t result = grv_fs_file_mod_time(_grvgm_state.spritesheet_path);
	if (!result.valid) {
		grv_abort(result.error);
	}
	return result.value;
}

void _grvgm_load_spritesheet(void) {
	grv_log_info(grv_str_ref("Loading sprite sheet."));
	i32 sprite_width = _grvgm_state.options.sprite_width;
	_grvgm_state.spritesheet.spr_w = sprite_width;
	_grvgm_state.spritesheet.spr_h = sprite_width;
	grv_error_t err;
	bool success = grv_spritesheet8_load_from_bmp(_grvgm_state.spritesheet_path, &_grvgm_state.spritesheet, &err);
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
// game state store
//==============================================================================

void _grvgm_game_state_store_init(void) {
    grvgm_game_state_store_t* store = &_grvgm_state.game_state_store;
	store->frame_data.initial_capacity = 1 * GRV_MEGABYTES;
	store->frame_data.capacity = store->frame_data.initial_capacity;
	store->frame_data.size = 0;
	store->frame_data.data = grv_alloc(store->frame_data.capacity);

    store->frame_info_data.initial_capacity = 2<<13;
    store->frame_info_data.capacity = store->frame_info_data.initial_capacity;
    store->frame_info_data.size = 0;
    store->frame_info_data.arr = grv_alloc(
        store->frame_info_data.capacity * sizeof(grvgm_frame_info_t));
}

void _grvgm_game_state_store_reset_size(i32 new_frame_index) {
    grvgm_game_state_store_t* store = &_grvgm_state.game_state_store;
    store->frame_info_data.size = new_frame_index;
	if (store->frame_info_data.size == 0) {
		store->frame_data.size = 0;
	} else {
		size_t prev_frame_idx = new_frame_index - 1;
		grvgm_frame_info_t prev_frame_info = store->frame_info_data.arr[prev_frame_idx];
		store->frame_data.size = prev_frame_info.offset + prev_frame_info.size;
	}
}

void _grvgm_game_state_push(void) {
	if (!_grvgm_state.options.use_game_state_store) return;
    grvgm_game_state_store_t* store = &_grvgm_state.game_state_store;

	if (store->current_frame_index < store->frame_info_data.size - 1) {
		_grvgm_game_state_store_reset_size(store->current_frame_index);
	}

    if (store->frame_info_data.size >= store->frame_info_data.capacity) {
        store->frame_info_data.capacity *= 2;
        store->frame_info_data.arr = grv_realloc(
            store->frame_info_data.arr,
            store->frame_info_data.capacity * sizeof(grvgm_frame_info_t));
    }

    grvgm_frame_info_t* frame_info = &store->frame_info_data.arr[store->frame_info_data.size++];
	*frame_info = (grvgm_frame_info_t){
		.frame_index=_grvgm_state.frame_index,
		.game_time_ms=_grvgm_state.game_time_ms,
        .offset = store->frame_data.size,
	};

	store->current_frame_index++;

	i32 max_data_size = ZSTD_compressBound(_grvgm_state.game_state_size);
	while (store->frame_data.size + max_data_size > store->frame_data.capacity) {
		store->frame_data.capacity *= 2;
		store->frame_data.data = grv_realloc(store->frame_data.data, store->frame_data.capacity);
        grv_log_info_cstr("Reallocating game state store.");
	}
	u8* dst = store->frame_data.data + frame_info->offset;
	size_t compressed_size = ZSTD_compress(
		dst, max_data_size, _grvgm_state.game_state, _grvgm_state.game_state_size, 1);

	store->frame_data.size += compressed_size;
	frame_info->size = compressed_size;
}

void _grvgm_game_state_restore(grvgm_frame_info_t frame_info) {
	u8* src = _grvgm_state.game_state_store.frame_data.data + frame_info.offset;
	size_t decompressed_size = ZSTD_decompress(
		_grvgm_state.game_state,
		_grvgm_state.game_state_size,
		src,
		frame_info.size);
	grv_assert(decompressed_size == _grvgm_state.game_state_size);
	_grvgm_state.frame_index = frame_info.frame_index;
	_grvgm_state.game_time_ms = frame_info.game_time_ms;
}

void _grvgm_game_state_jump(i32 delta) {
    grvgm_game_state_store_t* store = &_grvgm_state.game_state_store;
	size_t num_states = store->frame_info_data.size;
	if (num_states == 0) return;
	i32 new_frame_index = grv_clamp_i32(store->current_frame_index + delta, 0, num_states - 1);
    grvgm_frame_info_t frame_info = store->frame_info_data.arr[new_frame_index];
	_grvgm_game_state_restore(frame_info);
	store->current_frame_index = new_frame_index;
}

void _grvgm_game_state_pop(u64 count) {
    grvgm_game_state_store_t* store = &_grvgm_state.game_state_store;
	size_t num_states = store->frame_info_data.size;
	if (num_states == 0) return;
	u64 new_frame_index = num_states < count ? 0 : num_states - count;
    grvgm_frame_info_t frame_info = store->frame_info_data.arr[new_frame_index];
	_grvgm_game_state_restore(frame_info);
	_grvgm_game_state_store_reset_size(new_frame_index);
}

void _grvgm_game_state_reset_store(void) {
    grvgm_game_state_store_t* store = &_grvgm_state.game_state_store;
	store->frame_data.size = 0;
    grv_free(store->frame_data.data);
    store->frame_data.capacity = store->frame_data.initial_capacity;
    store->frame_data.data = grv_alloc(store->frame_data.capacity);

    store->frame_info_data.size = 0;
    grv_free(store->frame_info_data.arr);
    store->frame_info_data.capacity = store->frame_info_data.initial_capacity;
    store->frame_info_data.arr = grv_alloc(store->frame_info_data.capacity * sizeof(grvgm_frame_info_t));
}

//==============================================================================
// audio
//==============================================================================
void _grvgm_audio_callback(void* userdata, u8* buffer, i32 buffer_num_bytes) {
	GRV_UNUSED(userdata);
	i32 buffer_num_frames = buffer_num_bytes / 2 / sizeof(i16);
	if (_grvgm_state.dylib.on_audio) {
		u64 audio_frame_start_counter = SDL_GetPerformanceCounter();
		_grvgm_state.dylib.on_audio(
			_grvgm_state.game_state,
			(i16*)buffer,
			buffer_num_frames);
		u64 audio_frame_end_counter = SDL_GetPerformanceCounter();
		f64 audio_frame_time = (f64)(audio_frame_end_counter - audio_frame_start_counter) / SDL_GetPerformanceFrequency();
		f64 max_audio_frame_time = (f64)buffer_num_frames / (f64)GRVGM_SAMPLE_RATE;
		f64 audio_load = audio_frame_time / max_audio_frame_time;
		_grvgm_state.audio_load = _grvgm_state.audio_load * 0.99 + audio_load * 0.01;
	} else {
		memset(buffer, 0, buffer_num_bytes);
	}
}

void _grvgm_init_audio(void) {
	SDL_Init(SDL_INIT_AUDIO);
    struct SDL_AudioSpec want = {
        .freq = GRVGM_SAMPLE_RATE,
        .format = AUDIO_S16SYS,
        .channels = 2,         // Stereo
        .samples = 512,
        .callback = _grvgm_audio_callback,
        .userdata = NULL,
    };

	struct SDL_AudioSpec obtained = {0};

    _grvgm_state.sdl_audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &obtained, 0);

	printf("[INFO] Audio device opened with buffer size %d.\n", obtained.samples);

    SDL_PauseAudioDevice(_grvgm_state.sdl_audio_device, 0);
}

//==============================================================================
// main loop
//==============================================================================
void _grvgm_parse_command_line(int argc, char** argv) {
	grv_strarr_t args = grv_strarr_new_from_cstrarr(argv, argc);
	i32 i = 1;
	while (i < args.size) {
		grv_str_t arg = *grv_strarr_at(args, i);
		if (grv_str_starts_with_cstr(arg, "--fps=")) {
			grv_str_t fps_str = grv_str_split_tail_at_char(arg, '=');
			if (!grv_str_is_int(fps_str)) {
				grv_str_t error_msg = grv_str_format_cstr("Invalid syntax: {str}", arg);
				grv_exit(error_msg);
			}
			_grvgm_state.options.fps = grv_min_i32(grv_str_to_int(fps_str), 60);
		} else {
			grv_str_t error_msg = grv_str_format_cstr("Unknown option {str}", arg);
			grv_exit(error_msg);
		}
		i++;
	}
}

void _grvgm_init(int argc, char** argv) {
	_grvgm_parse_command_line(argc, argv);
	_grvgm_state.executable_path = grv_str_ref(argv[0]);
	grv_str_t executable_filename = grv_path_basename(_grvgm_state.executable_path);
	grv_str_t dynamic_library_name = grv_str_format_cstr("build/lib{str}.so", executable_filename);
	_grvgm_state.dynamic_library_name = grv_str_copy_to_cstr(dynamic_library_name);
	grv_str_t spritesheet_path = grv_str_format_cstr("assets/{str}_spritesheet.bmp", executable_filename);
	if (grv_file_exists(spritesheet_path)) {
		_grvgm_state.spritesheet_path = spritesheet_path;
	} else {
		_grvgm_state.spritesheet_path = grv_str_ref("assets/spritesheet.bmp");
		grv_str_free(&spritesheet_path);
	}
	grv_str_free(&dynamic_library_name);
	grv_str_free(&executable_filename);
    _grvgm_game_state_store_init();
	_grvgm_state.draw_arena = grv_alloc_zeros(sizeof(grv_arena_t));
	grv_arena_init(_grvgm_state.draw_arena, 1 * GRV_MEGABYTES);
}

void _grvgm_init_gfx() {
	_grvgm_load_spritesheet();
	grv_window_t* w = grv_window_new(
		_grvgm_state.options.screen_width,
		_grvgm_state.options.screen_height,
		2.0f, grv_str_ref(""));
	_grvgm_state.window = w;
	w->horizontal_align = GRV_WINDOW_HORIZONTAL_ALIGN_RIGHT;
	w->vertical_align = GRV_WINDOW_VERTICAL_ALIGN_TOP;
	_grvgm_state.framebuffer = &w->framebuffer;
	grv_color_palette_init_with_type(&w->framebuffer.palette, GRV_COLOR_PALETTE_PICO8);
	w->borderless = true;
	w->resizable = true;
	grv_window_show(w);
	_grvgm_state.font = grvgm_get_small_font();
}

i32 _grvgm_target_frame_time_ms(void) {
	i32 fps = _grvgm_state.options.fps;
	u64 frame_index = _grvgm_state.frame_index;
	if (fps == 30) {
		return frame_index % 3 == 0 ? 34 : 33;
	} else if (fps == 32) {
		return frame_index % 4 == 0 ? 32 : 31;
	} else if (fps == 60) {
		return frame_index % 3 ? 16 : 17;
	} else {
		return 1000 / fps;
	}
}

void _grvgm_draw_frame_statistics(f64 frame_time) {
	char str[16];
	snprintf(str, 16, "gfx %0.2f", (f32)(frame_time*60.0));
	grvgm_draw_text((vec2_i32){96,0}, grv_str_ref(str), 6);
	snprintf(str, 16, "snd %0.2f", (f32)_grvgm_state.audio_load);
	grvgm_draw_text((vec2_i32){96,6}, grv_str_ref(str), 6);
}

void _grvgm_execute_end_of_frame_callback_queue() {
	grvgm_callback_t** root = &_grvgm_state.end_of_frame_callback_queue.root;
	grvgm_callback_t** head = &_grvgm_state.end_of_frame_callback_queue.head;
	grvgm_callback_t* iter = *root;
	while (iter) {
		iter->func(iter->data);
		iter = iter->next;
	}
	*root = NULL;
	*head = NULL;
}

void _grvgm_on_update(f32 dt) {
	if (_grvgm_state.dylib.on_update) {
		_grvgm_state.dylib.on_update(_grvgm_state.game_state, dt);
		_grvgm_game_state_push();
	}
}

int grvgm_main(int argc, char** argv) {
	_grvgm_init(argc, argv);
	_grvgm_load_game_code();
	if (_grvgm_state.dylib.on_init)
		_grvgm_state.dylib.on_init(&_grvgm_state.game_state, &_grvgm_state.game_state_size);
	_grvgm_init_gfx();
	_grvgm_init_audio();

	printf("[INFO] game_state_size: %d (%.2fk/s)\n",
		   (int)_grvgm_state.game_state_size,
		   (f32)_grvgm_state.game_state_size * _grvgm_state.options.fps / 1024.0f);
	//u64 last_timestamp = SDL_GetTicks64();
	_grvgm_state.options.pause_enabled = _grvgm_state.game_state != NULL;

	bool pause = false;
	bool show_debug_ui = false;

	grv_window_t* w = _grvgm_state.window;
	grv_framebuffer_t* fb = &w->framebuffer;

	bool show_statistics = false;
	bool first_iteration = true;

	while (true) {
		u64 frame_start_counter = SDL_GetPerformanceCounter();
		grv_window_poll_events();
		grvgm_poll_keyboard();
		_grvgm_check_reload_spritesheet();
		_grvgm_check_reload_game_code();

		if (grvgm_key_was_pressed_with_mod('r', GRVGM_KEYMOD_CTRL)) {
			_grvgm_load_game_code();
			grv_log_info_cstr("Reloaded game code.");
			if (_grvgm_state.options.use_game_state_store
				&& grvgm_is_keymod_down(GRVGM_KEYMOD_SHIFT)) {
				grv_log_info_cstr("Resetting game state.");
				if (_grvgm_state.game_state) {
					grv_free(_grvgm_state.game_state);
				}
				if (_grvgm_state.dylib.on_init)
					_grvgm_state.dylib.on_init(&_grvgm_state.game_state, &_grvgm_state.game_state_size);
				_grvgm_game_state_reset_store();
			}
		}

		if (_grvgm_state.window->should_close) {
			break;
		}

		if (first_iteration) {
			first_iteration = false;
			_grvgm_state.game_time_ms = 0;
			_grvgm_on_update(0.0f);
		} else if (_grvgm_state.options.use_game_state_store
			&& _grvgm_state.options.pause_enabled 
			&& grvgm_key_was_pressed_with_mod('p', GRVGM_KEYMOD_CTRL)) {
			pause = !pause;
		} else if (pause == false || grvgm_key_was_pressed('n')) {
			_grvgm_state.frame_index++;
			_grvgm_state.game_time_ms += _grvgm_target_frame_time_ms();
			f32 delta_time = 1.0f/ (f32)_grvgm_state.options.fps; 
			_grvgm_on_update(delta_time);
		} else if (pause == true && grvgm_key_is_down('h')) {
			i32 frames_to_jump = grvgm_is_keymod_down(GRVGM_KEYMOD_SHIFT) ? 4 : 1;
			_grvgm_game_state_jump(-frames_to_jump);
		} else if (pause == true && grvgm_key_is_down('l')) {
			i32 frames_to_jump = grvgm_is_keymod_down(GRVGM_KEYMOD_SHIFT) ? 4 : 1;
			_grvgm_game_state_jump(frames_to_jump);
		} else if (pause == true && grvgm_key_was_pressed('j')) {
			_grvgm_game_state_jump(-1);
		} else if (pause == true && grvgm_key_was_pressed('k')) {
			_grvgm_game_state_jump(1);
		} else if (pause == true && grvgm_key_was_pressed('s')) {
			FILE* file = fopen("/tmp/game_state.dat", "wb");
            grvgm_game_state_store_t* store = &_grvgm_state.game_state_store;

            size_t num_frames = store->frame_info_data.size;
            fwrite(&num_frames, sizeof(size_t), 1, file);
			fwrite(store->frame_info_data.arr, sizeof(grvgm_frame_info_t), num_frames, file);

            size_t num_bytes = store->frame_data.size;
            fwrite(&num_bytes, sizeof(size_t), 1, file);
			fwrite(store->frame_data.data, 1, num_bytes, file);

			fclose(file);
			grv_log_info_cstr("Game state has been saved.");
		}

		if (grvgm_key_was_pressed_with_mod('i', GRVGM_KEYMOD_CTRL)) {
			show_statistics = !show_statistics;
		}

		if (_grvgm_state.dylib.on_draw)
			_grvgm_state.dylib.on_draw(_grvgm_state.game_state);

		_grvgm_execute_end_of_frame_callback_queue();

		if (show_debug_ui) {
			grvgm_draw_button_state(fb);
		}

		u64 frame_end_counter = SDL_GetPerformanceCounter();
		f64 frame_time = (f64)(frame_end_counter - frame_start_counter) / (f64)SDL_GetPerformanceFrequency();
		if (show_statistics) _grvgm_draw_frame_statistics(frame_time);

		// presenting the window will wait for vsync
		grv_window_present(w);
		grv_arena_reset(_grvgm_state.draw_arena);
	}

	return 0;
}



