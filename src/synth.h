#ifndef SYNTH_H
#define SYNTH_H

#include "grvgm.h"
#include "grv/grv_arena.h"
#include "grv/grv_math.h"

#define AUDIO_FRAME_SIZE 32
#define PPQN 24

typedef struct {
	bool activated;
	i32 note_value;
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
	f32 value;
	f32 min_value;
	f32 max_value;
	f32 smoothed_value;
} audio_parameter_t;

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
	audio_parameter_t attack;
	audio_parameter_t decay;
	audio_parameter_t sustain;
	audio_parameter_t release;
	f32 y;
	envelope_state_t state;
} envelope_t;

typedef struct {
	audio_parameter_t freq;
} oscillator_t;

typedef struct {
	oscillator_t oscillator;
	envelope_t envelope;
	audio_parameter_t vol;
	audio_parameter_t pan;
} simple_synth_t;

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

} audio_effect_t;


typedef struct {
	audio_parameter_t volume;
	audio_parameter_t pan;
} track_output_t;

typedef struct {
	simple_synth_t synth;
	audio_effect_t effects[16];
	track_output_t output;
} synth_track_t;

typedef struct {
	bool start_selected;
	i32 value;
	struct {
		synth_pattern_t arr[8];
		i32 capacity;
		i32 size;
	} patterns;
	struct {
		synth_track_t arr[8];
		i32 capacity;
		i32 size;
	} tracks;
	i32 current_pattern;
	i32 sample_rate;
	i64 sample_time;
	transport_state_t transport;
	synth_transient_state_t transient;
} synth_state_t;

#endif
