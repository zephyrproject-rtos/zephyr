# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Link the production discovery fragment without a target-memory scratch region."""

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
def test_discovery_is_nonallocated_and_keeps_targets(tmp_path, tools, stage):
    compiler, linker = tools
    source = tmp_path / "registration.c"
    source.write_text(
        "static volatile unsigned int calls;\n"
        "static void handler(const void *arg) { calls += arg != 0; }\n"
        "static const struct { const void *arg; void (*isr)(const void *); } entry\n"
        " __attribute__((used,section(\".irq.registration\"))) = {(const void *)&calls, handler};\n"
        "static const char records[8192] __attribute__((used,section(\".intList\"))) = {1};\n"
        "void _start(void) { for (;;) {} }\n"
    )
    obj = tmp_path / "registration.o"
    subprocess.run(
        compiler
        + [
            "-c",
            "-Os",
            "-ffunction-sections",
            "-fdata-sections",
            "-fno-pie",
            "-fno-pic",
            str(source),
            "-o",
            str(obj),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    fragment = tmp_path / "discovery.ld"
    defines = ["-DCONFIG_ISR_TABLES_LOCAL_DECLARATION=1"]
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
            str(ZEPHYR_BASE / "include/zephyr/linker/intlist.ld"),
            "-o",
            str(fragment),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    layout = tmp_path / "layout.ld"
    payload = "" if stage is None else "sw_isr_table 0x30000 : { KEEP(*(.irq.*)) }\n"
    layout.write_text(
        "ENTRY(_start)\nSECTIONS {\n"
        ".text 0x10000 : { *(.text*) }\n.data 0x20000 : { *(.data*) }\n"
        ".bss : { *(.bss*) }\n"
        + payload
        + f'INCLUDE "{fragment}"\n'
        + "/DISCARD/ : { *(.eh_frame*) *(.ARM.exidx*) *(.ARM.extab*) *(.comment) }\n}\n"
    )
    output = tmp_path / "image.elf"
    subprocess.run(
        linker + ["--gc-sections", "-T", str(layout), str(obj), "-o", str(output)],
        check=True,
        capture_output=True,
        text=True,
    )
    with output.open("rb") as stream:
        elf = ELFFile(stream)
        assert elf.get_section_by_name(".symtab").get_symbol_by_name("handler")
        records = elf.get_section_by_name(".intList")
        discovery = elf.get_section_by_name("isr_declarations")
        if stage is None:
            assert records["sh_size"] == 8192
            assert records["sh_flags"] & 2 == 0
            assert discovery["sh_flags"] & 2 == 0
            assert all(
                segment["p_type"] != "PT_LOAD" or not segment.section_in_segment(records)
                for segment in elf.iter_segments()
            )
        else:
            assert records is None
            assert discovery is None
            assert elf.get_section_by_name("sw_isr_table") is not None
