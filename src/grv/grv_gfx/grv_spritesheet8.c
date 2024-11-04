#include "grv_gfx/grv_spritesheet8.h"
#include "grv/grv_memory.h"

grv_spritesheet8_t grv_spritesheet8_create( i32 width, i32 height, i32 sprite_width, i32 sprite_height) {
    grv_spritesheet8_t res = {0};
    res.img.w = width;
    res.img.h = height;
    res.img.row_skip = width;
    res.img.pixel_data = grv_alloc_zeros(width * height);
    res.img.owns_data = true;
    res.spr_w = sprite_width;
    res.spr_h = sprite_height;
    res.num_rows = height / sprite_height;
    res.num_cols = width / sprite_width;
    return res;
}

grv_spritesheet8_t grv_spritesheet8_from_str(grv_str_t str, i32 width, i32 height, i32 sprite_width, i32 sprite_height) {
    grv_img8_t img = grv_img8_from_str(str, width, height);
    grv_spritesheet8_t res = {
        .img=img,
        .spr_w=sprite_width,
        .spr_h=sprite_height,
        .num_rows=height / sprite_height,
        .num_cols=width / sprite_width
    };
    return res;
}

bool grv_spritesheet8_load_from_bmp(grv_str_t filename, grv_spritesheet8_t* spritesheet, grv_error_t* err) {
    grv_assert(spritesheet->spr_w != 0);
    grv_assert(spritesheet->spr_h != 0);

    bool success = grv_img8_load_from_bmp(filename, &spritesheet->img, err);
    if (success) {
        spritesheet->num_rows = spritesheet->img.w / spritesheet->spr_w;
        spritesheet->num_cols = spritesheet->img.h / spritesheet->spr_h;
        return true;
    } else {
        return false;
    }
}

grv_img8_t grv_spritesheet8_get_img8(grv_spritesheet8_t* sprite_sheet, i32 row_idx, i32 col_idx) {
    grv_assert(col_idx < sprite_sheet->num_cols);
    grv_assert(row_idx < sprite_sheet->num_rows);
    u8* pixel_ptr = sprite_sheet->img.pixel_data + sprite_sheet->img.row_skip * sprite_sheet->spr_h * row_idx + sprite_sheet->spr_w * col_idx;
    return (grv_img8_t){
        .w=sprite_sheet->spr_w,
        .h=sprite_sheet->spr_h,
        .row_skip=sprite_sheet->img.row_skip,
        .owns_data=false,
        .pixel_data=pixel_ptr
    };
}

grv_img8_t grv_spritesheet8_get_img8_by_number(grv_spritesheet8_t* sprite_sheet, i32 number) {
    grv_assert(number < sprite_sheet->num_rows * sprite_sheet->num_cols); 
    i32 row_idx = number / sprite_sheet->num_cols;
    i32 col_idx = number % sprite_sheet->num_cols;
    return grv_spritesheet8_get_img8(sprite_sheet, row_idx, col_idx);
}

