#include "grv/grv.h"
#include "grv/grv_memory.h"
#include "grv/vec2f.h"
#include "grv_gfx/grv_frame_buffer.h"
#include "grv_gfx/grv_window.h"
#include "grv_gfx/grv_bitmap_font.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

typedef grv_frame_buffer_t grv_framebuffer_t;

//======================================================================================================================
// Keyboard and button state
//======================================================================================================================
static u8* _grvgame_previous_keyboard_state = NULL;
static u8* _grvgame_current_keyboard_state = NULL;
typedef enum {
    GRVGAME_BUTTON_CODE_LEFT  = 0,
    GRVGAME_BUTTON_CODE_RIGHT = 1,
    GRVGAME_BUTTON_CODE_UP    = 2,
    GRVGAME_BUTTON_CODE_DOWN  = 3,
    GRVGAME_BUTTON_CODE_A     = 4,
    GRVGAME_BUTTON_CODE_B     = 5,
    GRVGAME_BUTTON_CODE_X     = 6,
    GRVGAME_BUTTON_CODE_Y     = 7,
} grvgame_button_code_t;

bool _grvgame_is_key_down(int scancode) {
    if (_grvgame_current_keyboard_state == NULL) return false;
    return _grvgame_current_keyboard_state[scancode] != 0;
}

bool grvgame_is_button_down(grvgame_button_code_t button_code) {
    const u8* keyboard_state = SDL_GetKeyboardState(NULL);
    switch (button_code) {
        case GRVGAME_BUTTON_CODE_LEFT:
            return _grvgame_is_key_down(SDL_SCANCODE_LEFT) || _grvgame_is_key_down(SDL_SCANCODE_H);
        case GRVGAME_BUTTON_CODE_RIGHT:
            return _grvgame_is_key_down(SDL_SCANCODE_RIGHT) || _grvgame_is_key_down(SDL_SCANCODE_L);
        case GRVGAME_BUTTON_CODE_UP:
            return _grvgame_is_key_down(SDL_SCANCODE_UP) || _grvgame_is_key_down(SDL_SCANCODE_K);
        case GRVGAME_BUTTON_CODE_DOWN:
            return _grvgame_is_key_down(SDL_SCANCODE_DOWN) || _grvgame_is_key_down(SDL_SCANCODE_J);
        case GRVGAME_BUTTON_CODE_A:
            return _grvgame_is_key_down(SDL_SCANCODE_F);
        case GRVGAME_BUTTON_CODE_B:
            return _grvgame_is_key_down(SDL_SCANCODE_D);
        case GRVGAME_BUTTON_CODE_X:
            return _grvgame_is_key_down(SDL_SCANCODE_S);
        case GRVGAME_BUTTON_CODE_Y:
            return _grvgame_is_key_down(SDL_SCANCODE_A);
        default:
            return false;
    }
}

void grvgame_poll_keyboard() {
    int num_keys = 0;
    const u8* keyboard_state = SDL_GetKeyboardState(&num_keys);
    if (_grvgame_previous_keyboard_state == NULL) {
        _grvgame_previous_keyboard_state = grv_alloc_zeros(num_keys);
        _grvgame_current_keyboard_state = grv_alloc_zeros(num_keys);

        memcpy(_grvgame_previous_keyboard_state, keyboard_state, num_keys);
        memcpy(_grvgame_current_keyboard_state, keyboard_state, num_keys);
    } else {
        memcpy(_grvgame_previous_keyboard_state, _grvgame_current_keyboard_state, num_keys); 
        memcpy(_grvgame_current_keyboard_state, keyboard_state, num_keys);
    }
}

int _grvgame_char_to_sdl_scancode(char key) {
    switch(key) {
        case 'n': return SDL_SCANCODE_N;
        case 'p': return SDL_SCANCODE_P;
        default: return -1;
    }
}

bool grvgame_was_key_pressed(char key) {
    int scancode = _grvgame_char_to_sdl_scancode(key);
    if (scancode < 0) return false;
    return _grvgame_previous_keyboard_state[scancode] == 0 && _grvgame_current_keyboard_state[scancode] != 0;
}


