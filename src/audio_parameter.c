#include "audio_parameter.h"
#include "synth_base.h"
#include "synth_dsp.h"

f32* audio_parameter_smooth(audio_parameter_t* p, grv_arena_t* arena) {
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
