#include "src/synth.h"

f32 lerp(f32* a, f32 b, f32 c) {
	*a = (*a) * (1.0f - c) + c * b;
	return *a;
}

f32 smooth_filter(f32* a, f32 b) {
	f32 c = 0.002;
	return lerp(a, b, c);
}

f32* arena_alloc_buffer(grv_arena_t* arena) {
	return grv_arena_alloc(arena, AUDIO_FRAME_SIZE*sizeof(f32));
}

f32* smooth_parameter(grv_arena_t* arena, f32* param, f32 target_param, f32 smoothing_coefficient) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	f32 y = *param;
	f32 a = 1.0f - smoothing_coefficient;
	f32 b = smoothing_coefficient;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		y = y * a + target_param * b;
		*dst++ = y;
	}
	*param = y;
	return outptr;
}

f32* process_envelope(grv_arena_t* arena, f32* trigger, envelope_t* envelope, i32 num_frames) {
	f32* outptr = arena_alloc_buffer(arena);
	f32 y = envelope->y;
	envelope_state_t state = envelope->state;
	return outptr;
}

f32* fill_phase_buffer(grv_arena_t* arena, f32* freq, f32* phase_state) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	f32 sr = GRVGM_SAMPLE_RATE;
	f32 ph = *phase_state;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		ph += (*freq++) / sr;
		if (ph >= 1.0f) ph -= 1.0f;
		*dst++ = ph;
	}
	*phase_state = ph;
	return outptr;
}

f32* sine_osc(grv_arena_t* arena, f32* phase, f32* amp) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = (*amp++) * sinf(*phase++ * TWO_PI_F32);
	}
	return outptr;
}

void render_pcm_stereo(i16* out, f32* left, f32* right, i32 frame_idx) {
	out += frame_idx * 2 * AUDIO_FRAME_SIZE;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*out++ = (f32)GRV_MAX_I16 * (*left++);
		*out++ = (f32)GRV_MAX_I16 * (*right++);
	}
}

void copy_audio_frame(f32* dst, f32* src) {
	memcpy(dst, src, AUDIO_FRAME_SIZE * sizeof(f32));
}

void process_transport(transport_state_t* state) {
	if (state->is_playing) {
		f64 pulse_time = state->pulse_time;
		state->prev_pulse_time = state->pulse_time;
		f64 pulse_time_per_frame = (f64)AUDIO_FRAME_SIZE / (f64)GRVGM_SAMPLE_RATE * state->bpm / 60.0f * PPQN;
		pulse_time += pulse_time_per_frame;
		f64 pattern_length = PPQN * 4; 
		if (pulse_time >= pattern_length) {
			pulse_time -= pattern_length;
			state->prev_pulse_time -= pattern_length;
		}
		state->pulse_time = pulse_time;
		state->bar = (i32)pulse_time / 4 / PPQN;
		state->beat = (i32)(pulse_time / PPQN) % 4;
		state->pulse = (i32)pulse_time % PPQN;
	}
}

f32* process_sequencer(synth_state_t* state) {
	f32* trigger_buffer = grv_arena_alloc(
		&state->transient.audio_arena,
		state->num_patterns * sizeof(f32));

	f64 prev_pulse_time = state->transport.prev_pulse_time;
	f64 pulse_time = state->transport.pulse_time;

	i32 prev_step = grv_floor_f64(prev_pulse_time) * 4 / PPQN;
	i32 step = grv_floor_f64(pulse_time) * 4 / PPQN;

	for (i32 i = 0; i < state->num_patterns; i++) {
		synth_pattern_t* pattern = &state->patterns[i];
		f32 trigger_value = 0.0f;

		if (step != prev_step
			&& step >= 0 && step < pattern->num_steps
			&& pattern->steps[step].activated) {
			trigger_value = 1.0f;
		}

		trigger_buffer[i] = trigger_value;
	}
	return trigger_buffer;
}


f32* process_track(synth_state_t* state, f32* trigger_buffer, i32 track_idx) {
	f32 trigger_value = trigger_buffer[track_idx];
	return NULL;
}


void on_audio(void* state, i16* stream, i32 num_frames) {
	synth_state_t* synth_state = state;
	i64 sample_time = synth_state->sample_time;
	grv_arena_t* arena = &synth_state->transient.audio_arena;
	synth_engine_state_t* engine_state = &synth_state->transient.synth_engine_state;
	for (i32 i = 0; i < num_frames / AUDIO_FRAME_SIZE; i++) {
		grv_arena_push_frame(arena);
		f32 target_amp = synth_state->start_selected ? 0.25 : 0.0;
		f32 target_freq = 110 * pow(2.0, synth_state->value / 128.0f);
		f32 bpm = 120.0;
		
		f32* freq = smooth_parameter(arena, &engine_state->freq_state, target_freq, 0.001f);
		f32* amp = smooth_parameter(arena, &engine_state->amp_state, target_amp, 0.002f);
		f32* phase = fill_phase_buffer(arena, freq, &engine_state->phase_state);
		f32* sine = sine_osc(arena, phase, amp);
		render_pcm_stereo(stream, sine, sine, i);
		grv_arena_pop_frame(arena);
		process_transport(&synth_state->transport);
	}
	
	grv_arena_reset(arena);
}