//======================================================================================================================
// sprites
//======================================================================================================================
typedef struct {
    i32 w;
    i32 h;
    i32 row_skip;
    vec2i origin;
    u8* pixel_data;
    bool owns_data;
} grv_sprite_t;

typedef struct {
    i32 w, h;
    i32 spr_w, spr_h;
    i32 num_rows, num_cols;
    u8 pixel_data[];
} grv_spritesheet_t;


void parse_image_data_u8(u8* dst, grv_str_t input, i32 width, i32 height) {
    char* src = input.data;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            char c = *src++;
            c = (c == ' ') ? 0 : c - '0';
            *dst++ = c;
        }
    }
}

grv_spritesheet_t* grv_spritesheet_parse_image_data_u8(grv_str_t input, i32 width, i32 height, i32 sprite_width, i32 sprite_height) {
    grv_spritesheet_t* res = grv_alloc_zeros(sizeof(grv_spritesheet_t) + width * height);
    res->w = width;
    res->h = height;
    res->spr_w = sprite_width;
    res->spr_h = sprite_height;
    res->num_rows = height / sprite_height;
    res->num_cols = width / sprite_width;
    parse_image_data_u8(res->pixel_data, input, width, height);
    return res;
}

grv_sprite_t grv_spritesheet_get_sprite(grv_spritesheet_t* sprite_sheet, i32 row_idx, i32 col_idx) {
    grv_assert(col_idx < sprite_sheet->num_cols);
    grv_assert(row_idx < sprite_sheet->num_rows);
    u8* pixel_ptr = sprite_sheet->pixel_data + sprite_sheet->w * sprite_sheet->spr_h * row_idx + sprite_sheet->spr_w * col_idx;
    return (grv_sprite_t){
        .w=sprite_sheet->spr_w,
        .h=sprite_sheet->spr_h,
        .row_skip=sprite_sheet->w,
        .owns_data=false,
        .pixel_data=pixel_ptr
    };
}

grv_sprite_t grv_sprite_parse_from_string(grv_str_t input, i32 width, i32 height) {
    assert(width * height <= input.size);
    u8* pixel_data = grv_alloc(width * height);
    parse_image_data_u8(pixel_data, input, width, height);
    grv_sprite_t spr = {.w=width, .h=height, .row_skip=width, .pixel_data=pixel_data };
    return spr;
}

void grv_sprite_deinit(grv_sprite_t* spr) {
    if (spr->pixel_data && spr->owns_data) {
        grv_free(spr->pixel_data);
        spr->pixel_data = NULL;
    }
}

void grv_framebuffer_put_sprite(grv_framebuffer_t* fb, grv_sprite_t* spr, i32 x, i32 y) {
    x -= spr->origin.x;
    y -= spr->origin.y;
    i32 x_start = x;
    i32 x_end = x + spr->w - 1;
    i32 y_start = y;
    i32 y_end = y + spr->h - 1;
    if (x_start >= fb->width || x_end < 0 || y_start >= fb->height || y_end < 0) return;

    x_start = grv_clamp_i32(x_start, 0, fb->width - 1);
    x_end = grv_clamp_i32(x_end, 0, fb->width - 1);
    y_start = grv_clamp_i32(y_start, 0, fb->height - 1);
    y_end = grv_clamp_i32(y_end, 0, fb->height - 1);

    u8* src_row_ptr = spr->pixel_data + (y_start - y) * spr->row_skip + (x_start - x);
    u8* dst_row_ptr = fb->indexed_data + y_start * fb->width + x_start;
    for (i32 iy = y_start; iy <= y_end; ++iy) {
        u8* src_ptr = src_row_ptr;
        u8* dst_ptr = dst_row_ptr;
        for (i32 ix = x_start; ix <= x_end; ++ix) {
            u8 src_value = *src_ptr++;
            if (src_value > 0) *dst_ptr = src_value;
            dst_ptr++;
        }
        src_row_ptr += spr->row_skip;
        dst_row_ptr += fb->width;
    }    
}

typedef struct {
    grv_framebuffer_t* fb;
    grv_window_t* window;
} game_state_t;


