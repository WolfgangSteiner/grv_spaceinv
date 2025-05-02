#include "src/synth.h"
#include "SDL2/SDL.h"

f32* arena_alloc_buffer(grv_arena_t* arena) {
	return grv_arena_alloc(arena, AUDIO_FRAME_SIZE*sizeof(f32));
}

f32* arena_alloc_zero_buffer(grv_arena_t* arena) {
	f32* buffer = arena_alloc_buffer(arena);
	memset(buffer, 0, AUDIO_FRAME_SIZE*sizeof(f32));
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		buffer[i] = 0.0f;
	}
	return buffer;
}

f32 note_value_to_frequency(i32 note_value) {
	return 440.0f * powf(2.0f, (note_value - 69) / 12.0f);
}

f32 lerp(f32* a, f32 b, f32 c) {
	*a = (*a) * (1.0f - c) + c * b;
	return *a;
}

f32 smooth_filter(f32* a, f32 b) {
	f32 c = 0.002;
	return lerp(a, b, c);
}

void process_mul(f32* dst, f32* a, f32* b) {
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = (*a++) * (*b++);
	}
}

void audio_buffer_copy(f32* dst, f32* src) {
	memcpy(dst, src, AUDIO_FRAME_SIZE * sizeof(f32));
}

void audio_buffer_clear(f32* dst) {
	memset(dst, 0, AUDIO_FRAME_SIZE * sizeof(f32));
}

void audio_buffer_mul(f32* dst, f32* a, f32* b) {
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = *a++ * *b++;
	}
}

f32* audio_buffer_from_constant(f32 c, grv_arena_t* arena) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = c;
	}

	return outptr;
}

GRV_INLINE f32 from_db(f32 val_db) {
	return val_db <= -72.f ? 0.0f : powf(10.0f, val_db/20.0f);
}

void audio_buffer_from_db(f32* dst, f32* src) {
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = from_db(*src++);
	}
}


f32* smooth_audio_parameter(audio_parameter_t* p, grv_arena_t* arena) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	f32 y = p->smoothed_value;
	f32 y_target = p->value;
	f32 alpha = p->smoothing_coefficient == 0.0f ? 0.01 : p->smoothing_coefficient;
	f32 a = 1.0f - alpha;
	f32 b = alpha;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		y = y * a + y_target * b;
		*dst++ = y;
	}
	p->smoothed_value = y;
	return outptr;
}

f32* smooth_value(f32 y_target, f32* y_state, f32 alpha, grv_arena_t* arena){
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	f32 y = *y_state;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		y += (y_target - y) * alpha;
		*dst++ = y;
	}
	*y_state = y;
	return outptr;
}

void clear_buffer(f32* buffer) {
	memset(buffer, 0, AUDIO_FRAME_SIZE * sizeof(f32));
}

f32 envelope_compute_alpha(f32 t_samples, f32 target_ratio) {
	return t_samples <= 0.0f 
		? 1.0f 
		: expf(-logf((1.0f + target_ratio) / target_ratio) / t_samples);
} 

void envelope_set_attack_time(envelope_t* env, f32 t_s) {
	t_s = grv_clamp_f32(t_s, env->attack.min_value, env->attack.max_value);
	f32 attack_rate = t_s * GRVGM_SAMPLE_RATE;
	env->attack.value = t_s;
	env->alpha_attack = envelope_compute_alpha(attack_rate, env->target_ratio_attack);
	env->offset_attack = (1.0f + env->target_ratio_attack) * (1.0f - env->alpha_attack);
}

void envelope_set_decay_time(envelope_t* env, f32 t_s) {
	t_s = grv_clamp_f32(t_s, env->decay.min_value, env->decay.max_value);
	f32 decay_rate = t_s * GRVGM_SAMPLE_RATE;
	env->decay.value = t_s;
	env->alpha_decay = envelope_compute_alpha(decay_rate, env->target_ratio_decay_release);
	env->offset_decay = (env->sustain.value - env->target_ratio_decay_release) * (1.0f - env->alpha_decay);
}

