#include "synth.h"
#include "SDL2/SDL.h"
#include "dsp.h"
#include "envelope.h"
#include "oscillator.h"
#include "transport.h"
#include "note_processor.h"
#include "sequencer.h"

void process_mono_to_stereo(f32* left, f32* right, f32* in, f32* pan) {
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		f32 phi = HALF_PI_F32 * 0.5f * ((*pan++) + 1.0f);
		f32 in_value = *in++;
		*left++ = cosf(phi) * in_value;
		*right++ = sinf(phi) * in_value;
	}
}

f32* process_volume_to_amp(f32* volume_buffer, grv_arena_t* arena) {
	f32* amp_buffer = audio_buffer_alloc(arena);
	f32* src = volume_buffer;
	f32* dst = amp_buffer;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = from_db(*src++);
	}
	return amp_buffer;
}

void process_volume(f32* buffer_l, f32* buffer_r, audio_parameter_t* vol, grv_arena_t* arena) {
	grv_arena_push_frame(arena);
	f32* amp_db = audio_parameter_smooth(vol, arena);
	f32* amp = process_volume_to_amp(amp_db, arena);
	audio_buffer_mul(buffer_l, buffer_l, amp);
	audio_buffer_mul(buffer_r, buffer_r, amp);
	grv_arena_pop_frame(arena);
}

void synth_state_init(synth_state_t* state) {
	i32 num_tracks = 8;
	*state = (synth_state_t) {
		.sample_rate = GRVGM_SAMPLE_RATE,
		.patterns = {
			.size = num_tracks,
			.capacity = num_tracks,
		},
		.tracks = {
			.size = num_tracks,
			.capacity = num_tracks,
		}, 
		.master_volume = {
			.min_value=-72.0f,
			.max_value=0.0f,
			.value=-6.0f,
			.smoothing_coefficient=0.01f,
			.mapping_type=MAPPING_TYPE_VOLUME,
		},
	};

	for (i32 i = 0; i < num_tracks; i++) {
		pattern_init(&state->patterns.arr[i]);
		synth_track_init(&state->tracks.arr[i]);
	}

	transport_state_init(&state->transport);
}

void on_audio(void* state, i16* stream, i32 num_frames) {
	synth_state_t* synth_state = state;
	grv_arena_t* arena = &synth_state->transient.audio_arena;
	i32 num_tracks = synth_state->tracks.size;
	f32* out_l = audio_buffer_alloc_zero(arena);
	f32* out_r = audio_buffer_alloc_zero(arena);

	for (i32 frame_idx = 0; frame_idx < num_frames / AUDIO_FRAME_SIZE; frame_idx++) {
		grv_arena_push_frame(arena);
		audio_buffer_clear(out_l);
		audio_buffer_clear(out_r);

		transport_process(&synth_state->transport);
		note_event_t* note_event_buffer = sequencer_process(
			&synth_state->sequencer_state,
			&synth_state->transport,
			&synth_state->patterns,
			&synth_state->transient.audio_arena);
		
		for (i32 track_idx = 0; track_idx < num_tracks; track_idx++) {
			synth_track_t* track = &synth_state->tracks.arr[track_idx];
			track_process(out_l, out_r, track, &note_event_buffer[track_idx], arena);
		}

		process_volume(out_l, out_r, &synth_state->master_volume, arena);
		render_pcm_stereo(stream, out_l, out_r, frame_idx);

		grv_arena_pop_frame(arena);
	}
	grv_arena_reset(arena);
}
