#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>

#include "src/asmr.h"

extern bool debug;

asmr_t *
init_asmr()
{
    asmr_t *asmr = malloc(sizeof(asmr_t));
    asmr->labels_size = 1024;
    asmr->label_count = 0;
    asmr->labels = malloc(asmr->labels_size * sizeof(struct label *));

    asmr->missings_size = 1024;
    asmr->missing_count = 0;
    asmr->missing = malloc(asmr->labels_size * sizeof(struct label *));

    asmr->data_size = 1024;
    asmr->data_count = 0;
    asmr->data = malloc(asmr->data_size * sizeof(struct data *));

    asmr->prog = calloc(CPU_DATA - 1, sizeof(uint32_t));
    asmr->ip = asmr->prog;

    return asmr;
}

size_t
stack_cpu_asm(asmr_t *asmr, char *fn)
{
    // make room for jumping to _main
    asmr->ip += 4;

    parse_file(asmr, fn);

    asmr->prog_len = asmr->ip - asmr->prog;

    replace_sentinels(asmr);

    // jump to main
    uint32_t main_loc;

    size_t i;
    for (i = 0; i < asmr->label_count; i++) {
        if (strcmp(asmr->labels[i]->name, "main") == 0) {
            main_loc = asmr->labels[i]->addr;
            break;
        }
    }

    asmr->ip = asmr->prog;
    push(asmr, NULL, PUSH, 0);
    push(asmr, NULL, main_loc, 0);
    push(asmr, NULL, CALL, 0);
    push(asmr, NULL, HLT, 0);

    free(asmr->labels);
    free(asmr->missing);

    return asmr->prog_len;
}

void
parse_file(asmr_t *asmr, char *fn)
{
    // load asm file
    struct stat st;
    if (stat(fn, &st) < 0) {
        perror(fn);
        return;
    }

    FILE *fp;
    if ((fp = fopen(fn, "r")) == NULL) {
        perror(fn);
        return;
    }

    char *inp = malloc(st.st_size);
    fread(inp, st.st_size, 1, fp);
    fclose(fp);

    char *line;
    size_t lineno = 0;

    define_constant(asmr, "KBD", IO_KBD);
    define_constant(asmr, "CODE_PAGE", CPU_CODE);
    define_constant(asmr, "DATA_PAGE", CPU_DATA);
    define_constant(asmr, "STACK_PAGE", CPU_STACK);
    define_constant(asmr, "FRAMES_PAGE", CPU_FRAMES);
    define_constant(asmr, "IO_PAGE", CPU_IO);

    while ((line = strsep(&inp, "\n")) != NULL && inp != NULL) {
        lineno++;
        normalise_line(&line);

        char *ops = strsep(&line, "\t");

        if (ops[0] == '_') {
            make_label(asmr, ops);
            continue;
        }

        if (ops[0] == ';')
            continue;

        if (strcmp(ops, "%define") == 0) {
            char *name = strsep(&line, "\t");;
            uint32_t value;
            sscanf(strsep(&line, "\t"), "%x", &value);
            define_constant(asmr, name, value);
            continue;
        }

        if (strcmp(ops, "%import") == 0) {
            char *fn = strsep(&line, "\t");
            fn++; fn[strlen(fn) - 1] = '\0';
            parse_file(asmr, fn);
            continue;
        }

        if (strlen(ops) == 0)
            continue;

        op_t op = parse_op(ops);

        char *data;
        uint32_t addr;

        switch (op) {
            case PUSH:
                push(asmr, &line, op, 1);
                break;

            default:
                push(asmr, NULL, op, 0);
                break;
            }
    }

    free(inp);
}

uint32_t
read_value(asmr_t *asmr, char *s, uint32_t *ip)
{
    uint32_t arg = 0;

    if (s[0] == '0' && s[1] == 'x') {
        // memory address
        sscanf(s, "%x", &arg);

    } else if (s[0] == 'C' && s[1] == 'P' && s[2] == 'U') {
        // const
        if (strcmp(s, "STACK_CPU_CLOCK") == 0) {
        }

    } else if (s[0] == '[') {
        // pointer
        s++;
        s[strlen(s)] = '\0';
        sscanf(s, "%x", &arg);

    } else if (s[0] >= 'A' && s[0] <= 'Z') {
        // constant
        size_t i;
        for (i = 0; i < asmr->label_count; i++) {
            if (strcmp(asmr->labels[i]->name, s) == 0) {
                arg = asmr->labels[i]->addr;
                break;
            }
        }

    } else {
        // label
        bool variable = true;
        struct label *label = NULL;
        if (s[0] == '_') {
            s++;
            s[strlen(s)] = '\0';
            variable = false;
        }

        size_t i;
        for (i = 0; i < asmr->label_count; i++) {
            if (strcmp(asmr->labels[i]->name, s) == 0) {
                label = asmr->labels[i];
                break;
            }
        }

        if (label) {
            arg = asmr->labels[i]->addr;
        } else {
            asmr->missing[asmr->missing_count] =
                malloc(sizeof(struct label));
            asmr->missing[asmr->missing_count]->addr = ip - asmr->prog;
            asmr->missing[asmr->missing_count]->name = strdup(s);
            asmr->missing_count++;
            arg = 0xdeadbeef;
        }
    }

    return arg;
}

