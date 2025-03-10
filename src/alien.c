typedef struct {
    entity_t entity;
    grv_vec2_fixed32_t start_pos;
    grv_fixed32_t max_displacement;
} alien_entity_t;

void alien_entity_update(entity_t* entity, grv_fixed32_t delta_t) {
    alien_entity_t* alien = (alien_entity_t*)entity;
    grv_fixed32_t new_x = grv_fixed32_mula(entity->vel.x, delta_t, entity->sprite.pos.x);
    new_x = entity->sprite.pos.x;

    grv_fixed32_t displacement = grv_fixed32_abs(grv_fixed32_sub(new_x, alien->start_pos.x));

    if (grv_fixed32_ge(displacement, alien->max_displacement)) {
        entity->vel.x = grv_fixed32_neg(entity->vel.x);
        i32 sign = grv_fixed32_gt(entity->sprite.pos.x, alien->start_pos.x) ? 1 : -1;
        entity->sprite.pos.x = grv_fixed32_mula(
            alien->max_displacement,
            grv_fixed32_from_i32(sign),
            alien->start_pos.x);
        entity->sprite.pos.y = grv_fixed32_add(
            alien->entity.sprite.pos.y,
            alien->entity.vel.y);
    } else {
        entity->sprite.pos.x = new_x;
    }

    entity_update_bounding_box(entity);
}

alien_entity_t* alien_entity_create(grv_vec2_fixed32_t pos, i32 direction) {
    alien_entity_t* alien = GRV_ALLOC_OBJECT_ZERO(alien_entity_t);
    *alien = (alien_entity_t) {
        .entity = {
            .sprite = {
                .pos=pos,
                .index=16,
            },
            .vel = grv_vec2_fixed32_from_i32(20 * direction, 4),
            .bounding_box = {.w=grv_fixed32_from_i32(7), .h=grv_fixed32_from_i32(8)},
            .update_func = alien_entity_update,
            .draw_func = entity_draw,
            .is_alive = true,
        },
        .start_pos = pos,
        .max_displacement = grv_fixed32_from_i32(16)
    };
    return alien;
}

void alien_create_wave(scene_t* scene, i32 num_rows, i32 num_cols) {
    i32 space_width = 16;
    i32 space_height = 16;
    //i32 aligrv_gmen_width = 8;
    //i32 alien_height = 8;
    i32 total_width = space_width * (num_cols - 1);
    i32 screen_width = 128;
    //i32 screen_height = 128;
    i32 x_start = (screen_width - total_width) / 2;
    i32 y_start = 16;

    for (int row = 0; row < num_rows; ++row) {
        const i32 y = y_start + row * space_height;
        for (int col = 0; col < num_cols; ++col) {
            const i32 x = x_start + col * space_width;
            grv_vec2_fixed32_t pos = grv_vec2_fixed32_from_i32(x, y);
            i32 direction = (row % 2) == 0 ? 1 : -1;
            alien_entity_t* alien = alien_entity_create(pos, direction);
            scene_add_entity(scene, (entity_t*)alien);
        }
    }
}

