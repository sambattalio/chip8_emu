#ifndef HELPERS_H
#define HELPERS_H

#include "SDL2/SDL.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)


uint8_t sdl_to_hex(SDL_Keycode key);
int64_t currentTimeMillis(); // stacokoverflow bb

#endif