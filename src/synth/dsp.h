#ifndef SYNTH_DSP_H
#define SYNTH_DSP_H

#include "grv/grv_arena.h"

GRV_INLINE f32 from_db(f32 val_db) {
	return val_db <= -72.f ? 0.0f : powf(10.0f, val_db/20.0f);
}

GRV_INLINE f32 freq_to_cents(f32 freq, f32 min_freq) {
	return 1200 * log2f(freq/min_freq);
}

GRV_INLINE f32 cents_to_freq(f32 cents, f32 min_freq) {
	return min_freq * powf(2.0f, cents/1200);
}

GRV_INLINE f32 freq_to_log(f32 freq, f32 min_freq) {
	return log10f(freq/min_freq);
}

GRV_INLINE f32 log_to_freq(f32 log_freq, f32 min_freq) {
	return min_freq * powf(10.0f, log_freq);
}


f32 note_value_to_frequency(i32 note_value);
f32 lerp(f32* a, f32 b, f32 c);
f32 smooth_filter(f32* a, f32 b);
f32* audio_buffer_alloc(grv_arena_t* arena);
f32* audio_buffer_alloc_zero(grv_arena_t* arena);
void audio_buffer_add_to(f32* dst, f32* src);
void audio_buffer_mul(f32* dst, f32* a, f32* b);
void audio_buffer_copy(f32* dst, f32* src);
void audio_buffer_clear(f32* dst);
f32* audio_buffer_from_constant(f32 c, grv_arena_t* arena);
void audio_buffer_from_db(f32* dst, f32* src);
f32* audio_buffer_modulate_add(f32* dst, f32* src, f32 amount, grv_arena_t* arena);
void audio_buffer_denormalize_log_freq(f32* buffer, f32 min_value, f32 max_value);
void audio_buffer_denormalize_linear(f32* buffer, f32 min_value, f32 max_value);
f32* smooth_value(f32 y_target, f32* y_state, f32 alpha, grv_arena_t* arena);
void render_pcm_stereo(i16* out, f32* left, f32* right, i32 frame_idx);
#endif
