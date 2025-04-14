#include "grvgm.h"

void on_init(void** game_state, size_t* size) {

}

void on_update(void* game_state, float delta_time) {

}

void on_draw() {
	rect_fx32 screen_rect = grvgm_screen_rect();
	grvgm_draw_text_aligned(grv_str_ref("Welcome"), screen_rect, GRV_ALIGNMENT_CENTER, 7);
}
