#ifndef SYNTH_H
#define SYNTH_H

#include "grvgm.h"
#include "grv/grv_arena.h"
#include "grv/grv_math.h"
#include "synth_base.h"
#include "audio_parameter.h"
#include "filter.h"
#include "envelope.h"
#include "oscillator.h"
#include "transport.h"
#include "note_processor.h"
#include "simple_synth.h"
#include "sequencer.h"
#include "synth_track.h"

typedef struct {
	grv_arena_t audio_arena;
} synth_transient_state_t;

typedef struct {
	synth_pattern_arr_t patterns;
	sequencer_state_t sequencer_state;
	struct {
		synth_track_t arr[8];
		i32 capacity;
		i32 size;
	} tracks;
	i32 selected_track;
	i32 current_pattern;
	i32 sample_rate;
	i64 sample_time;
	transport_state_t transport;
	synth_transient_state_t transient;
	audio_parameter_t master_volume;
} synth_state_t;

void synth_state_init(synth_state_t* state);
grv_arena_t* get_arena(synth_state_t* state);

#endif