void envelope_set_release_time(envelope_t* env, f32 t_s) {
	t_s = grv_clamp_f32(t_s, env->release.min_value, env->release.max_value);
	f32 release_rate = t_s * GRVGM_SAMPLE_RATE;
	env->release.value = t_s;
	env->alpha_release = envelope_compute_alpha(release_rate, env->target_ratio_decay_release);
	env->offset_release = -env->target_ratio_decay_release * (1.0f - env->alpha_release);
}

void envelope_set_sustain(envelope_t* env, f32 sustain) {
	sustain = grv_clamp_f32(sustain, env->release.min_value, env->release.max_value);
	env->sustain.value = sustain;
	envelope_set_decay_time(env, env->decay.value);
}

void envelope_init(envelope_t* envelope) {
	f32 t_attack = 0.01f;
	f32 t_decay = 0.05f;
	f32 t_release = 0.2f;
	f32 sustain = 0.5f;

	*envelope = (envelope_t) {
		.attack = {
			.value = t_attack,
			.min_value = 0.001f,
			.max_value = 1.0f,
			.mapping_type = MAPPING_TYPE_LOG_TIME,
		},
		.decay = {
			.value = t_decay,
			.min_value = 0.001f,
			.max_value = 1.0f,
			.mapping_type = MAPPING_TYPE_LOG_TIME,
		},
		.release = {
			.value = t_release,
			.min_value = 0.001f,
			.max_value = 1.0f,
			.mapping_type = MAPPING_TYPE_LOG_TIME,
		},
		.sustain = {
			.value = sustain,
			.min_value = 0.0f,
			.max_value = 1.0f,
			.mapping_type = MAPPING_TYPE_LINEAR,
		},
		.target_ratio_attack = 0.3f,
		.target_ratio_decay_release = 0.0001f,
	};
}

f32* process_envelope(f32 gate, bool trigger_received, envelope_t* env, grv_arena_t* arena) {
	envelope_state_t* state = &env->state;
	
	if (env->attack.value != env->attack._prev_value) {
		envelope_set_attack_time(env, env->attack.value);
		env->attack._prev_value = env->attack.value;
	}
	if (env->decay.value != env->decay._prev_value) {
		envelope_set_decay_time(env, env->decay.value);
		env->decay._prev_value = env->decay.value;
	}
	if (env->release.value != env->release._prev_value) {
		envelope_set_release_time(env, env->release.value);
		env->release._prev_value = env->release.value;
	}
	if (env->sustain.value != env->sustain._prev_value) {
		envelope_set_sustain(env, env->sustain.value);
		env->sustain._prev_value = env->sustain.value;
	}

	if (gate < 0.5f && *state == ENVELOPE_RELEASE && env->y <= 0.0f) {
		*state = ENVELOPE_OFF;
		env->y = 0.0f;
		env->alpha = 0.0f;
		env->offset = 0.0f;
	} else if (gate < 0.5f && *state != ENVELOPE_OFF) {
		*state = ENVELOPE_RELEASE;
		env->alpha = env->alpha_release;
		env->offset = env->offset_release;
	} else if (trigger_received) {
		*state = ENVELOPE_ATTACK;
		env->alpha = env->alpha_attack;
		env->offset = env->offset_attack;
	} else if (*state == ENVELOPE_ATTACK && env->y >= 1.0f) {
		*state = ENVELOPE_DECAY;
		env->alpha = env->alpha_decay;
		env->offset = env->offset_decay;
	} else if (*state == ENVELOPE_DECAY && env->y <= env->sustain.value) {
		*state = ENVELOPE_SUSTAIN;
		env->alpha = 1.0f;
		env->offset = 0.0f;
		env->y = env->sustain.value;
	}

	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	if (*state == ENVELOPE_OFF) {
		clear_buffer(outptr);
	} else {
		f32 alpha = env->alpha;
		f32 offset = env->offset;
		f32 y = env->y;
		for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
			y = offset + y * alpha;
			y = grv_clamp_f32(y, 0.0f, 1.0f);
			*dst++ = y;
		}
		env->y = y;
	}
	env->_prev_gate = gate;

	return outptr;
}

