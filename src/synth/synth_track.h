#ifndef SYNTH_TRACK_H
#define SYNTH_TRACK_H
#include "synth_base.h"
#include "audio_parameter.h"
#include "simple_synth.h"
#include "audio_effect.h"

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

void synth_track_init(synth_track_t* track);

bool synth_track_serialize(grv_serializer_t* s, synth_track_t* track);

void track_process(
	f32* out_l,
	f32* out_r,
	synth_track_t* track,
	note_event_t* note_event,
	grv_arena_t* arena);
#endif
