#include "grvgm.h"
#include "grv/grv_math.h"
#include "synth.h"
#include <stdio.h>

#include "audio_parameter.c"
#include "synth_audio.c"
#include "dsp.c"
#include "filter.c"
#include "gui.c"
#include "envelope.c"
#include "oscillator.c"
#include "transport.c"
#include "note_processor.c"
#include "simple_synth.c"
#include "sequencer.c"
#include "synth_track.c"

grv_arena_t* get_arena(synth_state_t* state) {
	return &state->transient.audio_arena;
}

void on_init(void** game_state, size_t* size) {
	grvgm_set_screen_size(256, 256);
	grvgm_set_sprite_size(16);
	grvgm_set_use_game_state_store(false);
	synth_state_t* synth_state = grv_alloc_zeros(sizeof(synth_state_t));
	synth_state_init(synth_state);
	if (synth_state->transient.audio_arena.data == NULL) {
		grv_arena_init(&synth_state->transient.audio_arena, 1*GRV_MEGABYTES);
	}
	if (synth_state->transient.audio_ringbuffer.data == NULL) {
		grv_ringbuffer_init(&synth_state->transient.audio_ringbuffer, 1<<16);
	}
	*game_state = synth_state;
	*size = sizeof(synth_state_t) - sizeof(synth_transient_state_t);
}

u32 make_id_u32(char* id) {
	grv_assert(strlen(id) == 4);
	return (u32)id[0] | ((u32)id[1] << 8) | ((u32)id[2] << 16) | ((u32)id[3] << 24);
}

typedef struct {
	u32 chunk_id;
	u32 chunk_size;
	u32 wave_id;
	u32 sub_chunk_fmt_id;
	u32 sub_chunk_fmt_size;
	u16 audio_format;
	u16 num_channels;
	u32 sample_rate;
	u32 byte_rate;
	u16 block_align;
	u16 bits_per_sample;
	u32 sub_chunk_data_id;
	u32 sub_chunk_data_size;
} wav_header_t;

void wav_header_init(wav_header_t* header, i32 num_channels, i32 sample_rate) {
	i32 bytes_per_frame = num_channels * sizeof(i16);
	*header = (wav_header_t) {
		// RGBA
		.chunk_id = 0x46464952, // "RIFF"
		.chunk_size = 0,
		.wave_id = 0x45564157, // "WAVE"
		.sub_chunk_fmt_id = 0x20746d66, // "fmt "
		.sub_chunk_fmt_size = 16,
		.audio_format = 1, // PCM
		.num_channels = (u16)num_channels,
		.sample_rate = (u32)sample_rate,
		.byte_rate = (u32)(sample_rate*bytes_per_frame),
		.block_align = (u16)(bytes_per_frame),
		.bits_per_sample = sizeof(i16) * 8,
		.sub_chunk_data_id = 0x61746164, // "data"
		.sub_chunk_data_size = 0
	};
}

FILE* recording_file = NULL;

void write_recording_buffer(synth_state_t* state) {
	grv_ringbuffer_read_to_file(&state->transient.audio_ringbuffer, recording_file);
}

bool start_recording(synth_state_t* state) {
	wav_header_t wav_header;
	wav_header_init(&wav_header, 2, AUDIO_SAMPLE_RATE);
	recording_file = fopen("recording.wav", "wb");
	if (recording_file == NULL) {
		return false;
	}
	fwrite(&wav_header, sizeof(wav_header_t), 1, recording_file);
	write_recording_buffer(state);
	return true;
}

void finalize_recording(synth_state_t* state) {
	write_recording_buffer(state);
	i64 bytes_written = ftell(recording_file);
	u32 chunk_size = (u32)(bytes_written - 8);
	u32 sub_chunk_data_size = (u32)(bytes_written - sizeof(wav_header_t));
	fseek(recording_file, offsetof(wav_header_t, chunk_size), SEEK_SET);
	fwrite(&chunk_size, sizeof(u32), 1, recording_file);
	fseek(recording_file, offsetof(wav_header_t, sub_chunk_data_size), SEEK_SET);
	fwrite(&sub_chunk_data_size, sizeof(u32), 1, recording_file);
	fclose(recording_file);
	recording_file = NULL;
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

	transport_state_t* transport = &synth_state->transport;

	bool is_recording = atomic_load(&transport->is_recording);
	if (is_recording && !transport->_was_recording) {
		bool success = start_recording(synth_state);
		if (!success) {
			is_recording = false;
			atomic_store(&transport->is_recording, is_recording);
		}
	} else if (!is_recording && transport->_was_recording) {
		finalize_recording(synth_state);
	} else if (is_recording) {
		write_recording_buffer(synth_state);
	}
	transport->_was_recording = is_recording;
}

