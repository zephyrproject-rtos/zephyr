# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Resolve real Thumb literal loads emitted by ARM assemblers and linkers."""

import importlib
import subprocess

import pytest
import test_isr_metadata_finalize as finalizer_tests
from elftools.elf.descriptions import describe_reloc_type
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

tools = finalizer_tests.tools


@pytest.mark.parametrize("width,target_address", [("w", 0x10020), ("n", 0x10020)])
def test_thumb_literal_target_matches_linked_symbol(
    tools, tmp_path, monkeypatch, width, target_address
):
    compiler, linker = tools
    monkeypatch.syspath_prepend(str(finalizer_tests.ROOT / "scripts/build"))
    finalizer = importlib.import_module("isr_metadata_finalize")
    source = tmp_path / "literal.c"
    relocation_name = "R_ARM_THM_PC12" if width == "w" else "R_ARM_THM_PC8"
    immediate = "-4" if width == "w" else "1020"
    # GNU as resolves local literal pools before linking. Declare the standard
    # relocation explicitly so the real linker writes the final PC-relative
    # instruction; no executable bytes or ELF headers are edited afterwards.
    source.write_text(
        'const unsigned literal __attribute__((section(".literal"),used)) = 0x12345678;\n'
        'void loadliteral(void) __attribute__((naked,used));\n'
        'void loadliteral(void) { __asm__ volatile(".syntax unified\\n.Lload:\\nldr.'
        + width
        + ' r0,[pc,#'
        + immediate
        + ']\\n.reloc .Lload,'
        + relocation_name
        + ',literal\\nbx lr"); }\n'
    )
    obj, image = tmp_path / "literal.o", tmp_path / "literal.elf"
    subprocess.run(
        compiler
        + [
            "-mcpu=cortex-m3",
            "-mthumb",
            "-Os",
            "-ffreestanding",
            "-fno-pic",
            "-fno-pie",
            "-c",
            str(source),
            "-o",
            str(obj),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    script = tmp_path / "literal.ld"
    script.write_text(
        "ENTRY(loadliteral)\nSECTIONS { .text 0x10002 : { *(.text*) }\n"
        f".literal {target_address} : {{ KEEP(*(.literal)) }}\n"
        "/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) } }\n"
    )
    subprocess.run(
        linker + ["--emit-relocs", "-T", str(script), str(obj), "-o", str(image)],
        check=True,
        capture_output=True,
        text=True,
    )
    with image.open("rb") as stream:
        elf = ELFFile(stream)
        found = []
        for section in elf.iter_sections():
            if not isinstance(section, RelocationSection):
                continue
            symbol_table = elf.get_section(section["sh_link"])
            for relocation in section.iter_relocations():
                name = describe_reloc_type(relocation["r_info_type"], elf)
                if name not in {"R_ARM_THM_PC12", "R_ARM_THM_PC8"}:
                    continue
                symbol = symbol_table.get_symbol(relocation["r_info_sym"])
                assert relocation["r_offset"] % 4 == 2
                expected = "R_ARM_THM_PC12" if width == "w" else "R_ARM_THM_PC8"
                assert name == expected
                target = finalizer.relocation_target(
                    elf, elf.get_section(section["sh_info"]), relocation, symbol
                )
                assert target == symbol["st_value"] == target_address
                found.append(name)
        assert len(found) == 1
