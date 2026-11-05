#!/usr/bin/env python3
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Expose RESOLVE public names in the nonallocated ELF symbol table.

armlink resolves runtime relocations to its Image/Load symbols and removes
undefined public names. Export their values from this same completed link for
ELF consumers; never reconstruct addresses from a preceding link.
"""

import argparse
import hashlib
import json
import os
import re
import tempfile
from io import BytesIO
from pathlib import Path

from elftools.construct import Container
from elftools.elf.elffile import ELFFile

RESOLVE = re.compile(r"^RESOLVE\s+(\S+)\s+AS\s+(\S+)\s*$", re.MULTILINE)
SHF_ALLOC = 2


def load_image(elf):
    return [
        {
            key: segment[key]
            for key in (
                "p_type",
                "p_offset",
                "p_vaddr",
                "p_paddr",
                "p_filesz",
                "p_memsz",
                "p_flags",
                "p_align",
            )
        }
        | {"sha256": hashlib.sha256(segment.data()).hexdigest()}
        for segment in elf.iter_segments()
    ]


def transform(data, steering):
    elf = ELFFile(BytesIO(data))
    if elf["e_type"] != "ET_EXEC" or elf["e_machine"] != "EM_ARM":
        raise ValueError("armlink symbol export requires an ARM executable")
    tables = [
        (index, section)
        for index, section in enumerate(elf.iter_sections())
        if section["sh_type"] == "SHT_SYMTAB"
    ]
    if len(tables) != 1:
        raise ValueError("armlink symbol export requires one static symbol table")
    symbol_index, symbols = tables[0]
    strings = elf.get_section(symbols["sh_link"])
    if strings["sh_type"] != "SHT_STRTAB":
        raise ValueError("Invalid symbol string table")
    for section in (symbols, strings):
        if section["sh_flags"] & SHF_ALLOC:
            raise ValueError("Symbol export cannot replace allocated tables")
        for segment in elf.iter_segments():
            if (
                segment["p_type"] == "PT_LOAD"
                and section["sh_size"]
                and section["sh_offset"] < segment["p_offset"] + segment["p_filesz"]
                and segment["p_offset"] < section["sh_offset"] + section["sh_size"]
            ):
                raise ValueError("Symbol export tables overlap a load segment")
    by_name = {}
    for symbol in symbols.iter_symbols():
        by_name.setdefault(symbol.name, []).append(symbol)
    mappings = {}
    for public, canonical in RESOLVE.findall(steering):
        if public in mappings and mappings[public] != canonical:
            raise ValueError(f"Conflicting armlink mappings for {public}")
        mappings[public] = canonical
    if not mappings:
        raise ValueError("No RESOLVE mappings in armlink steering file")
    original_loads = load_image(elf)
    string_data = bytearray(strings.data())
    symbol_data = bytearray(symbols.data())
    added = {}
    for public, canonical in mappings.items():
        candidates = [
            symbol
            for symbol in by_name.get(canonical, [])
            if symbol["st_shndx"] != "SHN_UNDEF"
            and symbol["st_info"]["bind"] in {"STB_GLOBAL", "STB_WEAK"}
        ]
        if len(candidates) != 1:
            raise ValueError(f"Missing or ambiguous armlink target {canonical}")
        target = candidates[0]
        existing = [
            symbol for symbol in by_name.get(public, []) if symbol["st_shndx"] != "SHN_UNDEF"
        ]
        if existing:
            if (
                len(existing) != 1
                or existing[0]["st_value"] != target["st_value"]
                or existing[0]["st_info"]["bind"] not in {"STB_GLOBAL", "STB_WEAK"}
            ):
                raise ValueError(f"Conflicting existing armlink symbol {public}")
            continue
        if by_name.get(public):
            raise ValueError(f"Unresolved armlink public symbol {public}")
        name_offset = len(string_data)
        string_data.extend(public.encode("ascii") + b"\0")
        record = target.entry.copy()
        record["st_name"] = name_offset
        record["st_size"] = 0
        record["st_info"] = Container(bind="STB_GLOBAL", type="STT_NOTYPE")
        record["st_other"] = Container(visibility="STV_DEFAULT", local=0)
        symbol_data.extend(elf.structs.Elf_Sym.build(record))
        added[public] = {"canonical": canonical, "value": target["st_value"]}
    if not added:
        return data, {"exported": {}, "program_image_unchanged": True, "load_image": original_loads}
    result = bytearray(data)
    for index, section, payload in (
        (symbols["sh_link"], strings, string_data),
        (symbol_index, symbols, symbol_data),
    ):
        alignment = max(1, section["sh_addralign"])
        result.extend(b"\0" * (-len(result) % alignment))
        header = section.header.copy()
        header["sh_offset"], header["sh_size"] = len(result), len(payload)
        result.extend(payload)
        offset = elf["e_shoff"] + index * elf["e_shentsize"]
        result[offset : offset + elf["e_shentsize"]] = elf.structs.Elf_Shdr.build(header)
    after = ELFFile(BytesIO(result))
    if load_image(after) != original_loads:
        raise ValueError("armlink symbol export changed the program image")
    before_allocated = [
        (section.name, section.header, section.data())
        for section in elf.iter_sections()
        if section["sh_flags"] & SHF_ALLOC
    ]
    after_allocated = [
        (section.name, section.header, section.data())
        for section in after.iter_sections()
        if section["sh_flags"] & SHF_ALLOC
    ]
    if before_allocated != after_allocated:
        raise ValueError("armlink symbol export changed allocated sections")
    return bytes(result), {
        "exported": added,
        "program_image_unchanged": True,
        "load_image": original_loads,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--steering", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    temporary = None
    try:
        data, report = transform(args.elf.read_bytes(), args.steering.read_text())
        with tempfile.NamedTemporaryFile(dir=args.elf.parent, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
        os.chmod(temporary, args.elf.stat().st_mode)
        temporary.replace(args.elf)
        temporary = None
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    except (ValueError, KeyError, UnicodeError) as error:
        parser.exit(1, f"armlink symbol export refused: {error}; ELF unchanged\n")
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
