#!/usr/bin/env python3

# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Audits a build against a kernel feature scope.

A scope description (kernel/scopes/*.yaml) lists the Kconfig symbols, the
sources and the ELF symbols that a build selecting that scope must not
contain. The script checks the generated .config, the compile commands
database and the final ELF against it and writes a manifest recording the
scope, the configuration, the compiled sources and the ELF, each with its
SHA-256, so that a build can be traced back to its inputs or signed by a
release pipeline.

The build system runs it after the final link when CONFIG_KERNEL_SCOPE_DEFINITION
is set; a non-zero exit code fails the build.
"""

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

import yaml
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

SYMBOL_TYPES = ("STT_FUNC", "STT_OBJECT")


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument("--scope", required=True, type=Path, help="scope description (YAML)")
    parser.add_argument("--zephyr-base", required=True, type=Path, help="Zephyr base directory")
    parser.add_argument("--dotconfig", required=True, type=Path, help="generated .config")
    parser.add_argument(
        "--compile-commands", required=True, type=Path, help="compile_commands.json of the build"
    )
    parser.add_argument("--elf", required=True, type=Path, help="final ELF file")
    parser.add_argument("--manifest", type=Path, help="write the build manifest (JSON) here")
    return parser.parse_args()


def sha256_of(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_scope(path):
    with open(path, encoding="utf-8") as f:
        scope = yaml.safe_load(f)
    for key in ("name", "kconfig", "excluded_kconfig", "excluded_sources", "excluded_symbols"):
        if key not in scope:
            sys.exit(f"{path}: missing '{key}'")
    return scope


def load_dotconfig(path):
    """Return {symbol: value} for the symbols set in a .config file."""
    config = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = re.match(r"^CONFIG_(\w+)=(.*)$", line.strip())
            if m:
                config[m.group(1)] = m.group(2)
    return config


def check_kconfig(scope, config):
    errors = []
    if config.get(scope["kconfig"]) != "y":
        errors.append(f"CONFIG_{scope['kconfig']} is not enabled in the build")
    for symbol in scope["excluded_kconfig"]:
        if symbol in config:
            errors.append(f"CONFIG_{symbol}={config[symbol]} is outside scope '{scope['name']}'")
    return errors


def relative_source(path, zephyr_base):
    """Return 'path' relative to the Zephyr base, or None if it is outside."""
    try:
        return Path(path).resolve().relative_to(zephyr_base.resolve()).as_posix()
    except ValueError:
        return None


def compiled_sources(compile_commands, zephyr_base):
    """Return the compiled sources, as paths relative to the Zephyr base when
    they live there and absolute otherwise, without duplicates."""
    with open(compile_commands, encoding="utf-8") as f:
        entries = json.load(f)
    sources = set()
    for entry in entries:
        path = Path(entry["directory"], entry["file"])
        rel = relative_source(path, zephyr_base)
        sources.add(rel if rel is not None else path.resolve().as_posix())
    return sorted(sources)


def check_sources(scope, sources):
    errors = []
    for src in sources:
        for excluded in scope["excluded_sources"]:
            if src == excluded or (excluded.endswith("/") and src.startswith(excluded)):
                errors.append(f"{src} is compiled although outside scope '{scope['name']}'")
    return errors


def defined_symbols(elf_path):
    """Yield (name, type) for every function and object defined in the ELF."""
    with open(elf_path, "rb") as f:
        elf = ELFFile(f)
        for section in elf.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue
            for sym in section.iter_symbols():
                info = sym.entry["st_info"]
                if info["type"] in SYMBOL_TYPES and sym.entry["st_shndx"] != "SHN_UNDEF":
                    yield sym.name, info["type"]


def check_symbols(scope, elf_path):
    patterns = [re.compile(p) for p in scope["excluded_symbols"]]
    errors = []
    count = 0
    for name, sym_type in defined_symbols(elf_path):
        count += 1
        for pattern in patterns:
            if pattern.search(name):
                errors.append(
                    f"symbol {name} ({sym_type}) matches '{pattern.pattern}', "
                    f"outside scope '{scope['name']}'"
                )
                break
    return errors, count


def git_revision(zephyr_base):
    try:
        cp = subprocess.run(
            ["git", "-C", str(zephyr_base), "rev-parse", "HEAD"],
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError:
        return None
    return cp.stdout.strip() if cp.returncode == 0 else None


def write_manifest(path, args, scope, sources, symbol_count, results):
    def hashed(src):
        full = args.zephyr_base / src if not Path(src).is_absolute() else Path(src)
        return {"path": src, "sha256": sha256_of(full)}

    manifest = {
        "scope": scope["name"],
        "scope_file": {
            "path": relative_source(args.scope, args.zephyr_base) or str(args.scope),
            "sha256": sha256_of(args.scope),
        },
        "zephyr_revision": git_revision(args.zephyr_base),
        "dotconfig": {"path": str(args.dotconfig), "sha256": sha256_of(args.dotconfig)},
        "elf": {
            "path": str(args.elf),
            "sha256": sha256_of(args.elf),
            "defined_symbols": symbol_count,
        },
        "sources": [hashed(src) for src in sources],
        "checks": {name: ("pass" if not errs else "fail") for name, errs in results.items()},
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")


def main():
    args = parse_args()
    scope = load_scope(args.scope)

    config = load_dotconfig(args.dotconfig)
    sources = compiled_sources(args.compile_commands, args.zephyr_base)
    symbol_errors, symbol_count = check_symbols(scope, args.elf)
    results = {
        "kconfig": check_kconfig(scope, config),
        "sources": check_sources(scope, sources),
        "symbols": symbol_errors,
    }

    if args.manifest:
        write_manifest(args.manifest, args, scope, sources, symbol_count, results)

    errors = [err for errs in results.values() for err in errs]
    for err in errors:
        print(f"kernel scope '{scope['name']}': {err}", file=sys.stderr)
    if errors:
        sys.exit(f"kernel scope '{scope['name']}': {len(errors)} violation(s), see above")

    print(
        f"kernel scope '{scope['name']}': {len(sources)} sources and "
        f"{symbol_count} symbols audited, no violations"
    )


if __name__ == "__main__":
    main()
