#ifndef SPACEINV_H
#define SPACEINV_H

//==============================================================================
// sprite animation
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


//==============================================================================
// entity
//==============================================================================
typedef enum {
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_CLAW,
} entity_type_t;

typedef enum {
    PLAYER_STATE_NORMAL,
    PLAYER_STATE_EXPLODING,
    PLAYER_STATE_DEAD,
} player_state_t;

typedef struct {
    grv_fixed32_t last_shot_timestamp;
    grv_fixed32_t shot_delay;
    player_state_t state;
    grv_fixed32_t state_start_time;
} player_data_t;

typedef struct {
    grv_fixed32_t max_displacement;
} alien_claw_data_t;

typedef struct entity_s {
    entity_type_t entity_type;
    grvgm_sprite_t sprite;
    grv_vec2_fixed32_t start_pos;
    grv_vec2_fixed32_t vel;
    grv_rect_fixed32_t bounding_box;
    bool is_alive;
    union {
        player_data_t player;
        alien_claw_data_t alien_claw;
    };
} entity_t;

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
        entity_t arr[128];
        i32 size;
        i32 capacity;
    } entity_arr;
} scene_t;

//==============================================================================
// particle effect
//==============================================================================
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

//==============================================================================
// game state
//==============================================================================
typedef struct {
    scene_t scene;
    entity_t player;
    particle_effect_t player_explosion_effect;
    struct {
        shot_t arr[128];
        i32 size;
        i32 capacity;
    } shot_arr;
} spaceinv_state_t;

#endif
