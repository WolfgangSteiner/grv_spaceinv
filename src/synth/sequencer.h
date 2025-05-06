#ifndef SYNTH_SEQUENCER_H
#define SYNTH_SEQUENCER_H
#include "note_event.h"
#include "synth_base.h"
#include "transport.h"
#include "grv/grv_arena.h"

typedef struct {
	bool activated;
	i32 note_value;
	i32 length; // [ppqn]
	f32 amplitude;
} synth_pattern_step_t;

typedef struct {
	synth_pattern_step_t steps[16];
	i32 num_steps;
} synth_pattern_t;

typedef struct {
	synth_pattern_t arr[8];
	i32 capacity;
	i32 size;
} synth_pattern_arr_t;

typedef struct {
	f64 note_ticks;
} sequencer_track_state_t;

typedef struct {
	sequencer_track_state_t track_state[NUM_TRACKS];
} sequencer_state_t;

note_event_t* sequencer_process(
	sequencer_state_t* sequencer_state,
	transport_state_t* transport,
	synth_pattern_arr_t* patterns,
	grv_arena_t* arena);

void pattern_init(synth_pattern_t* pattern);

#endif
