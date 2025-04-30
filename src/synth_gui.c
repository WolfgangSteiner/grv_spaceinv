#include "src/synth.h"
#include "grvgm.h"
#include <stdio.h>

void draw_trigger_button(
	rect_i32 r, 
	synth_pattern_step_t* step,
	i32 step_idx,
	bool is_triggered) {
	if (grvgm_mouse_click_in_rect(r, GRVGM_BUTTON_MOUSE_LEFT)) {
		step->activated = !step->activated;
	}
	u8 fill_color = step_idx % 4 ? 5 : 13;
	grvgm_fill_rect(r, fill_color);
	if (step->activated) {
		grvgm_fill_rect(rect_i32_shrink(r, 1, 1), 6);
	}
	if (is_triggered) {
		grvgm_draw_rect(r, 9);
	}
}

void draw_pattern_triggers(rect_i32 rect, synth_state_t* state) {
	i32 gap = 1;
	i32 num_steps = 16;
	i32 trigger_size = (rect.w - (num_steps - 1) * gap) / num_steps;
	rect_i32 content_rect = {
		.w = num_steps * trigger_size + (num_steps - 1) * gap,
		.h = trigger_size
	};
	content_rect = rect_i32_align_to_rect(content_rect, rect, GRV_ALIGNMENT_BOTTOM_CENTER);
	synth_pattern_t* pattern = &state->patterns.arr[state->current_pattern];
	rect_i32 button_rect = {.y=content_rect.y, .w=trigger_size, .h=trigger_size};
	i32 playing_step_idx = (i32)state->transport.pulse_time * 4 / PPQN;
	for (i32 i = 0; i < num_steps; i++) {
		draw_trigger_button(
			button_rect,
			&pattern->steps[i],
			i,
			playing_step_idx == i);
		button_rect = rect_i32_shift_x(button_rect, trigger_size + gap);
	}
}

bool draw_text_button(rect_i32 rect, grv_str_t text, bool* selected) {
	if (grvgm_mouse_click_in_rect(rect, GRVGM_BUTTON_MOUSE_LEFT)) {
		*selected = !*selected;
	}
	u8 fill_color = *selected ? 9 : 1;
	grvgm_fill_rect_chamfered(rect, fill_color);
	grvgm_draw_rect_chamfered(rect, 6);
	grvgm_draw_text_aligned(rect, text, GRV_ALIGNMENT_CENTER, 7);
	return grvgm_mouse_click_in_rect(rect, 1);
}

void draw_rect_slider(rect_i32 rect, grv_str_t text, i32* value, i32 min_value, i32 max_value) {
	GRV_UNUSED(text);
	rect_i32 slider_rect = {0, 0, 8, 8};
	slider_rect = rect_i32_align_to_rect(slider_rect, rect, GRV_ALIGNMENT_CENTER);
	fx32 display_value =
		fx32_mul_i32(
			fx32_div(
				fx32_from_i32(*value - min_value),
				fx32_from_i32(max_value - min_value)),
			64);
	i32 num_px = fx32_round(display_value);
	for (i32 i = 0; i < num_px; i++) {
		i32 x = i / 8;
		i32 y = i % 8;
		vec2_i32 p = {x + slider_rect.x, slider_rect.y - y};
		grvgm_draw_pixel(p, 9);
	}
}

f32 slider_increment(audio_parameter_t* p) {
	vec2_i32 offset = vec2i_sub(grvgm_mouse_position(), grvgm_initial_drag_position());
	vec2_i32 abs_offset = vec2i_abs(offset);
	bool use_x = abs_offset.x > abs_offset.y;
	f32 increment_px = use_x ? offset.x : -offset.y;
	f32 sensitivity = p->sensitivity > 0.0f ? p->sensitivity : 0.02f;
	return increment_px * sensitivity;
}

f32 audio_parameter_relative_value(audio_parameter_t* p) {
	return (p->value - p->min_value) / (p->max_value - p->min_value);
}

f32 slider_map_value_from_gui(f32 val, audio_parameter_t* p) {
	val = grv_clamp_f32(val, 0.0f, 1.0f);
	switch (p->mapping_type) {
	case MAPPING_TYPE_VOLUME:
		return p->min_value + (p->max_value - p->min_value) * powf(val, 1.0f/3.0f);
	case MAPPING_TYPE_LOG_TIME:
		return p->min_value * powf(p->max_value / p->min_value, val);
	default:
		return p->min_value + (p->max_value - p->min_value) * val;
	}
	return 0.0f;
}

// gui_val = log10(val/min_val) / log10(max_val/min_val)
//
// gui_val * log10(max_val/min_val) = log10(val/min_val)
// log10(pow(max_val/min_val), gui_val) = log10(val/min_val)
// val = min_val * pow(max_val/_min_val, gui_val);

f32 slider_map_value_to_gui(audio_parameter_t* p) {
	f32 rel_value = audio_parameter_relative_value(p);
	switch (p->mapping_type) {
	case MAPPING_TYPE_VOLUME:
		return powf(rel_value, 3.0f);
	case MAPPING_TYPE_LOG_TIME:
		return p->value == 0.0f ? 0.0f : log10f(p->value/p->min_value) / log10f(p->max_value/p->min_value);
	default:
		return rel_value;
	}
}

char* slider_format_value(audio_parameter_t* p) {
	char* str = grvgm_draw_arena_alloc(32);
	switch (p->mapping_type) {
	case MAPPING_TYPE_LOG_TIME:
		if (p->value >= 0.1f) {
			snprintf(str, 16, "%.2fs", p->value);
		} else {
			snprintf(str, 16, "%.1fms", p->value * 1000.0f);
		}
		break;
	default:
		snprintf(str, 16, "%.2f", p->value);
	}
	return str;
}

