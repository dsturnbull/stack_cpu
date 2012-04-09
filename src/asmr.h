#ifndef __src_asmr_h
#define __src_asmr_h

#include <stdint.h>
#include <sys/types.h>

#include "src/stack_cpu.h"

typedef struct asmr_st {
    size_t labels_size;
    size_t label_count;
    struct label **labels;

    size_t missings_size;
    size_t missing_count;
    struct label **missing;

    size_t prog_len;
    uint32_t *prog;
    uint32_t *ip;

    size_t data_size;
    size_t data_count;
    struct data **data;
} asmr_t;

struct label {
    char *name;
    uint32_t addr;
};

struct data {
    char *name;
    uint32_t addr;
};

asmr_t * init_asmr();
size_t stack_cpu_asm(asmr_t *, char *);
void parse_file(asmr_t *, char *);
uint32_t read_value(asmr_t *, char *, uint32_t *);
void push(asmr_t *, char **, uint32_t, size_t);
void define_constant(asmr_t *, char *, uint32_t);
void make_label(asmr_t *, char *);
op_t parse_op(char *);
void normalise_line(char **);
void replace_sentinels(asmr_t *);
void print_prog(asmr_t *);

#endif

