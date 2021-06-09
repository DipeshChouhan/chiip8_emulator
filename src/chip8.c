#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>
#include"raylib.h"

#define START_ADDRESS 0x200
#define WIDTH 64
#define HEIGHT 32
#define FONT_SIZE 80
#define FONT_START_ADDRESS 0x50
#define VX ((cp->opcode & 0x0F00u) >> 8u)
#define BYTE ((cp->opcode) & 0x00FFu)
#define VY ((cp->opcode & 0x00F0u) >> 4u)
#define OP ((cp->opcode & 0xF000u) >> 12u)

typedef struct chip8 chip8;
typedef void(*inst_fn)(chip8*);
struct chip8{

    uint8_t registers[16];
    uint8_t memory[4096];
    uint16_t index;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t dt;
    uint8_t st;
    uint8_t key_code;
    /* uint8_t keypad[16]; */
    uint32_t video[32][64];
    uint16_t opcode;
    /* inst_fn table[16]; */
};



void load_rom(const char* filename, chip8* cp){
    FILE *file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    unsigned int size = ftell(file);
    rewind(file);

    fread((cp->memory) + START_ADDRESS, sizeof(char), size, file);
    fclose(file);
}

void init_chip8(chip8* cp){
    uint8_t fonts[FONT_SIZE] = { 0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    for(int i = 0; i < 32; i++){
        for(int j = 0; j < 64; j++){
            cp->video[i][j] = 0;
        }
    }
    for(int i = 0; i < 4096; i++){
        if(i < 16){
            cp->registers[i] = 0;
            cp->stack[i] = 0;
        }
        if(i < 80)
            cp->memory[i] = fonts[i];
        else
            cp->memory[i] = 0;
    }
    cp->index = 0;
    cp->pc = START_ADDRESS;
    cp->key_code = 16;
    cp->dt = 0;
    cp->st = 0;
    cp->sp = 0;
}
void cycle(chip8* cp){
    cp->opcode = (cp->memory[cp->pc] << 8u) | cp->memory[cp->pc + 1];

    uint8_t vx = VX;
    uint8_t vy = VY;
    uint8_t byte = BYTE;
    cp->pc += 2;

    switch(OP){
        case 0x0:
            if(byte == 0xE0){

                // clear screen
                int i = 0, j = 0;
                for(;i < 32; ++i){
                    for(;j < 64; ++j){
                        cp->video[i][j] = 0;
                    }
                    j = 0;
                }
            }else if(byte == 0xEE){
                --cp->sp;
                cp->pc = cp->stack[cp->sp];

            }else{
                printf("error at 0x0\n");
            }
            break;
            
        case 0x1:
            cp->pc = cp->opcode & 0x0FFFu;
            break;
            
        case 0x2:
            cp->stack[cp->sp] = cp->pc;
            ++cp->sp;
            cp->pc = cp->opcode & 0x0FFFu;
            break;
            
        case 0x3:
            if(cp->registers[vx] == byte)
                cp->pc += 2;
            break;

        case 0x4:
            if(cp->registers[vx] != byte)
                cp->pc += 2;
            break;

        case 0x5:
            if(cp->registers[vx] == cp->registers[vy])
                cp->pc += 2;
            break;
            
        case 0x6:
            cp->registers[vx] = byte;
            break;

        case 0x7:
            cp->registers[vx] += byte;
            break;

        case 0x8:
            switch(byte & 0x0Fu){
                case 0x0:
                    cp->registers[vx] = cp->registers[vy];
                    break;
                case 0x1:
                    cp->registers[vx] |= cp->registers[vy];
                    break;
                case 0x2:
                    cp->registers[vx] &= cp->registers[vy];
                    break;
                case 0x3:
                    cp->registers[vx] ^= cp->registers[vy];
                    break;
                case 0x4:
                    {
                        uint16_t sum = cp->registers[vx] + cp->registers[vy];
                        cp->registers[0xF] = sum > 255u;
                        cp->registers[vx] = sum & 0xFFu;
                    }
                    break;
                case 0x5:
                    cp->registers[0xF] = cp->registers[vx] > cp->registers[vy];
                    cp->registers[vx] -= cp->registers[vy];
                    break;
                case 0x6:
                    cp->registers[0xF] = cp->registers[vx] & 0x1u;
                    cp->registers[vx] >>= 1;
                    break;
                case 0x7:
                    cp->registers[0xF] = cp->registers[vy] > cp->registers[vx];
                    cp->registers[vx] = cp->registers[vy] - cp->registers[vx];
                    break;
                case 0xE:
                    cp->registers[0xF] = (cp->registers[vx] & 0x80u) >> 7u;
                    cp->registers[vx] <<= 1;
                    break;
            }
            break;
            
        case 0x9:
            if(cp->registers[vx] != cp->registers[vy])
                cp->pc += 2;
            break;
            
        case 0xA:
            cp->index = cp->opcode & 0x0FFFu;
            break;

        case 0xB:
            cp->pc = cp->registers[0] + (cp->opcode & 0x0FFFu);
            break;

        case 0xC:
            cp->registers[vx] = GetRandomValue(0, 255) & byte;
            break;
        case 0xD:
            {
                uint8_t n = cp->opcode & 0x000Fu;
                uint8_t xpos = cp->registers[vx] % 64;
                uint8_t ypos = cp->registers[vy] % 32;

                cp->registers[0xF] = 0;
                unsigned int row = 0;
                unsigned int col = 0;
                for(; row < n; ++row){
                    uint8_t sprite_byte = cp->memory[cp->index + row];
                    for(; col < 8; ++col){
                        uint8_t sprite_pixel = (sprite_byte >> (7-col)) & 1;
                        uint8_t screen_pixel = cp->video[ypos+row][xpos+col];

                        if(sprite_pixel){
                            if(screen_pixel){
                                cp->registers[0xF] = 1;
                            }
                            cp->video[ypos+row][xpos+col] ^= 1;
                        }
                    }
                    col = 0;
                }
            }
            break;

        case 0xE:
            if(byte == 0x9E){
                if(cp->registers[vx] == cp->key_code)
                    cp->pc += 2;
            }else if(byte == 0xA1){
                if(cp->registers[vx] != cp->key_code)
                    cp->pc += 2;
            }
            break;
            
        case 0xF:
            switch(byte){
                case 0x07:
                    cp->registers[vx] = cp->dt;
                    break;
                case 0x0A:
                    if(cp->key_code < 16)
                        cp->registers[vx] = cp->key_code;
                    else
                        cp->pc -= 2;
                    break;
                case 0x15:
                    cp->dt = cp->registers[vx];
                    break;
                case 0x18:
                    cp->st = cp->registers[vx];
                    break;
                case 0x1E:
                    cp->index += cp->registers[vx];
                    break;
                case 0x29:
                    cp->index = 5 * cp->registers[vx];
                    break;
                case 0x33:
                    {
                        uint8_t value = cp->registers[vx];
                        cp->memory[cp->index + 2] = value % 10;
                        value /= 10;

                        cp->memory[cp->index + 1] = value % 10;
                        value /= 10;

                        cp->memory[cp->index] = value % 10;
                        
                    }
                    break;
                case 0x55:
                    for(uint8_t i = 0;  i <= vx; ++i){
                        cp->memory[cp->index + i] = cp->registers[i];
                    }
                    break;
                case 0x65:
                    for(uint8_t i = 0; i <= vx; ++i){
                        cp->registers[i] = cp->memory[cp->index + i];
                    }
                    break;
                    
            }
            break;
        default:
            printf("wrong op code\n");
            
    }
            
    

    if(cp->dt > 0)
        --cp->dt;
    if(cp->st > 0)
        --cp->st;
}
int main(){
    chip8 cp;
    init_chip8(&cp);
    load_rom("./test_roms/invaders.rom", &cp);

    InitWindow(640, 320, "chip 8");
    SetTargetFPS(200);

    unsigned int i = 0;
    unsigned int j = 0;
    while(!WindowShouldClose()){

        if(IsKeyDown(KEY_ONE)){
            cp.key_code = 0x1;
        }else if(IsKeyDown(KEY_TWO)){
            cp.key_code = 0x2;

        }else if(IsKeyDown(KEY_THREE)){
            cp.key_code = 0x3;

        }else if(IsKeyDown(KEY_FOUR)){
            cp.key_code = 0xC;

        }else if(IsKeyDown(KEY_Q)){
            cp.key_code = 0x4;

        }else if(IsKeyDown(KEY_W)){
            cp.key_code = 0x5;

        }else if(IsKeyDown(KEY_E)){
            cp.key_code = 0x6;

        }else if(IsKeyDown(KEY_R)){
            cp.key_code = 0xD;

        }else if(IsKeyDown(KEY_A)){
            cp.key_code = 0x7;

        }else if(IsKeyDown(KEY_S)){
            cp.key_code = 0x8;

        }else if(IsKeyDown(KEY_D)){
            cp.key_code = 0x9;

        }else if(IsKeyDown(KEY_F)){
            cp.key_code = 0xE;

        }else if(IsKeyDown(KEY_Z)){
            cp.key_code = 0xA;

        }else if(IsKeyDown(KEY_X)){
            cp.key_code = 0x0;

        }else if(IsKeyDown(KEY_C)){
            cp.key_code = 0xB;

        }else if(IsKeyDown(KEY_V)){
            cp.key_code = 0xF;
        }
        cycle(&cp);
        BeginDrawing();
        ClearBackground(BLACK);
        
        for(; i < 32; ++i){
            for(; j < 64; ++j){
                if(cp.video[i][j]){
                    DrawRectangle(j * 10, i * 10, 10, 10, RAYWHITE);
                }
            }
            j = 0;
        }
        i = 0;
        EndDrawing();
        cp.key_code = 16;
    }
    return 0;
}
