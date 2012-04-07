#ifndef __src_stack_cpu_h
#define __src_stack_cpu_h

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define STACK_CPU_MEMORY_SZ 0x10000
#define STACK_CPU_CODE      0x0000
#define STACK_CPU_DATA      0x4000
#define STACK_CPU_STACK     0x6000
#define STACK_CPU_FRAMES    0x8000

#define GET_CONST(c) {                                                      \
    if (strcasecmp(op, #c) == 0) {                                          \
        return c;                                                           \
    }                                                                       \
} while (0)

#define MEM_FMT "%02x"

bool debug;

typedef struct stack_cpu_st {
    uint32_t mem[STACK_CPU_MEMORY_SZ];
    uint32_t *code, *data,  *stack, *frames;
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

    // 1 arg
    PUSH,   // 0x0011
} op_t;

stack_cpu_t * init_stack_cpu();

void load_prog(stack_cpu_t *, uint32_t *, size_t);
void run_prog(stack_cpu_t *);
void print_state(stack_cpu_t *);

#endif


