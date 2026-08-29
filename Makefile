# OnyxApps — build all apps with OnyxCC.
#
#   make            build every app into build/
#   make vim        build one app
#   ONYXCC=... make    custom compiler path
#
# onyxcc links libonyxc automatically; only the app's own directory is
# needed on the include path (for local *.h headers).

ONYXCC ?= onyxcc
APPS   := $(notdir $(wildcard apps/*))

.PHONY: all $(APPS) clean

all: $(APPS:%=build/%.onx)

$(APPS):
	@mkdir -p build
	$(ONYXCC) -I apps/$@ -o build/$@.onx $(wildcard apps/$@/*.c)

clean:
	rm -rf build
