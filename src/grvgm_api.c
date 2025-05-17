#include "grv/grv_pseudo_random.h"

//==============================================================================
// api
//==============================================================================
void grvgm_clear_screen(u8 color) {
	grv_framebuffer_fill_u8(_grvgm_framebuffer(), color);
} 

void grvgm_draw_sprite(vec2_i32 pos, grvgm_sprite_t sprite) {
	grv_spritesheet8_t* spritesheet = sprite.spritesheet ? sprite.spritesheet : _grvgm_spritesheet();
    i32 width = sprite.w == 0 ? 1 : sprite.w;
    i32 height = sprite.h == 0 ? 1 : sprite.h;
	grv_img8_t img = grv_spritesheet8_get_img8_by_index(spritesheet, sprite.index, width, height);
	grv_framebuffer_blit_img8(_grvgm_framebuffer(), &img, pos.x, pos.y);
}

void grvgm_draw_sprite_fx32(vec2_fx32 pos, grvgm_sprite_t sprite) {
	grvgm_draw_sprite(vec2_fx32_round(pos), sprite);
}

void grvgm_draw_pixel(vec2_i32 pos, u8 color) {
	grv_framebuffer_set_pixel_u8(_grvgm_framebuffer(), pos, color);
}
void grvgm_draw_pixel_fx32(vec2_fx32 pos, u8 color) {
	grvgm_draw_pixel(vec2_fx32_round(pos), color);
}

void grvgm_draw_line(vec2_i32 p1, vec2_i32 p2, u8 color) {
	grv_framebuffer_draw_line_u8(_grvgm_framebuffer(), p1, p2, color);
}

void grvgm_draw_line_fx32(vec2_fx32 p1, vec2_fx32 p2, u8 color) {
	vec2_i32 p1_i32 = vec2_fx32_round(p1);
	vec2_i32 p2_i32 = vec2_fx32_round(p2);
	grv_framebuffer_draw_line_u8(_grvgm_framebuffer(), p1_i32, p2_i32, color);
}

void grvgm_draw_rect(rect_i32 rect, u8 color) {
	grv_framebuffer_draw_rect_u8(_grvgm_framebuffer(), rect, color);
}
void grvgm_draw_rect_fx32(rect_fx32 rect, u8 color) {
	grvgm_draw_rect(rect_fx32_round(rect), color);
}

void grvgm_fill_rect(rect_i32 rect, u8 color) {
	grv_framebuffer_fill_rect_u8(_grvgm_framebuffer(), rect, color);
}
void grvgm_fill_rect_fx32(rect_fx32 rect, u8 color) {
	grvgm_fill_rect(rect_fx32_round(rect), color);
}

void grvgm_draw_rect_chamfered(rect_i32 rect, u8 color) {
	grv_framebuffer_draw_rect_chamfered_u8(_grvgm_framebuffer(), rect, color);
}
void grvgm_draw_rect_chamfered_fx32(rect_fx32 rect, u8 color) {
	grvgm_draw_rect_chamfered(rect_fx32_round(rect), color);
}

void grvgm_fill_rect_chamfered(rect_i32 rect, u8 color) {
	grv_framebuffer_fill_rect_chamfered_u8(_grvgm_framebuffer(), rect, color);
}
void grvgm_fill_rect_chamfered_fx32(rect_fx32 rect, u8 color) {
	grvgm_fill_rect_chamfered(rect_fx32_round(rect), color);
}

void grvgm_draw_circle(vec2_i32 pos, i32 r, u8 color) {
	grv_framebuffer_draw_circle_u8(_grvgm_framebuffer(), pos.x, pos.y, r, color);
}
void grvgm_draw_circle_fx32(vec2_fx32 pos, fx32 r, u8 color) {
	grvgm_draw_circle(vec2_fx32_round(pos), fx32_round(r), color);
}

void grvgm_fill_circle(vec2_i32 pos, i32 r, u8 color) {
	grv_framebuffer_fill_circle_u8(_grvgm_framebuffer(), pos.x, pos.y, r, color);
}
void grvgm_fill_circle_fx32(vec2_fx32 pos, fx32 r, u8 color) {
	grvgm_fill_circle(vec2_fx32_round(pos), fx32_round(r), color);
}

rect_i32 grvgm_text_rect(grv_str_t str) {
    vec2_i32 text_size = grv_bitmap_font_calc_size(_grvgm_font(), str);
    return (rect_i32){0, 0, text_size.x, text_size.y};
}
rect_fx32 grvgm_text_rect_fx32(grv_str_t str) {
	return rect_fx32_from_rect_i32(grvgm_text_rect(str));
}

void grvgm_draw_text(vec2_i32 pos, grv_str_t text, u8 color) {
	grv_put_text_u8(_grvgm_framebuffer(), text, pos, _grvgm_font(), color);
}

typedef struct {
	vec2_i32 pos;
	grv_str_t str;
	u8 color;
} _grvgm_draw_text_floating_t;

void _grvgm_draw_text_floating(void* data) {
	_grvgm_draw_text_floating_t* string_data = data;
	rect_i32 text_rect = grvgm_text_rect(string_data->str);
	text_rect = rect_i32_move_to(text_rect, string_data->pos);
	grvgm_fill_rect(rect_i32_expand(text_rect, 1, 0), 0);
	grvgm_draw_text(string_data->pos, string_data->str, string_data->color);
}

void grvgm_draw_text_floating(vec2_i32 pos, grv_str_t text, u8 color) {
	_grvgm_draw_text_floating_t* data = grvgm_draw_arena_alloc(sizeof(_grvgm_draw_text_floating_t));
	*data = (_grvgm_draw_text_floating_t) { 
		.pos=pos,
		.color=7,
		.str=text,
	};
	grvgm_defer(_grvgm_draw_text_floating, data);
}

