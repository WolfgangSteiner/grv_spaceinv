#ifndef SYNTH_H
#define SYNTH_H

#include "grvgm.h"
#include "grv/grv_arena.h"
#include "grv/grv_math.h"

#define AUDIO_FRAME_SIZE 32
#define PPQN 24

typedef struct {
	bool activated;
} synth_pattern_step_t;

typedef struct {
	synth_pattern_step_t steps[16];
	i32 num_steps;
} synth_pattern_t;

typedef struct {
	f32 freq_state;
	f32 amp_state;
	f32 phase_state;
} synth_engine_state_t;

typedef struct {
	grv_arena_t audio_arena;
	synth_engine_state_t synth_engine_state;
} synth_transient_state_t;

typedef enum {
	ADSR_OFF,
	ADSR_ATTACK,
	ADSR_SUSTAIN,
	ADSR_RELEASE
} envelope_state_t;

typedef struct {
	f32 attack;
	f32 decay;
	f32 sustain;
	f32 release;
	envelope_state_t state;
	f32 y;
	i32 cycle_time;
} envelope_t;

typedef struct {
	f64 pulse_time;
	f64 prev_pulse_time;
	f32 bpm;
	i32 bar;
	i32 beat;
	i32 pulse;
	bool is_playing;
} transport_state_t;

typedef struct {
	bool start_selected;
	i32 value;
	synth_pattern_t patterns[8];
	i32 current_pattern;
	i32 num_patterns;
	i32 sample_rate;
	i64 sample_time;
	transport_state_t transport;
	synth_transient_state_t transient;
} synth_state_t;

#endif
