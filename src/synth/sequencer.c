#include "note_event.h"
#include "sequencer.h"
#include "grv/grv_arena.h"

note_event_t* sequencer_process(
	sequencer_state_t* sequencer_state,
	transport_state_t* transport,
	synth_pattern_arr_t* patterns,
	grv_arena_t* arena) {
	i32 num_patterns = patterns->size;
	note_event_t* event_buffer = grv_arena_alloc_zero(
		arena,
		num_patterns * sizeof(note_event_t));

	f64 prev_pulse_time = transport->prev_pulse_time;
	f64 pulse_time = transport->pulse_time;
	f64 delta_ticks = pulse_time - prev_pulse_time;

	i32 prev_step = grv_floor_f64(prev_pulse_time) * 4 / PPQN;
	i32 step = grv_floor_f64(pulse_time) * 4 / PPQN;

	for (i32 i = 0; i < num_patterns; i++) {
		synth_pattern_t* pattern = &patterns->arr[i];
		sequencer_track_state_t* sequencer_track_state = &sequencer_state->track_state[i];
		note_event_t* event = &event_buffer[i];

		if (transport->is_playing
			&& step != prev_step
			&& step >= 0 && step < pattern->num_steps
			&& pattern->steps[step].activated) {
			synth_pattern_step_t* pattern_step = &pattern->steps[step];
			*event = (note_event_t){
				.type = NOTE_EVENT_ON,
				.note_value = pattern_step->note_value,
				.length = pattern_step->length,
				.velocity = pattern_step->velocity,
			};
			sequencer_track_state->note_ticks = pattern_step->length;
		} else if (sequencer_track_state->note_ticks) {
			sequencer_track_state->note_ticks =
				grv_max_f64(sequencer_track_state->note_ticks - delta_ticks, 0.0);
			if (sequencer_track_state->note_ticks == 0) {
				*event = (note_event_t) {
					.type = NOTE_EVENT_OFF
				};
			}
		}
	}
	return event_buffer;
}

void pattern_init(synth_pattern_t* pattern) {
	*pattern = (synth_pattern_t){
		.num_steps = 16,
	};

	for (i32 i = 0; i < pattern->num_steps; i++) {
		synth_pattern_step_t* step = &pattern->steps[i];
		*step = (synth_pattern_step_t){
			.note_value = 45, // 440Hz
			.length = PPQN / 4,
			.velocity = 1.0f,
		};
	}
}

void synth_pattern_step_serialize(synth_pattern_step_t* step, grv_serializer_t* s) {
	grv_serialize_struct_begin(s);
	grv_serialize_struct_field_bool(s, "activated", step->activated);
	grv_serialize_struct_field_int(s, "note_value", step->note_value);
	grv_serialize_struct_field_int(s, "length", step->length);
	grv_serialize_struct_field_float(s, "velocity", step->velocity);
	grv_serialize_struct_end(s);
}

void synth_pattern_serialize(synth_pattern_t* pattern, grv_serializer_t* s) {
	grv_serialize_struct_begin(s);
	grv_serialize_struct_field(s, "steps");
	grv_serialize_array_begin(s, pattern->num_steps);
	for (i32 i = 0; i < pattern->num_steps; i++) {
		synth_pattern_step_t* step = &pattern->steps[i];
		synth_pattern_step_serialize(step, s);
	}
	grv_serialize_struct_end(s);
}

#include "grv/grv_cstr.h"

bool synth_pattern_deserialize(synth_pattern_t* pattern, grv_serializer_t* s) {
	bool success = false;
	u32 version;
	success = grv_deserialize_struct_begin(s, &version);

	while (true) {
		char* field_name;
		success = grv_deserialize_struct_field_next(s, &field_name);
		if (success == false) return false;
		if (field_name == NULL) break;
		else if (grv_cstr_eq(field_name, ))
	}

	success = grv_deserialize_struct_end(s);

end:
	return success;
}
