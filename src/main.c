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
    u8 start_idx, end_idx, current_idx, frame_count;
    grv_sprite_animation_type_t animation_type;
    f32 prev_timestamp;
    f32 animation_delay;
} grvgm_sprite_animation_t;

// void grv_framebuffer_put_sprite(grv_framebuffer_t* fb, grvgm_sprite_t* spr, i32 x, i32 y) {
//     x -= spr->origin.x;
//     y -= spr->origin.y;
//     grv_framebuffer_blit_img8(fb, &spr->img, x, y);
// }


//==============================================================================
// entity
//==============================================================================
typedef struct entity_s {
    grvgm_sprite_t sprite;
    grv_vec2_fixed32_t vel;
    recti_t bounding_box;
    bool is_alive;
    bool is_player;
    void(*update_func)(struct entity_s*, grv_fixed32_t);
    void(*draw_func)(struct entity_s*);
} entity_t;

void entity_update(entity_t* entity, grv_fixed32_t delta_t) {
    entity->sprite.pos = grv_vec2_fixed32_smula(entity->vel, delta_t, entity->sprite.pos);
}

void entity_draw(entity_t* entity) {
    grvgm_draw_sprite(entity->sprite);
    grvgm_draw_pixel(entity->sprite.pos, 8);
}

//==============================================================================
// scene
//==============================================================================
typedef struct {
    struct {
        entity_t** arr;
        size_t size;
        size_t capacity;
    } entity_arr;
} grvgm_scene_t;

#define GRV_ALLOC_OBJECT_ZERO(TYPE) grv_alloc_zeros(sizeof(TYPE))

grvgm_scene_t* grvgm_scene_new(void) {
    grvgm_scene_t* s = GRV_ALLOC_OBJECT_ZERO(grvgm_scene_t);
    return s;
}

void grvgm_scene_add_entity(grvgm_scene_t* scene, entity_t* entity) {
    grv_arr_push(&scene->entity_arr, entity);
}

void grvgm_scene_update(grvgm_scene_t* scene, f32 dt) {
    grv_fixed32_t delta_t = grv_fixed32_from_f32(dt);
    for (size_t i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->update_func) e->update_func(e, delta_t);
    } 
}

void grvgm_scene_draw(grvgm_scene_t* scene) {
    for (size_t i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->draw_func) e->draw_func(e);
    }
}

//==============================================================================
// spaceinvaders game code
//==============================================================================
#include "player.c"
#include "alien.c"

typedef struct {
    grvgm_scene_t* scene;
    player_entity_t* player_entity;
} spaceinv_state_t;

static spaceinv_state_t state = {0};

void on_init(void) {
    state.scene = grvgm_scene_new();
    state.player_entity = player_entity_create();
    grvgm_scene_add_entity(state.scene, (entity_t*)state.player_entity);
    alien_create_wave(state.scene, 5, 8);
}

void on_update(f32 delta_time) {
    grvgm_scene_update(state.scene, delta_time);
}

void on_draw(void) {
    grvgm_clear_screen(0);
    grvgm_scene_draw(state.scene);
}

//==============================================================================
// main
//==============================================================================
int main(int argc, char** argv) {
    grvgm_main(argc, argv);

    return 0;
}
