#include "images-parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include "images.h"

/**
 * Helper function: Consumes whitespace and comments from the stream.
 * In PBM/PGM/PPM, a comment starts with '#' and ends at the newline.
 * Comments are treated effectively as whitespace.
 */
static void consume_comments_and_whitespace(FILE *stream)
{
    int ch;
    // Loop continuously until we find a non-whitespace, non-comment character
    while ((ch = fgetc(stream)) != EOF)
    {

        // 1. Skip standard whitespace (spaces, tabs, newlines)
        if (isspace(ch))
        {
            continue;
        }

        // Handle comments
        if (ch == IMAGE_PORTABLE_COMMENT_CHAR)
        {
            // Consume characters until the end of the line
            while ((ch = fgetc(stream)) != EOF && ch != '\n')
                ;
            continue; // Loop back to check for more whitespace/comments after the newline
        }

        // push back valid data character for further processing
        ungetc(ch, stream);
        return;
    }
}

int scan_skip_comments(FILE *stream, const char *format, ...)
{
    // First, clean up the stream pointer
    consume_comments_and_whitespace(stream);

    // Initialize variable arguments
    va_list args;
    va_start(args, format);

    // Pass the cleaned stream to vfscanf
    int result = vfscanf(stream, format, args);

    va_end(args);
    return result;
}

/**
 * @brief Parses a PGM (Portable Graymap) file from the given file pointer.
 *
 * This function reads the PGM header, including width, height, and max pixel value,
 * then reads the binary pixel data. It converts the grayscale values into ARGB format
 * and stores them in a dynamically allocated array.
 * The function expects the file pointer to be positioned just after the magic number. ("P5")
 *
 * @param fp        Pointer to the opened PGM file (must be in binary mode).
 * @param length    Pointer to a size_t variable where the number of pixels will be stored.
 * @param data      Pointer to a uint32_t pointer where the allocated pixel data will be stored.
 * @param width     Pointer to a size_t variable where the image width will be stored.
 * @param height    Pointer to a size_t variable where the image height will be stored.
 * @return int Returns 0 on success, non-zero on failure.
 * @note The caller is responsible for freeing the allocated pixel data array.
 */
int parse_pgm_file(FILE *fp, size_t *length, uint32_t **data, size_t *width, size_t *height)
{
    size_t w, h, max_val;
    if (scan_skip_comments(fp, "%zu %zu", &w, &h) != 2)
    {
        fprintf(stderr, "Error: Invalid PGM header (width/height)\n");
        return 1;
    }
    if (scan_skip_comments(fp, "%zu", &max_val) != 1)
    {
        fprintf(stderr, "Error: Invalid PGM header (max value)\n");
        return 1;
    }
    size_t img_size = w * h;
    *length = img_size;
    *width = w;
    *height = h;

    unsigned char *pgm_data = (unsigned char *)malloc(img_size);
    *data = (uint32_t *)malloc(img_size * sizeof(uint32_t));

    consume_comments_and_whitespace(fp);
    // Read the binary pixel data
    fread(pgm_data, 1, img_size, fp);
    for (size_t i = 0; i < img_size; i++)
    {
        // printf("PGM Pixel %zu: %d\n", i, pgm_data[i]);
        uint8_t gray = pgm_data[i];
        (*data)[i] = COLOR(255, gray, gray, gray); // Convert grayscale to ARGB
    }
    free(pgm_data);
    return 0;
}

/**
 * @brief Parses a PPM (Portable Pixmap) file from the given file pointer.
 *
 * This function reads the PPM header, including width, height, and max pixel value,
 * then reads the binary pixel data. It converts the RGB values into ARGB format
 * and stores them in a dynamically allocated array.
 * The function expects the file pointer to be positioned just after the magic number. ("P6")
 *
 * @param fp        Pointer to the opened PPM file (must be in binary mode).
 * @param length    Pointer to a size_t variable where the number of pixels will be stored.
 * @param data      Pointer to a uint32_t pointer where the allocated pixel data will be stored.
 * @param width     Pointer to a size_t variable where the image width will be stored.
 * @param height    Pointer to a size_t variable where the image height will be stored.
 * @return int Returns 0 on success, non-zero on failure.
 * @note The caller is responsible for freeing the allocated pixel data array.
 */
