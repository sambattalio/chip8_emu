// Main.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NIBBLE 4
#define INIT_PC 0x200
#define MAX_PC 0x600
#define STACK_SIZE 16
#define N_REGISTERS 16

const char* USAGE = "./emu ROM";

typedef struct proc {
    uint16_t I; // cool 16 bit register for some ops and storing addresses
    uint8_t registers[N_REGISTERS]; // Has 16 general purpose 8-bit registers. VF used as flag
    uint8_t delay_timer; // 
    uint8_t sound_timer; // Decremented at rate of 60Hz if not 0 (TODO WTF)
    uint16_t pc;
    uint8_t sp; // points at topmost level of stack
    uint16_t stack[STACK_SIZE]; // 16 levels of nested subroutines
    uint8_t memory[4096];
} proc; 

proc* proc_create();
void proc_delete(proc *p);
void proc_load_rom(proc *p, char* file_name);
void proc_cycle(proc *p);

void read_word(proc* p);

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("%s\n", USAGE);
        exit(1);
    }

    // seed random number gen
    srand(time(0));

    proc *p = proc_create();

    proc_load_rom(p, argv[1]);

    while (1) {
        proc_cycle(p);
    }

    return 0;
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
    if (size > (MAX_PC - INIT_PC)) {
        printf("Error: Program too big!\n");
        exit(1);
    }
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
}

