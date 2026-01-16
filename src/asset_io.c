#include "asset_io.h"
#include "includes.h"
#include "macros.h"

#ifdef EMBED_ASSETS
#include "embedded_assets.h"
#endif

typedef struct
{
    const char *disk_path;
    bool is_text;
#ifdef EMBED_ASSETS
    const unsigned char *raw_data;
    size_t raw_size;
    const unsigned char *compressed_data;
    size_t compressed_size;
    size_t original_size;
#endif
} asset_info_t;

global_variable const asset_info_t g_asset_info[ASSET_ID_COUNT] = {
    [ASSET_SHADER_LIGHTING_INSTANCING_VS] = {
        .disk_path = "./shaders/lighting_instancing.vs",
        .is_text = true,
#ifdef EMBED_ASSETS
        .raw_data = (const unsigned char *)shader_lighting_instancing_vs_data,
        .raw_size = 0,
#endif
    },
    [ASSET_SHADER_LIGHTING_FS] = {
        .disk_path = "./shaders/lighting.fs",
        .is_text = true,
#ifdef EMBED_ASSETS
        .raw_data = (const unsigned char *)shader_lighting_fs_data,
        .raw_size = 0,
#endif
    },
    [ASSET_FONT_PERFECT_DOS] = {
        .disk_path = "./assets/fonts/Perfect DOS VGA 437.ttf",
        .is_text = false,
#ifdef EMBED_ASSETS
        .compressed_data = font_perfect_dos_data,
        .compressed_size = font_perfect_dos_compressed_size,
        .original_size = font_perfect_dos_original_size,
#endif
    },
    [ASSET_ICON_APP] = {
        .disk_path = "./assets/images/app_icon.png",
        .is_text = false,
#ifdef EMBED_ASSETS
        .compressed_data = icon_app_data,
        .compressed_size = icon_app_compressed_size,
        .original_size = icon_app_original_size,
#endif
    },
    [ASSET_MODEL_EARTH] = {
        .disk_path = "./assets/Earth_1_12756_optimized.glb",
        .is_text = false,
#ifdef EMBED_ASSETS
        .compressed_data = model_earth_data,
        .compressed_size = model_earth_compressed_size,
        .original_size = model_earth_original_size,
#endif
    },
    [ASSET_DATA_ARCMIN_A] = {
        .disk_path = "./input_data/data_100k_arcmin.txt",
        .is_text = true,
#ifdef EMBED_ASSETS
        .compressed_data = data_arcmin_a_data,
        .compressed_size = data_arcmin_a_compressed_size,
        .original_size = data_arcmin_a_original_size,
#endif
    },
    [ASSET_DATA_ARCMIN_B] = {
        .disk_path = "./input_data/flat_100k_arcmin.txt",
        .is_text = true,
#ifdef EMBED_ASSETS
        .compressed_data = data_arcmin_b_data,
        .compressed_size = data_arcmin_b_compressed_size,
        .original_size = data_arcmin_b_original_size,
#endif
    },
    [ASSET_DATA_SEYFERT] = {
        .disk_path = "./input_data/redshift_input_data/seyfert.dat",
        .is_text = true,
#ifdef EMBED_ASSETS
        .compressed_data = data_seyfert_data,
        .compressed_size = data_seyfert_compressed_size,
        .original_size = data_seyfert_original_size,
#endif
    },
    [ASSET_DATA_SAGA_DR3] = {
        .disk_path = "./input_data/redshift_input_data/saga-dr3-satellites.txt",
        .is_text = true,
#ifdef EMBED_ASSETS
        .compressed_data = data_saga_dr3_data,
        .compressed_size = data_saga_dr3_compressed_size,
        .original_size = data_saga_dr3_original_size,
#endif
    },
};

#ifndef EMBED_ASSETS
internal i32
asset_io_read_file(const char *path, bool add_null_terminator, asset_blob_t *out_blob)
{
    if (out_blob == NULL || path == NULL)
    {
        return 1;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "[ERROR] Cannot open asset: %s\n", path);
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        fprintf(stderr, "[ERROR] Cannot seek asset: %s\n", path);
        return 1;
    }

    long file_size = ftell(file);
    if (file_size < 0)
    {
        fclose(file);
        fprintf(stderr, "[ERROR] Cannot read asset size: %s\n", path);
        return 1;
    }

    rewind(file);

    size_t alloc_size = (size_t)file_size + (add_null_terminator ? 1 : 0);
    u8 *buffer = (u8 *)malloc(alloc_size);
    if (buffer == NULL)
    {
        fclose(file);
        fprintf(stderr, "[ERROR] Out of memory for asset: %s\n", path);
        return 1;
    }

    size_t read_bytes = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);

    if (read_bytes != (size_t)file_size)
    {
        fprintf(stderr, "[ERROR] Short read for asset: %s\n", path);
        free(buffer);
        return 1;
    }

    if (add_null_terminator)
    {
        buffer[file_size] = 0;
    }

    out_blob->data = buffer;
    out_blob->size = (size_t)file_size;
    return 0;
}
#endif

