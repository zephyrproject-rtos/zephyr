# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Retain the real nonallocated metadata capability through compiler LTO."""

import subprocess
from pathlib import Path

import test_isr_metadata_finalize as finalizer_tests
from elftools.elf.elffile import ELFFile

tools = finalizer_tests.tools
ROOT = Path(__file__).resolve().parents[3]


def test_production_metadata_capability_survives_lto(tools, tmp_path):
    compiler, linker = tools
    version = subprocess.run(
        compiler + ["--version"], check=True, capture_output=True, text=True
    ).stdout
    clang = "clang" in version.lower()
    modes = ["-flto", "-flto=thin"] if clang else ["-flto"]
    include = tmp_path / "include/zephyr"
    include.mkdir(parents=True)
    (include / "device.h").write_text("struct device;\n")
    config = tmp_path / "config.h"
    config.write_text(
        "#define CONFIG_ARM 1\n#define CONFIG_ISR_TABLES_LOCAL_DECLARATION 1\n"
        "#define CONFIG_GEN_SW_ISR_TABLE 1\n#define CONFIG_GEN_SW_ISR_TABLE_ARRAY 1\n"
        "#define CONFIG_NUM_IRQS 4\n#define CONFIG_GEN_IRQ_START_VECTOR 0\n"
        "#define CONFIG_ARCH_SW_ISR_TABLE_ALIGN 4\n#define CONFIG_ASSERT_LEVEL 0\n"
        "#define Z_ISR_LOCAL_METADATA_NONALLOC 1\n"
    )
    script = tmp_path / "metadata.ld"
    script.write_text(
        "SECTIONS { .text 0x10000 : { *(.text*) }\n"
        ".intList 0 (INFO) : { KEEP(*(.irq_info)) }\n"
        "/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) } }\n"
    )
    for index, mode in enumerate(modes):
        obj, image = tmp_path / f"metadata-{index}.o", tmp_path / f"metadata-{index}.elf"
        options = [
            mode,
            "-Os",
            "-ffreestanding",
            "-fno-pic",
            "-fno-pie",
            "-include",
            str(config),
            "-I",
            str(tmp_path / "include"),
            "-I",
            str(ROOT / "include"),
        ]
        subprocess.run(
            compiler + options + ["-c", str(ROOT / "arch/common/isr_metadata.c"), "-o", str(obj)],
            check=True,
            capture_output=True,
            text=True,
        )
        driver = compiler + ([f"-fuse-ld={linker[0]}"] if clang else [])
        subprocess.run(
            driver
            + [
                mode,
                "-nostdlib",
                "-Wl,--gc-sections",
                f"-Wl,-T,{script}",
                str(obj),
                "-o",
                str(image),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        with image.open("rb") as stream:
            elf = ELFFile(stream)
            symbols = elf.get_section_by_name(".symtab")
            capability = symbols.get_symbol_by_name("z_isr_metadata_nonallocated")
            assert len(capability) == 1
            assert capability[0]["st_shndx"] == "SHN_ABS"
            assert capability[0]["st_value"] == 1
            assert symbols.get_symbol_by_name("_iheader")
