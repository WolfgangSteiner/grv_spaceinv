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


void player_create_shot(grv_vec2_fixed32_t pos) {
    grvgm_scene_t* scene = state.scene;
    if (scene->shot_arr.size < scene->shot_arr.capacity) {
        shot_t* shot = &scene->shot_arr.arr[scene->shot_arr.size++];
        shot->pos = pos;
        shot->vel = grv_vec2_fixed32_from_i32(0, -80);
    }
}


void player_entity_update(entity_t* entity, grv_fixed32_t delta_t) {
    player_entity_t* player = (player_entity_t*)entity;
    i32 spd_x_value = 160;
    i32 spd_x = 0;
    if (grvgm_is_button_down(GRVGM_BUTTON_CODE_LEFT)) spd_x -= spd_x_value;
    if (grvgm_is_button_down(GRVGM_BUTTON_CODE_RIGHT)) spd_x += spd_x_value;
    grv_vec2_fixed32_t vel = grv_vec2_fixed32_from_i32(spd_x, 0);
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
        player_create_shot(pos);
        player->last_shot_timestamp = current_time;
    }
}

player_entity_t* player_entity_create(void) {
    player_entity_t* p = grv_alloc_zeros(sizeof(player_entity_t));
    *p = (player_entity_t) {
        .entity={
            .sprite={
                .pos=grv_vec2_fixed32_from_i32(64, 118),
                .index=0,
            },
            .update_func=player_entity_update,
            .draw_func=entity_draw,
        },
        .shot_delay=grv_fixed32_from_f32(0.5)
    };
    return p;
}

