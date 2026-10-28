# OnyxApps - build all apps with OnyxCC.
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
ONYXCC_FILE := $(shell command -v "$(ONYXCC)" 2>/dev/null)
APPS   := $(notdir $(wildcard apps/*))
ROOT_CUSTOM_APPS := lua luac
CUSTOM_APPS := $(notdir $(patsubst %/build.py,%,$(wildcard apps/*/build.py)))
C_APPS := $(filter-out $(CUSTOM_APPS),$(APPS))

.PHONY: all $(APPS) clean

all: $(APPS:%=build/%.onx)

define APP_RULE
build/$(1).onx: $$(shell find apps/$(1) -name '*.c')
	@mkdir -p build
	$$(ONYXCC) -I apps/$(1) -o $$@ $$^
endef
$(foreach a,$(C_APPS),$(eval $(call APP_RULE,$(a))))

# Lua and luac share the pinned source archive and custom builder.
define CUSTOM_RULE
build/$(1).onx: $$(shell find apps/$(1) apps/lua -type f) Makefile $$(ONYXCC_FILE)
	python3 apps/$(1)/build.py --compiler "$$(ONYXCC)" --output "$$@" $$(if $$(filter $(1),$$(ROOT_CUSTOM_APPS)),--ring1,)
endef
$(foreach a,$(CUSTOM_APPS),$(eval $(call CUSTOM_RULE,$(a))))

$(APPS): %: build/%.onx

clean:
	rm -rf build
