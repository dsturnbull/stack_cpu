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

    // assemble it
    asmr_t *asmr = init_asmr();
    stack_cpu_asm(asmr, fn);

    for (size_t i = 0; i < asmr->prog_len; i++) {
        if (i % 16 == 0)
            printf("\ndata   %06lx: ", i);
        printf(MEM_FMT "  ", asmr->prog[i]);
    }
    printf("\n");

    free(asmr);

    return 0;
}

