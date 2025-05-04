#include "synth_filter.h"
#include "synth_base.h"
#include "grv/grv_math.h"

void synth_filter_process(
	f32* dst, f32* src, synth_filter_t* filter, grv_arena_t* arena) {
	grv_arena_push_frame(arena);
	f32* f = audio_parameter_smooth(&filter->f, arena);
	f32* q = audio_parameter_smooth(&filter->q, arena);

	f32 z11 = filter->state[0].z1;
	f32 z12 = filter->state[0].z2;
	f32 z21 = filter->state[1].z1;
	f32 z22 = filter->state[1].z2;

	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
	#if 1
		f32 w = 2.0f * tanf(PI_F32 * (*f++)/AUDIO_SAMPLE_RATE);
		f32 a = w / (*q++);
		f32 b = w * w;
		f32 c1 = (a + b) / (1.0f + a/2.0f + b/4.0f);
		f32 c2 = b / (a + b);
		f32 d0 = c1 * c2 / 4.0f;

		f32 x1 = *src++ - z11 - z12;
		z12 += c2 * z11;
		f32 y1 = d0 * x1 + z12;
		z11 += c1 * x1;

		f32 x2 = y1 - z21 - z22;
		z22 += c2 * z21;
		(*dst++) = d0 * x2 + z22;
		z21 += c1 * x2;
	#else
		*dst++ = *src++;
	#endif
	}

	filter->state[0].z1 = z11;
	filter->state[0].z2 = z12;
	filter->state[1].z1 = z21;
	filter->state[1].z2 = z22;
	grv_arena_pop_frame(arena);
}

void synth_filter_init(synth_filter_t* filter) {
	*filter = (synth_filter_t) {
		.filter_type = FILTER_TYPE_LP24,
		.f = {
			.value = 2000.0f,
			.min_value = 20.0f,
			.max_value = 20000.0f,
			.mapping_type = MAPPING_TYPE_LOG,
		},
		.q = {
			.value = 1.0f,
			.min_value = 1.0f,
			.max_value = 5.0f,
			.mapping_type = MAPPING_TYPE_LINEAR,
		},
	};
}
