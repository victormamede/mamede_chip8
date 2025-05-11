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

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

int sdlInit()
{
    int i;

    SDL_SetAppMetadata("Mamede CHIP-8 Emulator", "1.0", "com.vmamede.chip8");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer("CHIP8", SCREEN_WIDTH * PIXEL_SIZE, SCREEN_HEIGHT * PIXEL_SIZE, 0, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return 2;
    }

    return 0;
}

void sdlCleanup()
{
    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_DestroyWindow(window);
    window = NULL;

    SDL_Quit();
}

int main(int argc, char *argv[])
{
    // Initialize emulator
    Emulator emulator = {0};

    memcpy(&emulator.memory[FONT_LOAD_ADDRESS], characters, sizeof(characters));
    programLoad(argv[1], &emulator);

    emulator.programCounter = PROGRAM_LOAD_ADDRESS;

    // Main loop
    if (sdlInit() > 0)
    {
        sdlCleanup();
        return -1;
    }

    uint64_t previousTick = SDL_GetTicks();
    uint64_t accumulator = 0;
    uint64_t timerInterval = 1000 / REFRESH_RATE_HZ;

    bool quit = false;
    SDL_Event e;
    while (!quit)
    {
        // Timing
        uint64_t currentTick = SDL_GetTicks();
        uint64_t delta = currentTick - previousTick;
        previousTick = SDL_GetTicks();

        accumulator += delta;

        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }

            updateKeypad(&emulator, &e);
        }

        decodeAndExecute(&emulator);

        if (accumulator >= timerInterval)
        {
            // TIMING TRIGGER
            screenDraw(&emulator);

            SDL_RenderPresent(renderer);

            // Decrease timers
            if (emulator.delayTimer > 0)
                emulator.delayTimer--;

            if (emulator.soundTimer > 0)
                emulator.soundTimer--;

            accumulator -= timerInterval;
        }

        // SDL_Delay(16);
        SDL_DelayNS(1000 * 1000);
    }

    sdlCleanup();

    return 0;
}

void screenDraw(Emulator *emulator)
{
    if (emulator->soundTimer > 0)
    {
        SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    }
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    for (int y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            if (emulator->screen[y * SCREEN_WIDTH + x])
            {
                SDL_FRect rect = {.x = x * PIXEL_SIZE, .y = y * PIXEL_SIZE, .w = PIXEL_SIZE, .h = PIXEL_SIZE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

void screenClear(Emulator *emulator)
{
    memset(emulator->screen, 0, sizeof(emulator->screen));
}

int programLoad(char filename[], Emulator *emulator)
{
    SDL_Log("Opening file %s... ", filename);
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        SDL_Log("Error opening the file!");
        return 1;
    }

    fread(&emulator->memory[PROGRAM_LOAD_ADDRESS], sizeof(emulator->memory) - PROGRAM_LOAD_ADDRESS, 1, file);

    fclose(file);
    return 0;
}

void updateKeypad(Emulator *emulator, SDL_Event *e)
{
    bool keyStatus;

    if (e->type == SDL_EVENT_KEY_DOWN)
        keyStatus = 1;
    else if (e->type == SDL_EVENT_KEY_UP)
        keyStatus = 0;
    else
        return;

    SDL_Scancode keysSequence[] = {
        SDL_SCANCODE_X,
        SDL_SCANCODE_1,
        SDL_SCANCODE_2,
        SDL_SCANCODE_3,
        SDL_SCANCODE_Q,
        SDL_SCANCODE_W,
        SDL_SCANCODE_E,
        SDL_SCANCODE_A,
        SDL_SCANCODE_S,
        SDL_SCANCODE_D,
        SDL_SCANCODE_Z,
        SDL_SCANCODE_C,
        SDL_SCANCODE_4,
        SDL_SCANCODE_R,
        SDL_SCANCODE_F,
        SDL_SCANCODE_V,
    };

    for (int i = 0; i < 16; i++)
    {
        emulator->previousKeypadStatus[i] = emulator->keypadStatus[i];
        if (e->key.scancode == keysSequence[i])
        {
            emulator->keypadStatus[i] = keyStatus;
        }
    }
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

    SDL_Log("Registers: ");
    for (int i = 0; i < 16; i++)
    {
        SDL_Log("%1X: %04X ", i, emulator->vRegisters[i]);
    }
    SDL_Log("Keypad: ");
    for (int i = 0; i < 16; i++)
    {
        SDL_Log("%1X: %04X ", i, emulator->keypadStatus[i]);
    }
    SDL_Log("Index register %4X", emulator->indexRegister);
    SDL_Log("PC: %04X, Current instruction: %04X, Index: %02X, X: %02X, Y: %02X, N: %02X, NN: %02X, NNN: %04X",
            emulator->programCounter, instruction, index, vX, vY, n, nn, nnn);
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
    case 0xB:
        emulator->programCounter = nnn + emulator->vRegisters[0];
        break;
    case 0xC:
        Sint32 r = SDL_rand(256);
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
    case 0xE:
        if (nn == 0x9E)
        {
            if (emulator->keypadStatus[emulator->vRegisters[vX]])
                emulator->programCounter += 2;
        }
        else if (nn == 0xA1)
        {
            if (!emulator->keypadStatus[emulator->vRegisters[vX]])
                emulator->programCounter += 2;
        }

        break;
    case 0xF:
        if (nn == 0x07)
        {
            emulator->vRegisters[vX] = emulator->delayTimer;
        }
        if (nn == 0x0A)
        {
            if (emulator->previousKeypadStatus[emulator->vRegisters[vX]] && !emulator->keypadStatus[emulator->vRegisters[vX]])

                emulator->programCounter -= 2;
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
        else if (nn == 0x29)
        {
            // Test this
            emulator->indexRegister = FONT_LOAD_ADDRESS + vX * 8;
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
        break;
    default:
        break;
    }
}