void draw_rect_slider2(rect_i32 rect, char* label, audio_parameter_t* p) {
	GRV_UNUSED(label);
	rect_i32 slider_rect = {0, 0, 4, 10};
	slider_rect = rect_i32_align_to_rect(slider_rect, rect, GRV_ALIGNMENT_CENTER);
	u8 frame_color = 5;
	bool is_in_drag = grvgm_mouse_drag_started_in_rect(slider_rect);
	if (is_in_drag) {
		if (p->_initial_drag_value == GRV_MAX_F32) {
			p->_initial_drag_value = slider_map_value_to_gui(p);
		}
		frame_color=7;
		f32 gui_value = grv_clamp_f32(p->_initial_drag_value + slider_increment(p), 0.0f, 1.0f);
		f32 new_value = slider_map_value_from_gui(gui_value, p);
		if (p->set_value_func) {
			grv_assert(p->parent);
			p->set_value_func(p->parent, new_value);
		} else {
			p->value = new_value;
		}
	} else {
		p->_initial_drag_value = GRV_MAX_F32;
	}

	f32 display_value = (slider_rect.h - 2.0f) * slider_map_value_to_gui(p);
	i32 num_px = grv_round_f32(display_value);
	grvgm_draw_rect(slider_rect, frame_color);
	rect_i32 value_rect = {slider_rect.x + 1, slider_rect.y + 1 + (8 - num_px), slider_rect.w-2, num_px};
	grvgm_fill_rect(value_rect, 9);

	if (is_in_drag) label = slider_format_value(p);
	rect_i32 label_rect = rect_i32_align_to_rect(grvgm_text_rect(grv_str_ref(label)), slider_rect, GRV_ALIGNMENT_BELOW_CENTER);
	label_rect.y += 1;
	grvgm_draw_text_aligned(label_rect, grv_str_ref(label), GRV_ALIGNMENT_CENTER, 7);
}

void draw_rotary_slider(rect_i32 rect, grv_str_t text, i32* value, i32 min_value, i32 max_value) {
	rect_i32 slider_rect = {0, 0, 7, 7};
	slider_rect = rect_i32_align_to_rect(slider_rect, rect, GRV_ALIGNMENT_CENTER);

	fx32 display_value =
		fx32_mul_i32(
			fx32_div(
				fx32_from_i32(*value - min_value),
				fx32_from_i32(max_value - min_value)),
			8);
	grvgm_sprite_t spr = {
		.index = 32 + grv_clamp_i32(fx32_round(display_value), 0, 7)
	};
	grvgm_draw_sprite(rect_i32_pos(slider_rect), spr);
	grvgm_draw_text_aligned(slider_rect, text, GRV_ALIGNMENT_BELOW_CENTER, 6);
}

void draw_beat_time(transport_state_t* state, rect_i32 status_bar_rect) {
	char str[32];
	snprintf(str, 32, "%d.%d.%02d", state->bar, state->beat, state->pulse);
	grvgm_draw_text_aligned(status_bar_rect, grv_str_ref(str), GRV_ALIGNMENT_TOP_RIGHT, 6);
}

void draw_play_button(rect_i32 rect, transport_state_t* state) {
	i32 sprite_idx = state->is_playing ? 5 : 4;
	grvgm_sprite_t spr = {
		.index = sprite_idx,
	};
	grvgm_draw_sprite(rect_i32_pos(rect), spr);
	if (grvgm_mouse_click_in_rect(rect, GRVGM_BUTTON_MOUSE_LEFT)) {
		state->is_playing = !state->is_playing;
	}
}

void draw_envelope_gui(rect_i32 rect, envelope_t* env) {
	rect_i32 slider_rect = {.w=4,.h=16};
	i32 gap = 2;
	slider_rect = rect_i32_align_to_rect(slider_rect, rect, GRV_ALIGNMENT_TOP_LEFT);
	draw_rect_slider2(slider_rect, "A", &env->attack);
	slider_rect = rect_i32_clone_right(slider_rect, gap);
	draw_rect_slider2(slider_rect, "D", &env->decay);
	slider_rect = rect_i32_clone_right(slider_rect, gap);
	draw_rect_slider2(slider_rect, "S", &env->sustain);
	slider_rect = rect_i32_clone_right(slider_rect, gap);
	draw_rect_slider2(slider_rect, "R", &env->release);
}

void draw_synth_gui(rect_i32 rect, synth_state_t* state) {
	simple_synth_t* synth = &state->tracks.arr[state->current_pattern].synth;
	envelope_t* env = &synth->envelope;
	draw_envelope_gui(rect, env);
}

void on_draw(void* state) {
	synth_state_t* synth_state = state;
	grvgm_clear_screen(0);
	
	rect_i32 screen_rect = grvgm_screen_rect();
	rect_i32 status_bar_rect = {.w=screen_rect.w, .h=screen_rect.h/16 };
	rect_i32 content_rect = {.y=status_bar_rect.h, .w=screen_rect.w, .h=screen_rect.h - status_bar_rect.h };

	draw_play_button(
		rect_i32_align_to_rect((rect_i32){.w=7,.h=7}, status_bar_rect, GRV_ALIGNMENT_HORIZONTAL_CENTER),
		&synth_state->transport
	);
	draw_beat_time(&synth_state->transport, status_bar_rect);

	rect_i32 slider_rect = {.w=11,.h=16};
	slider_rect = rect_i32_align_to_rect(slider_rect, content_rect, GRV_ALIGNMENT_TOP_RIGHT);
	draw_rect_slider2(slider_rect, "VOL", &synth_state->master_volume);

	draw_synth_gui(content_rect, synth_state);
	draw_pattern_triggers(content_rect, synth_state);
}

