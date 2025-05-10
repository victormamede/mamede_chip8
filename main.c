#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include "main.h"

const uint8_t characters[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
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

int main(int argc, char *argv[])
{
    srand(time(NULL));

    // Initialize emulator
    Emulator emulator = {0};

    memcpy(&emulator.memory[FONT_LOAD_ADDRESS], characters, sizeof(characters));
    programLoad(argv[1], &emulator);

    emulator.programCounter = PROGRAM_LOAD_ADDRESS;

    // Main loop
    clock_t previousTrigger = clock();
    while (true)
    {
        decodeAndExecute(&emulator);

        clock_t current = clock();

        if (current >= (previousTrigger + CLOCKS_PER_SEC / REFRESH_RATE_HZ))
        {
            // TIMING TRIGGER
            screenDraw(&emulator);

            // Decrease timers
            if (emulator.delayTimer > 0)
                emulator.delayTimer--;

            if (emulator.soundTimer > 0)
                emulator.soundTimer--;

            previousTrigger = current;
        }
    }

    return 0;
}

void screenDraw(Emulator *emulator)
{
    printf("\e[1;1H\e[2J"); // Clear screen

    for (int y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            printf(emulator->screen[y * SCREEN_WIDTH + x] ? "##" : "  ");
        }
        printf("\n");
    }
}

void screenClear(Emulator *emulator)
{
    memset(emulator->screen, 0, sizeof(emulator->screen));
}

int programLoad(char filename[], Emulator *emulator)
{
    printf("Opening file %s... \n", filename);
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("Error opening the file!\n");
        return 1;
    }

    fread(&emulator->memory[PROGRAM_LOAD_ADDRESS], sizeof(emulator->memory) - PROGRAM_LOAD_ADDRESS, 1, file);

    fclose(file);
    return 0;
}

