# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Use the CMake layout base even when a linker emits separate vector sections."""

import subprocess
import sys
from pathlib import Path

import pytest
import test_isr_metadata_finalize as finalizer_tests

tools = finalizer_tests.tools
ROOT = Path(__file__).resolve().parents[3]
ROM_START = 0x1000
EXCEPTION_OFFSET = 128
EXCEPTION_BYTES = 64


@pytest.mark.parametrize("split", [False, True])
@pytest.mark.parametrize("mode", ["cmake", "missing_base", "template"])
def test_cortex_m_placement_uses_common_layout_symbol(tmp_path, tools, split, mode):
    compiler, linker = tools
    include = tmp_path / "include/zephyr"
    include.mkdir(parents=True)
    (include / "device.h").write_text("struct device;\n")
    settings = {
        "CONFIG_ARM": 1,
        "CONFIG_CPU_CORTEX_M": 1,
        "CONFIG_ISR_TABLES_LOCAL_DECLARATION": 1,
        "CONFIG_GEN_IRQ_VECTOR_TABLE": 1,
        "CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS": 1,
        "CONFIG_GEN_SW_ISR_TABLE": 1,
        "CONFIG_GEN_SW_ISR_TABLE_ARRAY": 1,
        "CONFIG_NUM_IRQS": 2,
        "CONFIG_GEN_IRQ_START_VECTOR": 0,
        "CONFIG_ARCH_SW_ISR_TABLE_ALIGN": 4,
        "CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN": 4,
        "CONFIG_ASSERT_LEVEL": 0,
    }
    if mode != "template":
        settings["CONFIG_CMAKE_LINKER_GENERATOR"] = 1
    config = tmp_path / "config.h"
    config.write_text("".join(f"#define {key} {value}\n" for key, value in settings.items()))
    source = tmp_path / "vectors.c"
    source.write_text(
        "#include <zephyr/sw_isr_table.h>\n"
        "void _isr_wrapper(void) {}\nvoid z_irq_spurious(const void *p) { (void)p; }\n"
        'const unsigned char exceptions[64] __attribute__((section(".exceptions"))) = {0};\n'
        'const void *const _irq_vector_table[2] __attribute__((section(".irq_vectors")))'
        " = {_isr_wrapper, _isr_wrapper};\n"
        "static __used void configuration(void) {\n"
        + "".join(
            f"GEN_ABSOLUTE_SYM({key}, {value});\n"
            for key, value in settings.items()
            if value and key != "CONFIG_ARM"
        )
        + "}\n"
    )
    objects = []
    for index, unit in enumerate([source, ROOT / "arch/common/isr_metadata.c"]):
        obj = tmp_path / f"unit-{index}.o"
        result = subprocess.run(
            compiler
            + [
                "-ffreestanding",
                "-Os",
                "-include",
                str(config),
                "-I",
                str(tmp_path / "include"),
                "-I",
                str(ROOT / "include"),
                "-c",
                str(unit),
                "-o",
                str(obj),
            ],
            capture_output=True,
            text=True,
        )
        assert result.returncode == 0, result.stderr
        objects.append(str(obj))
    script = tmp_path / "pre0.ld"
    vectors = (
        f".exceptions {ROM_START + EXCEPTION_OFFSET} : {{ *(.exceptions) }}\n"
        f".irq_vectors {ROM_START + EXCEPTION_OFFSET + EXCEPTION_BYTES} : {{ *(.irq_vectors) }}\n"
        if split
        else f".rom_start {ROM_START} : {{ . += {EXCEPTION_OFFSET}; "
        "*(.exceptions) *(.irq_vectors) }\n"
    )
    script.write_text(
        "SECTIONS {\n"
        + (f"__rom_start_start = {ROM_START};\n" if mode == "cmake" else "")
        + vectors
        + ".text 0x2000 : { *(.text*) }\n"
        ".intList 0 (INFO) : { KEEP(*(.irq_info)) }\n"
        "/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) } }\n"
    )
    image = tmp_path / "pre0.elf"
    subprocess.run(
        linker + ["-T", str(script), *objects, "-o", str(image)],
        check=True,
        capture_output=True,
        text=True,
    )
    cmake = tmp_path / "layout.cmake"
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "scripts/build/gen_isr_tables.py"),
            "--kernel",
            str(image),
            "--output-source",
            str(tmp_path / "tables.c"),
            "--vector-table",
            "--sw-isr-table",
            "--intlist-section",
            ".intList",
            "--cmake-output",
            str(cmake),
        ],
        capture_output=True,
        text=True,
    )
    if mode == "missing_base":
        assert result.returncode != 0
        assert ".rom_start base symbol" in result.stderr
    else:
        assert result.returncode == 0, result.stdout + result.stderr
        text = cmake.read_text()
        if mode == "cmake":
            assert rf"SECTION\;.rom_start\;OFFSET\;{EXCEPTION_OFFSET + EXCEPTION_BYTES}\;" in text
        else:
            assert ".rom_start" not in text
