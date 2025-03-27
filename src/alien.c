void alien_entity_update(entity_t* entity, grv_fixed32_t delta_t) {
    grv_fixed32_t new_x = grv_fixed32_mula(entity->vel.x, delta_t, entity->sprite.pos.x);
    grv_fixed32_t displacement = grv_fixed32_abs(grv_fixed32_sub(new_x, entity->start_pos.x));

    if (grv_fixed32_ge(displacement, entity->alien_claw.max_displacement)) {
        entity->vel.x = grv_fixed32_neg(entity->vel.x);
        i32 sign = grv_fixed32_gt(entity->sprite.pos.x, entity->start_pos.x) ? 1 : -1;
        entity->sprite.pos.x = grv_fixed32_mula(
            entity->alien_claw.max_displacement,
            grv_fixed32_from_i32(sign),
            entity->start_pos.x);
        entity->sprite.pos.y = grv_fixed32_add(
            entity->sprite.pos.y,
            entity->vel.y);
    } else {
        entity->sprite.pos.x = new_x;
    }
}

void alien_entity_create(entity_t* entity, grv_vec2_fixed32_t pos, i32 direction) {
    *entity = (entity_t) {
        .entity_type = ENTITY_TYPE_CLAW,
        .sprite = {
            .pos=pos,
            .index=16,
        },
        .vel = grv_vec2_fixed32_from_i32(20 * direction, 4),
        .bounding_box = {.w=grv_fixed32_from_i32(7), .h=grv_fixed32_from_i32(8)},
        .is_alive = true,
        .start_pos = pos,
        .alien_claw = { .max_displacement = grv_fixed32_from_i32(16) }
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
            grv_vec2_fixed32_t pos = grv_vec2_fixed32_from_i32(x, y);
            i32 direction = (row % 2) == 0 ? 1 : -1;
            entity_t* entity = scene_add_entity(scene);
            if (entity) alien_entity_create(entity, pos, direction);
        }
    }
}

