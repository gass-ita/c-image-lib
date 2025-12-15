LibC-Image Library
==================

**LibC-Image** is a lightweight, pure C library for image manipulation. It utilizes a layered architecture (similar to image editing software), supporting alpha blending, geometric primitives, and Netpbm file I/O.

Features
--------

- **Layered Architecture:** Images are composed of multiple layers blended together.

- **Alpha Blending:** Automatic transparency handling when flattening layers.

- **Drawing Primitives:**

  - Lines (Bresenham's algorithm)

  - Rectangles (Filled & Outlined)

  - Circles (Filled & Outlined)

  - Ellipses (Filled & Outlined)

- **File I/O:**

  - **Read:** PPM (Color), PGM (Grayscale). *(PBM reading is currently experimental/unimplemented)*.

  - **Write:** PPM (P6), PGM (P5), PBM (P4).

- **Memory Management:** Reference counting system for efficient layer sharing.

Build Instructions
------------------

The project includes a `Makefile` that compiles the source into a static library (`libc-image-lib.a`).

1. **Build the library:**

    Bash

    ```
    make

    ```

    This will create a `build/` directory containing the object files and the static library `libc-image-lib.a`.

2. **Clean the build:**

    Bash

    ```
    make clean

    ```

3. Compiling your project:

    When compiling your own code against this library, ensure you link the math library (-lm).

    Bash

    ```
    gcc main.c build/libc-image-lib.a -o my_program -lm

    ```

Quick Start
-----------

### 1\. Basic Drawing

C

```
#include "images.h"
#include "images-primitives.h"

int main() {
    // 1. Create a 800x600 image
    Image *img = create_image(800, 600);

    // 2. Add a new layer (automatically retained by the image)
    Layer *bg = add_layer(img);

    // 3. Fill with Opaque Blue (A=255, R=0, G=0, B=255)
    fill_layer(bg, COLOR(255, 0, 0, 255));

    // 4. Add a second layer for drawing
    Layer *shapes = add_layer(img);

    // Draw a Red Circle
    draw_circle_filled(shapes, 400, 300, 50, COLOR(255, 255, 0, 0));

    // 5. Save the result
    save_image(img, "output.ppm", IMAGE_FILE_PPM);

    // 6. Cleanup (releases all layers automatically)
    free_image(img);
    return 0;
}

```

### 2\. Loading and Compositing Images

**Important:** When parsing an image from a file, the returned layer has a reference count of **1**. If you add it to an image (which increments the count), you must release your local reference to avoid memory leaks.

C

```
#include "images.h"
#include "images-parser.h"

int main() {
    ImageFileType type;
    Image *canvas = create_image(800, 600);

    // 1. Load a layer from a file
    Layer *imported_layer = parse_image_file("sprite.ppm", &type);

    if (imported_layer) {
        // 2. Add to the canvas
        // (Image takes ownership: Refcount becomes 2)
        add_existing_layer(canvas, imported_layer);

        // 3. CRITICAL: Release local reference
        // (Refcount becomes 1, owned solely by 'canvas')
        release_layer(imported_layer);
    }

    save_image(canvas, "composition.ppm", IMAGE_FILE_PPM);
    free_image(canvas);
    return 0;
}

```

Memory Management
-----------------

This library uses **Reference Counting** for layers.

1. **Creation:** `create_layer` returns a Layer with `refcount = 1`.

2. **Adding to Image:** `add_existing_layer` increments the `refcount`.

3. **Removal:** `remove_layer` or `free_image` decrements the `refcount`.

4. **Destruction:** When `refcount` hits 0, the pixel data is freed.

**Rule of Thumb:** If you created the layer manually (via `create_layer` or `parse_image_file`) and added it to an image, you must call `release_layer` on your pointer. If you used `add_layer(img)`, the library handles the lifecycle for you.

File Format Support
-------------------

The library supports the **Netpbm** family of formats.

| **Format** | **Magic Number** | **Read Support** | **Write Support** | **Description** |
| --- | --- | --- | --- | --- |
| **PPM** | P6 | Yes | Yes | Full Color (RGB) |
| **PGM** | P5 | Yes | Yes | Grayscale |
| **PBM** | P4 | *Not Implemented* | Yes | 1-bit Black & White |
