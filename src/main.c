#include "grv/grv.h"
#include "grv/grv_memory.h"
#include "grv/vec2f.h"
#include "grv_gfx/grv_frame_buffer.h"
#include "grv_gfx/grv_window.h"
#include "grv_gfx/grv_bitmap_font.h"
#include "grv_gfx/grv_img8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

typedef grv_frame_buffer_t grv_framebuffer_t;

//======================================================================================================================
// Keyboard and button state
//======================================================================================================================
static u8* _grvgm_previous_keyboard_state = NULL;
static u8* _grvgm_current_keyboard_state = NULL;
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


//======================================================================================================================
// sprites
//======================================================================================================================
typedef enum {
    GRV_SPRITE_ANIMATION_NONE,
    GRV_SPRITE_ANIMATION_FORWARD,
    GRV_SPRITE_ANIMATION_BACKWARD,
    GRV_SPRITE_ANIMATION_PINGPONG,
} grv_sprite_animation_type_t;



typedef struct {
    grv_img8_t img;
    i32 spr_w, spr_h;
    i32 num_rows, num_cols;
} grv_spritesheet_t;

typedef struct {
    u8 start_idx, end_idx, current_idx;
    grv_sprite_animation_type_t animation_type;
    f32 prev_timestamp;
    f32 animation_delay;
} grvgm_sprite_animation_t;

typedef struct {
    u8 start_idx, end_idx, current_idx;
    f32 w, h;
    vec2i origin;
    grv_sprite_animation_type_t animation_type;
    f32 prev_timestamp;
    f32 animation_delay;
    bool flip_x, flip_y;
    grv_spritesheet_t* spritesheet;
} grvgm_sprite_t;



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

grv_spritesheet_t grv_spritesheet_create(i32 width, i32 height, i32 sprite_width, i32 sprite_height) {
    grv_spritesheet_t res = {0};
    res.img.w = width;
    res.img.h = height;
    res.img.row_skip = width;
    res.img.pixel_data = grv_alloc_zeros(width * height);
    res.img.owns_data = true;
    res.spr_w = sprite_width;
    res.spr_h = sprite_height;
    res.num_rows = height / sprite_height;
    res.num_cols = width / sprite_width;
    return res;
}

grv_spritesheet_t grv_spritesheet_parse_image_data_u8(grv_str_t input, i32 width, i32 height, i32 sprite_width, i32 sprite_height) {
    grv_spritesheet_t res = grv_spritesheet_create(width, height, sprite_width, sprite_height);
    parse_image_data_u8(res.img.pixel_data, input, width, height);
    return res;
}

bool grv_spritesheet_load_from_bmp(grv_str_t filename, grv_spritesheet_t* spritesheet, grv_error_t* err) {
    grv_assert(spritesheet->spr_w != 0);
    grv_assert(spritesheet->spr_h != 0);

    bool success = grv_img8_load_from_bmp(filename, &spritesheet->img, err);
    if (success) {
        spritesheet->num_rows = spritesheet->img.w / spritesheet->spr_w;
        spritesheet->num_cols = spritesheet->img.h / spritesheet->spr_h;
        return true;
    } else {
        return false;
    }
}

grv_img8_t grv_spritesheet_get_img8(grv_spritesheet_t* sprite_sheet, i32 row_idx, i32 col_idx) {
    grv_assert(col_idx < sprite_sheet->num_cols);
    grv_assert(row_idx < sprite_sheet->num_rows);
    u8* pixel_ptr = sprite_sheet->img.pixel_data + sprite_sheet->img.row_skip * sprite_sheet->spr_h * row_idx + sprite_sheet->spr_w * col_idx;
    return (grv_img8_t){
        .w=sprite_sheet->spr_w,
        .h=sprite_sheet->spr_h,
        .row_skip=sprite_sheet->img.row_skip,
        .owns_data=false,
        .pixel_data=pixel_ptr
    };
}

grv_img8_t grv_spritesheet_get_img8_by_number(grv_spritesheet_t* sprite_sheet, i32 number) {
    i32 row_idx = number / sprite_sheet->num_cols;
    i32 col_idx = number % sprite_sheet->num_cols;
    return grv_spritesheet_get_img8(sprite_sheet, row_idx, col_idx);
}

grv_img8_t grv_img8_parse_from_string(grv_str_t input, i32 width, i32 height) {
    assert(width * height <= input.size);
    u8* pixel_data = grv_alloc(width * height);
    parse_image_data_u8(pixel_data, input, width, height);
    grv_img8_t img = {.w=width, .h=height, .row_skip=width, .pixel_data=pixel_data };
    return img;
}

void grv_sprite_deinit(grvgm_sprite_t* spr) {
    GRV_UNUSED(spr);
}

