#ifndef SYNTH_PARAMETER_MAPPING_H
#define SYNTH_PARAMETER_MAPPING_H
#include "grv/grv_base.h"
#include "audio_parameter.h"
#include "dsp.h"

GRV_INLINE f32 map_normalized_volume_to_db_cubic(f32 value, f32 min_value, f32 max_value) {
	f32 val_db = min_value + (max_value - min_value) * powf(value, 1/3.0f);
	return val_db;
}

GRV_INLINE f32 map_normalized_volume_to_gui_cubic(f32 value, f32 min_value, f32 max_value) {
	return powf(value, 3.0f);
}

GRV_INLINE f32 map_normalized_volume_from_gui_cubic(f32 value, f32 min_value, f32 max_value) {
	return powf(value, 1.0f/3.0f);
}

GRV_INLINE f32 map_normalized_volume_to_db_linear(f32 value, f32 min_value, f32 max_value) {
	f32 val_db = min_value + (max_value - min_value) * value;
	return val_db;
}

GRV_INLINE f32 map_normalized_volume_to_amplitude_linear(f32 value, f32 min_value, f32 max_value) {
	f32 val_db = map_normalized_volume_to_db_linear(value, min_value, max_value);
	return powf(10, val_db/20);
}

GRV_INLINE f32 map_volume_db_to_normalized_linear(f32 volume_db, f32 min_value, f32 max_value) {
	return (volume_db - min_value) / (max_value - min_value);
}

GRV_INLINE f32 map_log_freq_to_normalized(f32 value, f32 min_value, f32 max_value) {
	return freq_to_log(value, min_value) / freq_to_log(max_value, min_value);
}

GRV_INLINE f32 map_normalized_log_freq_to_freq(f32 value, f32 min_value, f32 max_value) {
	return min_value * powf(max_value / min_value, value);
}

GRV_INLINE f32 map_tuning_step_to_normalized(f32 value, f32 min_value, f32 max_value) {
	return (value - min_value) / (max_value - min_value);
}

GRV_INLINE f32 map_normalized_tuning_to_tuning_step(f32 value, f32 min_value, f32 max_value) {
	return min_value + (max_value - min_value) * value;
}

GRV_INLINE f32 map_log_time_to_normalized(f32 t, f32 t_min, f32 t_max) {
	return log10f(t / t_min) / log10(t_max / t_min);
}

GRV_INLINE f32 map_normalized_log_time_to_time(f32 t, f32 t_min, f32 t_max) {
	return t_min * powf(t_max/t_min, t);
}

GRV_INLINE f32 map_normalized_to_linear(f32 norm_value, f32 min_value, f32 max_value) {
	return min_value + norm_value * (max_value - min_value);
}

GRV_INLINE f32 map_linear_to_normalized(f32 value, f32 min_value, f32 max_value) {
	return (value - min_value) / (max_value - min_value);
}

#endif