char* _grvgame_button_sprite_data = 
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

void grvgame_draw_button_state(grv_framebuffer_t* fb) {
    static grv_spritesheet_t* sprite_sheet = NULL;
    if (sprite_sheet == NULL) {
        sprite_sheet = grv_spritesheet_parse_image_data_u8(grv_str_ref(_grvgame_button_sprite_data), 32, 16, 8, 8);
    }

    const i32 left_row_idx = grvgame_is_button_down(GRVGAME_BUTTON_CODE_LEFT) ? 1 : 0;
    const i32 right_row_idx = grvgame_is_button_down(GRVGAME_BUTTON_CODE_RIGHT) ? 1 : 0;
    const i32 up_row_idx = grvgame_is_button_down(GRVGAME_BUTTON_CODE_UP) ? 1 : 0;
    const i32 down_row_idx = grvgame_is_button_down(GRVGAME_BUTTON_CODE_DOWN) ? 1 : 0;
    grv_sprite_t spr_left = grv_spritesheet_get_sprite(sprite_sheet, left_row_idx, 0);
    grv_sprite_t spr_right = grv_spritesheet_get_sprite(sprite_sheet, right_row_idx, 1);
    grv_sprite_t spr_up = grv_spritesheet_get_sprite(sprite_sheet, up_row_idx, 2);
    grv_sprite_t spr_down = grv_spritesheet_get_sprite(sprite_sheet, down_row_idx, 3);

    i32 x = 0;
    i32 y = 0;
    i32 w = 7;
    grv_frame_buffer_fill_rect_u8(fb, (recti_t){x,y,3*w,3*w}, 0);
    grv_framebuffer_put_sprite(fb, &spr_left, x, y + w);
    grv_framebuffer_put_sprite(fb, &spr_right, x + 2*w, y + w);
    grv_framebuffer_put_sprite(fb, &spr_up, x + w, y);
    grv_framebuffer_put_sprite(fb, &spr_down, x + w, y + 2*w);
}
    

//======================================================================================================================
// entity
//======================================================================================================================
typedef struct entity_s {
    vec2f pos;
    vec2f vel;
    recti_t bounding_box;
    bool is_alive;
    void(*update_func)(struct entity_s*, f32);
    void(*draw_func)(struct entity_s*, grv_framebuffer_t*);
    void(*free_func)(struct entity_s*);
} entity_t;

void entity_update(entity_t* entity, f32 delta_t) {
    entity->pos = vec2f_add(entity->pos, vec2f_smul(entity->vel, delta_t));
}


//======================================================================================================================
// scene
//======================================================================================================================
typedef struct {
    struct {
        entity_t** arr;
        size_t size;
        size_t capacity;
    } entity_arr;
} grvgame_scene_t;

#define GRV_ALLOC_OBJECT_ZERO(TYPE) grv_alloc_zeros(sizeof(TYPE))

grvgame_scene_t* grvgame_scene_new() {
    grvgame_scene_t* s = GRV_ALLOC_OBJECT_ZERO(grvgame_scene_t);
    return s;
}

void grvgame_scene_add_entity(grvgame_scene_t* scene, entity_t* entity) {
    grv_arr_push(&scene->entity_arr, entity);
}

void grvgame_scene_update(grvgame_scene_t* scene, f32 dt) {
    for (size_t i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->update_func) e->update_func(e, dt);
    } 
}

void grvgame_scene_draw(grvgame_scene_t* scene, grv_framebuffer_t* fb) {
    for (size_t i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->draw_func) e->draw_func(e, fb);
    }
}

typedef struct {
    grv_window_t* window;
    grv_bitmap_font_t* font;
} grvgm_state_t;

static grvgm_state_t _grvgm_state = {};

void _grvgm_init() {
    _grvgm_state.window = grv_window_new(128,128, 4.0f, grv_str_ref(""));
    grv_window_t* w = _grvgm_state.window;
    grv_color_palette_init_with_type(&w->frame_buffer.palette, GRV_COLOR_PALETTE_PICO8);
    w->borderless = true;
    grv_window_show(w);
    _grvgm_state.font = grv_get_cozette_font();
} 

