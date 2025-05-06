#include "transport.h"

void transport_state_init(transport_state_t* state) {
	*state = (transport_state_t) {
		.bpm = 120.0,
		.prev_pulse_time = -1.0e6,
	};
}

void transport_process(transport_state_t* state) {
	if (state->is_playing) {
		if (state->_was_playing) {
			f64 pulse_time = state->pulse_time;
			state->prev_pulse_time = state->pulse_time;
			f64 pulse_time_per_frame = (f64)AUDIO_FRAME_SIZE / (f64)AUDIO_SAMPLE_RATE * state->bpm / 60.0f * PPQN;
			pulse_time += pulse_time_per_frame;
			f64 pattern_length = PPQN * 4; 
			if (pulse_time >= pattern_length) {
				pulse_time -= pattern_length;
			}
			state->pulse_time = pulse_time;
			state->bar = (i32)pulse_time / 4 / PPQN;
			state->beat = (i32)(pulse_time / PPQN) % 4;
			state->pulse = (i32)pulse_time % PPQN;
		}
		state->_was_playing = true;
	} else {
		state->_was_playing = false;
	}
}


