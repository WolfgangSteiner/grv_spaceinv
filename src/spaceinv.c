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

#include "src/spaceinv.h"

//==============================================================================
// sprites
//==============================================================================

// void grv_framebuffer_put_sprite(grv_framebuffer_t* fb, grvgm_sprite_t* spr, i32 x, i32 y) {
//     x -= spr->origin.x;
//     y -= spr->origin.y;
//     grv_framebuffer_blit_img8(fb, &spr->img, x, y);
// }

//==============================================================================
// entity
//==============================================================================

void entity_update_bounding_box(entity_t* e) {
    e->bounding_box = grv_rect_fixed32_move_to(e->bounding_box, e->sprite.pos);
}

void alien_entity_update(entity_t*, grv_fixed32_t);

void entity_update(entity_t* entity, grv_fixed32_t delta_t) {
    switch (entity->entity_type) {
        case ENTITY_TYPE_CLAW:
            alien_entity_update(entity, delta_t);
            break;
        default:
            entity->sprite.pos = grv_vec2_fixed32_smula(
                entity->vel, delta_t, entity->sprite.pos);
    }

    entity_update_bounding_box(entity);
}

void entity_draw(entity_t* entity) {
    grvgm_draw_sprite(entity->sprite);
    //grvgm_draw_pixel(entity->sprite.pos, 8);
    //grvgm_draw_rect(entity->bounding_box, 7);
}


#define GRV_ALLOC_OBJECT_ZERO(TYPE) grv_alloc_zeros(sizeof(TYPE))

void scene_init(scene_t* s) {
    s->entity_arr.capacity = 128;
}

entity_t* scene_add_entity(scene_t* scene) {
    if (scene->entity_arr.size == scene->entity_arr.capacity) return NULL;
    return &scene->entity_arr.arr[scene->entity_arr.size++];
}

bool check_player_shot(scene_t* scene, shot_t* shot) {
    for (i32 i = 0; i < scene->entity_arr.size; i++) {
        entity_t* e = &scene->entity_arr.arr[i];
        if (!e->is_alive) continue;
        if (grv_rect_fixed32_point_inside(e->bounding_box, shot->pos)) {
            e->is_alive = false;
            return true;
        }
    }
    return false;
}


void update_shots(spaceinv_state_t* state, grv_fixed32_t delta_t) {
    grv_vec2_fixed32_t size = grvgm_screen_size();
    i32 i = 0;
    while (i < state->shot_arr.size) {
        shot_t* shot = state->shot_arr.arr + i;
        grv_vec2_fixed32_t pos = grv_vec2_fixed32_smula(shot->vel, delta_t, shot->pos);
        bool shot_in_range = (pos.y.val >= 0 && pos.y.val < size.y.val);
        bool shot_did_hit = check_player_shot(&state->scene, shot); 
        bool shot_alive = shot_in_range && !shot_did_hit;

        if (shot_alive) {
            shot->pos = pos;
            i++;
        } else {
            state->shot_arr.size--;
            *shot = state->shot_arr.arr[state->shot_arr.size];
        }
    }
}

void scene_update(scene_t* scene, grv_fixed32_t delta_t) {
    for (i32 i = 0; i < scene->entity_arr.size; ++i) {
        entity_t* e = &scene->entity_arr.arr[i];
        entity_update(e, delta_t);
    }
}

void scene_draw(scene_t* scene) {
    for (i32 i = 0; i < scene->entity_arr.size; i++) {
        entity_t* e = &scene->entity_arr.arr[i];
        if (e->is_alive) entity_draw(e);
    }
}

void shots_draw(spaceinv_state_t* state) {
    for (i32 i = 0; i < state->shot_arr.size; i++) {
        grvgm_draw_pixel(state->shot_arr.arr[i].pos, 8);
    }
}

//==============================================================================
// spaceinvaders game code
//==============================================================================


#include "player.c"
#include "alien.c"

void check_collision(scene_t* scene, entity_t* player) {
    grv_rect_fixed32_t player_bbox = player->bounding_box;
    for (i32 i = 0; i < scene->entity_arr.size; i++) {
        entity_t* e = &scene->entity_arr.arr[i];
        if (e->is_alive && grv_rect_fixed32_intersect(player_bbox, e->bounding_box)) {
            player->player.state = PLAYER_STATE_EXPLODING;
            player->player.state_start_time = grvgm_time();
            return;
        }
    }
}

void explosion_effect_update(particle_effect_t* e, grv_fixed32_t delta_t) {
    GRV_UNUSED(delta_t);
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
        p->params[1] = grv_fixed32_from_i32(24); // max radius
        p->params[2] = grv_fixed32_from_f32(24.0f / 30.0f); // growth speed
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

void explosion_effect_draw(spaceinv_state_t* state, particle_effect_t* e) {
    grv_vec2_fixed32_t player_pos = state->player.sprite.pos;
    for (i32 i = 0; i < e->particles.size; i++) {
        particle_t* p = &e->particles.arr[i];
        grv_vec2_fixed32_t pos = grv_vec2_fixed32_add(player_pos, p->pos);
        grv_fixed32_t radius = p->params[0];
        i32 color = grv_fixed32_round(p->params[3]);
        grvgm_fill_circle(pos, radius, color);
    }
}

void on_init(void** game_state, size_t* size) {
    spaceinv_state_t* state = grv_alloc_zeros(sizeof(spaceinv_state_t));
    scene_init(&state->scene);
    player_init(&state->player);
    alien_create_wave(&state->scene, 5, 8);
    explosion_effect_reset(&state->player_explosion_effect);
    state->shot_arr.capacity = 128;
    *game_state = state;
    *size = sizeof(spaceinv_state_t);
    printf("sizeof(spaceinv_state_t): %d\n", (int)sizeof(spaceinv_state_t));
}

void on_update(void* game_state, f32 delta_time) {
    spaceinv_state_t* state = game_state;
    grv_fixed32_t delta_t = grv_fixed32_from_f32(delta_time);
    scene_update(&state->scene, delta_t);
    player_update(state, delta_t); 
    update_shots(state, delta_t);
    check_collision(&state->scene, &state->player);
    if (state->player.player.state == PLAYER_STATE_EXPLODING) {
        explosion_effect_update(&state->player_explosion_effect, delta_t);
    }
}

void on_draw(void* game_state) {
    spaceinv_state_t* state = game_state;
    grvgm_clear_screen(3);
    grvgm_draw_text(grv_str_ref("0123456789:;<=>?"), grv_vec2_fixed32_from_i32(1, 1), 7);
    grvgm_draw_text(grv_str_ref("@ABCDEFGHIJKLMNO"), grv_vec2_fixed32_from_i32(1, 7), 7);
    grvgm_draw_text(grv_str_ref("PQRSTUVWXYZ"), grv_vec2_fixed32_from_i32(1, 13), 7);
    scene_draw(&state->scene);
    entity_draw(&state->player);
    shots_draw(state);
    if (state->player.player.state == PLAYER_STATE_EXPLODING) {
        explosion_effect_draw(state, &state->player_explosion_effect);
    }
}

