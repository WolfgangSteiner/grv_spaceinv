#include "state_variable_filter.h"
#include "grv/grv_math.h"

void state_variable_filter_compute_coefficients_low_pass(
	state_variable_filter_coefficients_t* c, f32 f, f32 q) {
	f32 w = 2*tanf(PI_F32 * f);
	f32 a = w / q;
	f32 b = w * w;
	c->c1 = (a+b) / (1.0f + a/2.0f + b/4.0f);
	c->c2 = b / (a + b);

	// lowpass
	c->d0 = c->c1 * c->c2 / 4.0f;
	c->d1 = c->c2;
	c->d2 = 1.0f;
}

void state_variable_filter_process_low_pass(
	f32* dst,
	f32* src,
	state_variable_filter_coefficients_t* c,
	state_variable_filter_state_t* s,
	i32 num_samples) {
	f32 z1 = s->z1;
	f32 z2 = s->z2;
	f32 d0 = c->d0;
	f32 c1 = c->c1;
	f32 c2 = c->c2;

	for (i32 i = 0; i < num_samples; i++) {
		f32 x = *src++ - z1 - z2;
		z2 += c2 * z1;
		*dst++ = d0 * x + z2;
		z1 += c1 * x;
	}
	s->z1 = z1;
	s->z2 = z2;
}
