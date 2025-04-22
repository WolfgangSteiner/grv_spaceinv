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
	content_rect = rect_i32_align_to_rect(content_rect, rect, GRV_ALIGNMENT_CENTER);
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

void draw_rect_slider2(rect_i32 rect, grv_str_t text, i32* value, i32 min_value, i32 max_value) {
	static i32 initial_value = GRV_MAX_I32;
	rect_i32 slider_rect = {0, 0, 4, 10};
	slider_rect = rect_i32_align_to_rect(slider_rect, rect, GRV_ALIGNMENT_CENTER);
	u8 frame_color = 5;
	if (grvgm_mouse_drag_started_in_rect(slider_rect)) {
		if (initial_value == GRV_MAX_I32) {
			initial_value = *value;
		}
		frame_color=7;
		vec2_i32 offset = vec2i_sub(grvgm_mouse_position(), grvgm_initial_drag_position());
		vec2_i32 abs_offset = vec2i_abs(offset);
		bool use_x = abs_offset.x > abs_offset.y;
		i32 offset_val = use_x ? offset.x : -offset.y;
		i32 new_val = grv_clamp_i32(
			initial_value + offset_val,
			min_value,
			max_value
		);
		*value = new_val;
	} else {
		initial_value = GRV_MAX_I32;
	}

	fx32 display_value =
		fx32_mul_i32(
			fx32_div(
				fx32_from_i32(*value - min_value),
				fx32_from_i32(max_value - min_value)),
			8);
	i32 num_px = fx32_round(display_value);
	grvgm_draw_rect(slider_rect, frame_color);
	rect_i32 value_rect = {slider_rect.x + 1, slider_rect.y + 1 + (8 - num_px), slider_rect.w-2, num_px};
	grvgm_fill_rect(value_rect, 9);
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
	rect_i32 label_rect = rect_i32_align_to_rect(grvgm_text_rect(text), slider_rect, GRV_ALIGNMENT_BELOW_CENTER);
	grvgm_draw_text_aligned(slider_rect, text, GRV_ALIGNMENT_BELOW_CENTER, 6);
}

void draw_beat_time(transport_state_t* state) {
	char str[32];
	snprintf(str, 32, "%d.%d.%02d", state->bar, state->beat, state->pulse);
	grvgm_draw_text((vec2i){0}, grv_str_ref(str), 6);
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

void on_draw(void* state) {
	synth_state_t* synth_state = state;
	grvgm_clear_screen(0);

	rect_i32 screen_rect = grvgm_screen_rect();
	rect_i32 button_rect = {0, 0, 25, 11};
	rect_i32 lower_rect = rect_i32_split_lower(grvgm_screen_rect(), 1, 1);
	button_rect = rect_i32_align_to_rect(button_rect, lower_rect, GRV_ALIGNMENT_CENTER);
	rect_i32 slider_rect = rect_i32_align_to_rect(button_rect, button_rect, GRV_ALIGNMENT_HORIZONTAL_CENTER | GRV_ALIGNMENT_VERTICAL_BELOW);
	draw_text_button(button_rect, grv_str_ref("start"), &synth_state->start_selected);
	draw_rotary_slider(slider_rect, grv_str_ref("A"), &synth_state->value, 0, 127);
	slider_rect = rect_i32_align_to_rect(button_rect, button_rect, GRV_ALIGNMENT_HORIZONTAL_CENTER | GRV_ALIGNMENT_VERTICAL_ABOVE);
	draw_rect_slider2(slider_rect, grv_str_ref("A"), &synth_state->value, 0, 127);

	vec2i screen_size = grvgm_screen_size();
	rect_i32 trigger_rect = {.y=screen_size.y - 7, .w=screen_size.x, .h=6};
	draw_pattern_triggers(trigger_rect, synth_state);
	draw_beat_time(&synth_state->transport);
	draw_play_button(
		rect_i32_align_to_rect((rect_i32){.w=7,.h=7}, grvgm_screen_rect(), GRV_ALIGNMENT_HORIZONTAL_CENTER),
		&synth_state->transport
	);

	if (synth_state->tracks.arr[0].synth.envelope.state >= 1) {
		grvgm_draw_pixel((vec2_i32){127,0}, 7);
	} else {
		grvgm_draw_pixel((vec2_i32){127,0}, 5);

	}
}

