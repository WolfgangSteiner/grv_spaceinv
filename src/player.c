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


void player_create_shot(spaceinv_state_t* state, vec2_fx32 pos) {
	if (state->shot_arr.size < state->shot_arr.capacity) {
		shot_t* shot = &state->shot_arr.arr[state->shot_arr.size++];
		shot->pos = pos;
		shot->vel = vec2_fx32_from_i32(0, -160);
	}
}

void player_update(spaceinv_state_t* state, fx32 delta_t) {
	entity_t* entity = &state->player;
	fx32 spd_x_value = entity->vel.x;
	fx32 spd_x = {0};
	if (grvgm_is_button_down(GRVGM_BUTTON_CODE_LEFT)) {
		spd_x = fx32_sub(spd_x, spd_x_value);
	}
	if (grvgm_is_button_down(GRVGM_BUTTON_CODE_RIGHT)) {
		spd_x = fx32_add(spd_x, spd_x_value);
	}
	vec2_fx32 vel = {.x=spd_x};
	entity->sprite.pos = vec2_fx32_clamp(
		vec2_fx32_smula(vel, delta_t, entity->sprite.pos),
		vec2_fx32_from_i32(0,0),
		vec2_fx32_from_i32(128 - 7,128 - 7)
	);
	fx32 current_time = grvgm_time();
	fx32 time_diff = fx32_sub(current_time, entity->player.last_shot_timestamp);
	if (grvgm_is_button_down(GRVGM_BUTTON_CODE_A)
		&& fx32_ge(time_diff, entity->player.shot_delay)) {
		vec2_fx32 pos = vec2_fx32_add(
			entity->sprite.pos,
			vec2_fx32_from_i32(3, 0));
		player_create_shot(state, pos);
		entity->player.last_shot_timestamp = current_time;
	}
	entity_update_bounding_box(entity);
}

void player_init(entity_t* player) {
	*player = (entity_t) {
		.entity_type = ENTITY_TYPE_PLAYER,
		.sprite={
			.pos=vec2_fx32_from_i32(64, 118),
			.index=0,
		},
		.vel=vec2_fx32_from_i32(120, 120),
		.is_alive=true,
		.bounding_box = {.w=fx32_from_i32(7), .h=fx32_from_i32(8)},
		.player.shot_delay=fx32_from_f32(0.25)
	};
}

