#include "grv/grv.h"
#include "grvgm.h"
#include "grv/grv_memory.h"
#include "grv/vec2f.h"
#include "grv_gfx/grv_spritesheet8.h"
#include "grv_gfx/grv_img8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include "grv/grv_rect_fixed32.h"
#include "grv/grv_math.h"
#include "grv/grv_pseudo_random.h"

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
} scene_t;

#define GRV_ALLOC_OBJECT_ZERO(TYPE) grv_alloc_zeros(sizeof(TYPE))

scene_t* scene_new(void) {
    scene_t* s = GRV_ALLOC_OBJECT_ZERO(scene_t);
    s->shot_arr.capacity = 128;
    s->shot_arr.arr = grv_alloc_zeros(sizeof(shot_t) * s->shot_arr.capacity);
    return s;
}

void scene_add_entity(scene_t* scene, entity_t* entity) {
    grv_arr_push(&scene->entity_arr, entity);
}

bool check_player_shot(scene_t* scene, shot_t* shot) {
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


void update_shots(scene_t* scene, grv_fixed32_t delta_t) {
    grv_vec2_fixed32_t size = grvgm_screen_size();
    i32 i = 0;
    while (i < scene->shot_arr.size) {
        shot_t* shot = scene->shot_arr.arr + i;
        grv_vec2_fixed32_t pos = grv_vec2_fixed32_smula(shot->vel, delta_t, shot->pos);
        bool shot_in_range = (pos.y.val >= 0 && pos.y.val < size.y.val);
        bool shot_did_hit = check_player_shot(scene, shot); 
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

void scene_update(scene_t* scene, grv_fixed32_t delta_t) {
    for (i32 i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = scene->entity_arr.arr[i];
        if (e->update_func) e->update_func(e, delta_t);
    }
}

void scene_draw(scene_t* scene) {
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
typedef enum {
    PLAYER_STATE_NORMAL,
    PLAYER_STATE_EXPLODING,
    PLAYER_STATE_DEAD,
} player_state_t;

typedef struct {
    grv_vec2_fixed32_t pos;
    grv_fixed32_t params[4];
} particle_t;


#define MAX_NUM_PARTICLES 32
#define MAX_NUM_EFFECTS 32

typedef struct {
    grv_fixed32_t timestamp;
    grv_fixed32_t generation_rate;
    i32 max_num_particles;
    struct {
        i32 size;
        particle_t arr[MAX_NUM_PARTICLES];
    } particles;
} particle_effect_t;

typedef struct {
    entity_t entity;
    grv_fixed32_t last_shot_timestamp;
    grv_fixed32_t shot_delay;
    player_state_t state;
    grv_fixed32_t state_start_time;
    particle_effect_t explosion_effect;
} player_t;

typedef struct {
    scene_t* scene;
    player_t* player;
} spaceinv_state_t;

static spaceinv_state_t state = {0};

#include "player.c"
#include "alien.c"

void check_collision(scene_t* scene, player_t* player) {
    grv_rect_fixed32_t player_bbox = player->entity.bounding_box;
    for (i32 i = 0; i < scene->entity_arr.size; i++) {
        entity_t* e = scene->entity_arr.arr[i];
        if (grv_rect_fixed32_intersect(player_bbox, e->bounding_box)) {
            player->state = PLAYER_STATE_EXPLODING;
            player->state_start_time = grvgm_time();
            return;
        }
    }
}

void explosion_effect_update(particle_effect_t* e, grv_fixed32_t delta_t) {
    // generate new particles
    if ((grv_fixed32_ge(grvgm_timediff(e->timestamp), e->generation_rate) 
            && e->particles.size < e->max_num_particles) 
        || e->particles.size == 0) {
        e->timestamp = grvgm_time();
        particle_t* p = &e->particles.arr[e->particles.size++];
        p->pos = grv_vec2_fixed32_from_i32(
            grv_pseudo_random_i32(0,7),
            grv_pseudo_random_i32(0,8));
        p->params[0] = grv_fixed32_from_i32(0); // current radius
        p->params[1] = grv_fixed32_from_i32(3); // max radius
        p->params[2] = grv_fixed32_from_f32(6.0f / 30.0f); // growth speed
        p->params[3] = grv_fixed32_from_i32(grv_pseudo_random_i32(7,10)); // color
    }

    // update particles
    i32 i = 0;
    while (i < e->particles.size) {
        particle_t* p = &e->particles.arr[i];
        if (grv_fixed32_gt(p->params[0], p->params[1])) {
            e->particles.arr[i] = e->particles.arr[--e->particles.size];
        } else {
            p->params[0] = grv_fixed32_add(p->params[0], p->params[2]);
            i++;
        }
    }
}

void explosion_effect_reset(particle_effect_t* e) {
    e->timestamp = grvgm_time();
    e->generation_rate = grv_fixed32_from_f32(6.0f / 30.0f);
    e->max_num_particles = 4;
    e->particles.size = 0;
}

void explosion_effect_draw(particle_effect_t* e) {
    grv_vec2_fixed32_t player_pos = state.player->entity.sprite.pos;
    for (i32 i = 0; i < e->particles.size; i++) {
        particle_t* p = &e->particles.arr[i];
        grv_vec2_fixed32_t pos = grv_vec2_fixed32_add(player_pos, p->pos);
        grv_fixed32_t radius = p->params[0];
        i32 color = grv_fixed32_round(p->params[3]);
        grvgm_draw_circle(pos, radius, color);
    }
}

void on_init(void) {
    state.scene = scene_new();
    state.player = player_create();
    alien_create_wave(state.scene, 5, 8);
    explosion_effect_reset(&state.player->explosion_effect);
}

void on_update(f32 delta_time) {
    grv_fixed32_t delta_t = grv_fixed32_from_f32(delta_time);
    scene_update(state.scene, delta_t);
    player_update(state.player, delta_t); 
    update_shots(state.scene, delta_t);
    check_collision(state.scene, state.player);
    explosion_effect_update(&state.player->explosion_effect, delta_t);
}

void on_draw(void) {
    grvgm_clear_screen(0);
    scene_draw(state.scene);
    entity_draw(&state.player->entity);
    explosion_effect_draw(&state.player->explosion_effect);
}

//==============================================================================
// main
//==============================================================================
int main(int argc, char** argv) {
    grvgm_main(argc, argv);
    return 0;
}
