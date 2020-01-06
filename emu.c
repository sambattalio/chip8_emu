// Main.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SDL2/SDL.h"

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

// stack overflow thanks
#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

const char* USAGE = "./emu ROM";

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

/* Fontset to be loaded into memory */
uint8_t font[16][SPRITE_W] = {
    {0xF0, 0x90, 0x90, 0x90, 0xF0}, // 0
    {0x20, 0x60, 0x20, 0x20, 0x70}, // 1
    {0xF0 ,0x10, 0xF0, 0x80, 0xF0}, // 2
    {0xF0, 0x10, 0xF0, 0x10, 0xF0}, // 3
    {0x90, 0x90, 0xF0, 0x10, 0x10}, // 4
    {0xF0, 0x80, 0xF0, 0x10, 0xF0}, // 5
    {0xF0, 0x80, 0xF0, 0x90, 0xF0}, // 6
    {0xF0, 0x10, 0x20, 0x40, 0x40}, // 7
    {0xF0, 0x90, 0xF0, 0x90, 0xF0}, // 8
    {0xF0, 0x90, 0xF0, 0x10, 0xF0}, // 9
    {0xF0, 0x90, 0xF0, 0x90, 0x90}, // A
    {0xE0, 0x90, 0xE0, 0x90, 0xE0}, // B
    {0xF0, 0x80, 0x80, 0x80, 0xF0}, // C
    {0xE0, 0x90, 0x90, 0x90, 0xE0}, // D
    {0xF0, 0x80, 0xF0, 0x80, 0xF0}, // E
    {0xF0, 0x80, 0xF0, 0x80, 0x80}, // F
};

// maps 1 to 1 w/ hex of key
uint8_t keys[16] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

proc* proc_create();
void proc_delete(proc *p);
void proc_load_rom(proc *p, char* file_name);
void proc_cycle(proc *p);
void proc_render(proc *p, SDL_Renderer* renderer, SDL_Texture* texture);

void read_word(proc* p);

uint8_t sdl_to_hex(SDL_Keycode key);
int64_t currentTimeMillis(); // stacokoverflow bb

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

    SDL_DestroyWindow(screen);
    SDL_Quit();

    return 0;
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

/* Create a chip8 processor... (mainly just sets PC) */
proc* proc_create() {
    proc *p = calloc(1, sizeof(proc));
    if (!p) {
        printf("Error: Cannot allocate for processor struct!\n");
        exit(1);
    }
    p->pc = INIT_PC;
    p->sp = -1;

    /* Load fontset into interpreter memory */
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 5 /* num bytes per thing */; j++) {
            p->memory[i + j] = font[i][j];
        }
    }

    return p;
}

void proc_delete(proc *p) {
    if (!p) return;
    free(p);
}

void proc_load_rom(proc *p, char* file_name) {
    // Open ROM
    FILE *f = fopen(file_name, "rb");
    if (!f) {
        printf("Error opening file '%s'!\n", file_name);
        exit(1);
    }

    // Find file size
    fseek(f, 0L, SEEK_END);
    int size = ftell(f);
    // check if size too big
    /*if (size > (MAX_PC - INIT_PC)) {
        printf("Error: Program too big!\n");
        exit(1);
    }*/
    // Reset pointer
    fseek(f, 0L, SEEK_SET);

    // create char buffer for ROM data
    unsigned char *buff = malloc(size);
    fread(buff, size, 1, f);

    // load data into memory
    for (int i = 0; i < size; i++) {
        p->memory[INIT_PC + i] = buff[i];
    }

    free(buff);
}

void proc_cycle(proc *p) {
    // clock cycle action
    // todo.... figure out 60hz cycling
    read_word(p);
    // update timers
    if (p->delay_timer) p->delay_timer--;
    if (p->sound_timer) p->sound_timer--;
}

