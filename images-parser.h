#pragma once
#include <stdint.h>
#include <stdio.h>
#include "images.h"

#define IMAGE_PORTABLE_COMMENT_CHAR '#'

Layer *parse_image_file(const char *filename, ImageFileType *out_type);