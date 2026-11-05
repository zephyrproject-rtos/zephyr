# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Discover direct ISRs without allocating or relocating their instruction slots."""

import json
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[3]


def run(command):
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


@pytest.mark.parametrize("variant", ["gcc", "gcc-lto", "clang", "clang-lto", "clang-thin"])
def test_code_discovery_keeps_handler_dependencies_without_branch_range(tmp_path, variant):
    gcc = shlex.split(os.environ.get("ISR_RISCV_CC", "riscv64-unknown-elf-gcc"))
    clang = os.environ.get("CLANG", "clang")
    lld = os.environ.get("ISR_RISCV_LLD", "ld.lld")
    if not shutil.which(gcc[0]):
        pytest.skip("A RISC-V GNU compiler and sysroot are required")
    llvm = variant.startswith("clang")
    if llvm and (not shutil.which(clang) or not shutil.which(lld)):
        pytest.skip("Clang and LLD are required")
    compiler = gcc
    if llvm:
        sysroot = run(gcc + ["-print-sysroot"]).strip()
        if not sysroot:
            # Relocatable SDKs can use target-relative headers without a configured sysroot.
            libc = Path(run(gcc + ["-print-file-name=libc.a"]).strip()).resolve()
            assert libc.is_file(), "The GNU toolchain must provide a target libc archive"
            sysroot = str(libc.parent.parent)
            assert (Path(sysroot) / "include/string.h").is_file(), (
                "The target libc headers must accompany its archive"
            )
        compiler = [clang, "--target=riscv64-unknown-elf", "--sysroot=" + sysroot]
    lto = (
        ["-flto=thin"] if variant.endswith("thin") else ["-flto"] if variant.endswith("lto") else []
    )
    include = tmp_path / "include/zephyr"
    (include / "arch").mkdir(parents=True)
    (include / "device.h").write_text("struct device;\n")
    (include / "arch/cpu.h").write_text("#include <zephyr/arch/riscv/irq_vector.h>\n")
    config = tmp_path / "config.h"
    settings = {
        "CONFIG_RISCV": 1,
        "CONFIG_64BIT": 1,
        "CONFIG_ISR_TABLES_LOCAL_DECLARATION": 1,
        "CONFIG_GEN_IRQ_VECTOR_TABLE": 1,
        "CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE": 1,
        "CONFIG_GEN_SW_ISR_TABLE": 1,
        "CONFIG_GEN_SW_ISR_TABLE_ARRAY": 1,
        "CONFIG_NUM_IRQS": 4,
        "CONFIG_GEN_IRQ_START_VECTOR": 0,
        "CONFIG_ARCH_SW_ISR_TABLE_ALIGN": 8,
        "CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN": 4,
        "CONFIG_ASSERT_LEVEL": 0,
    }
    config.write_text("".join(f"#define {key} {value}\n" for key, value in settings.items()))
    source = tmp_path / "registration.c"
    source.write_text(
        "#include <zephyr/arch/riscv/irq_vector.h>\n#include <zephyr/sw_isr_table.h>\n"
        "static volatile unsigned only_isr_object;\n"
        'static void __attribute__((section(".far.functions"))) direct_handler(void)\n'
        "{ only_isr_object++; }\n"
        "static void register_irq(void) { Z_ISR_DECLARE_DIRECT(0, 0, direct_handler); }\n"
        'void __attribute__((section(".far.functions"))) _isr_wrapper(void) {}\n'
        'void __attribute__((section(".far.functions"))) z_irq_spurious(const void *p)\n'
        "{ (void)p; }\nvoid _start(void) { register_irq(); }\n"
        "static __used void configuration(void) {\n"
        + "".join(
            f"GEN_ABSOLUTE_SYM({key}, {value});\n"
            for key, value in settings.items()
            if value and key not in {"CONFIG_RISCV", "CONFIG_NUM_IRQS"}
        )
        + "}\n"
    )
    flags = [
        "-std=gnu11",
        "-Os",
        "-ffreestanding",
        "-fno-pic",
        "-fno-pie",
        "-fno-asynchronous-unwind-tables",
        "-fno-unwind-tables",
        "-ffunction-sections",
        "-fdata-sections",
        "-march=rv64imac_zicsr_zifencei",
        "-mabi=lp64",
        "-mcmodel=medany",
        "-include",
        str(config),
        "-I",
        str(tmp_path / "include"),
        "-I",
        str(ROOT / "include"),
    ]
    objects = []
    for index, unit in enumerate(
        [source, ROOT / "arch/common/isr_metadata.c", ROOT / "arch/common/isr_tables.c"]
    ):
        obj = tmp_path / f"unit-{index}.o"
        run(compiler + flags + lto + ["-c", str(unit), "-o", str(obj)])
        objects.append(str(obj))
    fragments = []
    for name in ["intlist.ld", "isr-pre0-entries.ld", "isr-local-drop-unused.ld"]:
        output = tmp_path / name
        run(
            compiler
            + flags
            + [
                "-E",
                "-P",
                "-x",
                "assembler-with-cpp",
                str(ROOT / "include/zephyr/linker" / name),
                "-o",
                str(output),
            ]
        )
        fragments.append(f'INCLUDE "{output}"\n')
    pre_script = tmp_path / "pre0.ld"
    pre_script.write_text(
        "ENTRY(_start)\nMEMORY { TEXT (rx) : ORIGIN = 0x10000, LENGTH = 0x10000\n"
        "VECTORS (rx) : ORIGIN = 0x20000, LENGTH = 16\n"
        "FAR (rx) : ORIGIN = 0x400000, LENGTH = 0x10000\n"
        "RAM (rw) : ORIGIN = 0x600000, LENGTH = 0x20000 }\n"
        "SECTIONS { .text 0x10000 : { *(.text*) } > TEXT\n"
        ".vectors 0x20000 : { KEEP(*(.isr_pre0.vectors)) } > VECTORS\n"
        ".far 0x400000 : { *(.far.functions) } > FAR\n"
        ".sw 0x600000 : { KEEP(*(.isr_pre0.sw)) } > RAM\n"
        ".data 0x610000 : { *(.data*) *(.sdata*) *(.bss*) *(.sbss*) } > RAM\n"
        + "".join(fragments)
        + "}\n"
    )
    driver = compiler + flags + lto + (["--ld-path=" + lld] if llvm else [])
    pre = tmp_path / "pre0.elf"
    run(
        driver + ["-nostdlib", "-Wl,--gc-sections", "-T", str(pre_script), *objects, "-o", str(pre)]
    )
    with pre.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = elf.get_section_by_name(".symtab")
        names = [symbol.name for symbol in symbols.iter_symbols()]
        assert any(name.startswith("direct_handler") for name in names)
        assert any(name.startswith("only_isr_object") for name in names)
        assert symbols.get_symbol_by_name("_isr_wrapper")
        assert elf.get_section_by_name(".vectors")["sh_size"] == 16
        assert elf.get_section_by_name(".vectors").data() == bytes(16)
        assert elf.get_section_by_name("isr_declarations")["sh_flags"] & 2 == 0
        assert not any(section.name.startswith(".isr.") for section in elf.iter_sections())
    generated, vector, software, manifest = [
        tmp_path / name for name in ["generated.c", "vector.ld", "software.ld", "layout.json"]
    ]
    run(
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
        ]
    )
    generated_obj = tmp_path / "generated.o"
    run(compiler + flags + lto + ["-c", str(generated), "-o", str(generated_obj)])
    final_script = tmp_path / "final.ld"
    final_script.write_text(
        "ENTRY(_start)\nSECTIONS { .text 0x10000 : { *(.text*) }\n"
        ".far 0x400000 : { *(.far.functions) }\n"
        f'.vectors 0x400100 : {{ INCLUDE "{vector}" }}\n'
        f'.sw 0x600000 : {{ INCLUDE "{software}" }}\n'
        ".data 0x610000 : { *(.data*) *(.sdata*) *(.bss*) *(.sbss*) }\n"
        "/DISCARD/ : { *(.irq_info) *(.intList) *(.isr_vector_refs) *(.isr_vector_owners) } }\n"
    )
    final = tmp_path / "final.elf"
    run(
        driver
        + [
            "-nostdlib",
            "-Wl,--gc-sections",
            "-T",
            str(final_script),
            *objects[:2],
            str(generated_obj),
            "-o",
            str(final),
        ]
    )
    report = tmp_path / "check.json"
    run(
        [
            sys.executable,
            str(ROOT / "scripts/build/check_isr_tables.py"),
            "--elf",
            str(final),
            "--manifest",
            str(manifest),
            "--report",
            str(report),
        ]
    )
    assert json.loads(report.read_text())["status"] == "pass"
    with final.open("rb") as stream:
        elf = ELFFile(stream)
        table = elf.get_section_by_name(".vectors")
        word = int.from_bytes(table.data()[:4], "little")
        assert word & 0x7F == 0x6F and (word >> 7) & 0x1F == 0
        immediate = ((word >> 31) << 20) | (((word >> 21) & 0x3FF) << 1)
        immediate |= ((word >> 20) & 1) << 11 | ((word >> 12) & 0xFF) << 12
        if immediate & (1 << 20):
            immediate -= 1 << 21
        symbols = elf.get_section_by_name(".symtab")
        handlers = [
            symbol["st_value"]
            for symbol in symbols.iter_symbols()
            if symbol.name.startswith("direct_handler")
        ]
        assert handlers == [table["sh_addr"] + immediate]
