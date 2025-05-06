#include "synth_base.h"
#include "envelope.h"
#include "dsp.h"

f32 envelope_compute_alpha(f32 t_samples, f32 target_ratio) {
	return t_samples <= 0.0f 
		? 1.0f 
		: expf(-logf((1.0f + target_ratio) / target_ratio) / t_samples);
} 

void envelope_set_attack_time(envelope_t* env, f32 t_s) {
	t_s = grv_clamp_f32(t_s, env->attack.min_value, env->attack.max_value);
	f32 attack_rate = t_s * AUDIO_SAMPLE_RATE;
	env->attack.value = t_s;
	env->alpha_attack = envelope_compute_alpha(attack_rate, env->target_ratio_attack);
	env->offset_attack = (1.0f + env->target_ratio_attack) * (1.0f - env->alpha_attack);
}

void envelope_set_decay_time(envelope_t* env, f32 t_s) {
	t_s = grv_clamp_f32(t_s, env->decay.min_value, env->decay.max_value);
	f32 decay_rate = t_s * AUDIO_SAMPLE_RATE;
	env->decay.value = t_s;
	env->alpha_decay = envelope_compute_alpha(decay_rate, env->target_ratio_decay_release);
	env->offset_decay = (env->sustain.value - env->target_ratio_decay_release) * (1.0f - env->alpha_decay);
}

void envelope_set_release_time(envelope_t* env, f32 t_s) {
	t_s = grv_clamp_f32(t_s, env->release.min_value, env->release.max_value);
	f32 release_rate = t_s * AUDIO_SAMPLE_RATE;
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

f32* envelope_process(f32 gate, bool trigger_received, envelope_t* env, grv_arena_t* arena) {
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

	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	if (*state == ENVELOPE_OFF) {
		audio_buffer_clear(outptr);
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