i32 asset_io_get_blob(asset_id_t id, asset_blob_t *out_blob)
{
    if (out_blob == NULL)
    {
        return 1;
    }

    out_blob->data = NULL;
    out_blob->size = 0;

    if (id < 0 || id >= ASSET_ID_COUNT)
    {
        return 1;
    }

    const asset_info_t *info = &g_asset_info[id];

#ifdef EMBED_ASSETS
    if (info->raw_data != NULL)
    {
        size_t raw_size = info->raw_size;
        if (raw_size == 0)
        {
            raw_size = strlen((const char *)info->raw_data);
        }

        u8 *buffer = (u8 *)malloc(raw_size + (info->is_text ? 1 : 0));
        if (buffer == NULL)
        {
            fprintf(stderr, "[ERROR] Out of memory for asset id %d\n", id);
            return 1;
        }

        memcpy(buffer, info->raw_data, raw_size);
        if (info->is_text)
        {
            buffer[raw_size] = 0;
        }

        out_blob->data = buffer;
        out_blob->size = raw_size;
        return 0;
    }

    if (info->compressed_data == NULL || info->original_size == 0)
    {
        fprintf(stderr, "[ERROR] Embedded asset missing for id %d\n", id);
        return 1;
    }

    unsigned char *data = embedded_decompress_zlib(info->compressed_data,
                                                   info->compressed_size,
                                                   info->original_size);
    if (data == NULL)
    {
        fprintf(stderr, "[ERROR] Decompress failed for asset id %d\n", id);
        return 1;
    }

    out_blob->data = (u8 *)data;
    out_blob->size = info->original_size;
    return 0;
#else
    return asset_io_read_file(info->disk_path, info->is_text, out_blob);
#endif
}

void asset_blob_free(asset_blob_t *blob)
{
    if (blob == NULL || blob->data == NULL)
    {
        return;
    }

    free(blob->data);
    blob->data = NULL;
    blob->size = 0;
}

asset_blob_t asset_io_load_blob(asset_id_t id)
{
    asset_blob_t blob = {0};
    if (asset_io_get_blob(id, &blob) != 0)
    {
        blob.data = NULL;
        blob.size = 0;
    }
    return blob;
}

Shader asset_io_load_shader(asset_id_t vs_id, asset_id_t fs_id)
{
    asset_blob_t vs_blob = {0};
    asset_blob_t fs_blob = {0};
    Shader shader = {0};

    if (asset_io_get_blob(vs_id, &vs_blob) != 0)
    {
        return shader;
    }
    if (asset_io_get_blob(fs_id, &fs_blob) != 0)
    {
        asset_blob_free(&vs_blob);
        return shader;
    }

    shader = LoadShaderFromMemory((const char *)vs_blob.data, (const char *)fs_blob.data);

    asset_blob_free(&vs_blob);
    asset_blob_free(&fs_blob);
    return shader;
}

#ifdef EMBED_ASSETS
global_variable unsigned char *g_model_blob_data = NULL;
global_variable size_t g_model_blob_size = 0;

internal unsigned char *
asset_io_load_file_data(const char *fileName, int *dataSize)
{
    if (dataSize == NULL)
    {
        return NULL;
    }

    if (g_model_blob_data != NULL && g_model_blob_size > 0)
    {
        if (fileName != NULL && strstr(fileName, ".glb") != NULL)
        {
            unsigned char *data = (unsigned char *)RL_MALLOC(g_model_blob_size);
            if (data != NULL)
            {
                memcpy(data, g_model_blob_data, g_model_blob_size);
                *dataSize = (int)g_model_blob_size;
                return data;
            }
        }
    }

    *dataSize = 0;
    return NULL;
}
#endif

Model asset_io_load_model(asset_id_t model_id)
{
    Model model = {0};

    if (model_id < 0 || model_id >= ASSET_ID_COUNT)
    {
        return model;
    }

#ifdef EMBED_ASSETS
    asset_blob_t blob = {0};
    if (asset_io_get_blob(model_id, &blob) != 0)
    {
        return model;
    }

    g_model_blob_data = (unsigned char *)blob.data;
    g_model_blob_size = blob.size;

    SetLoadFileDataCallback(asset_io_load_file_data);
    model = LoadModel("embedded://earth.glb");
    SetLoadFileDataCallback(NULL);

    g_model_blob_data = NULL;
    g_model_blob_size = 0;
    asset_blob_free(&blob);

    return model;
#else
    const asset_info_t *info = &g_asset_info[model_id];
    if (info->disk_path == NULL)
    {
        return model;
    }

    return LoadModel(info->disk_path);
#endif
}

Font asset_io_load_font(asset_id_t font_id, i32 font_size, i32 *codepoints, i32 codepoint_count)
{
    Font font = {0};
    asset_blob_t blob = {0};

    if (asset_io_get_blob(font_id, &blob) != 0 || blob.data == NULL)
    {
        return font;
    }

    font = LoadFontFromMemory(".ttf", blob.data, (int)blob.size, font_size, codepoints, codepoint_count);
    asset_blob_free(&blob);
    return font;
}

Image asset_io_load_image(asset_id_t image_id, const char *file_ext)
{
    Image image = {0};
    asset_blob_t blob = {0};

    if (asset_io_get_blob(image_id, &blob) != 0 || blob.data == NULL)
    {
        return image;
    }

    image = LoadImageFromMemory(file_ext, blob.data, (int)blob.size);
    asset_blob_free(&blob);
    return image;
}
