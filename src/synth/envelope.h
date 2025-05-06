#ifndef SYNTH_ENVELOPE_H
#define SYNTH_ENVELOPE_H

#include "audio_parameter.h"

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

void envelope_init(envelope_t* envelope);
f32* envelope_process(f32 gate, bool trigger_received, envelope_t* env, grv_arena_t* arena);

#endif
