#include "images-primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void draw_pixel_safe(Layer *layer, int x, int y, uint32_t color)
{
    if (x >= 0 && x < layer->width && y >= 0 && y < layer->height)
    {
        layer->data[y * layer->width + x] = color;
    }
}

void fill_layer(Layer *layer, uint32_t color)
{
    size_t total_pixels = (size_t)layer->width * (size_t)layer->height;
    for (size_t i = 0; i < total_pixels; i++)
    {
        layer->data[i] = color;
    }
}

void draw_rect_filled(Layer *layer, int x, int y, int w, int h, uint32_t color)
{
    // 1. Calculate clipping bounds
    int x_start = (x < 0) ? 0 : x;
    int y_start = (y < 0) ? 0 : y;
    int x_end = (x + w > layer->width) ? layer->width : x + w;
    int y_end = (y + h > layer->height) ? layer->height : y + h;

    // 2. Iterate and fill
    for (int cy = y_start; cy < y_end; cy++)
    {
        // Optimization: Calculate row offset once per row
        int row_offset = cy * layer->width;
        for (int cx = x_start; cx < x_end; cx++)
        {
            layer->data[row_offset + cx] = color;
        }
    }
}

void draw_rect_outline(Layer *layer, int x, int y, int w, int h, uint32_t color)
{
    // Top and Bottom
    for (int px = x; px < x + w; px++)
    {
        draw_pixel_safe(layer, px, y, color);         // Top
        draw_pixel_safe(layer, px, y + h - 1, color); // Bottom
    }
    // Left and Right (skip corners to avoid double drawing)
    for (int py = y + 1; py < y + h - 1; py++)
    {
        draw_pixel_safe(layer, x, py, color);         // Left
        draw_pixel_safe(layer, x + w - 1, py, color); // Right
    }
}
void draw_line(Layer *layer, int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1)
    {
        draw_pixel_safe(layer, x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_circle_outline(Layer *layer, int xc, int yc, int r, uint32_t color)
{
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (y >= x)
    {
        // Draw all 8 octants
        draw_pixel_safe(layer, xc + x, yc + y, color);
        draw_pixel_safe(layer, xc - x, yc + y, color);
        draw_pixel_safe(layer, xc + x, yc - y, color);
        draw_pixel_safe(layer, xc - x, yc - y, color);
        draw_pixel_safe(layer, xc + y, yc + x, color);
        draw_pixel_safe(layer, xc - y, yc + x, color);
        draw_pixel_safe(layer, xc + y, yc - x, color);
        draw_pixel_safe(layer, xc - y, yc - x, color);

        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }
    }
}

void draw_circle_filled(Layer *layer, int xc, int yc, int r, uint32_t color)
{
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (y >= x)
    {
        // Draw horizontal lines between octant points
        // Line between (xc - x, yc + y) and (xc + x, yc + y)
        draw_line(layer, xc - x, yc + y, xc + x, yc + y, color);

        // Line between (xc - x, yc - y) and (xc + x, yc - y)
        draw_line(layer, xc - x, yc - y, xc + x, yc - y, color);

        // Line between (xc - y, yc + x) and (xc + y, yc + x)
        draw_line(layer, xc - y, yc + x, xc + y, yc + x, color);

        // Line between (xc - y, yc - x) and (xc + y, yc - x)
        draw_line(layer, xc - y, yc - x, xc + y, yc - x, color);

        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }
    }
}

void draw_ellipse_outline(Layer *layer, int xc, int yc, int rx, int ry, uint32_t color)
{
    long long rx2 = (long long)rx * rx;
    long long ry2 = (long long)ry * ry;
    long long twoRx2 = 2 * rx2;
    long long twoRy2 = 2 * ry2;
    long long p;
    long long x = 0;
    long long y = ry;
    long long px = 0;
    long long py = twoRx2 * y;

    /* --- Region 1: Slope dx/dy < 1 (Top and Bottom flat parts) --- */
    // Initial decision parameter for Region 1
    p = (long long)(ry2 - (rx2 * ry) + (0.25 * rx2));

    while (px < py)
    {
        // Draw 4 quadrants
        draw_pixel_safe(layer, xc + x, yc + y, color);
        draw_pixel_safe(layer, xc - x, yc + y, color);
        draw_pixel_safe(layer, xc + x, yc - y, color);
        draw_pixel_safe(layer, xc - x, yc - y, color);

        x++;
        px += twoRy2;
        if (p < 0)
        {
            p += ry2 + px;
        }
        else
        {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }
    }

    /* --- Region 2: Slope dx/dy >= 1 (Side steep parts) --- */
    // Initial decision parameter for Region 2
    p = (long long)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);

    while (y >= 0)
    {
        // Draw 4 quadrants
        draw_pixel_safe(layer, xc + x, yc + y, color);
        draw_pixel_safe(layer, xc - x, yc + y, color);
        draw_pixel_safe(layer, xc + x, yc - y, color);
        draw_pixel_safe(layer, xc - x, yc - y, color);

        y--;
        py -= twoRx2;
        if (p > 0)
        {
            p += rx2 - py;
        }
        else
        {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }
    }
}

void draw_ellipse_filled(Layer *layer, int xc, int yc, int rx, int ry, uint32_t color)
{
    long long rx2 = (long long)rx * rx;
    long long ry2 = (long long)ry * ry;
    long long twoRx2 = 2 * rx2;
    long long twoRy2 = 2 * ry2;
    long long p;
    long long x = 0;
    long long y = ry;
    long long px = 0;
    long long py = twoRx2 * y;

    /* --- Region 1 --- */
    p = (long long)(ry2 - (rx2 * ry) + (0.25 * rx2));

    while (px < py)
    {
        // Draw horizontal lines connecting the left and right sides
        // We do this for both upper and lower halves
        draw_line(layer, xc - x, yc + y, xc + x, yc + y, color);
        draw_line(layer, xc - x, yc - y, xc + x, yc - y, color);

        x++;
        px += twoRy2;
        if (p < 0)
        {
            p += ry2 + px;
        }
        else
        {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }
    }

    /* --- Region 2 --- */
    p = (long long)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);

    while (y >= 0)
    {
        draw_line(layer, xc - x, yc + y, xc + x, yc + y, color);
        draw_line(layer, xc - x, yc - y, xc + x, yc - y, color);

        y--;
        py -= twoRx2;
        if (p > 0)
        {
            p += rx2 - py;
        }
        else
        {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }
    }
}
