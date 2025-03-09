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

typedef struct {
    entity_t entity;
} player_entity_t;


void player_entity_update(entity_t* entity, grv_fixed32_t delta_t) {
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
        }
    };
    return p;
}