void proc_render(proc *p, SDL_Renderer* renderer, SDL_Texture *texture) {
    uint32_t pixels[SCREEN_W * SCREEN_H];
    memset(pixels,0, SCREEN_W * SCREEN_H * sizeof(uint32_t));
    for (int i = 0; i < SCREEN_H; i++) {
        for (int j = 0; j < SCREEN_W; j++) {
            pixels[i*SCREEN_W + j] = p->graphics[(i/(SCREEN_H/CHIP_H))*64 + j/(SCREEN_W/CHIP_W)] ? 0xFFFFFFFF : 0;
        }
    }
    
    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_W * sizeof(Uint32));

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int64_t currentTimeMillis() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (int64_t)(time.tv_sec) * 1000 + (time.tv_usec / 1000);
}

/*
    Read data word from location i in code and handle operation.
    instruction set sourced from: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
    MSB is first == big endian 
*/
void read_word(proc* p) {
    unsigned char *word = &p->memory[p->pc]; // get bytes @ the data words spot
    /* Helper variables from command */
    unsigned int cmd      = p->memory[p->pc] << 8 | p->memory[p->pc + 1]; //*word & 0xFFFF;
    //cmd = (cmd >> 8) | (cmd << 8); // thanks stack overflow
    unsigned int r_opcode = cmd & 0x000F;
    unsigned int l_opcode = (cmd & 0xF000) >> (3 * NIBBLE);
    unsigned int addr     = (cmd & 0x0FFF); // lowest 12 bits of instruction
    unsigned int x        = (cmd & 0x0F00) >> (2 * NIBBLE); // lower bit of high byte
    unsigned int y        = (cmd & 0x00F0) >> (NIBBLE); // high bit of lower byte
    unsigned int kk       = (cmd & 0x00FF); // lower byte
    // trust me... this should make simpler lower

    int bytes_ate = 2; // instructions are 2 bytes... but sometimes you want to skip a line

    switch (l_opcode) {
        case 0x0:
            /* Could be 0nnn, 00E0, or 00EE */
            if (kk == 0x0E) {
                /* CLS: clear display */
                debug_print("CLS\n", NULL);
                memset(p->graphics, 0, CHIP_W * CHIP_H);
            } else if (kk == 0xEE) {
                /* RET: return from subroutine */
                debug_print("RET\n", NULL);
                p->pc = p->stack[p->sp];
                p->sp -= 1;
                bytes_ate = 0;
            } else {
                /* SYS addr: jump to machine code routine @ nnn */
                /* should just be ignored */
                //printf("\n\n%04x: %04x\n\n", cmd, *word);
            }
			break;
        case 0x1:
            /* JP addr: jump to 12 bit addr nnn */
            debug_print("JP %x\n", addr);
            p->pc = addr;
            bytes_ate = 0;
			break;
        case 0x2:
            /* CALL addr: call subroutine @ nnn */
            debug_print("CALL %x\n", addr);
            p->sp += 1;
            if (p->sp >= STACK_SIZE) {
                printf("Error: Too many nested function calls!\n");
                exit(1);
            }
            p->stack[p->sp] = p->pc + 2; // next instrution
            p->pc = addr; 
            bytes_ate = 0;
			break;
        case 0x3:
            /* SE Vx, Byte: skip next inst if Vx == kk */
            debug_print("SE V%d, %02x\n", x, kk);
            if (p->registers[x] == kk) {
                bytes_ate = 4;
            }
			break;
        case 0x4:
            /* SNE Vx, byte: Skip next inst if Vx != kk */
            debug_print("SNE V%d, %02x\n", x, kk);
            if (p->registers[x] != kk) {
                bytes_ate = 4;
            }
			break;
        case 0x5:
            /* SE Vx, Vy: Skip next inst if Vx = Vy */
            debug_print("SE V%d, V%d\n", x, y);
            if (p->registers[x] == p->registers[y]) {
                bytes_ate = 4;
            }
			break;
        case 0x6:
            /* LD Vx, byte: set Vx = byte */
            debug_print("LD V%d, %02x\n", x, kk);
            p->registers[x] = kk;
			break;
        case 0x7:
            /* ADD Vx, byte: set Vx = Vx + byte */
            debug_print("ADD V%d, %02x\n", x, kk);
            p->registers[x] = p->registers[x] + kk;
			break;
        case 0x8:
            switch (r_opcode) {
                case 0x0:
                    /* LD Vx, Vy: store Vy -> Vx */
                    debug_print("LD V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[y];
                    break;
                case 0x1:
                    /* OR Vx, Vy: set Vx = Vx OR Vy */
                    debug_print("OR V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] | p->registers[y]; 
                    break;
                case 0x2:
                    /* AND Vx, Vy: set Vx = Vx AND Vy */
                    debug_print("AND V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] & p->registers[y];
                    break;
                case 0x3:
                    /* XOR Vx, Vy: set Vx = Vx XOR Vy */
                    debug_print("XOR V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] ^ p->registers[y];
                    break;
                case 0x4:
                    /* ADD Vx, Vy: set Vx = Vx + Vy... setting VF = Carry */
                    debug_print("ADD V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] + p->registers[y];
                    if (((int) p->registers[x] + (int) p->registers[y]) > 255) {
                        p->registers[0xF] = 1;
                    } else {
                       p->registers[0xF] = 0; 
                    }
                    break;
                case 0x5:
                    /* SUB Vx, Vy: set Vx = Vx - Vy, set Vf = (Vx > Vy) */
                    debug_print("SUB V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] - p->registers[y];
                    p->registers[0xF] = (p->registers[x] > p->registers[y]);
                    break;
                case 0x6:
                    /* SHR Vx, {, Vy}: set Vx = Vx SHR 1, Vf = (LSB Vx == 1) */
                    debug_print("SHR V%d, {, V%d}\n", x, y);
                    p->registers[0xF] = p->registers[x] & 1;
                    p->registers[x] = p->registers[x] >> 1;
                    break;
                case 0x7:
                    /* SUBN Vx, Vy: set Vx = Vy - Vx, set Fv = (Vy > Vx) */
                    debug_print("SUBN V%d, V%d\n", x, y);
                    p->registers[0xF] = p->registers[y] > p->registers[x];
                    p->registers[x] = p->registers[y] - p->registers[x];
                    break;
                case 0xE:
                    /* SHL Vx, {, Vy}: set Vx = Vx SHL 1, , Vf = (MSB Vx == 1)  */
                    debug_print("SHL V%d, {, V%d}\n", x, y);
                    p->registers[0xF] = p->registers[x] & 128; // Grab the MSB
                    p->registers[x] = p->registers[x] << 1;
                    break;
                default:
                    printf("err!\n");
            }
			break;
        case 0x9:
            /* SNE Vx, VyL skip next inst if Vx != Vy */
            debug_print("SNE V%d, V%d\n", x, y);
            if (p->registers[x] != p->registers[y]) {
                bytes_ate = 4;
            }
			break;
        case 0xA:
            /* LD I, addr: set I = nnn */
            debug_print("LD I, %03x\n", addr);
            p->I = addr;
			break;
        case 0xB:
            /* JP V0, addr: jump to location nnn + V0 */
            debug_print("JP V0, %03x\n", addr);
            p->pc = p->registers[0] + addr;
            bytes_ate = 0;
			break;
        case 0xC:
            /* RND Vx, byte: set Vx = random byte & kk */
            debug_print("RND V%d, %02x\n", x, kk);
            p->registers[x] = kk & (rand() % 255);
			break;
        case 0xD:
            /* DRW Vx, Vy, Nibble: Display n byte sprite starting at mem location I @ (Vx, Vy), set VF = collision */
            debug_print("DRW V%d, V%d, %01x: %04x\n", x, y, r_opcode, cmd);
            // default VF to 0 (no collision)
            p->registers[0xF] = 0;
            for (int i = 0; i < r_opcode; i++) { // r_opcode is height in this instruction
                // TODO maybe i is a bad variable, its more j in this case
                uint8_t pixel = p->memory[p->I + i];
                // now load in each individual bit of the byte (if that makes sense)
                for (int j = 0; j < 8; j++) {
                    // 1000 0000 == 0x80 ... shift right by j to find which pixel we want to mask and check
                    if (pixel & (0x80 >> j)) {
                        // check if already there (erased by xor)
                        if (p->graphics[(j + p->registers[x]) + (i + p->registers[y]) * CHIP_W]) {
                            p->registers[0xF] = 1;
                        }
                        // load into the graphics array
                        p->graphics[(j + p->registers[x]) + (i + p->registers[y]) * CHIP_W] ^= 1; // Xor as per spec
                    }
                }
            }

            p->dirty_screen = 1;
            break;
        case 0xE: {
            switch (kk) {
                case 0x9E: 
                    /* SKP Vx: skip inst if key with value of Vx is pressed */
                    debug_print("SKP V%d\n", x);
                    if (keys[p->registers[x]]) {
                        printf("pressed...\n");
                        bytes_ate = 4;
                    }
                    break;
                case 0xA1:
                    /* SKNP Vx: Skip next inst if key w/ value of Vx isn't pressed */
                    debug_print("SKNP V%d\n", x);
                    if (!keys[p->registers[x]]) {
                        bytes_ate = 4;
                    }
                    break;
                default:
                    printf("err: %04x\n", cmd);
            }
			break;
        }
        case 0xF:
            switch (kk) {
                case 0x07:
                    /* LD Vx, DT: set Vx = delay timer value */
                    debug_print("LD V%d, DT\n", x);
                    p->registers[x] = p->delay_timer;
                    break;
                case 0x0A:
                {
                    /* LD Vx, K: wait for a key press, store the value of the key in Vx */
                    debug_print("LD V%d, K\n", x);
                    SDL_Event event;
                    while (1) {
                        if(SDL_WaitEvent(&event)) {
                            if (event.type == SDL_KEYDOWN) {
                                // check if a valid key (0-F)
                                uint8_t hex = sdl_to_hex(event.key.keysym.sym);
                                if (hex <= 0xF) {
                                    p->registers[x] = hex;
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case 0x15:
                    /* LD DT, Vx: Set delay timer = Vx */
                    debug_print("LD DT, V%d\n", x);
                    p->delay_timer = p->registers[x];
                    break;
                case 0x18:
                    /* LD ST, Vx: Set sound timer = Vx */
                    debug_print("LD ST, V%d\n", x);
                    p->sound_timer = p->registers[x];
                    break;
                case 0x1E:
                    /* ADD I, Vx: Set I = I + Vx */
                    debug_print("LD I, V%d\n", x);
                    p->I = p->I + p->registers[x];
                    break;
                case 0x29:
                    /* LD F, Vx: set I = location of sprite for digit Vx */
                    debug_print("LD F, V%d\n", x);
                    p->I = p->registers[x] * SPRITE_W; // 5 is size of the sprite 
                    break;
                case 0x33:
                    /* LD B, Vx: Store BCD rep of Vx in memory locations I, I+1, I+2 */
                    debug_print("LD B, V%d\n", x);
                    p->memory[p->I] = p->registers[x] / 100; // 100s
                    p->memory[p->I + 1] = (p->registers[x] / 10) % 10; // 10s?
                    p->memory[p->I + 2] = p->registers[x] % 10; // 1s
                    break;
                case 0x55:
                    /* LD [I], Vx: Store registers V0 through Vx in memory starting at location I */
                    debug_print("LD [I], V%d\n", x);
                    for (int i = 0; i < x; i++) {
                        p->memory[p->I+i] = p->registers[i];
                    }
                    break;
                case 0x65:
                    /* LD Vx, [I]: Read registers V0 through Vx from memory starting @ location I */
                    debug_print("LD V%d, [I]\n", x);
                    for (int i = 0; i < x; i++) {
                        p->registers[i] = p->memory[p->I + i]; 
                    }
                    break;
                default:
                    printf("err: %04x\n", cmd);
            }
			break;
        default:
            printf("%04x\n", cmd);
    }

    p->pc += bytes_ate;
}
