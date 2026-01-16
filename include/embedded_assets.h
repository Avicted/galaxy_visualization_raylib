/**
 * embedded_assets.h - Unified header for embedded asset support
 *
 * When EMBED_ASSETS is defined, this header provides:
 * - Compressed asset data arrays
 * - Decompression utilities using miniz
 * - Helper macros for loading from memory
 */
#ifndef EMBEDDED_ASSETS_H
#define EMBEDDED_ASSETS_H

#ifdef EMBED_ASSETS

#include "redefines.h"
#include <stdlib.h>
#include <string.h>

// Miniz for zlib decompression (third-party, see vendor/)
#define MINIZ_NO_STDIO
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "../vendor/miniz.h"

// Include all generated embedded asset headers
#include "embedded/shader_lighting_instancing_vs.h"
#include "embedded/shader_lighting_fs.h"
#include "embedded/shader_bloom_vs.h"
#include "embedded/shader_bloom_fs.h"
#include "embedded/font_perfect_dos.h"
#include "embedded/font_abuget.h"
#include "embedded/icon_app.h"
#include "embedded/model_earth.h"
#include "embedded/data_arcmin_a.h"
#include "embedded/data_arcmin_b.h"
#include "embedded/data_seyfert.h"
#include "embedded/data_saga_dr3.h"

/**
 * Decompress zlib-compressed data
 * @param compressed_data Pointer to zlib-compressed data
 * @param compressed_size Size of compressed data
 * @param original_size Expected size of decompressed data
 * @return Pointer to decompressed data (caller must free), or NULL on error
 */
internal inline unsigned char *
embedded_decompress_zlib(const unsigned char *compressed_data, size_t compressed_size, size_t original_size)
{
    unsigned char *decompressed = (unsigned char *)malloc(original_size + 1);
    if (decompressed == NULL)
    {
        return NULL;
    }

    mz_ulong dest_len = (mz_ulong)original_size;
    int status = mz_uncompress(decompressed, &dest_len, compressed_data, (mz_ulong)compressed_size);

    if (status != MZ_OK)
    {
        fprintf(stderr, "[ERROR] Decompression failed with status: %d\n", status);
        free(decompressed);
        return NULL;
    }

    decompressed[original_size] = '\0'; // Null-terminate for text data
    return decompressed;
}

/**
 * Helper macro to decompress an embedded asset
 * Usage: EMBEDDED_DECOMPRESS(font_perfect_dos, &out_ptr, &out_size)
 */
#define EMBEDDED_DECOMPRESS(name, out_ptr, out_size)                                                      \
    do                                                                                                    \
    {                                                                                                     \
        *(out_ptr) = embedded_decompress_zlib(name##_data, name##_compressed_size, name##_original_size); \
        *(out_size) = (*(out_ptr) != NULL) ? name##_original_size : 0;                                    \
    } while (0)

#endif // EMBED_ASSETS

#endif // EMBEDDED_ASSETS_H
