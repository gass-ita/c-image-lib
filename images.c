#include "images.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

// Refcounting functions for Layer
void retain_layer(Layer *layer)
{
    if (layer)
    {
        layer->refcount++;
    }
}

void release_layer(Layer *layer)
{
    if (layer)
    {
        layer->refcount--;

        if (layer->refcount <= 0)
        {
            printf("Freeing layer memory\n");
            free(layer->data);
            free(layer);
        }
    }
}

Image *create_image(int width, int height)
{
    Image *img = (Image *)malloc(sizeof(Image));

    if (!img)
    {
        goto crate_image_err_alloc;
    }

    img->width = width;
    img->height = height;
    img->num_layers = 0;
    img->layer_capacity = IMAGE_INITIAL_LAYER_CAPACITY;

    img->layers = (Layer **)malloc(img->layer_capacity * sizeof(Layer *));
    if (!img->layers)
    {
        goto crate_image_err_alloc;
    }
    return img;

crate_image_err_alloc:
    fprintf(stderr, "Error: Unable to allocate memory for image\n");
    free(img->layers);
    free(img);
    return NULL;
}

/*
This function creates a new layer with the specified width and height.
It allocates memory for the Layer structure and its pixel data.
Its refcount is initialized to 1. (if used as a temporary layer, it should be released after use)
*/
Layer *create_layer(int width, int height)
{
    Layer *layer = (Layer *)malloc(sizeof(Layer));
    if (!layer)
        goto crate_image_layer_alloc;

    layer->width = width;
    layer->height = height;
    layer->data = (uint32_t *)malloc(width * height * sizeof(uint32_t));
    memset(layer->data, 0, width * height * sizeof(uint32_t)); // initialize to transparent black

    if (!layer->data)
        goto crate_image_layer_alloc;

    layer->refcount = 1; // initial refcount
    return layer;

crate_image_layer_alloc:
    free(layer->data);
    free(layer);
    return NULL;
}

int add_existing_layer(Image *img, Layer *layer)
{
    if (!img || !layer)
        return 1;

    // check if layer dimensions match image dimensions
    if (layer->width != img->width || layer->height != img->height)
    {
        fprintf(stderr, "Error: Layer dimensions do not match image dimensions\n");
        return 1;
    }

    // dynamically grow layer array if needed
    if (img->num_layers >= img->layer_capacity)
    {
        img->layer_capacity *= IMAGE_LAYER_GROWTH_FACTOR;
        img->layers = (Layer **)realloc(img->layers, img->layer_capacity * sizeof(Layer *));

        if (!img->layers)
        {
            // TODO: fall back to previous capacity?
            fprintf(stderr, "Error: Unable to allocate memory for layers\n");
            return 1;
        }
    }

    // finally add the layer
    img->layers[img->num_layers++] = layer;
    retain_layer(layer);
    return 0;
}

Layer *add_layer(Image *img)
{
    if (!img)
        return NULL;

    Layer *new_layer = create_layer(img->width, img->height);
    new_layer->refcount = 0; // the refcount will be incremented in add_existing_layer

    if (!new_layer)
    {
        fprintf(stderr, "Error: Unable to allocate memory for layer data\n");
        return NULL;
    }

    if (add_existing_layer(img, new_layer) != 0)
    {
        release_layer(new_layer);
        return NULL;
    }

    return new_layer;
}

void remove_layer(Image *img, int index)
{
    if (index < 0 || index >= img->num_layers)
    {
        fprintf(stderr, "Error: Invalid layer index %d\n", index);
        return;
    }

    release_layer(img->layers[index]);

    for (int i = index; i < img->num_layers - 1; i++)
    {
        img->layers[i] = img->layers[i + 1];
    }
    img->num_layers--;
}

void free_image(Image *img)
{
    for (int i = 0; i < img->num_layers; i++)
    {
        release_layer(img->layers[i]);
    }
    free(img->layers);
    free(img);
}

