# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Check logical ISR arrays spanning distinct, real allocated ELF sections."""

import importlib
import json
import subprocess
from pathlib import Path

import pytest
import test_isr_metadata_finalize as finalizer_tests
from elftools.elf.elffile import ELFFile

tools = finalizer_tests.tools
ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize("defect", [None, "gap", "overlap", "writable", "nonallocated"])
def test_absolute_table_aliases_cover_each_real_storage_fragment(
    tmp_path, tools, monkeypatch, defect
):
    compiler, linker = tools
    source = tmp_path / "entries.c"
    source.write_text(
        "unsigned argument = 7;\n"
        "void handler(const void *p) { argument += p != 0; }\n"
        "struct entry { const void *arg; void (*isr)(const void *); };\n"
        'const struct entry entry_a __attribute__((section(".entry_a")))'
        " = {&argument,handler};\n"
        + ("" if defect == "writable" else "const ")
        + 'struct entry entry_b __attribute__((section(".entry_b"))) = {0,handler};\n'
    )
    obj = tmp_path / "entries.o"
    subprocess.run(
        compiler + ["-ffreestanding", "-fno-pic", "-fno-pie", "-c", str(source), "-o", str(obj)],
        check=True,
        capture_output=True,
        text=True,
    )
    with obj.open("rb") as stream:
        elf = ELFFile(stream)
        symbol = elf.get_section_by_name(".symtab").get_symbol_by_name("entry_a")[0]
        size = symbol["st_size"]
        alignment = elf.get_section(symbol["st_shndx"])["sh_addralign"]
    base = 0x10000
    displacement = 4 if defect == "gap" else -4 if defect == "overlap" else 0
    second_address = base + size + displacement
    section_type = "(INFO)" if defect == "nonallocated" else ""
    script = tmp_path / "layout.ld"
    script.write_text(
        "ENTRY(handler)\nSECTIONS {\n"
        f".slot_a {base} : {{ KEEP(*(.entry_a)) }}\n"
        f".slot_b {second_address} {section_type} : {{ KEEP(*(.entry_b)) }}\n"
        ".text 0x20000 : { *(.text*) }\n.data 0x30000 : { *(.data*) }\n"
        f"_sw_isr_table = ABSOLUTE({base});\n"
        f"_sw_isr_table_end = ABSOLUTE({base + 2 * size});\n"
        + "".join(
            f"slot_{index} = ABSOLUTE({base + index * size});\n"
            f"slot_{index}_end = ABSOLUTE({base + (index + 1) * size});\n"
            for index in range(2)
        )
        + "/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) } }\n"
    )
    image = tmp_path / "image.elf"
    subprocess.run(
        linker + ["--no-check-sections", "-T", str(script), str(obj), "-o", str(image)],
        check=True,
        capture_output=True,
        text=True,
    )
    manifest = tmp_path / "layout.json"
    manifest.write_text(
        json.dumps(
            {
                "version": 1,
                "tables": [
                    {
                        "section": "logical_table",
                        "symbol": "_sw_isr_table",
                        "end_symbol": "_sw_isr_table_end",
                        "size": 2 * size,
                        "alignment": alignment,
                        "writable": False,
                        "inputs": [
                            {
                                "name": f".entry_{index}",
                                "symbol": f"slot_{index}",
                                "offset": index * size,
                                "size": size,
                                "alignment": alignment,
                            }
                            for index in range(2)
                        ],
                    }
                ],
            }
        )
    )
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    checker = importlib.import_module("check_isr_tables")
    code, report = checker.run(image, manifest)
    assert code == (0 if defect is None else 1), report
    if defect is None:
        with image.open("rb") as stream:
            elf = ELFFile(stream)
            alias = elf.get_section_by_name(".symtab").get_symbol_by_name("_sw_isr_table")[0]
            assert alias["st_shndx"] == "SHN_ABS"
