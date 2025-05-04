#ifndef SYNTH_DSP_H
#define SYNTH_DSP_H

#include "grv/grv_arena.h"

f32* arena_alloc_buffer(grv_arena_t* arena);
f32* arena_alloc_zero_buffer(grv_arena_t* arena);

#endif