void print_image_info(const Image *img)
{
    if (!img)
        return;

    printf("Image: %dx%d, Layers: %d\n", img->width, img->height, img->num_layers);
    for (int i = 0; i < img->num_layers; i++)
    {
        Layer *layer = img->layers[i];
        printf("  Layer %d: %dx%d, Refcount: %d\n", i, layer->width, layer->height, layer->refcount);
    }
}

// Alpha Blending: Result = FG * Alpha + BG * (1 - Alpha)
uint32_t blend_pixels(uint32_t bg_color, uint32_t fg_color)
{
    unsigned int alpha = GET_A(fg_color);

    // Optimization: If completely transparent, return background
    if (alpha == 0)
        return bg_color;

    // Optimization: If completely opaque, return foreground
    if (alpha == 255)
        return fg_color;

    unsigned int inv_alpha = 255 - alpha;

    unsigned int r = (GET_R(fg_color) * alpha + GET_R(bg_color) * inv_alpha) / 255;
    unsigned int g = (GET_G(fg_color) * alpha + GET_G(bg_color) * inv_alpha) / 255;
    unsigned int b = (GET_B(fg_color) * alpha + GET_B(bg_color) * inv_alpha) / 255;

    // Output is always opaque (Alpha = 255) because we flattened it
    return COLOR(255, r, g, b);
}

int save_image_ppm(FILE *fp, const Image *img)
{
    // 1. Write PPM Header (P6 = Binary RGB, width, height, max_val)
    fprintf(fp, "P6\n%d %d\n255\n", img->width, img->height);

    // Buffer to hold the RGB data for one row (3 bytes per pixel)
    size_t row_size = img->width * 3;
    unsigned char *row_buffer = (unsigned char *)malloc(row_size);

    if (!row_buffer)
    {
        fprintf(stderr, "Error: Memory allocation failed for row buffer.\n");
        return 1;
    }

    // Loop through every row (y)
    for (int y = 0; y < img->height; y++)
    {
        // 2. Loop through every pixel in the current row (x)
        for (int x = 0; x < img->width; x++)
        {
            uint32_t final_color = BACKGROUND_COLOR; // Start with Black

            // Calculate the 1D index
            size_t pixel_index = y * img->width + x;

            // Blend all layers for the current pixel
            for (int i = 0; i < img->num_layers; i++)
            {
                Layer *layer = img->layers[i];
                uint32_t layer_pixel = layer->data[pixel_index];
                final_color = blend_pixels(final_color, layer_pixel);
            }

            // Write the R, G, B bytes into the buffer (PPM P6 format is R, G, B triplets)
            size_t buf_index = x * 3;
            row_buffer[buf_index + 0] = (unsigned char)GET_R(final_color);
            row_buffer[buf_index + 1] = (unsigned char)GET_G(final_color);
            row_buffer[buf_index + 2] = (unsigned char)GET_B(final_color);
        }

        // 3. Write the entire row buffer to the file
        if (fwrite(row_buffer, 1, row_size, fp) != row_size)
        {
            fprintf(stderr, "Error: Failed to write full row to file.\n");
            free(row_buffer);
            return 1;
        }
    }

    free(row_buffer);
    return 0;
}