void
push(asmr_t *asmr, char **line, uint32_t op, size_t n)
{
    *(asmr->ip)++ = op;
    for (size_t i = 0; i < n; i++) {
        char *s  = strsep(line, "\t");
        uint32_t arg = read_value(asmr, s, asmr->ip);
        *(asmr->ip)++ = arg;
    }
}

void
define_constant(asmr_t *asmr, char *name, uint32_t value)
{
    asmr->labels[asmr->label_count] = malloc(sizeof(struct label));
    asmr->labels[asmr->label_count]->name = strdup(name);
    asmr->labels[asmr->label_count]->addr = value;
    asmr->label_count++;
}

void
make_label(asmr_t *asmr, char *s)
{
    // chomp
    s++; s[strlen(s) - 1] = '\0';

    asmr->labels[asmr->label_count] = malloc(sizeof(struct label));
    asmr->labels[asmr->label_count]->name = strdup(s);
    asmr->labels[asmr->label_count]->addr = asmr->ip - asmr->prog;
    /* printf("%s == " MEM_FMT "\n", s, */
    /*         asmr->labels[asmr->label_count]->addr); */
    asmr->label_count++;
}

op_t
parse_op(char *op)
{
    GET_CONST(NOP);
    GET_CONST(HLT);
    GET_CONST(OUT);
    GET_CONST(LOAD);
    GET_CONST(STORE);
    GET_CONST(ADD);
    GET_CONST(SUB);
    GET_CONST(MUL);
    GET_CONST(DIV);
    GET_CONST(JMP);
    GET_CONST(JZ);
    GET_CONST(JNZ);
    GET_CONST(CALL);
    GET_CONST(RET);
    GET_CONST(DUP);
    GET_CONST(POP);
    GET_CONST(SWAP);
    GET_CONST(INT);
    GET_CONST(DEBUG);
    GET_CONST(PUSH);

    printf("%s\n", op);
    assert(false);
}

void
normalise_line(char **line)
{
    // skip whitespace
    while (*(*line) == '\t' || *(*line) == ' ')
        (*line)++;

    // skip comments
    char *c;
    if ((c = strchr(*line, ';')) != NULL) {
        c--;
        while (*c == '\t' || *c == ' ')
            c--;
        *(c + 1) = '\0';
    }
}

void
replace_sentinels(asmr_t *asmr)
{
    for (size_t i = 0; i < asmr->prog_len; i++) {
        if (asmr->prog[i] == 0xdeadbeef) {
            bool found = false;
            /* printf(MEM_FMT " is deadbeef\n", i); */
            for (size_t j = 0; j < asmr->missing_count; j++) {
                if (asmr->missing[j]->addr == (uint32_t)i) {
                    /* printf(MEM_FMT " found\n", asmr->missing[j]->addr); */
                    for (size_t n = 0; n < asmr->label_count; n++) {
                        if (strcmp(asmr->labels[n]->name,
                                    asmr->missing[j]->name) == 0) {
                            /* printf("%s == %s\n", */
                            /*         asmr->labels[n]->name, */
                            /*         asmr->missing[j]->name); */
                            found = true;
                            asmr->prog[i] = asmr->labels[n]->addr;
                        }
                    }
                }

            }

            assert(found);
        }
    }
}

void
print_prog(asmr_t *asmr)
{
    uint32_t *p = asmr->prog;
    for (size_t i = 0; i < asmr->prog_len; i += 16) {
        printf(MEM_FMT ": ", (uint32_t)i);
        for (int j = 0; j < 16; j++) {
            printf(MEM_FMT " ", *(p++));
        }
        printf("\n");
    }
    printf("\n");
}