f32* fill_phase_buffer(grv_arena_t* arena, f32* freq, f32* phase_state) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	f32 ph = *phase_state;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		ph += (*freq++) / GRVGM_SAMPLE_RATE;
		if (ph >= 1.0f) ph -= 1.0f;
		*dst++ = ph;
	}
	*phase_state = ph;
	return outptr;
}

f32* sine_osc(grv_arena_t* arena, f32* phase) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = sinf(*phase++ * TWO_PI_F32);
	}
	return outptr;
}

f32* oscillatr_render_noise(grv_arena_t* arena) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = -1.0f + 2.0f * grvgm_random_f32();
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
		if (state->_was_playing) {
			f64 pulse_time = state->pulse_time;
			state->prev_pulse_time = state->pulse_time;
			f64 pulse_time_per_frame = (f64)AUDIO_FRAME_SIZE / (f64)GRVGM_SAMPLE_RATE * state->bpm / 60.0f * PPQN;
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

grv_arena_t* get_arena(synth_state_t* state) {
	return &state->transient.audio_arena;
}

note_event_t* process_sequencer(synth_state_t* state) {
	grv_arena_t* arena = get_arena(state);
	i32 num_patterns = state->patterns.size;
	sequencer_state_t* sequencer_state = &state->sequencer_state;
	note_event_t* event_buffer = grv_arena_alloc_zero(
		arena,
		num_patterns * sizeof(note_event_t));

	f64 prev_pulse_time = state->transport.prev_pulse_time;
	f64 pulse_time = state->transport.pulse_time;
	f64 delta_ticks = pulse_time - prev_pulse_time;

	i32 prev_step = grv_floor_f64(prev_pulse_time) * 4 / PPQN;
	i32 step = grv_floor_f64(pulse_time) * 4 / PPQN;

	for (i32 i = 0; i < num_patterns; i++) {
		synth_pattern_t* pattern = &state->patterns.arr[i];
		sequencer_track_state_t* sequencer_track_state = &sequencer_state->track_state[i];
		note_event_t* event = &event_buffer[i];

		if (state->transport.is_playing
			&& step != prev_step
			&& step >= 0 && step < pattern->num_steps
			&& pattern->steps[step].activated) {
			synth_pattern_step_t* pattern_step = &pattern->steps[step];
			*event = (note_event_t){
				.type = NOTE_EVENT_ON,
				.note_value = pattern_step->note_value,
				.length = pattern_step->length,
				.amplitude = pattern_step->amplitude,
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

void process_mono_to_stereo(f32* left, f32* right, f32* in, f32* pan) {
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		f32 phi = HALF_PI_F32 * 0.5f * ((*pan++) + 1.0f);
		f32 in_value = *in++;
		*left++ = cosf(phi) * in_value;
		*right++ = sinf(phi) * in_value;
	}
}

void process_add_to_buffer(f32* dst, f32* src) {
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ += *src++;
	}
}

void process_note_processor(note_processor_t* note_proc, note_event_t* event) {
	if (event->type == NOTE_EVENT_ON) {
		note_proc->freq = note_value_to_frequency(event->note_value);
		note_proc->gate = 1.0f;
		note_proc->trigger_received = true;
	} else if (event->type == NOTE_EVENT_OFF) {
		note_proc->gate = 0.0f;
	} else {
		note_proc->trigger_received = false;
	}
}

void note_processor_init(note_processor_t* note_proc) {
	*note_proc = (note_processor_t) {
		.legato = 0.1,
		.freq = 440.0f,
		.smoothed_freq = 440.0f
	};
}

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
	envelope_init(&synth->envelope);
	note_processor_init(&synth->note_proc);
}

f32* process_test_tone(grv_arena_t* arena, f32* phase) {
	f32* outptr = arena_alloc_buffer(arena);
	f32* dst = outptr;
	f32 phase_diff = 440.0f / GRVGM_SAMPLE_RATE;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = sinf(*phase * TWO_PI_F32);
		*phase += phase_diff;
		if (*phase >= 1.0f) *phase -= 1.0f;
	}
	return outptr;
}

f32* process_oscillator(oscillator_t* osc, f32* phase_buffer, grv_arena_t* arena) {
	switch (osc->wave_type) {
	case WAVE_TYPE_NOISE:
		return oscillatr_render_noise(arena);
	default:
		return sine_osc(arena, phase_buffer);
	}
}

void process_simple_synth(
	f32* buffer_l,
	f32* buffer_r,
	simple_synth_t* synth,
	note_event_t* note_event,
	grv_arena_t* arena) {
	grv_arena_push_frame(arena);
	process_note_processor(&synth->note_proc, note_event);
	f32* freq = smooth_value(
		synth->note_proc.freq,
		&synth->note_proc.smoothed_freq,
		0.01f,
		arena);
	f32* phase = fill_phase_buffer(arena, freq, &synth->phase_state);
	f32* mono_buffer = process_oscillator(&synth->oscillator, phase, arena);
	f32* amp_env = process_envelope(
		synth->note_proc.gate,
		synth->note_proc.trigger_received,
		&synth->envelope,
		arena);
	process_mul(mono_buffer, mono_buffer, amp_env);
	//f32* pan = smooth_audio_parameter(&synth->pan, arena);
	//process_mono_to_stereo(buffer_l, buffer_r, mono_buffer, pan);
	audio_buffer_copy(buffer_l, mono_buffer);
	audio_buffer_copy(buffer_r, mono_buffer);
	grv_arena_pop_frame(arena);
}

f32* process_volume_to_amp(f32* volume_buffer, grv_arena_t* arena) {
	f32* amp_buffer = arena_alloc_buffer(arena);
	f32* src = volume_buffer;
	f32* dst = amp_buffer;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = from_db(*src++);
	}
	return amp_buffer;
}

