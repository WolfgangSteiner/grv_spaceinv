#include "grv/grv.h"
#include "grvgm.h"
#include "grv/grv_memory.h"
#include "grv/vec2f.h"
#include "grv_gfx/grv_frame_buffer.h"
#include "grv_gfx/grv_spritesheet8.h"
#include "grv_gfx/grv_img8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

typedef grv_frame_buffer_t grv_framebuffer_t;



//==============================================================================
// sprites
//==============================================================================
typedef enum {
    GRV_SPRITE_ANIMATION_NONE,
    GRV_SPRITE_ANIMATION_FORWARD,
    GRV_SPRITE_ANIMATION_BACKWARD,
    GRV_SPRITE_ANIMATION_PINGPONG,
} grv_sprite_animation_type_t;

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
    grv_spritesheet8_t* spritesheet;
} grvgm_sprite_t;

void grv_sprite_deinit(grvgm_sprite_t* spr) {
    GRV_UNUSED(spr);
}

// void grv_framebuffer_put_sprite(grv_framebuffer_t* fb, grvgm_sprite_t* spr, i32 x, i32 y) {
//     x -= spr->origin.x;
//     y -= spr->origin.y;
//     grv_framebuffer_blit_img8(fb, &spr->img, x, y);
// }


//======================================================================================================================
// entity
//======================================================================================================================
typedef struct entity_s {
    vec2f pos;
    vec2f vel;
    recti_t bounding_box;
    bool is_alive;
    void(*update_func)(struct entity_s*, f32);
    void(*draw_func)(struct entity_s*);
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

void grvgm_scene_draw(grvgm_scene_t* scene) {
    for (size_t i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->draw_func) e->draw_func(e);
    }
}

//==============================================================================
// player_entity
//==============================================================================
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

void player_entity_draw(entity_t* entity) {
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

void alien_entity_draw(entity_t* entity) {
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
    grvgm_scene_draw(state.scene);
}

//======================================================================================================================
// main
//======================================================================================================================
int main(int argc, char** argv) {
    grvgm_main(argc, argv);

    return 0;
}
