# Lua 5.5.1 for OnyxOS

This builds the unmodified standard Lua interpreter and standard libraries
with OnyxCC and libonyxc. The MIT-licensed upstream archive, checksum and
license are in [vendor](vendor/README.md). Builds run offline and verify the
archive before extracting temporary sources. The helper also builds
[luac](../luac/README.md); upstream C files are not copied into this tree.

Build from the OnyxApps root:

```sh
make lua luac ONYXCC=/absolute/path/to/OnyxCompiller/onyxcc
```

Outputs are `build/lua.onx` and `build/luac.onx`. Custom builders also accept
`--compiler` and `--output` explicitly. The system builds both applications for ring 1. Root-shell launches retain
root access to system scripts and process creation; ordinary-user launches
remain subject to the kernel privilege policy. The OnyxOS image builder invokes these same builders.

On OnyxOS:

```sh
lua -v
lua -e 'print("hello", 6 * 7)'
lua script.lua
luac -o script.luac script.lua
lua script.luac
lua /usr/share/lua/5.5/hello.lua
lua /usr/share/lua/5.5/selftest.lua
```

Pure Lua `require` searches `/usr/share/lua/5.5/?.lua`,
`/usr/share/lua/5.5/?/init.lua`, `./?.lua` and `./?/init.lua`. Lua's usual
`LUA_PATH_5_5`/`LUA_PATH` environment overrides remain available. The port
uses the generic upstream platform configuration: native `.so` modules,
`dlopen`, readline and POSIX-specific Lua extensions are not enabled.
Bytecode is specific to compatible Lua versions and numeric configurations.

Standard libraries depend on the available libonyxc and kernel services;
process launching, file operations, environment and terminal behavior need
validation on the target system. A host emulator smoke test is provided:

```sh
python3 apps/lua/tests/smoke.py --runner /path/to/OnyxCompiller/tools/onx-run \
  --lua build/lua.onx --luac build/luac.onx
```

It checks CLI/version output, math, pure Lua modules, reported errors,
garbage collection, metamethods, coroutines, UTF-8/packing, file I/O and a
bytecode compilation/execution roundtrip. Emulator
results do not establish QEMU or OC2R hardware compatibility.
