#include "grvgm.h"

typedef struct {
	bool start_selected;
	i32 value;
} synth_state_t;

static synth_state_t synth_state = {0};

void on_init(void** game_state, size_t* size) {

}

void on_update(void* game_state, float delta_time) {

}

bool grvgm_mouse_in_rect(rect_i32 rect) {
	return rect_i32_point_inside(rect, grvgm_mouse_position());
}

bool draw_text_button(rect_i32 rect, grv_str_t text) {
	grvgm_fill_rect_chamfered(rect, 1);
	grvgm_draw_rect_chamfered(rect, 6);
	grvgm_draw_text_aligned(text, rect, GRV_ALIGNMENT_CENTER, 7);
	return grvgm_mouse_in_rect(rect);
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
		.pos = rect_i32_pos(slider_rect),
		.index = 32 + fx32_round(display_value)
	};
	grvgm_draw_sprite(spr);
	grvgm_draw_rect(rect_fx32_from_rect_i32(slider_rect), 9);
	rect_i32 label_rect = rect_i32_align_to_rect(grvgm_text_rect(text), slider_rect, GRV_ALIGNMENT_BELOW_CENTER);
	grvgm_draw_text_aligned(text, rect_fx32_from_rect_i32(label_rect), GRV_ALIGNMENT_CENTER, 8);
	grvgm_draw_rect(rect_fx32_from_rect_i32(label_rect), 10);
}

void on_draw() {
	grvgm_clear_screen(0);
	rect_i32 screen_rect = grvgm_screen_rect();
	grvgm_draw_text_aligned(grv_str_ref("Welcome"), rect_fx32_from_rect_i32(screen_rect), GRV_ALIGNMENT_CENTER, 7);
	grvgm_sprite_t sprite = {
		.pos = vec2_fx32_from_i32(62,40),
		.index = 0
	};
	grvgm_draw_sprite(sprite);
	rect_i32 button_rect = {0, 0, 26, 11};
	rect_i32 lower_rect = rect_i32_split_lower(grvgm_screen_rect(), 1, 1);
	button_rect = rect_i32_align_to_rect(button_rect, lower_rect, GRV_ALIGNMENT_CENTER);
	rect_i32 slider_rect = rect_i32_align_to_rect(button_rect, button_rect, GRV_ALIGNMENT_HORIZONTAL_CENTER | GRV_ALIGNMENT_VERTICAL_BELOW);
	draw_text_button(button_rect, grv_str_ref("start"));
	draw_rotary_slider(slider_rect, grv_str_ref("A"), &synth_state.value, 0, 127);
	grvgm_draw_pixel(grvgm_mouse_position(), 8);
}
