#include "grv/grv.h"
#include "grvgm.h"
#include "grv/grv_memory.h"
#include "grv/vec2f.h"
#include "grv_gfx/grv_spritesheet8.h"
#include "grv_gfx/grv_img8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include "grv/grv_rect_fixed32.h"


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
    grv_rect_fixed32_t bounding_box;
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
    //grvgm_draw_rect(entity->bounding_box, 7);
}

void entity_update_bounding_box(entity_t* e) {
    e->bounding_box = grv_rect_fixed32_move_to(e->bounding_box, e->sprite.pos);
}

//==============================================================================
// shot
//==============================================================================
typedef struct {
    grv_vec2_fixed32_t pos;
    grv_vec2_fixed32_t vel;
} shot_t;


//==============================================================================
// scene
//==============================================================================
typedef struct {
    struct {
        entity_t** arr;
        i32 size;
        i32 capacity;
    } entity_arr;
    struct {
        shot_t* arr;
        i32 size;
        i32 capacity;
    } shot_arr;
} grvgm_scene_t;

#define GRV_ALLOC_OBJECT_ZERO(TYPE) grv_alloc_zeros(sizeof(TYPE))

grvgm_scene_t* grvgm_scene_new(void) {
    grvgm_scene_t* s = GRV_ALLOC_OBJECT_ZERO(grvgm_scene_t);
    s->shot_arr.capacity = 128;
    s->shot_arr.arr = grv_alloc_zeros(sizeof(shot_t) * s->shot_arr.capacity);
    return s;
}

void grvgm_scene_add_entity(grvgm_scene_t* scene, entity_t* entity) {
    grv_arr_push(&scene->entity_arr, entity);
}

bool scene_check_player_shot(grvgm_scene_t* scene, shot_t* shot) {
    for (i32 i = 0; i < scene->entity_arr.size; i++) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->is_player || !e->is_alive) continue;
        if (grv_rect_fixed32_point_inside(e->bounding_box, shot->pos)) {
            e->is_alive = false;
            return true;
        }
    }
    return false;
}


void scene_update_shots(grvgm_scene_t* scene, grv_fixed32_t delta_t) {
    grv_vec2_fixed32_t size = grvgm_screen_size();
    i32 i = 0;
    while (i < scene->shot_arr.size) {
        shot_t* shot = scene->shot_arr.arr + i;
        grv_vec2_fixed32_t pos = grv_vec2_fixed32_smula(shot->vel, delta_t, shot->pos);
        bool shot_in_range = (pos.y.val >= 0 && pos.y.val < size.y.val);
        bool shot_did_hit = scene_check_player_shot(scene, shot); 
        bool shot_alive = shot_in_range && !shot_did_hit;

        if (shot_alive) {
            shot->pos = pos;
            i++;
        } else {
            scene->shot_arr.size--;
            *shot = scene->shot_arr.arr[scene->shot_arr.size];
        }
    }
}

void grvgm_scene_update(grvgm_scene_t* scene, f32 dt) {
    grv_fixed32_t delta_t = grv_fixed32_from_f32(dt);
    for (i32 i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->update_func) e->update_func(e, delta_t);
    }
    scene_update_shots(scene, delta_t);
}

void grvgm_scene_draw(grvgm_scene_t* scene) {
    for (i32 i = 0; i < scene->entity_arr.size; i++) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->is_alive && e->draw_func) e->draw_func(e);
    }
    for (i32 i = 0; i < scene->shot_arr.size; i++) {
        grvgm_draw_pixel(scene->shot_arr.arr[i].pos, 8);
    }
}


//==============================================================================
// spaceinvaders game code
//==============================================================================
typedef struct {
    entity_t entity;
    grv_fixed32_t last_shot_timestamp;
    grv_fixed32_t shot_delay;
} player_entity_t;


typedef struct {
    grvgm_scene_t* scene;
    player_entity_t* player_entity;
} spaceinv_state_t;

static spaceinv_state_t state = {0};

#include "player.c"
#include "alien.c"

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
