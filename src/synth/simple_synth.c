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
		.filter_envelope_to_frequency = {
			.value = 0.0f,
			.min_value = -1.0f,
			.max_value = 1.0f,
			.mapping_type = MAPPING_TYPE_LINEAR,
		},
		.filter_envelope_to_resonance = {
			.value = 0.0f,
			.min_value = -1.0f,
			.max_value = 1.0f,
			.mapping_type = MAPPING_TYPE_LINEAR,
		}
	};
	synth_filter_init(&synth->filter);
	envelope_init(&synth->envelope);
	envelope_init(&synth->filter_envelope);
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
	f32* filter_env = envelope_process(
		&synth->filter_envelope,
		synth->note_proc.gate,
		synth->note_proc.trigger_received,
		arena);

	f32* f = audio_parameter_smooth(&synth->filter.f, arena);
	f32* q = audio_parameter_smooth(&synth->filter.q, arena);

	f32 log_f_min = 0.0f;
	f32 log_f_max = freq_to_log(synth->filter.f.max_value, synth->filter.f.min_value);
	f = audio_buffer_modulate_add(f, filter_env, synth->filter_envelope_to_frequency.value, log_f_min, log_f_max, arena);

	synth_filter_process(signal_buffer, signal_buffer, &synth->filter, f, q, arena);

	f32* amp_env = envelope_process(
		&synth->envelope,
		synth->note_proc.gate,
		synth->note_proc.trigger_received,
		arena);
	audio_buffer_mul(signal_buffer, signal_buffer, amp_env);
	//f32* pan = audio_parameter_smooth(&synth->pan, arena);
	//process_mono_to_stereo(buffer_l, buffer_r, signal_buffer, pan);
	audio_buffer_copy(buffer_l, signal_buffer);
	audio_buffer_copy(buffer_r, signal_buffer);
	grv_arena_pop_frame(arena);
}
