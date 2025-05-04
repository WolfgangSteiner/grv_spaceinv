#include "synth_dsp.h"
#include "synth_base.h"

f32* arena_alloc_buffer(grv_arena_t* arena) {
	return grv_arena_alloc(arena, AUDIO_FRAME_SIZE*sizeof(f32));
}

f32* arena_alloc_zero_buffer(grv_arena_t* arena) {
	f32* buffer = arena_alloc_buffer(arena);
	memset(buffer, 0, AUDIO_FRAME_SIZE*sizeof(f32));
	return buffer;
}
