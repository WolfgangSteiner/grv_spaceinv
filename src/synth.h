#ifndef SYNTH_H
#define SYNTH_H

#include "grvgm.h"
#include "grv/grv_arena.h"
#include "grv/grv_math.h"

#define AUDIO_FRAME_SIZE 32
#define PPQN 24
#define NUM_TRACKS 8

typedef struct {
	bool activated;
	i32 note_value;
	i32 length; // [ppqn]
	f32 amplitude;
} synth_pattern_step_t;


typedef enum {
	NOTE_EVENT_NONE,
	NOTE_EVENT_ON,
	NOTE_EVENT_OFF,
} note_event_type_t;

typedef struct {
	note_event_type_t type;
	f32 amplitude; 
	i32 note_value;
	i32 length;
} note_event_t;

typedef struct {
	synth_pattern_step_t steps[16];
	i32 num_steps;
} synth_pattern_t;

typedef struct {
	f64 note_ticks;
} sequencer_track_state_t;

typedef struct {
	sequencer_track_state_t track_state[NUM_TRACKS];
} sequencer_state_t;

typedef struct {
	f32 freq_state;
	f32 amp_state;
	f32 phase_state;
} synth_engine_state_t;

typedef enum {
	MAPPING_TYPE_LINEAR,
	MAPPING_TYPE_LOG,
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

typedef struct {
	grv_arena_t audio_arena;
	synth_engine_state_t synth_engine_state;
} synth_transient_state_t;

typedef enum {
	ENVELOPE_OFF,
	ENVELOPE_ATTACK,
	ENVELOPE_DECAY,
	ENVELOPE_SUSTAIN,
	ENVELOPE_RELEASE
} envelope_state_t;


typedef struct {
	envelope_state_t state;
	audio_parameter_t attack;
	audio_parameter_t decay;
	audio_parameter_t sustain;
	audio_parameter_t release;
	f32 y;
	f32 alpha;
	f32 alpha_attack;
	f32 alpha_decay;
	f32 alpha_release;
	f32 target_ratio_attack;
	f32 target_ratio_decay_release;
	f32 offset;
	f32 offset_attack;
	f32 offset_decay;
	f32 offset_release;
	f32 _prev_gate;
} envelope_t;

typedef struct {
	f32 _phase;
} oscillator_t;

typedef struct {
	f32 legato;
	f32 freq;
	f32 smoothed_freq;
	f32 gate;
	bool trigger_received;
} note_processor_t;

typedef struct {
	note_processor_t note_proc;
	oscillator_t oscillator;
	envelope_t envelope;
	audio_parameter_t vol;
	audio_parameter_t pan;
	f32 legato;
	f32 phase_state;
	f32 freq_state;
} simple_synth_t;

typedef struct {
	f64 pulse_time;
	f64 prev_pulse_time;
	f32 bpm;
	i32 bar;
	i32 beat;
	i32 pulse;
	bool is_playing;
	bool _was_playing;
} transport_state_t;

typedef enum {
	AUDIO_EFFECT_DELAY,
} audio_effect_type_t;

typedef struct {
	audio_effect_type_t type;
} audio_effect_t;

typedef struct {
	audio_parameter_t volume;
	audio_parameter_t pan;
} track_output_t;

typedef struct {
	simple_synth_t synth;
	struct {
		audio_effect_t arr[16];
		i32 capacity;
		i32 size;
	} audio_effects;
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
	sequencer_state_t sequencer_state;
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
	audio_parameter_t master_volume;
} synth_state_t;

void synth_state_init(synth_state_t* state);


#endif
