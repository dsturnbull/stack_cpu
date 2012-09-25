#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <termios.h>

#include "src/asmr.h"
#include "src/stack_cpu.h"

extern bool debug;

stack_cpu_t *
init_stack_cpu()
{
    stack_cpu_t *cpu = calloc(1, sizeof(stack_cpu_t));

    cpu->code = &cpu->mem[CPU_CODE];
    cpu->ip = &cpu->code[0];
    cpu->data = &cpu->mem[CPU_DATA];
    cpu->stack = &cpu->mem[CPU_STACK];
    cpu->sp = &cpu->stack[0];
    cpu->frames = &cpu->mem[CPU_FRAMES];
    cpu->rp = &cpu->frames[0];
    cpu->io = &cpu->mem[CPU_IO];

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
                handle_interrupt(cpu);
                break;

            case PUSH:
                if (debug)
                    printf("pushing " MEM_FMT "\n", *(cpu->ip + 1));
                *(++cpu->sp) = *(++cpu->ip);
                break;

            case DEBUG:
                printf("debugging on\n");
                debug = true;
                break;
        }

        cpu->ip++;

        if (debug)
            print_state(cpu);
    }
}

void
handle_interrupt(stack_cpu_t *cpu)
{
    uint32_t callback = *(cpu->sp--), rsrc = *(cpu->sp--);

    switch (rsrc) {
        case IO_KBD:
            *(++cpu->sp) = readchar();
            *(cpu->rp++) = cpu->ip - cpu->code;
            cpu->ip = &cpu->code[callback] - 1;
            break;

        default:
            if (debug)
                printf("unknown resource " MEM_FMT "\n", rsrc);
    }
}

void
print_state(stack_cpu_t *cpu)
{
    // print final interesting things

    //printf("\033[2J");

    char status[3];
    status[0] = ' ';
    status[1] = ' ';
    status[2] = '\0';

    // code
    /*
    for (uint32_t i = 0; i < 128; i++) {
        if (i % 16 == 0)
            printf("\ncode   %06x: ", i);

        status[0] = ' ';
        status[1] = ' ';

        if (i == cpu->ip - cpu->code)
            status[1] = '*';

        printf("%s", status);
        printf(MEM_FMT, cpu->code[i]);
    }
    */

    // data
    for (int i = 0; i < 128; i++) {
        if (i % 16 == 0)
            printf("\ndata   %06x: ", i);

        status[0] = ' ';
        status[1] = ' ';

        if (i == cpu->ip - cpu->data)
            status[1] = '*';

        printf("%s", status);
        printf(MEM_FMT, cpu->data[i]);
    }

    // stack
    for (uint32_t i = 0; i < 32; i++) {
        if (i % 16 == 0)
            printf("\nstack  %06x: ", i);

        status[0] = ' ';
        status[1] = ' ';

        if (i == cpu->sp - cpu->stack)
            status[1] = '*';

        printf("%s", status);
        printf(MEM_FMT, cpu->stack[i]);
    }

    // frames
    for (uint32_t i = 0; i < 32; i++) {
        if (i % 16 == 0)
            printf("\nframes %06x: ", i);
        if (i == cpu->rp - cpu->frames)
            printf(" *");
        else
            printf("  ");
        printf(MEM_FMT, cpu->frames[i]);
    }

    // io
    /*
    for (uint32_t i = 0; i < 0x160; i++) {
        if (i % 16 == 0)
            printf("\nio     %06x: ", i);
        printf("  ");
        printf(MEM_FMT, cpu->io[i]);
    }
    */

    printf("\n");

    /*
    printf("ip: " MEM_FMT "\n", (uint32_t)(cpu->ip - cpu->code));
    printf("sp: " MEM_FMT "\n", (uint32_t)(cpu->sp - cpu->stack));
    printf("rp: " MEM_FMT "\n", (uint32_t)(cpu->rp - cpu->frames));
    printf("cy: %lu\n", cpu->cycles);
    */
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

