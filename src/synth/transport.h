#ifndef SYNTH_TRANSPORT_H
#define SYNTH_TRANSPORT_H
#include "synth_base.h"

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

void transport_state_init(transport_state_t* state);
void transport_process(transport_state_t* state);

#endif
