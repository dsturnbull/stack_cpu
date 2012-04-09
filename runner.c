#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "src/stack_cpu.h"
#include "src/asmr.h"

extern bool debug;

int
main(int argc, char *argv[])
{
    int ch;
    char *fn = "progs/forth.asmr";

    while ((ch = getopt(argc, argv, "vf:")) != -1) {
        switch (ch) {
            case 'v':
                debug = true;
                break;

            case 'f':
                fn = optarg;
                break;
        }
    }

    // init cpu
    stack_cpu_t *cpu = init_stack_cpu();

    // assemble it
    asmr_t *asmr = init_asmr();
    stack_cpu_asm(asmr, fn);
    load_prog(cpu, asmr->prog, asmr->prog_len);
    free(asmr);

    if (debug) {
        printf("Press enter to step through the code.\n");
        printf("^D to step through as fast as possible.\n");
    }

    // run program
    run_prog(cpu);

    print_state(cpu);

    return 0;
}

