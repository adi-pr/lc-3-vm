#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
/* unix only */
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; /* 65536 memory locations */

struct termios original_tio;

/* input buffering prototypes */
void handle_interrupt(int signal);
void disable_input_buffering();
void restore_input_buffering();
uint16_t check_key();

/* read image prototypes */
void read_image_file(FILE *file);
uint16_t swap16(uint16_t x);
int read_image(const char *image_path);

/* memory access prototypes */
void mem_write(uint16_t address, uint16_t val);
uint16_t mem_read(uint16_t address);

/* flags prototype */
void update_flags(uint16_t r);

/* opcode prototypes */

uint16_t sign_extended(uint16_t x, int bit_count);

typedef enum
{
  R_R0 = 0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,   /* general registers */
  R_PC,   /* program counter */
  R_COND, /* info about last calculation */
  R_COUNT,
} Registers;

uint16_t registers[R_COUNT];

typedef enum
{
  MR_KBSR = 0xFE00, /* keyboard status */
  MR_KBDA = 0xFE02, /* keyboard data */
} MemoryMappedRegisters;

typedef enum
{
  OP_BR = 0, /* branch */
  OP_ADD,    /* add */
  OP_LD,     /* load */
  OP_ST,     /* store */
  OP_JSR,    /* jump to subroutine */
  OP_AND,    /* bitwise and */
  OP_LDR,    /* load register */
  OP_STR,    /* store register */
  OP_RTI,    /* unused */
  OP_NOT,    /* bitwise not */
  OP_LDI,    /* load indirect */
  OP_STI,    /* store indirect */
  OP_JMP,    /* jump */
  OP_RES,    /* reserved (used) */
  OP_LEA,    /* load effective address */
  OP_TRAP    /* exectue trap */
} Opcodes;

typedef enum
{
  FL_POS = 1 << 0,
  FL_ZRO = 1 << 1,
  FL_NEG = 1 << 2,
} ConditionalFlags;

int main(int argc, const char *argv[])
{
  if (argc < 2)
  {
    /* show usage string */
    printf("lc3 [image-file1] ...\n");
    exit(2);
  }

  for (int j = 1; j < argc; ++j)
  {
    if (!read_image(argv[j]))
    {
      printf("failed to load image: %s\n", argv[j]);
      exit(1);
    }
  }

  signal(SIGINT, handle_interrupt);
  disable_input_buffering();

  registers[R_COND] = FL_ZRO;

  enum
  {
    PC_START = 0x3000
  };
  registers[R_PC] = PC_START;

  int running = 1;
  while (running)
  {
    /* FETCH */
    uint16_t instr = mem_read(registers[R_PC]++);
    uint16_t op = instr >> 12;

    switch (op)
    {
    case OP_ADD:
      /* destination register (DR)*/
      uint16_t r0 = (instr >> 9) & 0x7;
      /* first operand (SR1) */
      uint16_t r1 = (instr >> 6) & 0x7;
      /* immediate mode check */
      uint16_t imm_flag = (instr >> 5) & 0x1;

      if (imm_flag)
      {
        uint16_t imm5 = sign_extended(instr & 0x1F, 5);
        registers[r0] = registers[r1] + imm5;
      }
      else
      {
        uint16_t r2 = instr & 0x7;
        registers[r0] = registers[r1] + registers[r2];
      }

      update_flags(r0);
      break;

    case OP_AND:
      /* destination register */
      uint16_t r0 = (instr >> 9) & 0x7;
      /* first operand (SR1) */
      uint16_t r1 = (instr >> 6) & 0x7;
      /* immediate mode check */
      uint16_t imm_flag = (instr >> 5) & 0x1;

      if (imm_flag)
      {
        uint16_t imm5 = sign_extended(instr & 0x1F, 5);
        registers[r0] = registers[r1] & imm5;
      }
      else
      {
        uint16_t r2 = instr & 0x7;
        registers[r0] = registers[r1] & registers[r2];
      }

      update_flags(r0);
      break;

    case OP_NOT:
      /* destination register  */
      uint16_t r0 = (instr >> 9) & 0x7;
      /* first operand */
      uint16_t r1 = (instr >> 6) & 0x7;

      registers[r0] = ~r1;

      update_flags(r0);
      break;

    case OP_BR:
      /* PCoffset 9 */
      uint16_t pc_offset = sign_extended(instr & 0x1FF, 9);
      /* condition flag */
      uint16_t cond_flag = (instr >> 9) & 0x7;
      if (cond_flag & registers[R_COND])
      {
        registers[R_PC] += pc_offset;
      }

      break;

    case OP_JMP:
      /* BaseR */
      uint16_t base_r = (instr >> 6) & 0x7;
      registers[R_PC] = base_r;
      break;

    case OP_JSR:
      /* condition flag */
      uint16_t cond_flag = (instr >> 11) & 0x1;
      registers[R_R7] = registers[R_PC];

      if (!cond_flag)
      {
        uint16_t base_r = (instr >> 6) & 0x7;
        registers[R_PC] = base_r; /* JSRR */
      }
      else
      {
        /* PCoffset 11 */
        uint16_t pc_offset = sign_extended(instr & 0x1FF, 11);
        registers[R_PC] += pc_offset; /* JSR */
      }

      break;

    case OP_LD:
      break;

    case OP_LDI:
      /* destination register (DR) */
      uint16_t r0 = (instr >> 9) & 0x7;
      /* PCoffset 9 */
      uint16_t pc_offset = sign_extended(instr & 0x1FF, 9);

      /* add pc_offset to the current PC, look at that memory location to get the final address */
      registers[r0] = mem_read(mem_read(registers[R_PC] + pc_offset));

      update_flags(r0);
      break;
    case OP_LDR:
      break;
    case OP_LEA:
      break;
    case OP_ST:
      break;
    case OP_STI:
      break;
    case OP_STR:
      break;
    case OP_TRAP:
      break;
    case OP_RES:
      break;
    case OP_RTI:
      break;
    default:
      break;
    }
  }

  return 0;
}

