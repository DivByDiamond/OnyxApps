# Native host tests for OnyxApps pure-logic modules.
#
# These tests run the portable C99 modules (game logic, /proc parsers)
# on any Linux with gcc - no OnyxOS, onyxcc or RISC-V toolchain needed.
# Full .onx builds and ONX1 validation stay in CI (.github/workflows).
#
#   make test          build and run every test
#   make test_osnake   build one test binary
#   make clean
#
# Layout: max 4 files per folder (Makefile, README.md, two test drivers).

CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -Wno-unused-parameter
ROOT    := ../..

OSNAKE_SRC := $(ROOT)/apps/osnake/game/logic.c $(ROOT)/apps/osnake/game/food.c
OSNAKE_INC := -I $(ROOT)/apps/osnake

OTOP_SRC := $(ROOT)/apps/otop/probe/parse.c
OTOP_INC := -I $(ROOT)/apps/otop

.PHONY: test clean

test: test_osnake test_otop
	./test_osnake
	./test_otop

test_osnake: test_osnake.c $(OSNAKE_SRC)
	$(CC) $(CFLAGS) $(OSNAKE_INC) -o $@ test_osnake.c $(OSNAKE_SRC)

test_otop: test_otop.c $(OTOP_SRC)
	$(CC) $(CFLAGS) $(OTOP_INC) -o $@ test_otop.c $(OTOP_SRC)

clean:
	rm -f test_osnake test_otop
