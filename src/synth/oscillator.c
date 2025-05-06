#include "synth_base.h"
#include "oscillator.h"
#include "dsp.h"
#include "grvgm.h"

f32* oscillator_fill_phase_diff_buffer(f32* freq, grv_arena_t* arena) {
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = (*freq++) / AUDIO_SAMPLE_RATE;
	}
	return outptr;
}

f32* oscillator_fill_phase_buffer(f32* phase_diff, f32* phase_state, grv_arena_t* arena) {
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	f32 ph = *phase_state;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		ph += (*phase_diff++);
		if (ph >= 1.0f) ph -= 1.0f;
		*dst++ = ph;
	}
	*phase_state = ph;
	return outptr;
}

f32* oscillator_render_sine(f32* phase, grv_arena_t* arena) {
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = sinf(*phase++ * TWO_PI_F32);
	}
	return outptr;
}

f32* oscillatr_render_noise(grv_arena_t* arena) {
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		*dst++ = -1.0f + 2.0f * grvgm_random_f32();
	}
	return outptr;
}

f32 poly_blep(f32 t, f32 dt) {
    if (t < dt) {
        t /= dt;
        return 2*t - t*t - 1.0f;
    }
    else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t*t + 2*t + 1.0f;
    }
    else return 0.0f;
}

f32 poly_blep_rect(f32 t, f32 dt) {
    if (t < dt) {
        t /= dt;
        return 2*t - t*t - 1.0f;
    } else if (t - 0.5f >= 0.0f && t - 0.5f < dt) {
		t = (t - 0.5f) / dt;
		return -(2*t - t*t - 1.0f);
	} else if (0.5f - t >= 0 && 0.5f - t < dt) {
		t = (t - 0.5f) / dt;
		return -(t*t + 2*t + 1.0f);
	} else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t*t + 2*t + 1.0f;
    }
    else return 0.0f;
}

f32* oscillator_render_rect(f32* phase, f32* phase_diff, grv_arena_t* arena) {
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		f32 phi = (*phase++);
		f32 y = phi <= 0.5f ? 1.0f : -1.0f;
		y += poly_blep_rect(phi, *phase_diff++);
		*dst++ = y;
	}
	return outptr;
}

f32* oscillator_render_saw(f32* phase, f32* phase_diff, grv_arena_t* arena) {
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		f32 y = 2.0f * (*phase) - 1.0f;
		y -= poly_blep(*phase++, *phase_diff++);
		*dst++ = y;
	}
	return outptr;
}

f32* oscillator_process(
	oscillator_t* osc,
	f32* phase,
	f32* phase_diff,
	grv_arena_t* arena) {
	switch (osc->wave_type) {
	case WAVE_TYPE_RECT:
		return oscillator_render_rect(phase, phase_diff, arena);
	case WAVE_TYPE_SAW:
		return oscillator_render_saw(phase, phase_diff, arena);
	case WAVE_TYPE_NOISE:
		return oscillatr_render_noise(arena);
	default:
		return oscillator_render_sine(phase, arena);
	}
}
