#include "grvgm.h"
#include "grv/grv_math.h"
#include "src/synth.h"
#include <stdio.h>

void on_init(void** game_state, size_t* size) {
	synth_state_t* synth_state = grv_alloc_zeros(sizeof(synth_state_t));

	if (synth_state->transient.audio_arena.data == NULL) {
		grv_arena_init(&synth_state->transient.audio_arena, 1*GRV_MEGABYTES);
	}
	synth_state->sample_rate = GRVGM_SAMPLE_RATE;
	synth_state->transport.bpm = 120.0;
	synth_state->transport.prev_pulse_time = -1.0;
	*game_state = synth_state;
	*size = sizeof(synth_state_t) - sizeof(synth_transient_state_t);
}

void on_update(void* game_state, float delta_time) {
	//synth_state.value = fx32_round(fx32_mul_i32(grvgm_time(), 32)) % 128;
}

