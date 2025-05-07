#ifndef SYNTH_FILTER_H
#define SYNTH_FILTER_H

#include "state_variable_filter.h"
#include "audio_parameter.h"
#include "grv/grv_arena.h"

typedef enum {
	FILTER_TYPE_LP12,
	FILTER_TYPE_LP24,
	FILTER_TYPE_HP12,
	FILTER_TYPE_HP24,
} filter_type_t;

typedef struct {
	filter_type_t filter_type;
	audio_parameter_t f;
	audio_parameter_t q;
	state_variable_filter_coefficients_t coefficients;
	state_variable_filter_state_t state[2];
} synth_filter_t;

void synth_filter_init(synth_filter_t* filter);
void synth_filter_process(f32* dst, f32* src, synth_filter_t* filter, f32* f, f32* q, grv_arena_t* arena);

#endif
