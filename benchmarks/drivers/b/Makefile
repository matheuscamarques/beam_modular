# A/B drivers (lado B) - compilacao 100% autocontida.
# Monta libbench_ab.a a partir das fontes dos modulos (src/) e linka os
# drivers bench_*. Nao depende do CMakeLists raiz nem de build/. So gcc+ar.
# Uso: make -C benchmarks/drivers/b [BUILD_DIR=...]

CC        ?= gcc
AR        ?= ar
CFLAGS    ?= -O2 -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror
BUILD_DIR ?= ../../work/bench

SRC_ROOT = ../../../src
INC      = -I../../../include \
           -I$(SRC_ROOT)/memory  -I$(SRC_ROOT)/scheduler \
           -I$(SRC_ROOT)/emulator -I$(SRC_ROOT)/messaging \
           -I$(SRC_ROOT)/global  -I$(SRC_ROOT)/io

MOD_SRC = memory/beam_alloc.c memory/beam_gc.c \
          scheduler/erl_process.c scheduler/run_queue.c \
          emulator/beam_emu.c emulator/beam_bif.c emulator/beam_load.c \
          messaging/erl_message.c \
          global/atom_table.c global/ets.c \
          io/sys_poll.c
BENCH   = bench_loader bench_atom bench_ets bench_runqueue bench_alloc bench_msg bench_emu

OBJDIR  = $(BUILD_DIR)/obj
LIB     = $(OBJDIR)/libbench_ab.a
MOD_OBJ = $(addprefix $(OBJDIR)/mod/,$(MOD_SRC:.c=.o))
BIN     = $(addprefix $(BUILD_DIR)/,$(BENCH))

.PHONY: all clean
all: $(BIN)

$(OBJDIR)/mod/%.o: $(SRC_ROOT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(LIB): $(MOD_OBJ)
	$(AR) rcs $@ $^

$(OBJDIR)/bench/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BUILD_DIR)/bench_%: $(OBJDIR)/bench/bench_%.o $(LIB)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

clean:
	rm -rf $(BUILD_DIR)