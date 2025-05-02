#include "grvgm.h"
#include "grv/grv_math.h"
#include "src/synth.h"
#include <stdio.h>

void on_init(void** game_state, size_t* size) {
	synth_state_t* synth_state = grv_alloc_zeros(sizeof(synth_state_t));
	synth_state_init(synth_state);
	if (synth_state->transient.audio_arena.data == NULL) {
		grv_arena_init(&synth_state->transient.audio_arena, 1*GRV_MEGABYTES);
	}
	*game_state = synth_state;
	*size = sizeof(synth_state_t) - sizeof(synth_transient_state_t);
}

void on_update(void* game_state, float delta_time) {
	GRV_UNUSED(delta_time);
	synth_state_t* synth_state = game_state;
	i32 num_tracks = synth_state->tracks.size;

	if (grvgm_key_was_pressed(' ')) {
		synth_state->transport.is_playing = !synth_state->transport.is_playing;
	} else if (grvgm_key_was_pressed_with_mod('\t', GRVGM_KEYMOD_SHIFT)) {
		synth_state->selected_track = (synth_state->selected_track + num_tracks - 1) % num_tracks;
	} else if (grvgm_key_was_pressed('\t')) {
		synth_state->selected_track = (synth_state->selected_track + 1) % num_tracks;
	}
}

