#include "synth_track.h"
#include "dsp.h"

void track_process(
	f32* out_l,
	f32* out_r,
	synth_track_t* track,
	note_event_t* note_event,
	grv_arena_t* arena) {
	grv_arena_push_frame(arena);
	f32* buffer_l = audio_buffer_alloc(arena);
	f32* buffer_r = audio_buffer_alloc(arena);
	simple_synth_process(buffer_l, buffer_r, &track->synth, note_event, arena);
	//process_volume(buffer_l, buffer_r, &track->output.volume, arena);
	audio_buffer_add_to(out_l, buffer_l);
	audio_buffer_add_to(out_r, buffer_r);

	grv_arena_pop_frame(arena);
}

void synth_track_init(synth_track_t* track) {
	*track = (synth_track_t) {
		.audio_effects = {
			.capacity = 4,
		},
		.output = {
			.volume = {
				.value = -6.0f,
				.min_value = -96.0f,
				.max_value = 12.0f,
				.mapping_type = MAPPING_TYPE_LINEAR,
			},
			.pan = {
				.value = 0.0f,
				.min_value = -1.0f,
				.max_value = 1.0f,
				.mapping_type = MAPPING_TYPE_LINEAR,
			}
		}
	};
	simple_synth_init(&track->synth);
}

void synth_track_serialize(grv_serializer_t* s, synth_track_t* track) {
	grv_serialize_struct_begin(s, 0);

	grv_serialize_struct_field(s, "synth");
	simple_synth_serialize(s, &track->synth);

	grv_serialize_struct_field(s, "volume");
	audio_parameter_serialize(s, &track->output.volume);

	grv_serialize_struct_field(s, "pan");
	audio_parameter_serialize(s, &track->output.pan);

	grv_serialize_struct_end(s);
}


