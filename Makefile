CC=clang

#OPT=-O3
OPT=-g
CFLAGS+=$(OPT) -mtune=native -fcommon -pipe
CFLAGS+=-pedantic-errors -Wall -Werror -Wextra -Wformat=2 -Wswitch
CFLAGS+=-Wno-unused-variable -Wno-unused-parameter
CFLAGS+=-std=c11
CFLAGS+=-I.

SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:%.c=%.o)
DEPS=$(SRCS:%.c=%.d)

all: runner as

runner: $(OBJS) runner.o
	$(CC) $(CFLAGS) $^ -o $@

as: $(OBJS) as.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f runner as

analyze:
	$(CC) $(CFLAGS) --analyze $(SRCS)

$(DEPS):
	@$(CC) $(CFLAGS) -M -MM -MT $(@:%.d=%.o) $(@:%.d=%.c) > $@

-include $(DEPS)

.PHONY: all clean analyze $(DEPS)