void grvgm_draw_text_fx32(vec2_fx32 pos, grv_str_t text, u8 color) {
	grvgm_draw_text(vec2_fx32_round(pos), text, color);
}

void grvgm_draw_text_aligned(rect_i32 rect, grv_str_t text, grv_alignment_t alignment, u8 color) {
    rect_i32 text_rect = rect_i32_align_to_rect(grvgm_text_rect(text), rect, alignment);
    grvgm_draw_text(rect_i32_pos(text_rect), text, color);
}
void grvgm_draw_text_aligned_fx32(rect_fx32 rect, grv_str_t text, grv_alignment_t alignment, u8 color) {
	grvgm_draw_text_aligned(rect_fx32_round(rect), text, alignment, color);
}

vec2_i32 grvgm_screen_size(void) {
	i32 w = _grvgm_state.options.screen_width;
	i32 h = _grvgm_state.options.screen_height;
	return (vec2_i32) {w, h};
}
vec2_fx32 grvgm_screen_size_fx32(void) {
	i32 w = _grvgm_state.options.screen_width;
	i32 h = _grvgm_state.options.screen_height;
	return vec2_fx32_from_i32(w, h);
}

rect_i32 grvgm_screen_rect(void) {
	vec2_i32 screen_size = grvgm_screen_size();
	return (rect_i32) {
		.w=screen_size.x,
		.h=screen_size.y
	};
}

rect_fx32 grvgm_screen_rect_fx32(void) {
	vec2_fx32 screen_size = grvgm_screen_size_fx32();
	return (rect_fx32) {
		.w=screen_size.x,
		.h=screen_size.y
	};
}

fx32 grvgm_time(void) {
	return (fx32){.val = (i32)(_grvgm_game_time_ms() * 1024 / 1000)};
}

fx32 grvgm_timediff(fx32 timestamp) {
	return fx32_sub(grvgm_time(), timestamp);
}

f32 grvgm_time_f32(void) {
	return _grvgm_game_time_ms() / 1000.0f;
}

u64 grvgm_ticks(void) {
	return SDL_GetTicks64();
}

void* grvgm_draw_arena_alloc(size_t size) {
	return grv_arena_alloc_zero(_grvgm_state.draw_arena, size);
}

grv_arena_t* grvgm_draw_arena(void) {
	return _grvgm_state.draw_arena;
}

void grvgm_defer(void(*func)(void*), void* data) {
	grvgm_callback_t** root = &_grvgm_state.end_of_frame_callback_queue.root;
	grvgm_callback_t** head = &_grvgm_state.end_of_frame_callback_queue.head;
	grv_arena_t* arena = _grvgm_state.draw_arena;
	grvgm_callback_t* callback = grv_arena_alloc(arena, sizeof(grvgm_callback_t));
	*callback = (grvgm_callback_t) { .func=func, .data=data };

	if (*head == NULL) {
		*root = callback;
		*head = callback;
	} else {
		(*head)->next = callback;
		(*head) = (*head)->next;
	}
}

vec2_i32 grvgm_mouse_position(void) {
	return vec2f_round(_grvgm_window()->mouse_view_pos);
}

vec2_fx32 grvgm_mouse_position_fx32(void) {
	return vec2_fx32_from_vec2_f32(_grvgm_window()->mouse_view_pos);
}

bool grvgm_mouse_in_rect(rect_i32 rect) {
	return rect_i32_point_inside(rect, grvgm_mouse_position());
}

vec2_i32 grvgm_initial_drag_position(void) {
	return vec2f_round(_grvgm_window()->mouse_drag_initial_view_pos);
}

vec2_fx32 grvgm_initial_drag_position_fx32(void) {
	return vec2_fx32_from_vec2_f32(_grvgm_window()->mouse_drag_initial_view_pos);
}

bool grvgm_mouse_click_in_rect(rect_i32 rect, i32 button_id) {
	grv_window_t* w = _grvgm_window();
	grv_mouse_button_info_t* button_info = &w->mouse_button_info[button_id];
	if (button_info->is_down && !button_info->was_down && button_info->click_count == 0) {
		vec2_i32 pos = vec2f_round(button_info->initial_view_pos);
		if (rect_i32_point_inside(rect, pos)) return true;
	}
	return false;
}

bool grvgm_mouse_click_in_rect_with_id(u64 id, rect_i32 rect, i32 button_id) {
	// register rect with id as click recipient
	grvgm_mouse_event_type_t event_type =
		button_id == GRVGM_BUTTON_MOUSE_LEFT
		? MOUSE_EVENT_TYPE_LEFT_CLICK
		: MOUSE_EVENT_TYPE_RIGHT_CLICK;
	_grvgm_push_mouse_event_receiver(id, rect, event_type);
	
	return _grvgm_state.mouse_event_receiver == id;
}

bool grvgm_mouse_drag_started_in_rect(rect_i32 rect) {
	grv_window_t* w = _grvgm_window();
	return w->is_in_drag
	  && rect_i32_point_inside(rect, vec2f_round(w->mouse_drag_initial_view_pos));
}

f32 grvgm_random_f32(void) {
	return grv_pseudo_random_f32();
}

void grvgm_set_screen_size(i32 w, i32 h) {
	_grvgm_state.options.screen_width = w;
	_grvgm_state.options.screen_height = h;

}

void grvgm_set_sprite_size(i32 w) {
	_grvgm_state.options.sprite_width = w;
}

void grvgm_set_fps(i32 fps) {
	_grvgm_state.options.fps = fps;
}

void grvgm_set_use_game_state_store(bool flag) {
	_grvgm_state.options.use_game_state_store = flag;
}