/* Input buffering */
void handle_interrupt(int signal)
{
  restore_input_buffering();
  printf("\n");
  exit(-2);
}

void disable_input_buffering()
{
  tcgetattr(STDIN_FILENO, &original_tio);
  struct termios new_tio = original_tio;
  new_tio.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

uint16_t check_key()
{
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

/* Image file handlers */
void read_image_file(FILE *file)
{
  /* the orgin is where in memory to place the image */
  uint16_t orgin;
  fread(&orgin, sizeof(orgin), 1, file);
  orgin = swap16(orgin);

  uint16_t max_read = MEMORY_MAX - orgin;
  uint16_t *p = memory + orgin;
  size_t read = fread(p, sizeof(uint16_t), max_read, file);

  /* swap to little endian */
  while (read-- > 0)
  {
    *p = swap16(*p);
    ++p;
  }
}

uint16_t swap16(uint16_t x)
{
  return (x << 8) | (x >> 8);
}

int read_image(const char *image_path)
{
  FILE *file = fopen(image_path, "rb");
  if (!file)
  {
    return 0;
  };
  read_image_file(file);
  fclose(file);
  return 1;
}

/* Memory Access */
void mem_write(uint16_t address, uint16_t val)
{
  memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
  if (address == MR_KBSR)
  {
    if (check_key())
    {
      memory[MR_KBSR] = (1 << 15);
      memory[MR_KBSR] = getchar();
    }
    else
    {
      memory[MR_KBSR] = 0;
    }
  }
  return memory[address];
}

void update_flags(uint16_t r)
{
  if (registers[r] == 0)
  {
    registers[R_COND] = FL_ZRO;
  }
  else if (registers[r] >> 15)
  {
    registers[R_COND] = FL_NEG;
  }
  else
  {
    registers[R_COND] = FL_POS;
  }
}

/* Extends the sign of an integer from bit_count bits to a 16-bit uint16_t. */
uint16_t sign_extended(uint16_t x, int bit_count)
{
  if ((x >> (bit_count - 1)) & 1)
  {
    x |= (0xFFFF << bit_count);
  }

  return x;
}

/* Opcode Functions */
