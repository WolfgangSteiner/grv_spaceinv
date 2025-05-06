#ifndef SYNTH_NOTE_PROCESSOR_H
#define SYNTH_NOTE_PROCESSOR_H
#include "synth_base.h"
#include "note_event.h"

typedef struct {
	f32 legato;
	f32 freq;
	f32 smoothed_freq;
	f32 gate;
	bool trigger_received;
} note_processor_t;

void note_processor_init(note_processor_t* note_proc);
void note_processor_process(note_processor_t* note_proc, note_event_t* event);

#endif
