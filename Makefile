# OnyxApps - build all apps with OnyxCC.
#
#   make            build every app into build/
#   make build      alias for the default target (unified ecosystem
#                   entry point)
#   make test       native host tests, no onyxcc or RISC-V toolchain
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
# otablet persists notes under /users, and the kernel only lets ring 1 create
# files; build it as a root-ring system app like osh and login.
ROOT_APPS := otablet mux
CUSTOM_APPS := $(notdir $(patsubst %/build.py,%,$(wildcard apps/*/build.py)))
C_APPS := $(filter-out $(CUSTOM_APPS),$(APPS))

.PHONY: all build test $(APPS) clean

all: $(APPS:%=build/%.onx)

# Unified ecosystem entry points (make build / make test in every repo).
# test delegates to tests/native: pure-logic host tests that need only
# gcc. Full .onx builds and ONX1 validation stay in CI.
build: all

test:
	$(MAKE) -C tests/native test

define APP_RULE
build/$(1).onx: $$(shell find apps/$(1) -name '*.c')
	@mkdir -p build
	$$(ONYXCC) $$(if $$(filter $(1),$$(ROOT_APPS)),--ring1) -I apps/$(1) -o $$@ $$^
endef
$(foreach a,$(C_APPS),$(eval $(call APP_RULE,$(a))))

# Lua and luac share the pinned source archive and custom builder. ochat and
# ochat-gui use their own build.py to add lib/ochat, lib/oc2rpc (and the
# otablet toolkit for ochat-gui); both run in the user ring: /dev/hvc0 opens for uid 0 without ring 1.
define CUSTOM_RULE
build/$(1).onx: $$(shell find apps/$(1) apps/lua lib apps/otablet -type f) Makefile $$(ONYXCC_FILE)
	python3 apps/$(1)/build.py --compiler "$$(ONYXCC)" --output "$$@" $$(if $$(filter $(1),$$(ROOT_CUSTOM_APPS)),--ring1,)
endef
$(foreach a,$(CUSTOM_APPS),$(eval $(call CUSTOM_RULE,$(a))))

$(APPS): %: build/%.onx

clean:
	rm -rf build
