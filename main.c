#include "images.h"
#include "images-primitives.h"
#include "images-parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    Image *img = create_image(100, 100);
    if (!img)
    {
        fprintf(stderr, "Failed to create image\n");
        return 1;
    }
    Layer *layer1 = add_layer(img);
    Layer *layer2 = add_layer(img);

    draw_circle_filled(layer1, 50, 50, 30, COLOR(255, 255, 0, 0));      // Red circle
    fill_layer(layer2, COLOR(128, 0, 0, 255));                          // Semi-transparent Blue circle
    draw_ellipse_filled(layer2, 50, 50, 20, 40, COLOR(128, 0, 255, 0)); // Semi-transparent Green ellipse
    if (save_image(img, "output.ppm", IMAGE_FILE_PPM) != 0)
    {
        fprintf(stderr, "Failed to save image\n");
        free_image(img);
        return 1;
    }
}
