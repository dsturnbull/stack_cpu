#ifndef __src_stack_cpu_h
#define __src_stack_cpu_h

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define CPU_MEMORY_SZ 0x100000
#define CPU_CODE      0x000000
#define CPU_DATA      0x040000
#define CPU_STACK     0x060000
#define CPU_FRAMES    0x080000
#define CPU_IO        0x090000

#define IO_KBD                  0x00    // <uint32_t:...> (8 at a time)

#define GET_CONST(c) {                                                      \
    if (strcasecmp(op, #c) == 0) {                                          \
        return c;                                                           \
    }                                                                       \
} while (0)

#define MEM_FMT "%04x"

bool debug;

typedef struct stack_cpu_st {
    uint32_t mem[CPU_MEMORY_SZ];
    uint32_t *code, *data,  *stack, *frames,    *io;
    uint32_t *ip,           *sp,    *rp;
    size_t cycles;
} stack_cpu_t;

typedef enum op_e {
    // 0 args
    NOP,    // 0x0000
    HLT,    // 0x0001
    OUT,    // 0x0002
    LOAD,   // 0x0003
    STORE,  // 0x0004
    ADD,    // 0x0005
    SUB,    // 0x0006
    MUL,    // 0x0007
    DIV,    // 0x0008
    JMP,    // 0x0009
    JZ ,    // 0x000a
    JNZ,    // 0x000b
    CALL,   // 0x000c
    RET,    // 0x000d
    DUP,    // 0x000e
    POP,    // 0x000f
    SWAP,   // 0x0010
    INT,    // 0x0011
    DEBUG,  // 0x0012

    // 1 arg
    PUSH,   // 0x0013
} op_t;

stack_cpu_t * init_stack_cpu();

void load_prog(stack_cpu_t *, uint32_t *, size_t);
void run_prog(stack_cpu_t *);
void handle_interrupt(stack_cpu_t *);
void print_state(stack_cpu_t *);
uint8_t readchar();

#endif

