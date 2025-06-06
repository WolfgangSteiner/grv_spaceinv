void alien_entity_update(entity_t* entity, fx32 delta_t) {
	fx32 new_x = fx32_mula(entity->vel.x, delta_t, entity->pos.x);
	fx32 displacement = fx32_abs(fx32_sub(new_x, entity->start_pos.x));

	if (fx32_ge(displacement, entity->alien_claw.max_displacement)) {
		entity->vel.x = fx32_neg(entity->vel.x);
		i32 sign = fx32_gt(entity->pos.x, entity->start_pos.x) ? 1 : -1;
		entity->pos.x = fx32_mula(
			entity->alien_claw.max_displacement,
			fx32_from_i32(sign),
			entity->start_pos.x);
		entity->pos.y = fx32_add(
			entity->pos.y,
			entity->vel.y);
	} else {
		entity->pos.x = new_x;
	}
}

void alien_entity_create(entity_t* entity, vec2_fx32 pos, i32 direction) {
	*entity = (entity_t) {
		.entity_type = ENTITY_TYPE_CLAW,
		.sprite = {
			.index=16,
		},
		.pos=pos,
		.vel = vec2_fx32_from_i32(20 * direction, 4),
		.bounding_box = {.w=fx32_from_i32(7), .h=fx32_from_i32(8)},
		.is_alive = true,
		.start_pos = pos,
		.alien_claw = { .max_displacement = fx32_from_i32(16) }
	};
}

void alien_create_wave(scene_t* scene, i32 num_rows, i32 num_cols) {
	i32 space_width = 16;
	i32 space_height = 16;
	i32 total_width = space_width * (num_cols - 1);
	i32 screen_width = 128;
	i32 x_start = (screen_width - total_width) / 2;
	i32 y_start = 16;

	for (int row = 0; row < num_rows; ++row) {
		const i32 y = y_start + row * space_height;
		for (int col = 0; col < num_cols; ++col) {
			const i32 x = x_start + col * space_width;
			vec2_fx32 pos = vec2_fx32_from_i32(x, y);
			i32 direction = (row % 2) == 0 ? 1 : -1;
			entity_t* entity = scene_add_entity(scene);
			if (entity) alien_entity_create(entity, pos, direction);
		}
	}
}

