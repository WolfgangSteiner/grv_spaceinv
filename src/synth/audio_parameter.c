#include "audio_parameter.h"
#include "synth_base.h"
#include "dsp.h"
#include "parameter_mapping.h"
#include "libc/snprintf.h"
#include "grv/grv_log.h"

f32* audio_parameter_smooth(audio_parameter_t* p, grv_arena_t* arena) {
	f32* outptr = audio_buffer_alloc(arena);
	f32* dst = outptr;
	f32 y = p->smoothed_value;
	f32 y_target = p->value;
	f32 alpha = p->smoothing_coefficient == 0.0f ? 0.01 : p->smoothing_coefficient;
	f32 a = 1.0f - alpha;
	f32 b = alpha;
	for (i32 i = 0; i < AUDIO_FRAME_SIZE; i++) {
		y = y * a + y_target * b;
		*dst++ = y;
	}
	p->smoothed_value = y;
	return outptr;
}

// maps from an absolute user_value to normalized
f32 audio_parameter_normalize_value(audio_parameter_t* p, f32 user_value) {
	user_value = grv_clamp_f32(user_value, p->min_value, p->max_value);
	switch(p->mapping_type) {
		case MAPPING_TYPE_VOLUME:
			return map_volume_db_to_normalized_linear(user_value, p->min_value, p->max_value);
		case MAPPING_TYPE_LOG_FREQUENCY:
			return map_log_freq_to_normalized(user_value, p->min_value, p->max_value);
		case MAPPING_TYPE_LOG_TIME:
			return map_log_time_to_normalized(user_value, p->min_value, p->max_value);
		case MAPPING_TYPE_TUNING_OCTAVES:
		case MAPPING_TYPE_TUNING_SEMITONES:
		case MAPPING_TYPE_TUNING_CENTS: {
			map_tuning_step_to_normalized(user_value, p->min_value, p->max_value);
		}
		default:
			return user_value - p->min_value / (p->max_value / p->min_value);
	}
}

bool audio_parameter_is_discrete(audio_parameter_t* p) {
	return p->mapping_type >= MAPPING_TYPE_TUNING_OCTAVES
		&& p->mapping_type <= MAPPING_TYPE_TUNING_CENTS;
}


f32 audio_parameter_map_to_user_value(audio_parameter_t* p) {
	f32 value = p->value;
	f32 min_value = p->min_value;
	f32 max_value = p->max_value;

	switch (p->mapping_type) {
		case MAPPING_TYPE_VOLUME:
			return map_normalized_volume_to_db_linear(value, min_value, max_value);
		case MAPPING_TYPE_LOG_TIME:
			return map_normalized_log_time_to_time(value, min_value, max_value);
		case MAPPING_TYPE_LOG_FREQUENCY:
			return map_normalized_log_freq_to_freq(value, min_value, max_value);
		case MAPPING_TYPE_TUNING_OCTAVES:
		case MAPPING_TYPE_TUNING_SEMITONES:
		case MAPPING_TYPE_TUNING_CENTS: {
			return map_normalized_tuning_to_tuning_step(value, min_value, max_value);
		}
		default:
			return min_value + (max_value - min_value) * value;
	}
	return 0.0f;
}

char* audio_parameter_value_as_string(audio_parameter_t* p, grv_arena_t* arena) {
	f32 user_value = audio_parameter_map_to_user_value(p);
	char* res = grv_arena_alloc_zero(arena, 16);
	switch (p->mapping_type) {
		case MAPPING_TYPE_VOLUME:
			snprintf(res, 16, "%.1fdB", user_value);
			break;
		case MAPPING_TYPE_LOG_TIME:
			if (user_value <= 0.1f) {
				snprintf(res, 16, "%.1fms", user_value * 1000.0f);
			} else if ( user_value <= 1.0f) {
				snprintf(res, 16, "%.2fs", user_value);
			} else {
				snprintf(res, 16, "%.1fs", user_value);
			}
			break;
		case MAPPING_TYPE_LOG_FREQUENCY:
			if (user_value < 1000.0f) {
				snprintf(res, 16, "%.1fHz", user_value);
			} else  {
				snprintf(res, 16, "%.2fkHz", user_value / 1000.0f);
			}
			break;
		default:
			snprintf(res, 16, "%.2f", user_value);
			break;
	}

	return res;
}

f32 audio_parameter_map_to_slider_value(audio_parameter_t* p) {
	switch (p->mapping_type) {

		default:
			return p->value;
	}
}


// gui_val = log10(val/min_val) / log10(max_val/min_val)
//
// gui_val * log10(max_val/min_val) = log10(val/min_val)
// log10(pow(max_val/min_val), gui_val) = log10(val/min_val)
// val = min_val * pow(max_val/_min_val, gui_val);

f32 audio_parameter_map_to_gui_relative_value(audio_parameter_t* p) {
	f32 rel_value = p->value;
	// if (p->mapping_type == MAPPING_TYPE_VOLUME) {
	// 		rel_value = map_normalized_volume_to_gui_cubic(
	// 			p->value, p->min_value, p->max_value);
	// }
	if (audio_parameter_is_bipolar(p)) {
		// map to -0.5 .. 0.5
		rel_value -= 0.5f;
	}

	return rel_value;
}

bool audio_parameter_is_bipolar(audio_parameter_t* p) {
	return p->min_value < 0.0f && p->min_value == -p->max_value;
}

void audio_parameter_serialize(grv_serializer_t* s, audio_parameter_t* p) {
	grv_serialize_struct_begin(s, 0);
	grv_serialize_struct_field_float(s, "value", p->value);
	grv_serialize_struct_end(s);
}

bool audio_parameter_deserialize(grv_serializer_t* s, audio_parameter_t* p) {
	u16 version;
	bool success = grv_deserialize_struct_begin(s, version);
	if (success == false || version != 0) {
		grv_log_error_cstr("audio_parameter_t failed deserialization");
		return false;
	}
}
