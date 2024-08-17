#include <stdint.h>
#include <stdio.h>

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; /* 65536 memory locations */

typedef enum {
  R_R0 = 0, R_R1, R_R2, R_R3, R_R4, R_R5, R_R6, R_R7, /* general registers */ 
  R_PC, /* program counter */
  R_COND, /* info about last calculation */
  R_COUNT,
} Registers;

uint16_t registers[R_COUNT];

typedef enum {
  OP_BR = 0,  /* branch */ 
  OP_ADD,     /* add */
  OP_LD,      /* load */
  OP_ST,      /* store */
  OP_JSR,     /* jump register */
  OP_AND,     /* bitwise and */
  OP_LDR,     /* load register */
  OP_STR,     /* store register */
  OP_RTI,     /* unused */
  OP_NOT,     /* bitwise not */
  OP_LDI,     /* load indirect */
  OP_STI,     /* store indirect */
  OP_JMP,     /* jump */
  OP_RES,     /* reserved (used) */
  OP_LEA,     /* load effective address */
  OP_TRAP     /* exectue trap */
} Opcodes;

typedef enum {
  FL_POS = 1 << 0; 
  FL_ZRO = 1 << 1;
  FL_NEG = 1 << 2, 
} ConditionalFlags;
