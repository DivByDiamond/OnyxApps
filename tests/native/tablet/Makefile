CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter -O2
APP := ../../../apps/otablet
CORE := $(APP)/core/state.c $(APP)/core/render.c $(APP)/graphics/paint.c $(APP)/graphics/raster.c $(APP)/dashboard.c $(APP)/config/panel.c $(APP)/device/geometry.c $(APP)/service/session.c
.PHONY: test clean
test: check demo device-test portable-test
	./check
	python3 protocol.py
check: test.c $(CORE)
	$(CC) $(CFLAGS) -I$(APP) -o $@ $^
demo: $(APP)/main.c $(CORE) $(APP)/host/font.c $(APP)/host/snapshot.c $(APP)/device/frontend.c
	$(CC) $(CFLAGS) -I$(APP) -o $@ $^
clean:
	rm -f check demo

DEVICE_FLAGS := -D__onyx__ -Dopen=mock_open -Dclose=mock_close -Dwrite=mock_write -Dlseek=mock_lseek -Dusleep=mock_usleep
.PHONY: device-test
device-test:
	$(CC) $(CFLAGS) -D_DEFAULT_SOURCE $(DEVICE_FLAGS) -include device/mock.h -I$(APP) -o /tmp/otablet-device-test device/test.c $(CORE) $(APP)/host/font.c $(APP)/device/frontend.c
	/tmp/otablet-device-test

ONYXCC ?= ../../../../OnyxCompiller/onyxcc
ONXRUN ?= ../../../../OnyxCompiller/tools/onx-run
.PHONY: portable-test target-test
portable-test:
	$(CC) $(CFLAGS) -I$(APP) -o /tmp/otablet-portable-test portable.c $(CORE)
	/tmp/otablet-portable-test
target-test:
	$(ONYXCC) -I$(APP) -o /tmp/otablet-portable.onx portable.c $(CORE)
	$(ONXRUN) /tmp/otablet-portable.onx
