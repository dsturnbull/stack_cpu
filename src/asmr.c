#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

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

    asmr->prog = calloc(1024, sizeof(uint32_t));
    asmr->ip = asmr->prog;

    return asmr;
}

size_t
stack_cpu_asm(asmr_t *asmr, const char *code)
{
    char *inp = strdup(code);
    char *line;
    size_t lineno = 0;

    while ((line = strsep(&inp, "\n")) != NULL) {
        lineno++;
        normalise_line(&line);

        char *ops = strsep(&line, "\t");

        if (ops[0] == '_') {
            make_label(asmr, ops);
            continue;
        }

        if (ops[0] == ';')
            continue;

        if (strlen(ops) == 0)
            continue;

        op_t op = parse_op(ops);

        char *data;
        uint32_t addr;

        switch (op) {
            case NOP:
            case HLT:
            case OUT:
            case LOAD:
            case STORE:
            case ADD:
            case SUB:
            case MUL:
            case DIV:
            case JMP:
            case JZ:
            case JNZ:
            case CALL:
            case RET:
            case DUP:
            case POP:
            case SWAP:
                push(asmr, NULL, op, 0);
                break;

            case PUSH:
                push(asmr, &line, op, 1);
                break;
            }
    }

    asmr->prog_len = asmr->ip - asmr->prog;

    replace_sentinels(asmr);
    
    free(asmr->labels);
    free(asmr->missing);
    free(inp);

    return asmr->prog_len;
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

    } else if (s[0] == 'r' && s[1] >= 0x30 && s[1] <= 0x37) {
        // register
        //arg = R0 + (s[1] - 0x30) * R0;

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
make_label(asmr_t *asmr, char *s)
{
    // chomp
    s++; s[strlen(s) - 1] = '\0';

    asmr->labels[asmr->label_count] = malloc(sizeof(struct label));
    asmr->labels[asmr->label_count]->name = strdup(s);
    asmr->labels[asmr->label_count]->addr = asmr->ip - asmr->prog;
    /* printf("%s == " MEM_FMT "x\n", s, */
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
    for (int i = 0; i < STACK_CPU_MEMORY_SZ; i++)
        if (asmr->prog[i] == 0xdeadbeef) {
            /* printf(MEM_FMT "x is deadbeef\n", i); */
            for (size_t j = 0; j < asmr->missing_count; j++)
                if (asmr->missing[j]->addr == (uint32_t)i) {
                    /* printf(MEM_FMT "x found\n", asmr->missing[j]->addr); */
                    for (size_t n = 0; n < asmr->label_count; n++) {
                        /* printf("%s == %s\n", */
                        /*         asmr->labels[n]->name, */
                        /*         asmr->missing[j]->name); */
                        if (strcmp(asmr->labels[n]->name,
                                    asmr->missing[j]->name) == 0)
                            asmr->prog[i] = asmr->labels[n]->addr;
                    }
                }
        }
}

void
print_prog(asmr_t *asmr)
{
    uint32_t *p = asmr->prog;
    for (size_t i = 0; i < asmr->prog_len; i += 16) {
        printf(MEM_FMT "lx: ", i);
        for (int j = 0; j < 16; j++) {
            if (j % 4 == 0)
                printf(" ");
            printf(MEM_FMT "x ", *(p++));
        }
        printf("\n");
    }
    printf("\n");
}

