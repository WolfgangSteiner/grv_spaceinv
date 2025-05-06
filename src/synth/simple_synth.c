#include "simple_synth.h"
#include "dsp.h"

void simple_synth_init(simple_synth_t* synth) {
	*synth = (simple_synth_t) {
		.oscillator = {
			.wave_type = WAVE_TYPE_SINE,
		},
		.vol = {
			.value = 0.0f,
			.min_value = -96.0f,
			.max_value = 0.0f,
			.mapping_type = MAPPING_TYPE_LINEAR,
		},
		.pan = {
			.value = 0.0f,
			.min_value = -1.0f,
			.max_value = 1.0f,
			.mapping_type = MAPPING_TYPE_LINEAR,
		},
	};
	synth_filter_init(&synth->filter);
	envelope_init(&synth->envelope);
	note_processor_init(&synth->note_proc);
}

void simple_synth_process(
	f32* buffer_l,
	f32* buffer_r,
	simple_synth_t* synth,
	note_event_t* note_event,
	grv_arena_t* arena) {
	grv_arena_push_frame(arena);
	note_processor_process(&synth->note_proc, note_event);
	f32* freq = smooth_value(
		synth->note_proc.freq,
		&synth->note_proc.smoothed_freq,
		0.01f,
		arena);
	f32* phase_diff = oscillator_fill_phase_diff_buffer(freq, arena);
	f32* phase = oscillator_fill_phase_buffer(phase_diff, &synth->phase_state, arena);
	f32* signal_buffer = oscillator_process(&synth->oscillator, phase, phase_diff, arena);

	synth_filter_process(signal_buffer, signal_buffer, &synth->filter, arena);

	f32* amp_env = envelope_process(
		synth->note_proc.gate,
		synth->note_proc.trigger_received,
		&synth->envelope,
		arena);
	audio_buffer_mul(signal_buffer, signal_buffer, amp_env);
	//f32* pan = audio_parameter_smooth(&synth->pan, arena);
	//process_mono_to_stereo(buffer_l, buffer_r, signal_buffer, pan);
	audio_buffer_copy(buffer_l, signal_buffer);
	audio_buffer_copy(buffer_r, signal_buffer);
	grv_arena_pop_frame(arena);
}
