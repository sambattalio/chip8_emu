#include "helpers.h"

int64_t currentTimeMillis() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (int64_t)(time.tv_sec) * 1000 + (time.tv_usec / 1000);
}

uint8_t sdl_to_hex(SDL_Keycode key) {
    switch (key) {
        case SDLK_0:
            return 0x0;
        case SDLK_1:
            return 0x1;
        case SDLK_2:
            return 0x2;
        case SDLK_3:
            return 0x3;
        case SDLK_4:
            return 0x4;
        case SDLK_5:
            return 0x5;
        case SDLK_6:
            return 0x6;
        case SDLK_7:
            return 0x7;
        case SDLK_8:
            return 0x8;
        case SDLK_9:
            return 0x9;
        case SDLK_a:
            return 0xA;
        case SDLK_b:
            return 0xB;
        case SDLK_c:
            return 0xC;
        case SDLK_d:
            return 0xD;
        case SDLK_e:
            return 0xE;
        case SDLK_f:
            return 0xF;
        default:
            return 0x1F;
    }
}