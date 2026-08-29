# OnyxApps — build all apps with OnyxCC.
#
#   make            build every app into build/
#   make vim        build one app
#   ONYXCC=... make    custom compiler path
#
# onyxcc links libonyxc automatically; only the app's own directory tree
# is needed on the include path (for local *.h headers).
#
# App layout rules (project-wide): max 4 files per folder, max 200 lines
# per file, one responsibility per file (see apps/vim for the reference).

ONYXCC ?= onyxcc
APPS   := $(notdir $(wildcard apps/*))

.PHONY: all $(APPS) clean

all: $(APPS:%=build/%.onx)

define APP_RULE
build/$(1).onx: $$(shell find apps/$(1) -name '*.c')
	@mkdir -p build
	$$(ONYXCC) -I apps/$(1) -o $$@ $$^
endef
$(foreach a,$(APPS),$(eval $(call APP_RULE,$(a))))

$(APPS):
	@mkdir -p build
	$(ONYXCC) -I apps/$@ -o build/$@.onx $(shell find apps/$@ -name '*.c')

clean:
	rm -rf build
