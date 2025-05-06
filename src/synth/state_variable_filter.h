#ifndef STATE_VARIABLE_FILTER_H
#define STATE_VARIABLE_FILTER_H

#include "grv/grv_base.h"

typedef struct {
	f32 c1;
	f32 c2;
	f32 d0;
	f32 d1;
	f32 d2;
} state_variable_filter_coefficients_t;

typedef struct {
	f32 z1, z2;
} state_variable_filter_state_t;

void state_variable_filter_compute_coefficients_low_pass(
	state_variable_filter_coefficients_t* c, f32 f, f32 q);

void state_variable_filter_process_low_pass(
	f32* dst,
	f32* src,
	state_variable_filter_coefficients_t* c,
	state_variable_filter_state_t* s,
	i32 num_samples);

#endif
