.DEFAULT_GOAL := all

CC      = gcc
CFLAGS  = -Wall -std=c11
TIME    = time

PROGS   = loop_invariants string_loop array_loop

# default FAST value (can override: make FAST=1 ...)
FAST ?= 0

# -------- build rules --------
loop_invariants: loop_invariants.c
	$(CC) $(CFLAGS) $(OPT) -DFAST=$(FAST) $< -o $@

string_loop: string_loop.c
	$(CC) $(CFLAGS) $(OPT) -DFAST=$(FAST) $< -o $@

array_loop: array_loop.c
	$(CC) $(CFLAGS) $(OPT) -DFAST=$(FAST) $< -o $@

# -------- run rules --------
run_invariants: loop_invariants
	$(TIME) ./loop_invariants > /dev/null

run_string: string_loop
	$(TIME) ./string_loop > /dev/null

run_array: array_loop
	$(TIME) ./array_loop > /dev/null

# -------- meta target --------
all:
	@echo "========== Loop Invariants =========="
	@$(MAKE) clean FAST=0 OPT=-O0 loop_invariants run_invariants
	@$(MAKE) clean FAST=0 OPT=-O2 loop_invariants run_invariants
	@$(MAKE) clean FAST=0 OPT=-O3 loop_invariants run_invariants
	@$(MAKE) clean FAST=1 OPT=-O0 loop_invariants run_invariants
	@$(MAKE) clean FAST=1 OPT=-O2 loop_invariants run_invariants
	@$(MAKE) clean FAST=1 OPT=-O3 loop_invariants run_invariants

	@echo "========== String Loop =========="
	@$(MAKE) clean FAST=0 OPT=-O0 string_loop run_string
	@$(MAKE) clean FAST=0 OPT=-O2 string_loop run_string
	@$(MAKE) clean FAST=0 OPT=-O3 string_loop run_string
	@$(MAKE) clean FAST=1 OPT=-O0 string_loop run_string
	@$(MAKE) clean FAST=1 OPT=-O2 string_loop run_string
	@$(MAKE) clean FAST=1 OPT=-O3 string_loop run_string

	@echo "========== Array Loop =========="
	@$(MAKE) clean FAST=0 OPT=-O0 array_loop run_array
	@$(MAKE) clean FAST=0 OPT=-O2 array_loop run_array
	@$(MAKE) clean FAST=0 OPT=-O3 array_loop run_array
	@$(MAKE) clean FAST=1 OPT=-O0 array_loop run_array
	@$(MAKE) clean FAST=1 OPT=-O2 array_loop run_array
	@$(MAKE) clean FAST=1 OPT=-O3 array_loop run_array

clean:
	rm -f $(PROGS)

.PHONY: all clean run_invariants run_string run_array

