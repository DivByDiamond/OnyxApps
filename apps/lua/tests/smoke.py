#!/usr/bin/env python3
"""Exercise both Onyx Lua CLIs using the host onx-run emulator."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


def execute(runner, binary, arguments, directory, success=True):
    result = subprocess.run([str(runner), str(binary), *map(str, arguments)],
                            cwd=directory, capture_output=True, text=True, timeout=45)
    if (result.returncode == 0) != success or result.returncode < 0:
        raise AssertionError(f"{binary.name} {arguments}: exit {result.returncode}\n"
                             f"{result.stdout[-2000:]}\n{result.stderr[-4000:]}")
    if "onx-run: fault" in result.stderr or "instruction limit" in result.stderr:
        raise AssertionError(result.stderr[-4000:])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--lua", type=Path, required=True)
    parser.add_argument("--luac", type=Path, required=True)
    args = parser.parse_args()
    runner, lua, luac = (path.resolve() for path in (args.runner, args.lua, args.luac))
    fixtures = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="onyx-lua-smoke-") as temporary:
        directory = Path(temporary)
        for name in ("smoke.lua", "smoke_module.lua", "runtime.lua"):
            shutil.copyfile(fixtures / name, directory / name)
        version = execute(runner, lua, ["-v"], directory)
        assert "Lua 5.5.1" in version.stdout + version.stderr
        assert execute(runner, lua, ["-e", "print(6 * 7)"], directory).stdout == "42\n"
        expected = "lua-smoke-ok\t42\t9\n"
        assert execute(runner, lua, ["smoke.lua"], directory).stdout == expected
        execute(runner, luac, ["-o", "smoke.luac", "smoke.lua"], directory)
        assert (directory / "smoke.luac").read_bytes().startswith(b"\x1bLua")
        assert execute(runner, lua, ["smoke.luac"], directory).stdout == expected
        runtime = execute(runner, lua, ["runtime.lua"], directory)
        assert runtime.stdout.count("PASS ") == 10 and runtime.stdout.endswith("SMOKE-END\n")
        failed = execute(runner, lua, ["-e", "error('expected-smoke-error')"], directory, False)
        assert "expected-smoke-error" in failed.stderr
    print("PASS Lua version, CLI, math, require, errors, GC and luac roundtrip")


if __name__ == "__main__":
    main()
