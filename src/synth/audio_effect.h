#ifndef SYNTH_AUDIO_EFFECT_H
#define SYNTH_AUDIO_EFFECT_H

typedef enum {
	AUDIO_EFFECT_DELAY,
} audio_effect_type_t;

typedef struct {
	audio_effect_type_t type;
} audio_effect_t;

#endif
