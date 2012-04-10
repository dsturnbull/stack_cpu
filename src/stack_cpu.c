#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <termios.h>

#include "src/asmr.h"
#include "src/stack_cpu.h"

extern bool debug;

stack_cpu_t *
init_stack_cpu()
{
    stack_cpu_t *cpu = calloc(1, sizeof(stack_cpu_t));

    cpu->code = &cpu->mem[STACK_CPU_CODE];
    cpu->ip = &cpu->code[0];
    cpu->data = &cpu->mem[STACK_CPU_DATA];
    cpu->stack = &cpu->mem[STACK_CPU_STACK];
    cpu->sp = &cpu->stack[0];
    cpu->frames = &cpu->mem[STACK_CPU_FRAMES];
    cpu->rp = &cpu->frames[0];
    cpu->io = &cpu->mem[STACK_CPU_IO];
    cpu->kbd = &cpu->io[STACK_CPU_IO_KBD];

    return cpu;
}

void
load_prog(stack_cpu_t *stack_cpu, uint32_t *prog, size_t len)
{
    for (size_t i = 0; i < len; i++)
        stack_cpu->mem[i] = prog[i];
}

void
run_prog(stack_cpu_t *cpu)
{
    op_t op;

    uint32_t t;

    // then ip is set, make it point to the previous instruction,
    // because this loop will increment it at the end.

    while ((op = *(cpu->ip)) != 0) {
        if (debug) {
            getchar();

            printf("ip: " MEM_FMT " "
                   "sp: " MEM_FMT " "
                   "rp: " MEM_FMT " "
                   "op: " MEM_FMT ": "
                   "cy: %lu ",
                    (uint32_t)(cpu->ip - cpu->code),
                    (uint32_t)(cpu->sp - cpu->stack),
                    (uint32_t)(cpu->rp - cpu->frames),
                    op,
                    cpu->cycles);

        }

        cpu->cycles++;
        //gettimeofday(&cpu->time, NULL);
        //cpu->mem[STACK_CPU_CLOCK] = cpu->time.tv_sec;

        switch (op) {
            case NOP:
                if (debug)
                    printf("nop\n");
                break;

            case HLT:
                if (debug)
                    printf("halt\n");
                return;

            case OUT:
                if (debug)
                    printf("-> " MEM_FMT "\n", *(cpu->sp--));
                else
                    printf("%c", *(cpu->sp--));
                break;

            case LOAD:
                if (debug)
                    printf("loading " MEM_FMT " from " MEM_FMT "\n",
                            cpu->data[*(cpu->sp)], *(cpu->sp));
                *(cpu->sp) = cpu->data[*(cpu->sp)];
                break;

            case STORE:
                if (debug)
                    printf("storing " MEM_FMT " at " MEM_FMT "\n",
                            *(cpu->sp),
                            *(cpu->sp - 1));
                cpu->data[*(cpu->sp--)] = *(cpu->sp--);
                break;

            case ADD:
                if (debug)
                    printf(MEM_FMT " + " MEM_FMT " == " MEM_FMT "\n",
                            *(cpu->sp - 1),
                            *(cpu->sp),
                            *(cpu->sp - 1) + *(cpu->sp));
                *(cpu->sp - 1) = *(cpu->sp - 1) + *(cpu->sp);
                cpu->sp--;
                break;

            case SUB:
                if (debug)
                    printf(MEM_FMT " - " MEM_FMT " == " MEM_FMT "\n",
                            *(cpu->sp - 1),
                            *(cpu->sp),
                            *(cpu->sp - 1) - *(cpu->sp));
                *(cpu->sp - 1) = *(cpu->sp - 1) - *(cpu->sp);
                cpu->sp--;
                break;

            case MUL:
                assert(false);

            case DIV:
                if (debug)
                    printf(MEM_FMT " / " MEM_FMT "\n",
                        *(cpu->sp - 1), *(cpu->sp));
                t = *(cpu->sp);
                *(cpu->sp) = *(cpu->sp - 1) % *(cpu->sp);
                *(cpu->sp - 1) = *(cpu->sp - 1) / t;
                break;

            case JMP:
                if (debug)
                    printf("jump " MEM_FMT "\n", *(cpu->sp));
                cpu->ip = &cpu->code[*(cpu->sp--)] - 1;
                break;

            case JZ:
                if (debug)
                    printf(MEM_FMT " == 0 ", *(cpu->sp - 1));
                if (*(cpu->sp - 1) == 0) {
                    if (debug)
                        printf("jumping to " MEM_FMT "\n", *(cpu->sp));
                    cpu->ip = &cpu->code[*(cpu->sp)] - 1;
                } else {
                    if (debug)
                        printf("not jumping\n");
                }
                cpu->sp -= 2;
                break;

            case JNZ:
                if (debug)
                    printf(MEM_FMT " != 0 ", *(cpu->sp - 1));
                if (*(cpu->sp - 1) != 0) {
                    if (debug)
                        printf("jumping to " MEM_FMT "\n", *(cpu->sp));
                    cpu->ip = &cpu->code[*(cpu->sp)] - 1;
                } else {
                    if (debug)
                        printf("not jumping\n");
                }
                cpu->sp -= 2;
                break;

            case CALL:
                if (debug)
                    printf("calling " MEM_FMT " from " MEM_FMT "\n",
                            *(cpu->sp),
                            (uint32_t)(cpu->ip - cpu->code));
                *(cpu->rp++) = cpu->ip - cpu->code;
                cpu->ip = &cpu->code[*(cpu->sp--)] - 1;
                break;

            case RET:
                if (debug)
                    printf("returning from " MEM_FMT " to " MEM_FMT "\n",
                            (uint32_t)(cpu->ip - cpu->code),
                            *(cpu->rp - 1));
                cpu->ip = cpu->code + *(--cpu->rp);
                break;

            case DUP:
                if (debug)
                    printf("dup of " MEM_FMT "\n", *(cpu->sp));
                *(++cpu->sp) = *(cpu->sp);
                break;

            case POP:
                if (debug)
                    printf("pop\n");
                cpu->sp--;
                break;

            case SWAP:
                if (debug)
                    printf("swap\n");
                t = *(cpu->sp);
                *(cpu->sp) = *(cpu->sp - 1);
                *(cpu->sp - 1) = t;
                break;

            case INT:
                if (debug)
                    printf("int\n");
                t = *(cpu->sp--);
                if (*(cpu->sp--) == STACK_CPU_IO_KBD) {
                    *(++cpu->sp) = readchar();
                    cpu->ip = &cpu->code[t] - 1;
                }
                break;

            case PUSH:
                if (debug)
                    printf("pushing " MEM_FMT "\n", *(cpu->ip + 1));
                *(++cpu->sp) = *(++cpu->ip);
                break;

            case DEBUG:
                debug = true;
                break;
        }

        if (debug)
            print_state(cpu);

        cpu->ip++;
    }
}

