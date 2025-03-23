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


void player_create_shot(spaceinv_state_t* state, grv_vec2_fixed32_t pos) {
    scene_t* scene = state->scene;
    if (scene->shot_arr.size < scene->shot_arr.capacity) {
        shot_t* shot = &scene->shot_arr.arr[scene->shot_arr.size++];
        shot->pos = pos;
        shot->vel = grv_vec2_fixed32_from_i32(0, -160);
    }
}

void player_update(spaceinv_state_t* state, grv_fixed32_t delta_t) {
    player_t* player = state->player;
    entity_t* entity = &player->entity;
    grv_fixed32_t spd_x_value = entity->vel.x;
    grv_fixed32_t spd_x = {0};
    if (grvgm_is_button_down(GRVGM_BUTTON_CODE_LEFT)) {
        spd_x = grv_fixed32_sub(spd_x, spd_x_value);
    }
    if (grvgm_is_button_down(GRVGM_BUTTON_CODE_RIGHT)) {
        spd_x = grv_fixed32_add(spd_x, spd_x_value);
    }
    grv_vec2_fixed32_t vel = {.x=spd_x};
    entity->sprite.pos = grv_vec2_fixed32_clamp(
        grv_vec2_fixed32_smula(vel, delta_t, entity->sprite.pos),
        grv_vec2_fixed32_from_i32(0,0),
        grv_vec2_fixed32_from_i32(128 - 7,128 - 7)
    );
    grv_fixed32_t current_time = grvgm_time();
    grv_fixed32_t time_diff = grv_fixed32_sub(current_time, player->last_shot_timestamp);
    if (grvgm_is_button_down(GRVGM_BUTTON_CODE_A)
        && grv_fixed32_ge(time_diff, player->shot_delay)) {
        grv_vec2_fixed32_t pos = grv_vec2_fixed32_add(
            entity->sprite.pos,
            grv_vec2_fixed32_from_i32(3, 0));
        player_create_shot(state, pos);
        player->last_shot_timestamp = current_time;
    }

    entity_update_bounding_box(entity);
}

player_t* player_create(void) {
    player_t* p = grv_alloc_zeros(sizeof(player_t));
    *p = (player_t) {
        .entity={
            .sprite={
                .pos=grv_vec2_fixed32_from_i32(64, 118),
                .index=0,
            },
            .entity_type = ENTITY_TYPE_PLAYER,
            .vel=grv_vec2_fixed32_from_i32(120, 120),
            .is_alive=true,
            .is_player=true,
            .bounding_box = {.w=grv_fixed32_from_i32(7), .h=grv_fixed32_from_i32(8)},
        },
        .shot_delay=grv_fixed32_from_f32(0.25)
    };
    return p;
}

