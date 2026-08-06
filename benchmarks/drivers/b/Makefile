# A/B drivers (lado B) - compilacao 100% autocontida.
# Monta libbench_ab.a a partir das fontes dos modulos (src/<subsystem>/*.c) e
# linka os drivers bench_*. Totalmente independente do CMakeLists raiz
# (que esta em fluxo) e do conteudo de build/. Requer apenas gcc/ar.
#
# Uso: make -C benchmarks/drivers/b [BUILD_DIR=...]

CC       ?= gcc
AR       ?= ar
CFLAGS   ?= -O2 -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror
BUILD_DIR ?= ../../../build/benchmarks

SRC_ROOT = ../../../../src
INC      = -I../../../../include \
           -I$(SRC_ROOT)/memory  -I$(SRC_ROOT)/scheduler \
           -I$(SRC_ROOT)/emulator -I$(SRC_ROOT)/messaging \
           -I$(SRC_ROOT)/global  -I$(SRC_ROOT)/io

MODS = memory/beam_alloc.c memory/beam_gc.c \
       scheduler/erl_process.c scheduler/run_queue.c \
       emulator/beam_emu.c emulator/beam_bif.c emulator/beam_load.c \
       messaging/erl_message.c \
       global/atom_table.c global/ets.c \
       io/sys_poll.c

BINS = bench_loader bench_atom bench_ets bench_runqueue bench_alloc bench_msg bench_emu
OBJDIR = $(BUILD_DIR)/obj
LIB    = $(OBJDIR)/libbench_ab.a
OBJS   = $(patsubst %.c,$(OBJDIR)/%.o,$(SRC))

.PHONY: all clean
all: $(BINS)

$(OBJDIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(LIBJS): $(OBJS)
	$(AR) rcs $@ $^

$(OBJDIR)/bench_%: $(OBJDIR)/bench_%.o $(LIBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) $< $(LIBJS) -o $@

$(OBJDIR)/bench_%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BINS): %: $(OBJDIR)/% $(OBJDIR)/bench_loader
	cp $(OBJDIR)/$@ $(BUILD_DIR)/$@

clean:
	rm -rf $(BUILD_DIR)