void
print_state(stack_cpu_t *cpu)
{
    // print final interesting things

    //printf("\033[2J");

    /*
    for (uint32_t i = 0; i < 128; i++) {
        if (i % 16 == 0)
            printf("\ncode   %06x: ", i);
        printf(MEM_FMT "", cpu->code[i]);
        if (i == cpu->ip - cpu->code)
            printf("* ");
        else
            printf("  ");
    }
    */

    for (int i = 0; i < 128; i++) {
        if (i % 16 == 0)
            printf("\ndata   %06x: ", i);
        printf(MEM_FMT "  ", cpu->data[i]);
    }

    for (uint32_t i = 0; i < 32; i++) {
        if (i % 16 == 0)
            printf("\nstack  %06x: ", i);
        printf(MEM_FMT, cpu->stack[i]);
        if (i == cpu->sp - cpu->stack)
            printf("* ");
        else
            printf("  ");
    }

    for (uint32_t i = 0; i < 32; i++) {
        if (i % 16 == 0)
            printf("\nframes %06x: ", i);
        printf(MEM_FMT, cpu->frames[i]);
        if (i == cpu->rp - cpu->frames)
            printf("* ");
        else
            printf("  ");
    }

    printf("\n");
    /* printf("ip: " MEM_FMT "\n", (uint32_t)(cpu->ip - cpu->code)); */
    /* printf("sp: " MEM_FMT "\n", (uint32_t)(cpu->sp - cpu->stack)); */
    /* printf("rp: " MEM_FMT "\n", (uint32_t)(cpu->rp - cpu->frames)); */
    /* printf("cy: %lu\n", cpu->cycles); */
}

uint8_t
readchar()
{
    struct termios old_tio, new_tio;
    tcgetattr(1, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);

    tcsetattr(1, TCSANOW, &new_tio);
    uint8_t c = getchar();
    tcsetattr(1, TCSANOW, &old_tio);

    return c;
}

