#pragma once

#include "pebble.h"

#define CLIP_MIN(x, min) ((x) < (min) ? (min) : (x))
#define CLIP_MAX(x, max) ((x) > (max) ? (max) : (x))
#define CLIP(x, min, max) (CLIP_MIN(CLIP_MAX((x), (max)), (min)))

void floatToString(char *buffer, int bufferSize, double number);
