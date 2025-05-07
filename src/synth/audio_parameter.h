#ifndef AUDIO_PARAMETER_H
#define AUDIO_PARAMETER_H

#include "grv/grv_base.h"
#include "grv/grv_arena.h"

typedef enum {
	MAPPING_TYPE_LINEAR,
	MAPPING_TYPE_LOG,
	MAPPING_TYPE_LOG_FREQUENCY,
	MAPPING_TYPE_DB,
	MAPPING_TYPE_VOLUME,
	MAPPING_TYPE_LOG_TIME,
} mapping_type_t;

typedef struct {
	f32 value;
	f32 min_value;
	f32 max_value;
	f32 smoothed_value;
	f32 smoothing_coefficient;
	f32 sensitivity;
	f32 _initial_drag_value;
	mapping_type_t mapping_type;
	f32 _prev_value;
} audio_parameter_t;

f32* audio_parameter_smooth(audio_parameter_t* p, grv_arena_t* arena);
bool audio_parameter_is_bipolar(audio_parameter_t* p);

#endif
