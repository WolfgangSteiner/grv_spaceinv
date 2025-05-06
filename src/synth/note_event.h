#ifndef SYNTH_NOTE_EVENT_H
#define SYNTH_NOTE_EVENT_H

#include "synth_base.h"

typedef enum {
	NOTE_EVENT_NONE,
	NOTE_EVENT_ON,
	NOTE_EVENT_OFF,
} note_event_type_t;

typedef struct {
	note_event_type_t type;
	f32 amplitude; 
	i32 note_value;
	i32 length;
} note_event_t;

#endif
