#include "dsp.h"
#include "synth_base.h"
#include "libc/memcpy.h"
#include "parameter_mapping.h"

f32* audio_buffer_alloc(grv_arena_t* arena) {
	return grv_arena_alloc(arena, AUDIO_FRAME_SIZE*sizeof(f32));
}

f32* audio_buffer_alloc_zero(grv_arena_t* arena) {
	f32* buffer = audio_buffer_alloc(arena);
	memset(buffer, 0, AUDIO_FRAME_SIZE*sizeof(f32));
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

void audio_buffer_add_to(f32* dst, f32* src) {
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ += *src++;
	}
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
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = c;
	}

	return outptr;
}

void audio_buffer_from_db(f32* dst, f32* src) {
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = from_db(*src++);
	}
}

f32* audio_buffer_modulate_add(f32* signal, f32* mod, f32 amount, grv_arena_t* arena) {
	f32* out = audio_buffer_alloc(arena);
	f32* dst = out;
	amount = (amount - 0.5f) * 2.0f;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		f32 val = (*signal++) + (*mod++) * amount;
		(*dst++) =  grv_clamp_f32(val, 0.0f, 1.0f);
	}
	return out;
}

void audio_buffer_denormalize_log_freq(f32* buffer, f32 min_value, f32 max_value) {
	f32* smp = buffer;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		f32 val = *smp;
		*smp++ = map_normalized_log_freq_to_freq(val, min_value, max_value);
	}
}

void audio_buffer_denormalize_linear(f32* buffer, f32 min_value, f32 max_value) {
	f32* smp = buffer;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		f32 val = *smp;
		*smp++ = map_normalized_to_linear(val, min_value, max_value);
	}
}

f32* smooth_value(f32 y_target, f32* y_state, f32 alpha, grv_arena_t* arena){
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	f32 y = *y_state;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		y += (y_target - y) * alpha;
		*dst++ = y;
	}
	*y_state = y;
	return outptr;
}

void render_pcm_stereo(i16* out, f32* left, f32* right, i32 frame_idx) {
	out += frame_idx * 2 * AUDIO_FRAME_SIZE;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*out++ = (f32)GRV_MAX_I16 * (*left++);
		*out++ = (f32)GRV_MAX_I16 * (*right++);
	}
}

f32* generate_test_tone(grv_arena_t* arena, f32* phase) {
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	f32 phase_diff = 440.0f / AUDIO_SAMPLE_RATE;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = sinf(*phase * TWO_PI_F32);
		*phase += phase_diff;
		if (*phase >= 1.0f) *phase -= 1.0f;
	}
	return outptr;
}
