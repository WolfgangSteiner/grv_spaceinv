#include "grv/grv.h"
#include "grvgm.h"
#include "grv/grv_memory.h"
#include "grv/vec2f.h"
#include "grv_gfx/grv_spritesheet8.h"
#include "grv_gfx/grv_img8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include "grv_gfx/rect_fx32.h"
#include "grv/grv_pseudo_random.h"
#include "src/spaceinv.h"

//==============================================================================
// sprites
//==============================================================================

// void grv_framebuffer_put_sprite(grv_framebuffer_t* fb, grvgm_sprite_t* spr, i32 x, i32 y) {
//	   x -= spr->origin.x;
//	   y -= spr->origin.y;
//	   grv_framebuffer_blit_img8(fb, &spr->img, x, y);
// }

//==============================================================================
// entity
//==============================================================================

void entity_update_bounding_box(entity_t* e) {
	e->bounding_box = rect_fx32_move_to(e->bounding_box, e->pos);
}

void alien_entity_update(entity_t*, fx32);

f64 grvgm_cos_f64(f64 x) { return cos(x * 2 * M_PI); }
f64 grvgm_time_f64(void) { return fx32_to_f64(grvgm_time()); }

void title_text_update(entity_t* entity, fx32 delta_t) {
	GRV_UNUSED(delta_t);
#if 0
	f64 x = fx32_to_f64(entity->pos.x);
	f64 t = grvgm_time_f64();
	f64 y = 8 * grvgm_cos_f64(0.5 * t + x / 128) + 44;
	entity->pos.y = fx32_from_f64(y);
#elif 0
	fx32 x = entity->pos.x;
	fx32 phase_time = fx32_mul_f64(grvgm_time(), 0.5);
	fx32 phase_x = fx32_div_f64(entity->pos.x, 128);
	fx32 phase = fx32_add(phase_time, phase_x);
	entity->pos.y = fx32_mula_f64(grvgm_cos(phase), 8, 44);
#else
	fx32 x = entity->pos.x;
	fx32 phase = fx32_div_i32(grvgm_time(), 2);
	phase = fx32_add(phase, fx32_div_i32(x, 128));
	entity->pos.y = fx32_mula_i32(grvgm_cos(phase), 8, 44);
#endif
}

void entity_update(entity_t* entity, fx32 delta_t) {
	switch (entity->entity_type) {
		case ENTITY_TYPE_CLAW:
			alien_entity_update(entity, delta_t);
			break;
        case ENTITY_TYPE_TITLE_TEXT:
            title_text_update(entity, delta_t);
            break;
		default:
			entity->pos = vec2_fx32_smula(
				entity->vel, delta_t, entity->pos);
	}

	entity_update_bounding_box(entity);
}

void entity_draw(entity_t* entity) {
	grvgm_draw_sprite_fx32(entity->pos, entity->sprite);
	//grvgm_draw_pixel(entity->pos, 8);
	//grvgm_draw_rect(entity->bounding_box, 7);
}


#define GRV_ALLOC_OBJECT_ZERO(TYPE) grv_alloc_zeros(sizeof(TYPE))

void scene_init(scene_t* s) {
	s->entity_arr.capacity = 128;
}

void scene_clear(scene_t* s) {
    s->entity_arr.size = 0;
}

i32 scene_num_aliens(scene_t* scene) {
    i32 count = 0;
	for (i32 i = 0; i < scene->entity_arr.size; i++) {
		entity_t* e = &scene->entity_arr.arr[i];
        if (e->entity_type == ENTITY_TYPE_CLAW && e->is_alive) count++;
    }
    return count;
}

entity_t* scene_add_entity(scene_t* scene) {
	if (scene->entity_arr.size == scene->entity_arr.capacity) return NULL;
	return &scene->entity_arr.arr[scene->entity_arr.size++];
}

bool check_player_shot(scene_t* scene, shot_t* shot) {
	for (i32 i = 0; i < scene->entity_arr.size; i++) {
		entity_t* e = &scene->entity_arr.arr[i];
		if (!e->is_alive) continue;
		if (rect_fx32_point_inside(e->bounding_box, shot->pos)) {
			e->is_alive = false;
			return true;
		}
	}
	return false;
}