void grv_framebuffer_blit_img8(grv_framebuffer_t* fb, grv_img8_t* img, i32 x, i32 y) {
    i32 x_start = x;
    i32 x_end = x + img->w - 1;
    i32 y_start = y;
    i32 y_end = y + img->h - 1;
    if (x_start >= fb->width || x_end < 0 || y_start >= fb->height || y_end < 0) return;

    x_start = grv_clamp_i32(x_start, 0, fb->width - 1);
    x_end = grv_clamp_i32(x_end, 0, fb->width - 1);
    y_start = grv_clamp_i32(y_start, 0, fb->height - 1);
    y_end = grv_clamp_i32(y_end, 0, fb->height - 1);

    u8* src_row_ptr = img->pixel_data + (y_start - y) * img->row_skip + (x_start - x);
    u8* dst_row_ptr = fb->indexed_data + y_start * fb->width + x_start;
    for (i32 iy = y_start; iy <= y_end; ++iy) {
        u8* src_ptr = src_row_ptr;
        u8* dst_ptr = dst_row_ptr;
        for (i32 ix = x_start; ix <= x_end; ++ix) {
            u8 src_value = *src_ptr++;
            if (src_value > 0) *dst_ptr = src_value;
            dst_ptr++;
        }
        src_row_ptr += img->row_skip;
        dst_row_ptr += fb->width;
    }    
}

// void grv_framebuffer_put_sprite(grv_framebuffer_t* fb, grvgm_sprite_t* spr, i32 x, i32 y) {
//     x -= spr->origin.x;
//     y -= spr->origin.y;
//     grv_framebuffer_blit_img8(fb, &spr->img, x, y);
// }

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
    static grv_spritesheet_t sprite_sheet = {0};
    if (sprite_sheet.img.pixel_data == NULL) {
        sprite_sheet = grv_spritesheet_parse_image_data_u8(grv_str_ref(_grvgm_button_sprite_data), 32, 16, 8, 8);
    }

    const i32 left_row_idx = grvgm_is_button_down(GRVGM_BUTTON_CODE_LEFT) ? 1 : 0;
    const i32 right_row_idx = grvgm_is_button_down(GRVGM_BUTTON_CODE_RIGHT) ? 1 : 0;
    const i32 up_row_idx = grvgm_is_button_down(GRVGM_BUTTON_CODE_UP) ? 1 : 0;
    const i32 down_row_idx = grvgm_is_button_down(GRVGM_BUTTON_CODE_DOWN) ? 1 : 0;
    grv_img8_t spr_left = grv_spritesheet_get_img8(&sprite_sheet, left_row_idx, 0);
    grv_img8_t spr_right = grv_spritesheet_get_img8(&sprite_sheet, right_row_idx, 1);
    grv_img8_t spr_up = grv_spritesheet_get_img8(&sprite_sheet, up_row_idx, 2);
    grv_img8_t spr_down = grv_spritesheet_get_img8(&sprite_sheet, down_row_idx, 3);

    i32 x = 0;
    i32 y = 0;
    i32 w = 8;
    grv_frame_buffer_fill_rect_u8(fb, (recti_t){x,y,4*w,w}, 0);
    grv_framebuffer_blit_img8(fb, &spr_left, x, y);
    grv_framebuffer_blit_img8(fb, &spr_right, x + w, y);
    grv_framebuffer_blit_img8(fb, &spr_up, x + 2*w, y);
    grv_framebuffer_blit_img8(fb, &spr_down, x + 3*w, y);
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
} grvgm_scene_t;

#define GRV_ALLOC_OBJECT_ZERO(TYPE) grv_alloc_zeros(sizeof(TYPE))

grvgm_scene_t* grvgm_scene_new() {
    grvgm_scene_t* s = GRV_ALLOC_OBJECT_ZERO(grvgm_scene_t);
    return s;
}

void grvgm_scene_add_entity(grvgm_scene_t* scene, entity_t* entity) {
    grv_arr_push(&scene->entity_arr, entity);
}

void grvgm_scene_update(grvgm_scene_t* scene, f32 dt) {
    for (size_t i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->update_func) e->update_func(e, dt);
    } 
}

void grvgm_scene_draw(grvgm_scene_t* scene, grv_framebuffer_t* fb) {
    for (size_t i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->draw_func) e->draw_func(e, fb);
    }
}

typedef struct {
    grv_window_t* window;
    grv_frame_buffer_t* framebuffer;
    grv_bitmap_font_t* font;
    grv_spritesheet_t spritesheet;
    u64 timestamp;
    u64 game_time;
} grvgm_state_t;

static grvgm_state_t _grvgm_state = {0};

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
    bool success = grv_spritesheet_load_from_bmp(grv_str_ref("assets/spritesheet.bmp"), &_grvgm_state.spritesheet, &err);
    if (success == false) {
        grv_abort(err);
    }
} 

