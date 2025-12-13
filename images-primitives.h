#pragma once
#include "images.h"

// --- Drawing Primitives ---

/*
 * Sets a single pixel at (x, y).
 * Safely ignores coordinates outside layer bounds.
 */
void draw_pixel_safe(Layer *layer, int x, int y, uint32_t color);

void fill_layer(Layer *layer, uint32_t color);

/*
 * Draws a filled rectangle.
 * Handles clipping automatically.
 */
void draw_rect_filled(Layer *layer, int x, int y, int w, int h, uint32_t color);

/*
 * Draws a 1px outline of a rectangle.
 */
void draw_rect_outline(Layer *layer, int x, int y, int w, int h, uint32_t color);

/*
 * Draws a line using Bresenham's algorithm.
 */
void draw_line(Layer *layer, int x0, int y0, int x1, int y1, uint32_t color);

/*
 * Draws a filled circle.
 */
void draw_circle_filled(Layer *layer, int xc, int yc, int r, uint32_t color);

/*
 * Draws a 1px outline of a circle.
 */
void draw_circle_outline(Layer *layer, int xc, int yc, int r, uint32_t color);

/*
 * Draws a 1px outline of an ellipse centered at (xc, yc).
 * rx: Radius in X direction (Width/2)
 * ry: Radius in Y direction (Height/2)
 */
void draw_ellipse_outline(Layer *layer, int xc, int yc, int rx, int ry, uint32_t color);

/*
 * Draws a filled ellipse.
 */
void draw_ellipse_filled(Layer *layer, int xc, int yc, int rx, int ry, uint32_t color);