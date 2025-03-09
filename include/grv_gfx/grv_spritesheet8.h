#ifndef GRV_SPRITESHEET8_H
#define GRV_SPRITESHEET8_H

#include "grv_gfx/grv_img8.h"

typedef struct {
    grv_img8_t img;
    i32 spr_w, spr_h;
    i32 num_rows, num_cols;
} grv_spritesheet8_t;

grv_spritesheet8_t grv_spritesheet8_create(
        i32 width, i32 height, i32 sprite_width, i32 sprite_height);

grv_spritesheet8_t grv_spritesheet8_from_str(
        grv_str_t input, i32 width, i32 height, i32 sprite_width, i32 sprite_height); 

bool grv_spritesheet8_load_from_bmp(grv_str_t filename, grv_spritesheet8_t* spritesheet, grv_error_t* err); 

grv_img8_t grv_spritesheet8_get_img8(grv_spritesheet8_t* sprite_sheet, i32 row_idx, i32 col_idx); 

grv_img8_t grv_spritesheet8_get_img8_by_index(grv_spritesheet8_t* sprite_sheet, i32 index); 

#endif