void grvgm_cls(u8 color) {
    grv_frame_buffer_fill_u8(_grvgm_state.framebuffer, color);
} 

void grvgm_spr(i32 number, f32 x, f32 y) {
    grv_img8_t img = grv_spritesheet_get_img8_by_number(&_grvgm_state.spritesheet, number);
    grv_framebuffer_blit_img8(&_grvgm_state.window->frame_buffer, &img, grv_round_f32(x), grv_round_f32(y));   
}

void grvgm_pset(f32 x, f32 y, u8 color) {
    grv_frame_buffer_set_pixel_u8(&_grvgm_state.window->frame_buffer, vec2i_make(grv_round_f32(x), grv_round_f32(y)), color);
}

void grvgm_load_spritesheet_from_string() {
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
    grvgm_sprite_t sprite;
} player_entity_t;


void player_entity_update(entity_t* entity, f32 delta_t) {
    player_entity_t* player_entity = (player_entity_t*)entity;
    f32 spd_x_value = 160.0f;
    f32 spd_x = 0;
    if (grvgm_is_button_down(GRVGM_BUTTON_CODE_LEFT)) spd_x -= spd_x_value;
    if (grvgm_is_button_down(GRVGM_BUTTON_CODE_RIGHT)) spd_x += spd_x_value;
    entity->vel = vec2f_make(spd_x, 0);
    entity->pos = vec2f_clamp(vec2f_add(entity->pos, vec2f_smul(entity->vel, delta_t)), vec2f_make(0,0), vec2f_make(128,128));
}

void player_entity_draw(entity_t* entity, grv_framebuffer_t* fb) {
    player_entity_t* player_entity = (player_entity_t*)entity;
    grvgm_spr(0, entity->pos.x, entity->pos.y);
    grvgm_pset(entity->pos.x, entity->pos.y, 8);
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
    vec2f start_pos;
    f32 speed_x;
    f32 speed_y;
    f32 max_displacement;
} alien_entity_t;

void alien_entity_update(entity_t* entity, f32 delta_t) {
    alien_entity_t* alien = (alien_entity_t*)entity;
    f32 new_x = entity->pos.x + entity->vel.x * delta_t;
    if (grv_abs_f32(new_x - alien->start_pos.x) > alien->max_displacement) {
        entity->vel.x *= -1.0f;
        f32 sign = entity->pos.x > alien->start_pos.x ? 1.0f : -1.0f;
        entity->pos.x = alien->start_pos.x + alien->max_displacement * sign;
        entity->pos.y += delta_t * alien->speed_y;
    } else {
        entity->pos.x = new_x;
    }
}

void alien_entity_draw(entity_t* entity, grv_framebuffer_t* fb) {
    grvgm_spr(16, entity->pos.x, entity->pos.y);
    grvgm_pset(entity->pos.x, entity->pos.y, 8);
}

void alien_entity_free(entity_t* entity) {
    grv_free(entity);
}

alien_entity_t* alien_entity_create(vec2f pos) {
    alien_entity_t* alien = GRV_ALLOC_OBJECT(alien_entity_t);
    alien->entity.pos = pos;
    alien->start_pos = pos;
    alien->entity.update_func = alien_entity_update;
    alien->entity.draw_func = alien_entity_draw;
    alien->entity.free_func = alien_entity_free;
    alien->speed_x = 20.0f;
    alien->speed_y = 30.0f;
    alien->max_displacement = 16.0f;
    alien->entity.vel = vec2f_make(alien->speed_x, -alien->speed_y);
    return alien;
}

void alien_create_wave(grvgm_scene_t* scene, i32 num_rows, i32 num_cols) {
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
            grvgm_scene_add_entity(scene, (entity_t*)alien);
        }
        printf("\n");
    }
}

//======================================================================================================================
// spaceinvaders game code
//======================================================================================================================

typedef struct {
    grvgm_scene_t* scene;
    player_entity_t* player_entity;
} spaceinv_state_t;

static spaceinv_state_t state = {};

void on_init() {
    state.scene = grvgm_scene_new();
    state.player_entity = player_entity_create();
    grvgm_scene_add_entity(state.scene, (entity_t*)state.player_entity);
    alien_create_wave(state.scene, 5, 8);
}

void on_update(f32 delta_time) {
    grvgm_scene_update(state.scene, delta_time);
}

void on_draw() {
    grvgm_cls(0);
    grvgm_scene_draw(state.scene, &_grvgm_state.window->frame_buffer);
}

//======================================================================================================================
// main
//======================================================================================================================
int main(int argc, char** argv) {
    grvgm_main(argc, argv);

    return 0;
}
