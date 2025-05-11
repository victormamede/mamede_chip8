# mamede_chip8

`mamede_chip8` is a CHIP-8 emulator written in C, built for learning and fun. CHIP-8 is a simple interpreted programming language from the 1970s, and emulating it is a great way to understand how interpreters and low-level systems work.

## Features

- [x] Interpreter for CHIP-8 instructions
- [x] 64x32 monochrome display
- [x] Hex-based keypad input
- [x] Memory and register management
- [x] Basic ROM loading
- [x] Sound and timers

## Getting Started

Clone the repository:

```bash
git clone https://github.com/vmamede/mamede_chip8.git
cd mamede_chip8
```

Compile the project:

```bash
gcc -lSDL3 -o chip8 main.c
```

Run a ROM:

```bash
./chip8 path/to/rom.ch8
```

## Dependencies

This project uses SDL3 (Simple DirectMedia Layer 3) for graphics, input, and sound handling. Make sure SDL3 is installed on your system before compiling.

On Linux, you can install it via your package manager or build from source.
On Windows, you'll have to link it yourself

## Resources

- Tobias V. Langhoff: [https://tobiasvl.github.io/blog/write-a-chip-8-emulator/](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)

## License

MIT License