/*
    Read data word from location i in code and handle operation.
    instruction set sourced from: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
    MSB is first == big endian 
*/
void read_word(proc* p) {
    unsigned char *word = &p->memory[p->pc]; // get bytes @ the data words spot
    /* Helper variables from command */
    unsigned int cmd      = *word & 0xFFFF;
    cmd = (cmd >> 8) | (cmd << 8); // thanks stack overflow
    unsigned int r_opcode = cmd & 0x000F;
    unsigned int l_opcode = (cmd & 0xF000) >> (3 * NIBBLE);
    unsigned int addr     = (cmd & 0x0FFF); // lowest 12 bits of instruction
    unsigned int x        = (cmd & 0x0F00) >> (2 * NIBBLE); // lower bit of high byte
    unsigned int y        = (cmd & 0x00F0) >> (NIBBLE); // high bit of lower byte
    unsigned int kk       = (cmd & 0x00FF); // lower byte
    // trust me... this should make simpler lower

    int bytes_ate = 2; // instructions are 2 bytes... but sometimes you want to skip a line

    if (cmd == 0x00EE || cmd == 0xEE00) {
        printf("HERE\n\n");
    }


    switch (l_opcode) {
        case 0x0:
            /* Could be 0nnn, 00E0, or 00EE */
            if (kk == 0x0E) {
                /* CLS: clear display */
                printf("CLS\n");
            } else if (kk == 0xEE) {
                /* RET: return from subroutine */
                printf("RET\n");
                p->pc = p->stack[p->sp];
                p->sp -= 1;
            } else {
                /* SYS addr: jump to machine code routine @ nnn */
                /* should just be ignored */
                printf("\n\n%04x: %04x\n\n", cmd, *word);
            }
			break;
        case 0x1:
            /* JP addr: jump to 12 bit addr nnn */
            printf("JP %x\n", addr);
            p->pc = addr;
			break;
        case 0x2:
            /* CALL addr: call subroutine @ nnn */
            printf("CALL %x\n", addr);
            p->sp += 1;
            if (p->sp > STACK_SIZE) {
                printf("Error: Too many nested function calls!\n");
                exit(1);
            }
            p->stack[p->sp] = p->pc;
            p->pc = addr; 
			break;
        case 0x3:
            /* SE Vx, Byte: skip next inst if Vx == kk */
            printf("SE V%d, %02x\n", x, kk);
            if (p->registers[x] == kk) {
                bytes_ate = 4;
            }
			break;
        case 0x4:
            /* SNE Vx, byte: Skip next inst if Vx != kk */
            printf("SNE V%d, %02x\n", x, kk);
            if (p->registers[x] != kk) {
                bytes_ate = 4;
            }
			break;
        case 0x5:
            /* SE Vx, Vy: Skip next inst if Vx = Vy */
            printf("SE V%d, V%d\n", x, y);
            if (p->registers[x] == p->registers[y]) {
                bytes_ate = 4;
            }
			break;
        case 0x6:
            /* LD Vx, byte: set Vx = byte */
            printf("LD V%d, %02x\n", x, kk);
            p->registers[x] = kk;
			break;
        case 0x7:
            /* ADD Vx, byte: set Vx = Vx + byte */
            printf("ADD V%d, %02x\n", x, kk);
            p->registers[x] = p->registers[x] + kk;
			break;
        case 0x8:
            switch (r_opcode) {
                case 0x0:
                    /* LD Vx, Vy: store Vy -> Vx */
                    printf("LD V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[y];
                    break;
                case 0x1:
                    /* OR Vx, Vy: set Vx = Vx OR Vy */
                    printf("OR V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] | p->registers[y]; 
                    break;
                case 0x2:
                    /* AND Vx, Vy: set Vx = Vx AND Vy */
                    printf("AND V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] & p->registers[y];
                    break;
                case 0x3:
                    /* XOR Vx, Vy: set Vx = Vx XOR Vy */
                    printf("XOR V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] ^ p->registers[y];
                    break;
                case 0x4:
                    /* ADD Vx, Vy: set Vx = Vx + Vy... setting VF = Carry */
                    printf("ADD V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] + p->registers[y];
                    if (((int) p->registers[x] + (int) p->registers[y]) > 255) {
                        p->registers[0xF] = 1;
                    } else {
                       p->registers[0xF] = 0; 
                    }
                    break;
                case 0x5:
                    /* SUB Vx, Vy: set Vx = Vx - Vy, set Vf = (Vx > Vy) */
                    printf("SUB V%d, V%d\n", x, y);
                    p->registers[x] = p->registers[x] - p->registers[y];
                    p->registers[0xF] = (p->registers[x] > p->registers[y]);
                    break;
                case 0x6:
                    /* SHR Vx, {, Vy}: set Vx = Vx SHR 1, Vf = (LSB Vx == 1) */
                    printf("SHR V%d, {, V%d}\n", x, y);
                    p->registers[0xF] = p->registers[x] & 1;
                    p->registers[x] = p->registers[x] >> 1;
                    break;
                case 0x7:
                    /* SUBN Vx, Vy: set Vx = Vy - Vx, set Fv = (Vy > Vx) */
                    printf("SUBN V%d, V%d\n", x, y);
                    p->registers[0xF] = p->registers[y] > p->registers[x];
                    p->registers[x] = p->registers[y] - p->registers[x];
                    break;
                case 0xE:
                    /* SHL Vx, {, Vy}: set Vx = Vx SHL 1, , Vf = (MSB Vx == 1)  */
                    printf("SHL V%d, {, V%d}\n", x, y);
                    p->registers[0xF] = p->registers[x] & 128; // Grab the MSB
                    p->registers[x] = p->registers[x] << 1;
                    break;
                default:
                    printf("err!\n");
            }
			break;
        case 0x9:
            /* SNE Vx, VyL skip next inst if Vx != Vy */
            printf("SNE V%d, V%d\n", x, y);
            if (p->registers[x] != p->registers[y]) {
                bytes_ate = 4;
            }
			break;
        case 0xA:
            /* LD I, addr: set I = nnn */
            printf("LD I, %03x\n", addr);
            p->I = addr;
			break;
        case 0xB:
            /* JP V0, addr: jump to location nnn + V0 */
            printf("JP V0, %03x\n", addr);
            p->pc = p->registers[0] + addr;
			break;
        case 0xC:
            /* RND Vx, byte: set Vx = random byte & kk */
            printf("RND V%d, %02x\n", x, kk);
            p->registers[x] = kk & (rand() % 255);
			break;
        case 0xD:
            /* DRW Vx, Vy, Nibble: Display n byte sprite starting at mem location I @ (Vx, Vy), set VF = collision */
            printf("DRW V%d, V%d, %01x\n", x, y, r_opcode);
            break;
        case 0xE:
            switch (kk) {
                case 0x9E:
                    /* SKP Vx: skip inst if key with value of Vx is pressed */
                    printf("SKP V%d\n", x);
                    break;
                case 0xA1:
                    /* SKNP Vx: Skip next inst if key w/ value of Vx isn't pressed */
                    printf("SKNP V%d\n", x);
                    break;
                default:
                    printf("err!\n");
            }
			break;
        case 0xF:
            switch (kk) {
                case 0x07:
                    /* LD Vx, DT: set Vx = delay timer value */
                    printf("LD V%d, DT\n", x);
                    p->registers[x] = p->delay_timer;
                    break;
                case 0x0A:
                    /* LD Vx, K: wait for a key press, store the value of the key in Vx */
                    printf("LD V%d, K\n", x);
                    break;
                case 0x15:
                    /* LD DT, Vx: Set delay timer = Vx */
                    printf("LD DT, V%d\n", x);
                    p->delay_timer = p->registers[x];
                    break;
                case 0x18:
                    /* LD ST, Vx: Set sound timer = Vx */
                    printf("LD ST, V%d\n", x);
                    p->sound_timer = p->registers[x];
                    break;
                case 0x1E:
                    /* ADD I, Vx: Set I = I + Vx */
                    printf("LD I, V%d\n", x);
                    p->I = p->I + p->registers[x];
                    break;
                case 0x29:
                    /* LD F, Vx: set I = location of sprite for digit Vx */
                    printf("LD F, V%d\n", x);
                    break;
                case 0x33:
                    /* LD B, Vx: Store BCD rep of Vx in memory locations I, I+1, I+2 */
                    printf("LD B, V%d\n", x);
                    break;
                case 0x55:
                    /* LD [I], Vx: Store registers V0 through Vx in memory starting at location I */
                    printf("LD [I], V%d\n", x);
                    for (int i = 0; i < x; i++) {
                        p->memory[p->I+i] = p->registers[i];
                    }
                    break;
                case 0x65:
                    /* LD Vx, [I]: Read registers V0 through Vx from memory starting @ location I */
                    printf("LD V%d, [I]\n", x);
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