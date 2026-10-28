# luac 5.5.1 for OnyxOS

The standard Lua bytecode compiler uses the same verified upstream archive
and shared build helper as [lua](../lua/README.md), under Lua's MIT license.
No network access or separate upstream source checkout is needed.

```sh
make luac ONYXCC=/absolute/path/to/OnyxCompiller/onyxcc
luac -o program.luac program.lua
lua program.luac
```

The build output is `build/luac.onx`. `luac -l program.luac` lists bytecode;
`luac -p program.lua` checks syntax. Bytecode requires a compatible Lua
version, endianness and numeric configuration. This port uses 64-bit
integers and double-precision numbers, matching the interpreter build.
