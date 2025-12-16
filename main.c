#include "images.h"
#include "images-primitives.h"
#include "images-parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    ImageFileType file_type;
    Layer *layer = parse_image_file("sample_5184x3456.ppm", &file_type);
    if (!layer)
    {
        fprintf(stderr, "Failed to parse image file\n");
        return 1;
    }
    Image *img = create_image(layer->width, layer->height);
    add_existing_layer(img, layer);
    release_layer(layer);
    if (!img)
    {
        fprintf(stderr, "Failed to create image\n");
        return 1;
    }

    printf("Image created with dimensions: %dx%d\n", img->width, img->height);

    Layer *l = add_layer(img);
    if (!l)
    {
        fprintf(stderr, "Failed to add layer to image\n");
        return 1;
    }

    fill_layer(l, COLOR(100, 255, 255, 255));                                              // Fill the new layer with solid red color
    draw_circle_filled(l, img->width / 2, img->height / 2, 1000, COLOR(100, 255, 0, 255)); // Draw a filled circle in the center
    if (save_image(img, "output_image.ppm", file_type) != 0)
    {
        fprintf(stderr, "Failed to save image to file\n");
        return 1;
    }
    return 0;
}