void update_shots(spaceinv_state_t* state, fx32 delta_t) {
	vec2_i32 size = grvgm_screen_size();
	i32 i = 0;
	while (i < state->shot_arr.size) {
		shot_t* shot = state->shot_arr.arr + i;
		vec2_fx32 pos = vec2_fx32_smula(shot->vel, delta_t, shot->pos);
		bool shot_in_range = (fx32_round(pos.y) >= 0 && fx32_round(pos.y) < size.y);
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

void scene_update(scene_t* scene, fx32 delta_t) {
	for (i32 i = 0; i < scene->entity_arr.size; ++i) {
		entity_t* e = &scene->entity_arr.arr[i];
		entity_update(e, delta_t);
	}
}

void scene_draw(scene_t* scene) {
	for (i32 i = 0; i < scene->entity_arr.size; i++) {
		entity_t* e = &scene->entity_arr.arr[i];
		if (e->entity_type == ENTITY_TYPE_TITLE_TEXT || e->is_alive) entity_draw(e);
	}
}

void shots_draw(spaceinv_state_t* state) {
	for (i32 i = 0; i < state->shot_arr.size; i++) {
		grvgm_draw_pixel_fx32(state->shot_arr.arr[i].pos, 8);
	}
}

//==============================================================================
// spaceinvaders game code
//==============================================================================
#include "player.c"
#include "alien.c"

void check_collision(scene_t* scene, entity_t* player) {
	rect_fx32 player_bbox = player->bounding_box;
	for (i32 i = 0; i < scene->entity_arr.size; i++) {
		entity_t* e = &scene->entity_arr.arr[i];
		if (e->is_alive && rect_fx32_intersect(player_bbox, e->bounding_box)) {
			player->player.state = PLAYER_STATE_EXPLODING;
			player->player.state_start_time = grvgm_time();
			return;
		}
	}
}

void explosion_effect_update(particle_effect_t* e, fx32 delta_t) {
	GRV_UNUSED(delta_t);
	// generate new particles
	if ((fx32_ge(grvgm_timediff(e->timestamp), e->generation_rate) 
			&& e->particles.size < e->max_num_particles) 
		|| e->particles.size == 0) {
		e->timestamp = grvgm_time();
		particle_t* p = &e->particles.arr[e->particles.size++];
		p->pos = vec2_fx32_from_i32(
			grv_pseudo_random_i32(0,7),
			grv_pseudo_random_i32(0,8));
		p->params[0] = fx32_from_i32(0); // current radius
		p->params[1] = fx32_from_i32(24); // max radius
		p->params[2] = fx32_from_f32(24.0f / 30.0f); // growth speed
		p->params[3] = fx32_from_i32(grv_pseudo_random_i32(7,10)); // color
	}

	// update particles
	i32 i = 0;
	while (i < e->particles.size) {
		particle_t* p = &e->particles.arr[i];
		if (fx32_gt(p->params[0], p->params[1])) {
			e->particles.arr[i] = e->particles.arr[--e->particles.size];
		} else {
			p->params[0] = fx32_add(p->params[0], p->params[2]);
			i++;
		}
	}
}

void explosion_effect_reset(particle_effect_t* e) {
	e->timestamp = grvgm_time();
	e->generation_rate = fx32_from_f32(6.0f / 30.0f);
	e->max_num_particles = 4;
	e->particles.size = 0;
}

void explosion_effect_draw(spaceinv_state_t* state, particle_effect_t* e) {
	vec2_fx32 player_pos = state->player.pos;
	for (i32 i = 0; i < e->particles.size; i++) {
		particle_t* p = &e->particles.arr[i];
		vec2_fx32 pos = vec2_fx32_add(player_pos, p->pos);
		fx32 radius = p->params[0];
		i32 color = fx32_round(p->params[3]);
		grvgm_fill_circle_fx32(pos, radius, color);
	}
}

void title_init(spaceinv_state_t* state) {
    i32 row_idx = 0;
    for (i32 i = 0; i < 8; i++) {
        i32 w = i == 5 ? 1 : 2;
        i32 offset = i > 5 ? -2 : 0;
        i32 x0 = 5;
        entity_t* e = scene_add_entity(&state->scene);
        *e = (entity_t){
            .entity_type=ENTITY_TYPE_TITLE_TEXT,
            .pos=vec2_fx32_from_i32(x0+row_idx*8+offset,44),
            .sprite={
                .index=16*13 + row_idx,
                .w=w,
                .h=3
            }
        };
        row_idx += w;
    }
}

typedef vec2i vec2_i32;

star_t star_init(i32 y) {
    vec2_i32 screen_size = grvgm_screen_size();
    i32 layer = grv_pseudo_random_i32(1,4);
    f32 v = 20.0f * layer;
    i32 x = grv_pseudo_random_i32(0,screen_size.x-1);
    u8 color = grv_min_i32(6, layer + 4);

    star_t star = {
        .pos = vec2_fx32_from_i32(x,y),
        .vel = {
            .y = fx32_from_f32(v)
        },
        .color = color
    };
    return star;
}

void starfield_init(starfield_t* starfield) {
    starfield->capacity = 64;
    starfield->size = 32;
    for (i32 i = 0; i < starfield->size; ++i) {
        starfield->arr[i] = star_init(grv_pseudo_random_i32(0,127));
    }
}

void starfield_draw(starfield_t* starfield) {
    for (i32 i=0; i<starfield->size; i++) {
        star_t star = starfield->arr[i];
        grvgm_draw_pixel_fx32(star.pos, star.color);
    }
}

void starfield_update(starfield_t* starfield, fx32 delta_t) {
    rect_i32 screen_rect = grvgm_screen_rect();
    for (i32 i=0; i<starfield->size; i++) {
        star_t* star = &starfield->arr[i];
        star->pos = vec2_fx32_smula(star->vel, delta_t, star->pos);
        if(fx32_round(star->pos.y) >= screen_rect.h) {
            *star = star_init(0);
        }
    }
}

void on_init(void** game_state, size_t* size) {
	spaceinv_state_t* state = grv_alloc_zeros(sizeof(spaceinv_state_t));
    state->level = -1;
	scene_init(&state->scene);
    starfield_init(&state->starfield);
	player_init(&state->player);
	explosion_effect_reset(&state->player_explosion_effect);
	state->shot_arr.capacity = 128;
	*game_state = state;
	*size = sizeof(spaceinv_state_t);
	printf("sizeof(spaceinv_state_t): %d\n", (int)sizeof(spaceinv_state_t));

    title_init(state);
}

void on_update(void* game_state, f32 delta_time) {
	spaceinv_state_t* state = game_state;
    fx32 delta_t = fx32_from_f32(delta_time);

    starfield_update(&state->starfield, delta_t);

    if (state->level == -1) {
        scene_update(&state->scene, delta_t);
        if (grvgm_was_button_pressed(GRVGM_BUTTON_CODE_A)) {
            scene_clear(&state->scene);
	        alien_create_wave(&state->scene, 5, 8);
            state->level=1;
        }
    } else {
        scene_update(&state->scene, delta_t);
        if (scene_num_aliens(&state->scene) == 0) {
            state->wave_cleared = true;
        }
        player_update(state, delta_t); 
        update_shots(state, delta_t);
        check_collision(&state->scene, &state->player);
        if (state->player.player.state == PLAYER_STATE_EXPLODING) {
            explosion_effect_update(&state->player_explosion_effect, delta_t);
        }
    }

}

void on_draw(void* game_state) {
	spaceinv_state_t* state = game_state;
	grvgm_clear_screen(0);

    starfield_draw(&state->starfield);
    if (state->level == -1) {
	    scene_draw(&state->scene);
		rect_i32 text_rect;
        rect_i32_split_vertically(grvgm_screen_rect(), 3, NULL, 2, &text_rect),
        grvgm_draw_text_aligned(
			text_rect,
            grv_str_ref("press fire to start"),
            GRV_ALIGNMENT_CENTER,
            6
        );
    } else {
        scene_draw(&state->scene);
        entity_draw(&state->player);
        shots_draw(state);
        if (state->player.player.state == PLAYER_STATE_EXPLODING) {
            explosion_effect_draw(state, &state->player_explosion_effect);
        }
        if (state->wave_cleared) {
            grvgm_draw_text_aligned(
                grvgm_screen_rect(),
                grv_str_ref("wave cleared"),
                GRV_ALIGNMENT_CENTER,
                6
            );
        }
    }
}

