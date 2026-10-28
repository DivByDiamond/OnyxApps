#!/usr/bin/env python3
"""Build either Lua CLI from the checked-in, checksum-pinned upstream archive."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile

VERSION = "5.5.1"
SHA256 = "1c4b4068d67061f2a2231ad2b5422e77acea1487ea9890f6320af614f4373dce"
ARCHIVE = Path(__file__).resolve().parents[1] / "vendor" / f"lua-{VERSION}.tar.gz"


def extract(destination):
    """Verify before extracting; reject paths, links and special files."""
    digest = hashlib.sha256(ARCHIVE.read_bytes()).hexdigest()
    if digest != SHA256:
        raise ValueError(f"Lua archive checksum mismatch: {digest}")
    with tarfile.open(ARCHIVE, "r:gz") as archive:
        for member in archive.getmembers():
            path = destination / member.name
            if not path.resolve().is_relative_to(destination.resolve()):
                raise ValueError(f"Unsafe archive path: {member.name}")
            if not (member.isfile() or member.isdir()):
                raise ValueError(f"Unsupported archive entry: {member.name}")
        archive.extractall(destination, filter="data")
    return destination / f"lua-{VERSION}" / "src"


def build(application):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--ring1", action="store_true")
    args = parser.parse_args()
    located = shutil.which(args.compiler)
    if not located:
        raise ValueError(f"Compiler not found: {args.compiler}")
    compiler = Path(located).resolve()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f"{application}-", dir=output.parent) as temporary:
        directory = Path(temporary)
        source = extract(directory)
        omitted = "luac.c" if application == "lua" else "lua.c"
        sources = sorted(path for path in source.glob("*.c") if path.name != omitted)
        product = directory / f"{application}.onx"
        search = "/usr/share/lua/5.5/?.lua;/usr/share/lua/5.5/?/init.lua;./?.lua;./?/init.lua"
        ring = ["--ring1"] if args.ring1 else []
        subprocess.run([str(compiler), *ring, "-I", str(source),
                        f'-DLUA_PATH_DEFAULT="{search}"', '-DLUA_CPATH_DEFAULT=""',
                        "-o", str(product), *map(str, sources)], check=True)
        if not product.is_file() or product.read_bytes()[:4] != b"ONX1":
            raise ValueError(f"Compiler did not produce a valid {application} executable")
        product.replace(output)
    print(f"Built {output} from Lua {VERSION} ({SHA256})")
