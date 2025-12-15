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
#include <stdio.h>

typedef enum ImageFileType
{
    IMAGE_FILE_PBM = 4, // Portable Bitmap (Black & White)
    IMAGE_FILE_PGM = 5, // Portable Graymap (Grayscale)
    IMAGE_FILE_PPM = 6, // Portable Pixmap (Color)
    IMAGE_FILE_UNKNOWN = -1,
} ImageFileType;

/**
 * formats for exporting raw image data to arrays.
 */
typedef enum
{
    ARRAY_DATA_FORMAT_RGBA32,     // 4 bytes per pixel (uint32_t)
    ARRAY_DATA_FORMAT_RGB24,      // 3 bytes per pixel (R, G, B)
    ARRAY_DATA_FORMAT_GRAYSCALE8, // 1 byte per pixel
    ARRAY_DATA_FORMAT_BINARY1     // 1 bit per pixel (packed)
} ArrayDataFormat;

typedef struct
{
    // the pixel data is stored as a 32-bit integer 0xAARRGGBB
    uint32_t *data; // ARGB pixel data
    int width, height;

    uint8_t refcount;
} Layer;

typedef struct
{
    Layer **layers; // Dynamic array of pointers to Layers
    int num_layers;

    int width, height;

    int layer_capacity;
} Image;

/* =========================================================================
 * MEMORY MANAGEMENT
 * ========================================================================= */

/**
 * @brief Increments the reference count of a specific layer.
 * * This ensures the layer is not freed while it is still being used
 * by another part of the system (e.g., added to an image).
 * * @param layer A pointer to the Layer struct to retain.
 */
void retain_layer(Layer *layer);

/**
 * @brief Decrements the reference count of a layer and frees it if the count reaches zero.
 * * If the refcount drops to 0 or less, the function frees the pixel data
 * and the Layer struct itself.
 * * @param layer A pointer to the Layer struct to release.
 */
void release_layer(Layer *layer);

/* =========================================================================
 * CREATION & INITIALIZATION
 * ========================================================================= */

/**
 * @brief Allocates and initializes a new Image structure.
 * * Allocates memory for the image struct and initializes the dynamic layer array.
 * * @param width The width of the image in pixels.
 * @param height The height of the image in pixels.
 * @return A pointer to the new Image, or NULL on allocation failure.
 */
Image *create_image(int width, int height);

/**
 * @brief Allocates and initializes a new Layer.
 * * The pixel data is initialized to transparent black (0).
 * The reference count is initialized to 1.
 * * @param width The width of the layer in pixels.
 * @param height The height of the layer in pixels.
 * @return A pointer to the new Layer, or NULL on allocation failure.
 */
Layer *create_layer(int width, int height);

/* =========================================================================
 * LAYER MANAGEMENT
 * ========================================================================= */

/**
 * @brief Attaches an existing layer to an image.
 * * Verifies dimensions match. If the image's layer array is full, it dynamically
 * resizes. Increments the layer's reference count.
 * * @param img The target image.
 * @param layer The layer to attach.
 * @return 0 on success, 1 on failure (dimension mismatch or memory error).
 */
int add_existing_layer(Image *img, Layer *layer);

/**
 * @brief Creates a new layer and immediately adds it to the image.
 * * @param img The target image.
 * @return A pointer to the newly created Layer, or NULL on failure.
 */
Layer *add_layer(Image *img);

/**
 * @brief Removes a layer from the image at the specified index.
 * * Releases the layer (decrementing refcount) and shifts subsequent layers.
 * * @param img The target image.
 * @param index The index of the layer to remove (0-based).
 */
void remove_layer(Image *img, int index);

/* =========================================================================
 * CLEANUP & UTILITIES
 * ========================================================================= */

/**
 * @brief Destroys the image and releases all associated layers.
 * * Calls release_layer on all attached layers and frees the Image struct.
 * * @param img The image to free.
 */
void free_image(Image *img);

/**
 * @brief Prints debug information about the image to stdout.
 * * Displays dimensions, layer count, and refcounts for individual layers.
 * * @param img The image to inspect.
 */
void print_image_info(const Image *img);

/* =========================================================================
 * PIXEL OPERATIONS
 * ========================================================================= */

/**
 * @brief Performs alpha blending between two pixels.
 * * Formula: Result = FG * Alpha + BG * (1 - Alpha).
 * * @param bg_color The background pixel (ARGB).
 * @param fg_color The foreground pixel (ARGB).
 * @return The blended pixel color.
 */
uint32_t blend_pixels(uint32_t bg_color, uint32_t fg_color);

/* =========================================================================
 * FILE I/O & EXPORT
 * ========================================================================= */

/**
 * @brief Saves the image to a file in the specified format.
 * * Opens the file in binary write mode and flattens the layers before saving.
 * * @param img The image to save.
 * @param filename The output file path.
 * @param type The format to save as (PPM, PGM, or PBM).
 * @return 0 on success, 1 on failure.
 */
int save_image(const Image *img, const char *filename, ImageFileType type);

/**
 * @brief Flattens and exports a multi-layered image into a single raw data array.
 *
 * This function iterates through the given image, blends all layers for every pixel
 * starting from the background color, and converts the resulting color into the
 * specified target format. It dynamically allocates memory for the output array.
 *
 * @param[in]  img        Pointer to the source Image structure.
 * @param[out] out_array  Pointer to a void pointer. The function will allocate memory
 * and point *out_array to it.
 * @param[out] len        Pointer to an integer where the total size of the allocated
 * array (in bytes or elements depending on usage) will be stored.
 * @param[in]  format     The desired ImageDataFormat (RGBA32, RGB24, GRAYSCALE8, BINARY1).
 *
 * @return int Returns 0 on success, or 1 on failure.
 * Failure conditions include:
 * - The input 'img' is NULL.
 * - Memory allocation for 'out_array' failed.
 * - The out_array datatype depens on the selected format:
 *  - RGBA32: uint32_t array of size width * height
 *  - RGB24: uint8_t array of size width * height * 3
 *  - GRAYSCALE8: uint8_t array of size width * height
 *  - BINARY1: uint8_t array of size ceil((width * height) / 8)
 *
 * @note Memory Management:
 * This function calls malloc(). The caller is responsible for freeing
 * the memory pointed to by *out_array using free().
 *
 * @note Conversion Details:
 * - GRAYSCALE8: Uses standard luminance weights (0.299R + 0.587G + 0.114B).
 * - BINARY1: Converts to grayscale first. Pixels with luminance < 128
 * are set to 1 (bit set), others are 0. Bits are packed MSB-first.
 */
int export_to_array(const Image *img, void **out_array, size_t *len, ArrayDataFormat format);
