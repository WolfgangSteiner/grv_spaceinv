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
	MAPPING_TYPE_TUNING_OCTAVES,
	MAPPING_TYPE_TUNING_SEMITONES,
	MAPPING_TYPE_TUNING_CENTS,
} mapping_type_t;

typedef struct {
	f32 value;
	mapping_type_t mapping_type;
	f32 min_value;
	f32 max_value;
	f32 smoothed_value;
	f32 smoothing_coefficient;
	f32 sensitivity;
	bool is_bipolar;
	f32 _prev_value;
	f32 _initial_drag_value;
} audio_parameter_t;

bool audio_parameter_is_discrete(audio_parameter_t* p);
bool audio_parameter_is_bipolar(audio_parameter_t* p);
f32* audio_parameter_smooth(audio_parameter_t* p, grv_arena_t* arena);
void audio_parameter_set_to_user_value(audio_parameter_t* p, f32 user_value);
f32 audio_parameter_map_to_physical_value(audio_parameter_t* p);
char* audio_parameter_value_as_string(audio_parameter_t* p, grv_arena_t* arena);
bool audio_parameter_is_bipolar(audio_parameter_t* p);
f32 audio_parameter_map_to_gui_relative_value(audio_parameter_t* p);
#endif