int save_image_pgm(FILE *fp, const Image *img)
{
    // 1. Write PPM Header (P5 = Binary Grayscale, width, height, max_val)
    fprintf(fp, "P5\n%d %d\n255\n", img->width, img->height);

    // Buffer to hold the grayscale data for one row (1 byte per pixel)
    size_t row_size = img->width;
    unsigned char *row_buffer = (unsigned char *)malloc(row_size);

    if (!row_buffer)
    {
        fprintf(stderr, "Error: Memory allocation failed for row buffer.\n");
        return 1;
    }

    // Loop through every row (y)
    for (int y = 0; y < img->height; y++)
    {
        // 2. Loop through every pixel in the current row (x)
        for (int x = 0; x < img->width; x++)
        {
            uint32_t final_color = BACKGROUND_COLOR; // Start with Black

            // Calculate the 1D index
            size_t pixel_index = y * img->width + x;

            // Blend all layers for the current pixel
            for (int i = 0; i < img->num_layers; i++)
            {
                Layer *layer = img->layers[i];
                uint32_t layer_pixel = layer->data[pixel_index];
                final_color = blend_pixels(final_color, layer_pixel);
            }

            // Write the grayscale byte into the buffer
            size_t buf_index = x * 1;
            // Convert to grayscale
            unsigned char gray = (unsigned char)(0.299 * GET_R(final_color) +
                                                 0.587 * GET_G(final_color) +
                                                 0.114 * GET_B(final_color));
            row_buffer[buf_index] = gray;
        }

        // 3. Write the entire row buffer to the file
        if (fwrite(row_buffer, 1, row_size, fp) != row_size)
        {
            fprintf(stderr, "Error: Failed to write full row to file.\n");
            free(row_buffer);
            return 1;
        }
    }

    free(row_buffer);
    return 0;
}

int save_image_pbm(FILE *fp, const Image *img)
{
    // 1. Write PBM Header (P4 = Binary Bitmap, width, height)
    // NOTE: PBM does NOT have a "Max Value" line like PGM/PPM.
    fprintf(fp, "P4\n%d %d\n", img->width, img->height);

    // Calculate bytes per row: ceil(width / 8)
    // Example: Width 10 -> needs 2 bytes (8 bits + 2 bits)
    size_t row_size = (img->width + 7) / 8;

    // Allocate buffer for one packed row
    unsigned char *row_buffer = (unsigned char *)calloc(row_size, 1); // Use calloc to ensure padding bits are 0

    if (!row_buffer)
    {
        fprintf(stderr, "Error: Memory allocation failed for row buffer.\n");
        return 1;
    }

    // Loop through every row (y)
    for (int y = 0; y < img->height; y++)
    {
        // Reset buffer to 0 for the new row (crucial because we use bitwise OR)
        memset(row_buffer, 0, row_size);

        // 2. Loop through every pixel in the current row (x)
        for (int x = 0; x < img->width; x++)
        {
            uint32_t final_color = BACKGROUND_COLOR;

            // Calculate the 1D index
            size_t pixel_index = y * img->width + x;

            // Blend all layers
            for (int i = 0; i < img->num_layers; i++)
            {
                Layer *layer = img->layers[i];
                uint32_t layer_pixel = layer->data[pixel_index];
                final_color = blend_pixels(final_color, layer_pixel);
            }

            // Convert to grayscale intensity
            unsigned char gray = (unsigned char)(0.299 * GET_R(final_color) +
                                                 0.587 * GET_G(final_color) +
                                                 0.114 * GET_B(final_color));

            // 3. Convert to Black & White (Thresholding)
            // PBM Standard: 1 = Black, 0 = White.
            // Logic: If intensity is low (dark), it's a 1. If high (bright), it's a 0.
            if (gray < 128)
            {
                // We have a black pixel, so we need to set the bit to 1.

                // Which byte is this pixel in?
                size_t byte_index = x / 8;

                // Which bit inside that byte? (MSB is first, so 7 - remainder)
                int bit_index = 7 - (x % 8);

                // Set the bit
                row_buffer[byte_index] |= (1 << bit_index);
            }
        }

        // 4. Write the packed row to the file
        if (fwrite(row_buffer, 1, row_size, fp) != row_size)
        {
            fprintf(stderr, "Error: Failed to write full row to file.\n");
            free(row_buffer);
            return 1;
        }
    }

    free(row_buffer);
    return 0;
}