void grvgm_cls(u8 color) {
    grv_frame_buffer_fill_u8(&_grvgm_state.window->frame_buffer, color);
} 

extern void on_init();
extern void on_update(f32);
extern void on_draw();

int grvgm_main(int argc, char** argv) {
    _grvgm_init();

    on_init();

    u64 last_timestamp = SDL_GetTicks64();
    f32 delta_time = 0.0f;
    bool pause = true;
    bool show_debug_ui = true;

    grv_window_t* w = _grvgm_state.window;
    grv_frame_buffer_t* fb = &w->frame_buffer;

    while (true) {
        grv_window_poll_events();
        grvgame_poll_keyboard();

        if (grvgame_was_key_pressed('p')) {
            pause = !pause;
        }

        if (_grvgm_state.window->should_close) {
            break;
        }

        const u64 current_timestamp = SDL_GetTicks64();
        const f32 delta_time = (f32)(current_timestamp - last_timestamp) / 1000.0f;
        last_timestamp = current_timestamp;

        if (!pause || grvgame_was_key_pressed('n')) {
            on_update(delta_time);
        }

        on_draw();

        if (show_debug_ui) {
            grvgame_draw_button_state(fb);
        }

        grv_window_present(w);
        grv_sleep(0.03);
    }

    return 0;
}

//======================================================================================================================
// player_entity
//======================================================================================================================
char* player_sprite_data = 
"   7    "
"   7    "
"  7 7   "
" 7   7  "
" 7   7  "
"7777777 "
"77   77 "
"        ";

typedef struct {
    entity_t entity;
    grv_sprite_t sprite;
} player_entity_t;

void player_entity_update(entity_t* entity, f32 delta_t) {
    player_entity_t* player_entity = (player_entity_t*)entity;
    f32 spd_x_value = 160.0f;
    f32 spd_x = 0;
    if (grvgame_is_button_down(GRVGAME_BUTTON_CODE_LEFT)) spd_x -= spd_x_value;
    if (grvgame_is_button_down(GRVGAME_BUTTON_CODE_RIGHT)) spd_x += spd_x_value;
    entity->vel = vec2f_make(spd_x, 0);
    entity->pos = vec2f_clamp(vec2f_add(entity->pos, vec2f_smul(entity->vel, delta_t)), vec2f_make(0,0), vec2f_make(128,128));
}

void player_entity_draw(entity_t* entity, grv_framebuffer_t* fb) {
    player_entity_t* player_entity = (player_entity_t*)entity;
    grv_framebuffer_put_sprite(fb, &player_entity->sprite, entity->pos.x, entity->pos.y);
    grv_frame_buffer_set_pixel_u8(fb, vec2i_make(entity->pos.x, entity->pos.y), 8);
}

void player_entity_free(entity_t* entity) {
    player_entity_t* player_entity = (player_entity_t*)entity;
    grv_sprite_deinit(&player_entity->sprite);
    grv_free(player_entity);
}

player_entity_t* player_entity_create() {
    player_entity_t* p = grv_alloc_zeros(sizeof(player_entity_t));
    p->entity.pos = vec2f_make(64, 118);
    p->entity.update_func = player_entity_update;
    p->entity.draw_func = player_entity_draw;
    p->entity.free_func = player_entity_free;
    p->sprite = grv_sprite_parse_from_string(grv_str_ref(player_sprite_data), 8, 8);
    p->sprite.origin = vec2i_make(3, 5);
    return p;
}

//======================================================================================================================
// alien entity
//======================================================================================================================
char* alien_sprite_data =
"   77   "
" 77  77 "
"7      7"
"7      7"
" 777777 "
"  7  7  "
" 7    7 "
"  7  7  ";

typedef struct {
    entity_t entity;
    grv_sprite_t sprite;
    vec2f start_pos;
    f32 speed_x;
    f32 speed_y;
    f32 max_displacement;
} alien_entity_t;

