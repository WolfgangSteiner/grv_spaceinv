#ifndef SYNTH_SIMPLE_SYNTH_H
#define SYNTH_SIMPLE_SYNTH_H
#include "note_processor.h"
#include "oscillator.h"
#include "filter.h"
#include "envelope.h"

typedef struct {
	note_processor_t note_proc;
	oscillator_t oscillator;
	synth_filter_t filter;
	envelope_t filter_envelope;
	audio_parameter_t filter_envelope_to_frequency;
	audio_parameter_t filter_envelope_to_resonance;
	envelope_t envelope;
	audio_parameter_t vol;
	audio_parameter_t pan;
	f32 legato;
	f32 phase_state;
	f32 freq_state;
} simple_synth_t;

void simple_synth_init(simple_synth_t* synth);
void simple_synth_process(
	f32* buffer_l,
	f32* buffer_r,
	simple_synth_t* synth,
	note_event_t* note_event,
	grv_arena_t* arena);

bool simple_synth_serialize(grv_serializer_t* s, simple_synth_t* synth);

#endif
