# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Preserve SoC-owned hardware vectors when only software tables are generated."""

import os
import shlex
import shutil
import subprocess
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile

ZEPHYR_BASE = Path(__file__).resolve().parents[3]


@pytest.fixture
def tools():
    compiler = shlex.split(os.environ.get("ISR_DISCOVERY_CC", "cc"))
    linker = shlex.split(os.environ.get("ISR_DISCOVERY_LD", "ld"))
    if not shutil.which(compiler[0]) or not shutil.which(linker[0]):
        pytest.skip("An ELF compiler and GNU-compatible linker are required")
    version = subprocess.run(linker + ["--version"], capture_output=True, text=True)
    if not any(name in version.stdout for name in ("GNU ld", "LLD", "ELD")):
        pytest.skip("Set ISR_DISCOVERY_CC and ISR_DISCOVERY_LD to ELF tools")
    return compiler, linker


@pytest.mark.parametrize("stage", [None, "LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_FINAL"])
@pytest.mark.parametrize("architecture", ["arm", "arm64", "riscv"])
def test_manual_hardware_vector_retained(tmp_path, tools, stage, architecture):
    compiler, linker = tools
    if architecture in ("arm", "arm64"):
        manual_section = ".isr_vector_table" if architecture == "arm" else ".vectors"
        declarations = (
            'const unsigned int exceptions[16] '
            '__attribute__((used,section(".exc_vector_table"))) = {0};\n'
            'const unsigned int manual_vectors[16] '
            f'__attribute__((used,section("{manual_section}"))) = {{0x12345678}};\n'
        )
        fragments = [
            "arch/arm/core/vector_table.ld",
            "soc/infineon/cat1c/cyt4dn/ld/vector_table.ld",
        ]
        if architecture == "arm64":
            # Exercise the actual vector clause of the monolithic architecture script.
            full_script = (ZEPHYR_BASE / "include/zephyr/arch/arm64/scripts/linker.ld").read_text()
            clause = full_script.split("_vector_start = .;", 1)[1].split("_vector_end = .;", 1)[0]
            fragment = tmp_path / "arm64-vectors.ld"
            fragment.write_text("_vector_start = .;\n" + clause + "_vector_end = .;\n")
            fragments = [str(fragment)]
        expected_offset, vector_size = 64, 64
    else:
        declarations = (
            'const unsigned int manual_vectors[36] '
            '__attribute__((used,section(".vectors.ivt"))) = {0x12345678};\n'
        )
        fragments = ["arch/riscv/core/vector_table.ld"]
        expected_offset, vector_size = 0, 144
    source = tmp_path / "vectors.c"
    source.write_text(declarations + 'void _start(void) { for (;;) {} }\n')
    obj = tmp_path / "vectors.o"
    subprocess.run(
        compiler + ["-c", "-Os", "-fno-pie", "-fno-pic", str(source), "-o", str(obj)],
        check=True,
        capture_output=True,
        text=True,
    )
    script = tmp_path / "layout.in"
    includes = "\n".join(f'#include "{ZEPHYR_BASE / path}"' for path in fragments)
    script.write_text(
        '#include <zephyr/linker/sections.h>\n'
        'ENTRY(_start)\nSECTIONS {\n.rom_start 0x10000 : {\n'
        'manual_start = .;\n' + includes + '\nmanual_end = .;\n}\n'
        '.text : { *(.text*) }\n'
        '#include <zephyr/linker/isr-local-drop-unused.ld>\n'
        '/DISCARD/ : { *(.eh_frame*) *(.comment) }\n}\n'
    )
    # This file must not be selected: GEN_IRQ_VECTOR_TABLE is deliberately disabled.
    (tmp_path / "isr_tables_vt.ld").write_text('unexpected_generated_vectors = .;\n')
    layout = tmp_path / "layout.ld"
    defines = ["-D_LINKER", "-DCONFIG_ISR_TABLES_LOCAL_DECLARATION=1", "-DCONFIG_NUM_IRQS=6368"]
    if stage is not None:
        defines.append(f"-D{stage}=1")
    subprocess.run(
        compiler
        + [
            "-E",
            "-P",
            "-x",
            "assembler-with-cpp",
            *defines,
            "-I",
            str(ZEPHYR_BASE / "include"),
            str(script),
            "-o",
            str(layout),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    output = tmp_path / "image.elf"
    subprocess.run(
        linker
        + ["--gc-sections", "-L", str(tmp_path), "-T", str(layout), str(obj), "-o", str(output)],
        check=True,
        capture_output=True,
        text=True,
    )
    with output.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = elf.get_section_by_name(".symtab")
        assert symbols.get_symbol_by_name("unexpected_generated_vectors") is None
        vectors = symbols.get_symbol_by_name("manual_vectors")
        assert len(vectors or []) == 1
        assert vectors[0]["st_value"] == 0x10000 + expected_offset
        assert vectors[0]["st_size"] == vector_size
        assert symbols.get_symbol_by_name("manual_end")[0]["st_value"] == (
            0x10000 + expected_offset + vector_size
        )
        section = elf.get_section_by_name(".rom_start")
        assert (
            int.from_bytes(
                section.data()[expected_offset : expected_offset + 4],
                "little" if elf.little_endian else "big",
            )
            == 0x12345678
        )
        if architecture in ("arm", "arm64"):
            assert symbols.get_symbol_by_name("_vector_start")[0]["st_value"] == 0x10000
            assert symbols.get_symbol_by_name("_vector_end")[0]["st_value"] == 0x10080


@pytest.mark.parametrize("stage", [None, "LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_FINAL"])
def test_arm64_generated_vector_stage_selection(tmp_path, tools, stage):
    compiler, linker = tools
    source = tmp_path / "vectors.c"
    source.write_text(
        'const unsigned int placeholder[16] '
        '__attribute__((used,section(".isr_pre0.vectors"))) = {1};\n'
        'const unsigned int generated[16] '
        '__attribute__((used,section(".irq.vector.0"))) = {2};\n'
        'void _start(void) { for (;;) {} }\n'
    )
    obj = tmp_path / "vectors.o"
    subprocess.run(
        compiler + ["-c", "-Os", "-fno-pie", "-fno-pic", str(source), "-o", str(obj)],
        check=True,
        capture_output=True,
        text=True,
    )
    full_script = (ZEPHYR_BASE / "include/zephyr/arch/arm64/scripts/linker.ld").read_text()
    clause = full_script.split("_vector_start = .;", 1)[1].split("_vector_end = .;", 1)[0]
    script = tmp_path / "layout.in"
    script.write_text(
        'ENTRY(_start)\nSECTIONS {\n.vectors 0x10000 : {\n'
        + clause
        + '\n}\n.text : { *(.text*) }\n'
        '/DISCARD/ : { *(.eh_frame*) *(.comment) }\n}\n'
    )
    (tmp_path / "isr_tables_vt.ld").write_text('KEEP(*(.irq.vector.0))\n')
    layout = tmp_path / "layout.ld"
    defines = ["-DCONFIG_ISR_TABLES_LOCAL_DECLARATION=1", "-DCONFIG_GEN_IRQ_VECTOR_TABLE=1"]
    if stage is not None:
        defines.append(f"-D{stage}=1")
    subprocess.run(
        compiler
        + ["-E", "-P", "-x", "assembler-with-cpp", *defines, str(script), "-o", str(layout)],
        check=True,
        capture_output=True,
        text=True,
    )
    output = tmp_path / "image.elf"
    subprocess.run(
        linker
        + ["--gc-sections", "-L", str(tmp_path), "-T", str(layout), str(obj), "-o", str(output)],
        check=True,
        capture_output=True,
        text=True,
    )
    with output.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = elf.get_section_by_name(".symtab")
        assert bool(symbols.get_symbol_by_name("placeholder")) is (stage is None)
        assert bool(symbols.get_symbol_by_name("generated")) is (stage is not None)
        assert elf.get_section_by_name(".vectors")["sh_size"] == 64
