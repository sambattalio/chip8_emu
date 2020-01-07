// Main.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "proc.h"
#include "helpers.h"

const char* USAGE = "./emu ROM";

extern uint8_t keys[];

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("%s\n", USAGE);
        exit(1);
    }

    // seed random number gen
    srand(time(0));

    proc *p = proc_create();

    proc_load_rom(p, argv[1]);

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Error: SDL having issues starting.\n");
        exit(1);
    }

    // dzone tutorial on sdl yeet
    SDL_Window* screen;
    SDL_Renderer* renderer; // i hardly know her
    SDL_CreateWindowAndRenderer(512, 256, 0, &screen, &renderer);
    SDL_SetWindowTitle(screen, "Chad-8: Chip-8 Emulator");
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SCREEN_W, SCREEN_H);

    if (!screen) {
        printf("Error: SDL couldn't create window!\n");
        exit(1);
    }

    SDL_Event event;

    int64_t currFrame, lastFrame;
    lastFrame = currentTimeMillis();
    struct timespec to_sleep;

    while (1) {
        /* Regulate FPS */
        currFrame = currentTimeMillis();
        int64_t delta = currFrame - lastFrame;
        lastFrame = currFrame;
        if (delta < 1000/FPS) {
            to_sleep.tv_sec = (1000/FPS - delta) / 1000;
            to_sleep.tv_nsec = ((1000/FPS - delta) % 1000) * 1000 * 1000;
            nanosleep(&to_sleep, NULL);
        }

        /* Handle quit or keypress */
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) break;

        if (event.type == SDL_KEYDOWN) {
            uint8_t hex = sdl_to_hex(event.key.keysym.sym);
            if (hex <= 0xF) keys[hex] = 1;
        } else if (event.type == SDL_KEYUP) {
            uint8_t hex = sdl_to_hex(event.key.keysym.sym);
            if (hex <= 0xF) keys[hex] = 0;
        }

        /* Emulator work */
        proc_cycle(p);
        
        if (p->dirty_screen) {
            proc_render(p, renderer, texture);
            p->dirty_screen = !p->dirty_screen;
        }
    }

    proc_delete(p);
    SDL_DestroyWindow(screen);
    SDL_Quit();

    return 0;
}