void decodeAndExecute(Emulator *emulator)
{
    uint16_t instruction = (emulator->memory[emulator->programCounter]) << 8 |
                           emulator->memory[emulator->programCounter + 1];
    uint8_t index = (instruction & 0xF000) >> 12;
    uint8_t vX = (instruction & 0x0F00) >> 8;
    uint8_t vY = (instruction & 0x00F0) >> 4;
    uint8_t n = (instruction & 0x000F);
    uint8_t nn = (instruction & 0x00FF);
    uint16_t nnn = (instruction & 0x0FFF);

#ifdef DEBUG
    screenDraw(emulator);

    printf("Registers: ");
    for (int i = 0; i < 0xF; i++)
    {
        printf("%1X: %04X ", i, emulator->vRegisters[i]);
    }
    printf("\n");
    printf("Index register %4X\n", emulator->indexRegister);
    printf("PC: %04X, Current instruction: %04X, Index: %02X, X: %02X, Y: %02X, N: %02X, NN: %02X, NNN: %04X\n",
           emulator->programCounter, instruction, index, vX, vY, n, nn, nnn);
    getchar();
#endif

    emulator->programCounter += 2;

    switch (index)
    {
    case 0x0:
        if (instruction == 0x00E0)
            screenClear(emulator);

        if (instruction == 0x00EE)
        {
            emulator->stackPointer--;
            emulator->programCounter = emulator->stack[emulator->stackPointer];
        }
        break;
    case 0x1:
        emulator->programCounter = nnn;
        break;
    case 0x2:
        emulator->stack[emulator->stackPointer] = emulator->programCounter;
        emulator->stackPointer++;

        emulator->programCounter = nnn;
        break;
    case 0x3:
        if (emulator->vRegisters[vX] == nn)
            emulator->programCounter += 2;

        break;
    case 0x4:
        if (emulator->vRegisters[vX] != nn)
            emulator->programCounter += 2;

        break;
    case 0x5:
        if (emulator->vRegisters[vX] == emulator->vRegisters[vY])
            emulator->programCounter += 2;
        break;
    case 0x6:
        emulator->vRegisters[vX] = nn;
        break;
    case 0x7:
        emulator->vRegisters[vX] += nn;
        break;
    case 0x8:
        if (n == 0)
            emulator->vRegisters[vX] = emulator->vRegisters[vY];
        else if (n == 1)
            emulator->vRegisters[vX] |= emulator->vRegisters[vY];
        else if (n == 2)
            emulator->vRegisters[vX] &= emulator->vRegisters[vY];
        else if (n == 3)
            emulator->vRegisters[vX] ^= emulator->vRegisters[vY];
        else if (n == 4)
        {
            uint8_t result = emulator->vRegisters[vX] + emulator->vRegisters[vY];
            if (result > 255)
                emulator->vRegisters[0xF] = 1;
            emulator->vRegisters[vX] = result % 255;
        }
        else if (n == 5)
            emulator->vRegisters[vX] = emulator->vRegisters[vX] - emulator->vRegisters[vY];
        else if (n == 6)
        {
            emulator->vRegisters[0xF] = emulator->vRegisters[vX] & 1;
            emulator->vRegisters[vX] >>= 1;
        }
        else if (n == 7)
            emulator->vRegisters[vX] = emulator->vRegisters[vY] - emulator->vRegisters[vX];
        else if (n == 0xE)
        {
            emulator->vRegisters[0xF] = emulator->vRegisters[vX] >> 7;
            emulator->vRegisters[vX] <<= 1;
        }
        break;
    case 0x9:
        if (emulator->vRegisters[vX] != emulator->vRegisters[vY])
            emulator->programCounter += 2;
        break;
    case 0xA:
        emulator->indexRegister = nnn;
        break;
    case 0xC:
        int r = rand();
        emulator->vRegisters[vX] = r & nn;
        break;
    case 0xD:
        uint8_t x = emulator->vRegisters[vX] % SCREEN_WIDTH;
        uint8_t y = emulator->vRegisters[vY] % SCREEN_HEIGHT;
        emulator->vRegisters[0xF] = 0;

        for (int row = 0; row < n; row++)
        {
            uint8_t spriteRow = emulator->memory[emulator->indexRegister + row];

            for (int col = 0; col < 8; col++)
            {
                int state = (spriteRow >> (7 - col)) & 1;

                if (state == 0)
                    continue;

                if (x + col >= SCREEN_WIDTH)
                    break;

                int screenIndex = ((y + row) * SCREEN_WIDTH) + (x + col);
                if (emulator->screen[screenIndex])
                {
                    emulator->screen[screenIndex] = 0;
                    emulator->vRegisters[0xF] = 1;
                }
                else
                {
                    emulator->screen[screenIndex] = 1;
                }
            }

            if (y + row >= SCREEN_HEIGHT)
                break;
        }

        break;
    case 0xF:
        if (nn == 0x07)
        {
            emulator->vRegisters[vX] = emulator->delayTimer;
        }
        else if (nn == 0x15)
        {
            emulator->delayTimer = emulator->vRegisters[vX];
        }
        else if (nn == 0x18)
        {
            emulator->soundTimer = emulator->vRegisters[vX];
        }
        else if (nn == 0x1E)
        {
            // https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#fx1e-add-to-index
            emulator->indexRegister += emulator->vRegisters[vX];
        }
        else if (nn == 0x55)
        {
            for (int i = 0; i <= vX; i++)
            {
                emulator->memory[emulator->indexRegister + i] = emulator->vRegisters[i];
            }
        }
        else if (nn == 0x65)
        {
            for (int i = 0; i <= vX; i++)
            {
                emulator->vRegisters[i] = emulator->memory[emulator->indexRegister + i];
            }
        }
        else if (nn == 0x33)
        {
            uint8_t value = emulator->vRegisters[vX];
            uint16_t index = emulator->indexRegister;

            uint8_t i0 = value % 10;
            uint8_t i1 = ((value - i0) / 10) % 10;
            uint8_t i2 = ((value - i1 * 10 - i0) / 100) % 10;

            emulator->memory[index] = i2;
            emulator->memory[index + 1] = i1;
            emulator->memory[index + 2] = i0;
        }
        break;
    default:
        break;
    }
}