void process_volume(f32* buffer_l, f32* buffer_r, audio_parameter_t* vol, grv_arena_t* arena) {
	grv_arena_push_frame(arena);
	f32* amp_db = smooth_audio_parameter(vol, arena);
	f32* amp = process_volume_to_amp(amp_db, arena);
	process_mul(buffer_l, buffer_l, amp);
	process_mul(buffer_r, buffer_r, amp);
	grv_arena_pop_frame(arena);
}

void process_track(
	f32* out_l,
	f32* out_r,
	synth_track_t* track,
	note_event_t* note_event,
	grv_arena_t* arena) {
	grv_arena_push_frame(arena);
	f32* buffer_l = arena_alloc_buffer(arena);
	f32* buffer_r = arena_alloc_buffer(arena);
	process_simple_synth(buffer_l, buffer_r, &track->synth, note_event, arena);
	//process_volume(buffer_l, buffer_r, &track->output.volume, arena);
	process_add_to_buffer(out_l, buffer_l);
	process_add_to_buffer(out_r, buffer_r);

	grv_arena_pop_frame(arena);
}

void transport_state_init(transport_state_t* state) {
	*state = (transport_state_t) {
		.bpm = 120.0,
		.prev_pulse_time = -1.0e6,
	};
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

void pattern_init(synth_pattern_t* pattern) {
	*pattern = (synth_pattern_t){
		.num_steps = 16,
	};

	for (i32 i = 0; i < pattern->num_steps; i++) {
		synth_pattern_step_t* step = &pattern->steps[i];
		*step = (synth_pattern_step_t){
			.note_value = 45, // 440Hz
			.length = PPQN / 4,
			.amplitude = 1.0f,
		};
	}
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
	f32* out_l = arena_alloc_zero_buffer(arena);
	f32* out_r = arena_alloc_zero_buffer(arena);

	for (i32 frame_idx = 0; frame_idx < num_frames / AUDIO_FRAME_SIZE; frame_idx++) {
		grv_arena_push_frame(arena);
		audio_buffer_clear(out_l);
		audio_buffer_clear(out_r);

		process_transport(&synth_state->transport);
		note_event_t* note_event_buffer = process_sequencer(synth_state);
		
		for (i32 track_idx = 0; track_idx < num_tracks; track_idx++) {
			synth_track_t* track = &synth_state->tracks.arr[track_idx];
			process_track(out_l, out_r, track, &note_event_buffer[track_idx], arena);
		}

		process_volume(out_l, out_r, &synth_state->master_volume, arena);
		render_pcm_stereo(stream, out_l, out_r, frame_idx);

		grv_arena_pop_frame(arena);
	}
	grv_arena_reset(arena);
}
