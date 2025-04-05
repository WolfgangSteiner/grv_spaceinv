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
	i32 w = _grvgm_framebuffer()->width;
	i32 h = _grvgm_framebuffer()->height;
	return (vec2_i32) {w, h};
}
vec2_fx32 grvgm_screen_size_fx32(void) {
	i32 w = _grvgm_framebuffer()->width;
	i32 h = _grvgm_framebuffer()->height;
	return vec2_fx32_from_i32(w, h);
}

rect_i32 grvgm_screen_rect(void) {
	return (rect_i32) {
		.x=0, .y=0,
		.w=_grvgm_framebuffer()->width,
		.h=_grvgm_framebuffer()->height
	};
}

rect_fx32 grvgm_screen_rect_fx32(void) {
	return (rect_fx32) {
		.x=0, .y=0,
		.w=fx32_from_i32(_grvgm_framebuffer()->width),
		.h=fx32_from_i32(_grvgm_framebuffer()->height)
	};
}

fx32 grvgm_time(void) {
	return (fx32){.val = (i32)(_grvgm_game_time_ms() * 1024 / 1000)};
}

fx32 grvgm_timediff(fx32 timestamp) {
	return fx32_sub(grvgm_time(), timestamp);
}

vec2_i32 grvgm_mouse_position(void) {
	return vec2f_round(_grvgm_window()->mouse_view_pos);
}

vec2_fx32 grvgm_mouse_position_fx32(void) {
	return vec2_fx32_from_vec2_f32(_grvgm_window()->mouse_view_pos);
}
