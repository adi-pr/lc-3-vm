# LC-3 Virtual Machine

This project implements a simple LC-3 (Little Computer 3) virtual machine in C. The LC-3 is a teaching tool used to demonstrate how low-level computer systems work. This virtual machine executes LC-3 machine language programs, simulating the behavior of an LC-3 processor.

## Features

- 16-bit memory (65,536 memory locations)
- Simulates LC-3 opcodes: ADD, AND, BR, JMP, JSR, LDI, LDR, LEA, NOT, LD, ST, etc.
- Memory-mapped I/O for handling keyboard input
- Supports loading and executing LC-3 image files
- Input buffering and signal handling for clean shutdowns

## Requirements

- A C compiler (e.g., `gcc`)
- Unix-based system (Linux, macOS, etc.) for terminal handling (`termios`)

## Compilation

To compile the project, use the following command in the terminal:

```bash
gcc -o lc3 lc3_vm.c
```

## Usage

Once compiled, you can run the virtual machine by specifying an LC-3 image file:

```bash
./lc3 [image-file1] [image-file2] ...
```

For example, if you have an LC-3 image file `program.obj`:

```bash
./lc3 program.obj
```

## Memory Layout

- **Memory**: The LC-3 virtual machine has a 16-bit address space with 65,536 memory locations.
- **Registers**: The LC-3 has 8 general-purpose registers (R0 to R7), along with a Program Counter (PC) and Condition Flags register.

## Input/Output

- **Input**: The machine reads input from the terminal using memory-mapped I/O. Specifically, it listens for keyboard input via the Keyboard Status Register (`MR_KBSR`) and Keyboard Data Register (`MR_KBDA`).
- **Output**: The machine uses standard output to print results or debug information.

## Signals and Input Buffering

The machine handles input using signal interrupts to cleanly exit when a SIGINT (Ctrl+C) is received. Input buffering is disabled to allow immediate response to key presses, but is restored when the machine exits.

- **Disable input buffering**: Prevents the terminal from requiring an "Enter" key press after each input.
- **Restore input buffering**: Restores the terminal to its original state when the program exits.

## LC-3 Opcodes Supported

- **BR**: Conditional branch
- **ADD**: Add (with immediate mode support)
- **AND**: Bitwise AND (with immediate mode support)
- **NOT**: Bitwise NOT
- **LD**: Load
- **LDI**: Load indirect
- **LDR**: Load register
- **LEA**: Load effective address
- **ST**: Store
- **STR**: Store register
- **JMP**: Jump
- **JSR**: Jump to subroutine

## How to Load Image Files

LC-3 image files can be loaded using the `read_image` function. This function reads binary data into the virtual machine's memory, converting it from big-endian to little-endian format. You can load multiple image files by passing them as arguments to the program.

## Functions Overview

### Memory Access

- `mem_read(uint16_t address)`: Reads a value from memory, with special handling for memory-mapped I/O.
- `mem_write(uint16_t address, uint16_t val)`: Writes a value to memory.

### Flag Updates

- `update_flags(uint16_t r)`: Updates the condition flags (`FL_POS`, `FL_ZRO`, `FL_NEG`) based on the value in register `r`.

### Sign Extension

- `sign_extended(uint16_t x, int bit_count)`: Extends the sign of an integer from a smaller bit width to 16 bits.

### Input Handling

- `check_key()`: Checks if a key has been pressed.
- `disable_input_buffering()`: Disables terminal input buffering.
- `restore_input_buffering()`: Restores terminal input buffering.
- `handle_interrupt(int signal)`: Signal handler for `SIGINT`.

## License

This project is open source. You are free to use, modify, and distribute the code as needed.