int parse_ppm_file(FILE *fp, size_t *length, uint32_t **data, size_t *width, size_t *height)
{
    size_t w, h, max_val;
    if (scan_skip_comments(fp, "%zu %zu", &w, &h) != 2)
    {
        fprintf(stderr, "Error: Invalid PPM header (width/height)\n");
        return 1;
    }
    if (scan_skip_comments(fp, "%zu", &max_val) != 1)
    {
        fprintf(stderr, "Error: Invalid PPM header (max value)\n");
        return 1;
    }
    size_t img_size = w * h;
    *length = img_size;
    *width = w;
    *height = h;

    unsigned char *ppm_data = (unsigned char *)malloc(img_size * 3);
    *data = (uint32_t *)malloc(img_size * sizeof(uint32_t));

    consume_comments_and_whitespace(fp);
    // Read the binary pixel data
    fread(ppm_data, 1, img_size * 3, fp);
    for (size_t i = 0; i < img_size; i++)
    {
        uint8_t r = ppm_data[i * 3 + 0];
        uint8_t g = ppm_data[i * 3 + 1];
        uint8_t b = ppm_data[i * 3 + 2];
        (*data)[i] = COLOR(255, r, g, b); // Convert RGB to ARGB
    }
    free(ppm_data);
    return 0;
}

/**
 * @brief Parses an image file (Netpbm format) and creates a new Layer from its data.
 *
 * This function detects the file type (PBM, PGM, or PPM) based on the file's
 * magic number. It allocates a new Layer, populates it with the pixel data,
 * and sets the output file type.
 *
 * @param[in]  filename  The path to the file to be opened and parsed.
 * @param[out] out_type  Pointer to an ImageFileType enum. On success, this will
 * be updated with the detected type (e.g., IMAGE_FILE_PGM,
 * IMAGE_FILE_PPM).
 *
 * @return A pointer to the newly created Layer on success, or NULL if the file
 * could not be opened, parsed, or if memory allocation failed.
 *
 * @note **Memory Management & Reference Counting:**
 * The returned Layer is created with an initial reference count of **1**.
 * If you add this layer to an Image/Container (which typically increments
 * the reference count to 2 to retain ownership), you **must** call
 * `layer_release()` (or `layer_decref`) on the returned pointer immediately
 * after adding it. This ensures the reference count returns to 1 (owned
 * solely by the Image) and prevents memory leaks when the Image is eventually destroyed.
 *
 * @warning PBM (Magic number P4) parsing is currently not implemented.
 */
Layer *parse_image_file(const char *filename, ImageFileType *out_type)
{
    Layer *layer = NULL;
    if (!filename || !out_type)
    {
        fprintf(stderr, "Error: Invalid arguments to parse_image_file\n");
        return NULL;
    }

    *out_type = IMAGE_FILE_UNKNOWN;

    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "Error: Could not open file %s for reading\n", filename);
        return NULL;
    }

    int magic_number;
    scan_skip_comments(f, "P%d", &magic_number);

    size_t length;
    uint32_t *data;
    size_t width, height;

    switch (magic_number)
    {
    case IMAGE_FILE_PBM:
        *out_type = IMAGE_FILE_PBM;
        fprintf(stderr, "Error: PBM parsing not implemented yet\n");
        fclose(f);
        break;

    case IMAGE_FILE_PGM:
        *out_type = IMAGE_FILE_PGM;
        if (parse_pgm_file(f, &length, &data, &width, &height) != 0)
        {
            fprintf(stderr, "Error: Failed to parse PGM file data\n");
            fclose(f);
            return NULL;
        }
        layer = create_layer(width, height);
        if (!layer)
        {
            fprintf(stderr, "Error: Unable to allocate memory for layer data\n");
            fclose(f);
            return NULL;
        }

        memcpy(layer->data, data, length * sizeof(uint32_t));
        free(data);
        break;

    case IMAGE_FILE_PPM:
        *out_type = IMAGE_FILE_PPM;
        if (parse_ppm_file(f, &length, &data, &width, &height) != 0)
        {
            fprintf(stderr, "Error: Failed to parse PPM file data\n");
            fclose(f);
            return NULL;
        }
        layer = create_layer(width, height);
        if (!layer)
        {
            fprintf(stderr, "Error: Unable to allocate memory for layer data\n");
            fclose(f);
            return NULL;
        }

        memcpy(layer->data, data, length * sizeof(uint32_t));
        free(data);
        break;

    default:
        break;
    }
    fclose(f);
    return layer;
}