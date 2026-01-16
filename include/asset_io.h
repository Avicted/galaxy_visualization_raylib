#ifndef ASSET_IO_H
#define ASSET_IO_H

#include <stdbool.h>

#include "redefines.h"
#include "raylib_includes.h"

typedef enum
{
    ASSET_SHADER_LIGHTING_INSTANCING_VS = 0,
    ASSET_SHADER_LIGHTING_FS,
    ASSET_FONT_PERFECT_DOS,
    ASSET_ICON_APP,
    ASSET_MODEL_EARTH,
    ASSET_DATA_ARCMIN_A,
    ASSET_DATA_ARCMIN_B,
    ASSET_DATA_SEYFERT,
    ASSET_DATA_SAGA_DR3,
    ASSET_ID_COUNT,
} asset_id_t;

typedef struct
{
    u8 *data;
    size_t size;
} asset_blob_t;

i32 asset_io_get_blob(asset_id_t id, asset_blob_t *out_blob);
void asset_blob_free(asset_blob_t *blob);
asset_blob_t asset_io_load_blob(asset_id_t id);

Shader asset_io_load_shader(asset_id_t vs_id, asset_id_t fs_id);
Model asset_io_load_model(asset_id_t model_id);
Font asset_io_load_font(asset_id_t font_id, i32 font_size, i32 *codepoints, i32 codepoint_count);
Image asset_io_load_image(asset_id_t image_id, const char *file_ext);

#endif // ASSET_IO_H