void alien_entity_update(entity_t* entity, f32 delta_t) {
    alien_entity_t* alien = (alien_entity_t*)entity;
    f32 new_x = entity->pos.x + entity->vel.x * delta_t;
    if (grv_abs_f32(new_x - alien->start_pos.x) >= alien->max_displacement) {
        entity->vel.x *= -1.0f;
        f32 sign = entity->pos.x > alien->start_pos.x ? 1.0f : -1.0f;
        entity->pos.x = alien->start_pos.x + alien->max_displacement * sign;
        entity->pos.y += delta_t * alien->speed_y;
    } else {
        entity->pos.x = new_x;
    }
}

void alien_entity_draw(entity_t* entity, grv_framebuffer_t* fb) {
    alien_entity_t* alien = (alien_entity_t*)entity;
    grv_framebuffer_put_sprite(fb, &alien->sprite, entity->pos.x, entity->pos.y);
    grv_frame_buffer_set_pixel_u8(fb, vec2i_make(entity->pos.x, entity->pos.y), 8);
}

void alien_entity_free(entity_t* entity) {
    alien_entity_t* alien = (alien_entity_t*)entity;
    grv_free(alien->sprite.pixel_data);
    grv_free(entity);
}

alien_entity_t* alien_entity_create(vec2f pos) {
    alien_entity_t* alien = GRV_ALLOC_OBJECT(alien_entity_t);
    alien->entity.pos = pos;
    alien->start_pos = pos;
    alien->sprite = grv_sprite_parse_from_string(grv_str_ref(alien_sprite_data), 8, 8);
    alien->sprite.origin = vec2i_make(3,3);
    alien->entity.update_func = alien_entity_update;
    alien->entity.draw_func = alien_entity_draw;
    alien->entity.free_func = alien_entity_free;
    alien->speed_x = 40.0f;
    alien->speed_y = 30.0f;
    alien->max_displacement = 16.0f;
    alien->entity.vel = vec2f_make(alien->speed_x, -alien->speed_y);
    return alien;
}

void alien_create_wave(grvgame_scene_t* scene, i32 num_rows, i32 num_cols) {
    const i32 space_width = 16;
    const i32 space_height = 16;
    const i32 alien_width = 8;
    const i32 alien_height = 8;
    const i32 total_width = space_width * (num_cols - 1);
    const i32 screen_width = 128;
    const i32 screen_height = 128;
    const i32 x_start = (screen_width - total_width) / 2;
    const i32 y_start = 16;

    for (int row = 0; row < num_rows; ++row) {
        const i32 y = y_start + row * space_height;
        for (int col = 0; col < num_cols; ++col) {
            const i32 x = x_start + col * space_width;
            alien_entity_t* alien = alien_entity_create(vec2f_make(x, y));
            f32 sign = (row % 2) == 0 ? 1.0f : -1.0f;
            alien->entity.vel.x = alien->speed_x * sign;
            alien->entity.vel.y = 0.0f;
            grvgame_scene_add_entity(scene, (entity_t*)alien);
        }
        printf("\n");
    }
}

//======================================================================================================================
// spaceinvaders game code
//======================================================================================================================

typedef struct {
    grv_sprite_t player_sprite;
    grv_sprite_t alien_sprite;
    grvgame_scene_t* scene;
    player_entity_t* player_entity;
} spaceinv_state_t;

static spaceinv_state_t state = {};

void on_init() {
    state.player_sprite = grv_sprite_parse_from_string(grv_str_ref(player_sprite_data), 8, 8);
    state.alien_sprite = grv_sprite_parse_from_string(grv_str_ref(alien_sprite_data), 8, 8);
    state.scene = grvgame_scene_new();
    state.player_entity = player_entity_create();
    grvgame_scene_add_entity(state.scene, (entity_t*)state.player_entity);
    alien_create_wave(state.scene, 5, 8);
}

void on_update(f32 delta_time) {
    grvgame_scene_update(state.scene, delta_time);
}

void on_draw() {
    grvgm_cls(0);
    grvgame_scene_draw(state.scene, &_grvgm_state.window->frame_buffer);
}

//======================================================================================================================
// main
//======================================================================================================================
int main(int argc, char** argv) {
    grvgm_main(argc, argv);

    return 0;
}
