CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter -O2
APP := ../../../apps/otablet
CORE := $(APP)/core/state.c $(APP)/core/render.c $(APP)/graphics/paint.c $(APP)/graphics/raster.c $(APP)/dashboard.c
.PHONY: test clean
test: check demo
	./check
	python3 protocol.py
check: test.c $(CORE)
	$(CC) $(CFLAGS) -I$(APP) -o $@ $^
demo: $(APP)/main.c $(CORE) $(APP)/host/font.c $(APP)/host/snapshot.c
	$(CC) $(CFLAGS) -I$(APP) -o $@ $^
clean:
	rm -f check demo
