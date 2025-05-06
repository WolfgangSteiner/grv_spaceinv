#ifndef SYNTH_OSCILLATOR_H
#define SYNTH_OSCILLATOR_H

#include "synth_base.h"
#include "grv/grv_arena.h"

typedef enum {
	WAVE_TYPE_SINE,
	WAVE_TYPE_RECT,
	WAVE_TYPE_SAW,
	WAVE_TYPE_TRIANGLE,
	WAVE_TYPE_NOISE,
	WAVE_TYPE_COUNT,
} wave_type_t;

typedef struct {
	wave_type_t wave_type;
	f32 _phase;
} oscillator_t;

f32* oscillator_process(
	oscillator_t* osc,
	f32* phase,
	f32* phase_diff,
	grv_arena_t* arena);

f32* oscillator_fill_phase_diff_buffer(
	f32* freq,
	grv_arena_t* arena);

f32* oscillator_fill_phase_buffer(
	f32* phase_diff,
	f32* phase_state,
	grv_arena_t* arena);

#endif