int save_image(const Image *img, const char *filename, ImageFileType type)
{
    if (!img || !filename)
        return 1;

    // Use binary mode "wb" for writing the binary data
    FILE *f = fopen(filename, "wb");
    if (!f)
    {
        fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
        return 1;
    }

    switch (type)
    {
    case IMAGE_FILE_PPM:
        if (save_image_ppm(f, img) != 0)
            goto image_save_err;
        break;
    case IMAGE_FILE_PGM:
        if (save_image_pgm(f, img) != 0)
            goto image_save_err;
        break;
    case IMAGE_FILE_PBM:
        if (save_image_pbm(f, img) != 0)
            goto image_save_err;

    default:
        break;
    }

    fclose(f);
    return 0;

image_save_err:
    fclose(f);
    return 1;
}

int export_to_array(const Image *img, void **out_array, size_t *len, ArrayDataFormat format)
{
    if (!img)
        return 1;

    switch (format)
    {
    case ARRAY_DATA_FORMAT_RGBA32:
        *len = img->width * img->height;
        *out_array = malloc(*len * sizeof(uint32_t));
        break;
    case ARRAY_DATA_FORMAT_RGB24:
        *len = img->width * img->height * 3;
        printf("Calculated array length for RGB24: %d\n", *len);
        *out_array = malloc(*len * sizeof(uint8_t));
        break;
    case ARRAY_DATA_FORMAT_GRAYSCALE8:
        *len = img->width * img->height;
        *out_array = malloc(*len * sizeof(uint8_t));
        break;
    case ARRAY_DATA_FORMAT_BINARY1:
        *len = (img->width * img->height + 7) / 8;
        *out_array = malloc(*len * sizeof(uint8_t));
        *out_array = memset(*out_array, 0, *len); // initialize all bits to 0 (white)
        break;
    default:
        break;
    }

    if (!*out_array)
        return 1;

    for (int y = 0; y < img->height; y++)
    {
        for (int x = 0; x < img->width; x++)
        {
            uint32_t final_color = BACKGROUND_COLOR; // Start with Black

            // Calculate the 1D index
            size_t pixel_index = y * img->width + x;

            // Blend all layers for the current pixel
            for (int i = 0; i < img->num_layers; i++)
            {
                Layer *layer = img->layers[i];
                uint32_t layer_pixel = layer->data[pixel_index];
                final_color = blend_pixels(final_color, layer_pixel);
            }

            // Store in the requested format
            switch (format)
            {
            case ARRAY_DATA_FORMAT_RGBA32:
            {
                uint32_t *arr = (uint32_t *)(*out_array);
                arr[pixel_index] = final_color;
                break;
            }
            case ARRAY_DATA_FORMAT_RGB24:
            {
                uint8_t *arr = (uint8_t *)(*out_array);
                size_t base_index = pixel_index * 3;
                arr[base_index + 0] = (uint8_t)GET_R(final_color);
                arr[base_index + 1] = (uint8_t)GET_G(final_color);
                arr[base_index + 2] = (uint8_t)GET_B(final_color);
                break;
            }
            case ARRAY_DATA_FORMAT_GRAYSCALE8:
            {
                uint8_t *arr = (uint8_t *)(*out_array);
                arr[pixel_index] = (uint8_t)(0.299 * GET_R(final_color) +
                                             0.587 * GET_G(final_color) +
                                             0.114 * GET_B(final_color));
                break;
            }
            case ARRAY_DATA_FORMAT_BINARY1:
            {
                uint8_t *arr = (uint8_t *)(*out_array);
                size_t byte_index = pixel_index / 8;
                int bit_index = 7 - (pixel_index % 8);
                unsigned char gray = (unsigned char)(0.299 * GET_R(final_color) +
                                                     0.587 * GET_G(final_color) +
                                                     0.114 * GET_B(final_color));
                if (gray < 128)
                {
                    arr[byte_index] |= (1 << bit_index);
                }
                else
                {
                    arr[byte_index] &= ~(1 << bit_index);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    return 0;
}
