# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Compile real declarations with both counter and line-number registration IDs."""

import json
import subprocess
import sys
from pathlib import Path

import pytest
import test_isr_metadata_finalize as finalizer_tests

tools = finalizer_tests.tools
ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize("line_counter", [False, True])
@pytest.mark.parametrize("metadata_nonallocated", [False, True])
def test_same_line_different_handlers_link_and_generate(
    tmp_path, tools, line_counter, metadata_nonallocated
):
    compiler, linker = tools
    include = tmp_path / "include/zephyr"
    include.mkdir(parents=True)
    # The minimal ELF has no device model; all ISR definitions/macros come
    # from the production tree, including its actual metadata compilation unit.
    (include / "device.h").write_text("struct device;\n")
    config = tmp_path / "config.h"
    config.write_text(
        "#if defined(__arm__)\n#define CONFIG_ARM 1\n"
        "#elif defined(__riscv)\n#define CONFIG_RISCV 1\n"
        "#elif defined(__mips__)\n#define CONFIG_MIPS 1\n#endif\n"
        "#define CONFIG_ISR_TABLES_LOCAL_DECLARATION 1\n"
        "#define CONFIG_GEN_SW_ISR_TABLE 1\n#define CONFIG_GEN_SW_ISR_TABLE_ARRAY 1\n"
        "#define CONFIG_GEN_IRQ_VECTOR_TABLE 1\n"
        "#define CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS 1\n"
        "#define CONFIG_NUM_IRQS 4\n#define CONFIG_GEN_IRQ_START_VECTOR 0\n"
        "#define CONFIG_ARCH_SW_ISR_TABLE_ALIGN 4\n"
        "#define CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN 4\n#define CONFIG_ASSERT_LEVEL 0\n"
    )
    source = tmp_path / "registration.c"
    source.write_text(
        "#include <zephyr/sw_isr_table.h>\n"
        + ("#undef __COUNTER__\n#define __COUNTER__ __LINE__\n" if line_counter else "")
        + "static volatile unsigned calls;\n"
        "static void handler_a(const void *arg) { calls += arg != 0; }\n"
        "static void handler_b(const void *arg) { calls += arg == 0; }\n"
        "static void direct_a(void) { calls += 2; }\n"
        "static void direct_b(void) { calls += 3; }\n"
        "Z_ISR_DECLARE(0, 0, handler_a, &calls); Z_ISR_DECLARE(1, 0, handler_b, 0);\n"
        "Z_ISR_DECLARE_DIRECT(2, 0, direct_a); Z_ISR_DECLARE_DIRECT(3, 0, direct_b);\n"
        "void z_irq_spurious(const void *arg) { calls += arg != 0; }\n"
        "void _isr_wrapper(void) { calls++; }\nvoid _start(void) { calls++; }\n"
    )
    symbols = tmp_path / "symbols.c"
    symbols.write_text(
        "#include <zephyr/toolchain.h>\nGEN_ABS_SYM_BEGIN(isr_test_configuration)\n"
        + "\n".join(
            f"GEN_ABSOLUTE_SYM({name}, {value});"
            for name, value in (
                ("CONFIG_ISR_TABLES_LOCAL_DECLARATION", 1),
                ("CONFIG_GEN_SW_ISR_TABLE", 1),
                ("CONFIG_GEN_SW_ISR_TABLE_ARRAY", 1),
                ("CONFIG_GEN_IRQ_VECTOR_TABLE", 1),
                ("CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS", 1),
                ("CONFIG_ARCH_SW_ISR_TABLE_ALIGN", 4),
                ("CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN", 4),
            )
        )
        + "\nGEN_ABS_SYM_END\n"
    )
    options = [
        "-std=gnu11",
        "-Os",
        "-ffreestanding",
        "-fno-pic",
        "-fno-pie",
        "-fno-asynchronous-unwind-tables",
        "-fno-unwind-tables",
        "-Werror=attributes",
        "-include",
        str(config),
        "-I",
        str(tmp_path / "include"),
        "-I",
        str(ROOT / "include"),
    ]
    if metadata_nonallocated:
        options.append("-DZ_ISR_LOCAL_METADATA_NONALLOC=1")
    objects = []
    for item in (source, ROOT / "arch/common/isr_metadata.c", symbols):
        output = tmp_path / f"{item.stem}.o"
        subprocess.run(
            compiler + options + ["-c", str(item), "-o", str(output)],
            check=True,
            capture_output=True,
            text=True,
        )
        objects.append(str(output))
    pre_script = tmp_path / "pre.ld"
    pre_script.write_text(
        "ENTRY(_start)\nSECTIONS { .text 0x10000 : { *(.text*) }\n"
        ".data 0x20000 : { *(.data*) *(.bss*) *(.sdata*) *(.sbss*) }\n"
        ".intList 0 (INFO) : { KEEP(*(.irq_info)) KEEP(*(.intList)) }\n"
        "registrations 0 (INFO) : { KEEP(*(.irq.*)) KEEP(*(.isr.*)) }\n"
        "/DISCARD/ : { *(.eh_frame*) *(.ARM.exidx*) *(.ARM.extab*) } }\n"
    )
    pre = tmp_path / "pre.elf"
    subprocess.run(
        linker + ["-T", str(pre_script), *objects, "-o", str(pre)],
        check=True,
        capture_output=True,
        text=True,
    )
    generated = tmp_path / "generated.c"
    vector = tmp_path / "vector.ld"
    software = tmp_path / "software.ld"
    manifest = tmp_path / "layout.json"
    subprocess.run(
        [
            sys.executable,
            str(ROOT / "scripts/build/gen_isr_tables.py"),
            "--kernel",
            str(pre),
            "--output-source",
            str(generated),
            "--vector-table",
            "--sw-isr-table",
            "--intlist-section",
            ".intList",
            "--linker-output-files",
            str(vector),
            str(software),
            "--layout-output",
            str(manifest),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    layout = json.loads(manifest.read_text())
    assert layout["metadata_policy"] == ("nonallocated" if metadata_nonallocated else "absent")
    tables = layout["tables"]
    registered = [
        item["name"]
        for table in tables
        for item in table["inputs"]
        if item["name"].startswith((".irq.", ".isr."))
    ]
    assert len(registered) == len(set(registered)) == 4
    for handler in ("handler_a", "handler_b", "direct_a", "direct_b"):
        assert sum(f".{handler}." in name for name in registered) == 1
    generated_object = tmp_path / "generated.o"
    subprocess.run(
        compiler + options + ["-c", str(generated), "-o", str(generated_object)],
        check=True,
        capture_output=True,
        text=True,
    )
    final_script = tmp_path / "final.ld"
    final_script.write_text(
        "ENTRY(_start)\nSECTIONS { .text 0x10000 : { *(.text*) }\n"
        f'irq_vector_table 0x18000 : {{ INCLUDE "{vector}" }}\n'
        f'sw_isr_table 0x19000 : {{ INCLUDE "{software}" }}\n'
        ".data 0x20000 : { *(.data*) *(.bss*) *(.sdata*) *(.sbss*) }\n"
        "/DISCARD/ : { *(.irq_info) *(.intList) *(.eh_frame*) *(.ARM.exidx*) *(.ARM.extab*) } }\n"
    )
    final = tmp_path / "final.elf"
    subprocess.run(
        linker
        + [
            "--gc-sections",
            "-T",
            str(final_script),
            *objects,
            str(generated_object),
            "-o",
            str(final),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        [
            sys.executable,
            str(ROOT / "scripts/build/check_isr_tables.py"),
            "--elf",
            str(final),
            "--manifest",
            str(manifest),
            "--report",
            str(tmp_path / "check.json"),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
