#pragma once

#define COLOR(a, r, g, b) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define GET_A(c) (((c) >> 24) & 0xFF)
#define GET_R(c) (((c) >> 16) & 0xFF)
#define GET_G(c) (((c) >> 8) & 0xFF)
#define GET_B(c) ((c) & 0xFF)

#define IMAGE_INITIAL_LAYER_CAPACITY 4
#define IMAGE_LAYER_GROWTH_FACTOR 2
#define BACKGROUND_COLOR COLOR(255, 0, 0, 0) // opaque black

#include <stdint.h>

typedef enum ImageFileType
{
    IMAGE_FILE_PPM,
    IMAGE_FILE_PGM,
    IMAGE_FILE_PBM,
} ImageFileType;

typedef struct
{
    // the pixel data is stored as a 32-bit integer 0xAARRGGBB
    uint32_t *data;
    int width, height;

    uint8_t refcount;
} Layer;

typedef struct
{
    Layer **layers;
    int num_layers;

    int width, height;

    int layer_capacity;
} Image;

Image *create_image(int width, int height);
Layer *create_layer(int width, int height);
int add_existing_layer(Image *img, Layer *layer);
Layer *add_layer(Image *img);
void remove_layer(Image *img, int index);
void retain_layer(Layer *layer);
void release_layer(Layer *layer);
void free_image(Image *img);
void print_image_info(const Image *img);

/*
 * Flattens all layers and saves the result to a file.
 * Supports multiple image file types based on the ImageFileType enum.
 */
int save_image(const Image *img, const char *filename, ImageFileType type);
