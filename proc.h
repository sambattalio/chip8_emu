#ifndef PROC_H
#define PROC_H

#include <stdio.h>
#include <stdlib.h>
#include "SDL2/SDL.h"
#include "helpers.h"

#define NIBBLE 4
#define INIT_PC 0x200
#define MAX_PC 0x600
#define STACK_SIZE 16
#define N_REGISTERS 16
#define SCREEN_W 640
#define SCREEN_H 320
#define CHIP_W 64
#define CHIP_H 32
#define SPRITE_W 5
#define FPS 120 // 60hz for timers so lets just do 60 fps

typedef struct proc {
    uint16_t I; // cool 16 bit register for some ops and storing addresses
    uint8_t registers[N_REGISTERS]; // Has 16 general purpose 8-bit registers. VF used as flag
    uint8_t delay_timer; // 
    uint8_t sound_timer; // Decremented at rate of 60Hz if not 0 (TODO WTF)
    uint16_t pc;
    uint8_t sp; // points at topmost level of stack
    uint16_t stack[STACK_SIZE]; // 16 levels of nested subroutines
    uint8_t memory[4096]; // 4KB of ram
    uint8_t graphics[CHIP_W * CHIP_H]; // 64x32 monochrome displai
    uint8_t dirty_screen; // dirty bit to see if you should update screen texture
} proc;

proc* proc_create();
void proc_delete(proc *p);
void proc_load_rom(proc *p, char* file_name);
void proc_cycle(proc *p);
void proc_render(proc *p, SDL_Renderer* renderer, SDL_Texture* texture);
void proc_read_word(proc *p);


#endif