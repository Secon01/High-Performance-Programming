.DEFAULT_GOAL := all

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11
TIME    = time

OPT_00  = -O0
OPT_02  = -O2
OPT_03  = -O3

PROGS = regularcode mallocycode sleepycode threadedcode

regularcode: regularcode.c
	$(CC) $(CFLAGS) $(OPT) $< -o $@

mallocycode: mallocycode.c
	$(CC) $(CFLAGS) $(OPT) $< -o $@

sleepycode: sleepycode.c
	$(CC) $(CFLAGS) $(OPT) $< -o $@

threadedcode: threadedcode.c
	$(CC) $(CFLAGS) $(OPT) $< -o $@ -pthread

clean:
	rm -f $(PROGS)

run_regular:
	@for i in 1 2 3 4 5; do $(TIME) ./regularcode > /dev/null; done

run_mallocycode:
	@for i in 1 2 3 4 5; do $(TIME) ./mallocycode > /dev/null; done

run_sleepy:
	@for i in 1 2 3 4 5; do $(TIME) ./sleepycode > /dev/null; done

run_threaded:
	@for i in 1 2 3 4 5; do $(TIME) ./threadedcode > /dev/null; done

all:
	@echo "============================="
	@echo " regularcode: O0 / O2 / O3"
	@echo "============================="
	@$(MAKE) clean OPT=$(OPT_00) regularcode run_regular
	@$(MAKE) clean OPT=$(OPT_02) regularcode run_regular
	@$(MAKE) clean OPT=$(OPT_03) regularcode run_regular

	@echo "============================="
	@echo " mallocycode: O0 / O2 / O3"
	@echo "============================="
	@$(MAKE) clean OPT=$(OPT_00) mallocycode run_mallocycode
	@$(MAKE) clean OPT=$(OPT_02) mallocycode run_mallocycode
	@$(MAKE) clean OPT=$(OPT_03) mallocycode run_mallocycode

	@echo "============================="
	@echo " sleepycode: O0 / O2 / O3"
	@echo "============================="
	@$(MAKE) clean OPT=$(OPT_00) sleepycode run_sleepy
	@$(MAKE) clean OPT=$(OPT_02) sleepycode run_sleepy
	@$(MAKE) clean OPT=$(OPT_03) sleepycode run_sleepy

	@echo "============================="
	@echo " threadedcode: O0 / O2 / O3"
	@echo "============================="
	@$(MAKE) clean OPT=$(OPT_00) threadedcode run_threaded
	@$(MAKE) clean OPT=$(OPT_02) threadedcode run_threaded
	@$(MAKE) clean OPT=$(OPT_03) threadedcode run_threaded

