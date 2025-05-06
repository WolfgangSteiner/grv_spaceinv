#include "note_processor.h"
#include "dsp.h"

void note_processor_process(note_processor_t* note_proc, note_event_t* event) {
	if (event->type == NOTE_EVENT_ON) {
		note_proc->freq = note_value_to_frequency(event->note_value);
		note_proc->gate = 1.0f;
		note_proc->trigger_received = true;
	} else if (event->type == NOTE_EVENT_OFF) {
		note_proc->gate = 0.0f;
	} else {
		note_proc->trigger_received = false;
	}
}

void note_processor_init(note_processor_t* note_proc) {
	*note_proc = (note_processor_t) {
		.legato = 0.1,
		.freq = 440.0f,
		.smoothed_freq = 440.0f
	};
}

