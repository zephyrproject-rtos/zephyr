# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Check SoC-owned CYT4DN vectors without creating a C reference to them."""

import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile

CPU_IRQ_COUNT = 16
CORE_EXCEPTION_COUNT = 16
VECTOR_BYTES = 4


def unique(symbols, name):
    matches = symbols.get_symbol_by_name(name) or []
    if len(matches) != 1:
        raise ValueError(f"{name}: expected one definition, found {len(matches)}")
    return matches[0]


def check(path):
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = elf.get_section_by_name(".symtab")
        table = unique(symbols, "_irq_vector_table")
        wrapper = unique(symbols, "_isr_wrapper")["st_value"]
        start = unique(symbols, "_vector_start")["st_value"]
        end = unique(symbols, "_vector_end")["st_value"]
        size = CPU_IRQ_COUNT * VECTOR_BYTES
        if table["st_info"]["type"] != "STT_OBJECT" or table["st_size"] != size:
            raise ValueError(f"{path.name}: manual table must contain {CPU_IRQ_COUNT} pointers")
        if table["st_value"] != start + CORE_EXCEPTION_COUNT * VECTOR_BYTES:
            raise ValueError(f"{path.name}: manual table does not follow the exception header")
        if end != table["st_value"] + size:
            raise ValueError(f"{path.name}: vector end does not follow the manual table")
        section = elf.get_section(table["st_shndx"])
        offset = table["st_value"] - section["sh_addr"]
        data = section.data()[offset : offset + size]
        byte_order = "little" if elf.little_endian else "big"
        if len(data) != size or any(
            int.from_bytes(data[i : i + VECTOR_BYTES], byte_order) != wrapper
            for i in range(0, size, VECTOR_BYTES)
        ):
            raise ValueError(f"{path.name}: manual vector target differs from _isr_wrapper")
        return table["st_value"], size


def main():
    build = Path(sys.argv[1])
    layouts = [check(build / f"{stage}.elf") for stage in ("zephyr_pre0", "zephyr_pre1", "zephyr")]
    if len(set(layouts)) != 1:
        raise ValueError("Manual vector address/size changed across link stages")
    print("Manual hardware vectors retained in all three link stages")


if __name__ == "__main__":
    main()
