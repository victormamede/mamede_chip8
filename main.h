#pragma once

#include <stdint.h>

#include <SDL3/SDL.h>

#define MEMORY_SIZE 4096
#define STACK_SIZE 4096

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

#define PIXEL_SIZE 16

#define REFRESH_RATE_HZ 60

#define FONT_LOAD_ADDRESS 0x050
#define PROGRAM_LOAD_ADDRESS 0x200

typedef struct Emulator
{
    uint8_t memory[MEMORY_SIZE];
    uint16_t programCounter;
    uint16_t indexRegister;
    uint16_t stack[STACK_SIZE];
    uint8_t stackPointer;

    uint8_t delayTimer;
    uint8_t soundTimer;

    uint8_t vRegisters[16];

    bool keypadStatus[16];
    bool screen[SCREEN_WIDTH * SCREEN_HEIGHT];
} Emulator;

int programLoad(char filename[], Emulator *emulator);

void screenDraw(Emulator *emulator);
void screenClear(Emulator *emulator);

/**
 * Decode and execute step
 *
 * @param emulator data can be mutated
 * @param e SDL event
 */
void updateKeypad(Emulator *emulator, SDL_Event *e);

/**
 * Decode and execute step
 *
 * @param emulator data can be mutated
 */
void decodeAndExecute(Emulator *emulator);