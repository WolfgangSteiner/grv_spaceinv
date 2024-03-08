#include "grv/grv.h"
#include "grv_gfx/grv_window.h"
#include "grv_gfx/grv_bitmap_font.h"

char* player_sprite_data = 
"   7    "
"   7    "
"  7 7   "
" 7   7  "
" 7   7  "
"7777777 "
"77   77 "
"        ";

typedef s32 i32;

typedef struct {
    i32 w;
    i32 h;
    i32 row_skip; 
    u8* pixel_data;
} grv_sprite_t;

grv_sprite_t parse_sprite_data(grv_str_t input, i32 width, i32 height) {
    assert(width * height <= input.size);
    grv_sprite_t spr = {.w=width, .h=height, .row_skip=width, .pixel_data=grv_alloc_zeros(width*height) };
    char* src = input.data;
    u8* dst = spr.pixel_data;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            char c = *src++;
            c = (c == ' ') ? 0 : c - '0';
            *dst++ = c;
        }
    }
    return spr;
}

void grv_framebuffer_put_sprite(grv_frame_buffer_t* fb, grv_sprite_t* spr, i32 x, i32 y) {
    i32 x_start = x;
    i32 x_end = x + spr->w - 1;
    i32 y_start = y;
    i32 y_end = y + spr->h - 1;
    if (x_start >= fb->width || x_end < 0 || y_start >= fb->height || y_end < 0) return;

    x_start = grv_clamp_s32(x_start, 0, fb->width - 1);
    x_end = grv_clamp_s32(x_end, 0, fb->width - 1);
    y_start = grv_clamp_s32(y_start, 0, fb->height - 1);
    y_end = grv_clamp_s32(y_end, 0, fb->height - 1);

    u8* src_row_ptr = spr->pixel_data + (y_start - y) * spr->row_skip + (x_start - x);
    u8* dst_row_ptr = fb->indexed_data + y_start * fb->width + x_start;
    for (i32 iy = y_start; iy <= y_end; ++iy) {
        u8* src_ptr = src_row_ptr;
        u8* dst_ptr = dst_row_ptr;
        for (i32 ix = x_start; ix <= x_end; ++ix) {
            *dst_ptr++ = *src_ptr++;
        }
        src_row_ptr += spr->row_skip;
        dst_row_ptr += fb->width;
    }    
}

int main(int argc, char** argv) {
    grv_sprite_t player_sprite = parse_sprite_data(grv_str_ref(player_sprite_data), 8, 8);
    grv_window_t* window = grv_window_new(128, 128, 4.0f, grv_str_ref("spaceinv"));
    i32 window_width = 128;
    i32 window_height = 128;

    grv_color_palette_init_with_type(&window->frame_buffer.palette, GRV_COLOR_PALETTE_PICO8);
    window->borderless = true;
    grv_window_show(window);
    grv_bitmap_font_t* font = grv_get_cozette_font();
    grv_frame_buffer_t* fb = &window->frame_buffer;

    while (true) {
        grv_window_poll_events();
        if (window->should_close) {
            break;
        }
        grv_frame_buffer_clear(&window->frame_buffer);

        grv_framebuffer_put_sprite(fb, &player_sprite, 64, 64);         

        grv_window_present(window);
        grv_sleep(0.03);
    }

    return 0;
}
