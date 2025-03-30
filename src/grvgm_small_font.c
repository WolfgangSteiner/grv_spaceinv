#include "grv_gfx/grv_bitmap_font.h"
#include "grv/grv.h"

char* small_font_data = 
"xxx  x  xxx xxx x x xxx xxx xxx xxx xxx           x     x   xxx "
"x x  x    x   x x x x   x     x x x x x  x   x   x  xxx  x    x "
"x x  x  xx  xxx xxx xxx xxx   x xxx xxx         x         x  xx "
"x x  x  x     x   x   x x x   x x x   x  x   x   x  xxx  x      "
"xxx  x  xxx xxx   x xxx xxx   x xxx xxx     x     x     x    x  "

" x  xxx xxx  xx xx  xxx xxx  xx x x xxx xxx x x x   xxx xx   xx "
"x x x x x x x   x x x   x   x   x x  x   x  x x x   xxx x x x x "
"x x xxx xx  x   x x xx  xx  x   xxx  x   x  xx  x   x x x x x x "
"x   x x x x x   x x x   x   x x x x  x   x  x x x   x x x x x x "
" xx x x xxx  xx xx  xxx x   xxx x x xxx xx  x x xxx x x x x xx  "

"xxx  xx xxx  xx xxx x x x x x x x x x x xxx xx  x    xx  x      "
"x x x x x x x    x  x x x x x x x x x x   x x    x    x x x     "
"xxx x x xx  xxx  x  x x x x x x  x   x   x  x    x    x         " 
"x   xx  x x   x  x  x x xxx xxx x x  x  x   x    x    x         "
"x    xx x x xx   x   xx  x  xxx x x  x  xxx xx    x  xx     xxx "
;

static grv_bitmap_font_t* small_font = NULL;

grv_bitmap_font_t* grvgm_get_small_font(void) {
    i32 glyphs_per_line = 16;
    i32 glyph_count = glyphs_per_line * 3;
    i32 glyph_width = 3;
    i32 glyph_raster_width = 4;
    i32 glyph_hskip = 4;
    i32 glyph_height = 5;
    i32 glyph_byte_count = 5;

    if (small_font == NULL) {
        small_font = grv_alloc_zeros(sizeof(grv_bitmap_font_t));
        *small_font = (grv_bitmap_font_t) {
            .glyph_start_idx=0x30,
            .glyph_count=glyph_count,
            .glyph_width=glyph_width,
            .glyph_height=glyph_height,
            .glyph_byte_count=glyph_height,
            .mirrored_definition=true,
            .hskip=glyph_hskip,
            .vskip=glyph_height + 1,
            .em=glyph_width,
            .uppercase_only=true,
            .topskip=0
        };
        small_font->glyph_data = grv_alloc_zeros(256 * glyph_byte_count);
        
        for (i32 glyph_idx = 0; glyph_idx < glyph_count; glyph_idx++) {
            i32 row_idx = glyph_idx / glyphs_per_line;
            i32 bytes_per_line = glyph_raster_width * glyphs_per_line;
            i32 bytes_per_row = bytes_per_line * glyph_height;
            char* src = small_font_data + row_idx * bytes_per_row + glyph_raster_width * (glyph_idx % glyphs_per_line);
            u8* dst = small_font->glyph_data + (glyph_idx * glyph_byte_count);
            for (i32 row = 0; row < glyph_height; row++) {
                u8 row_byte = 0;
                for (i32 col = 0; col < glyph_width; col++) {
                    row_byte |= (src[col] == ' ' ? 0 : 1) << col;
                }
                *dst++ = row_byte;
                src += bytes_per_line;
            }
        }
    }

    return small_font;
}

