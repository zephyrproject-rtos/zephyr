# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Preserve XIP copy alignment and MMU deltas around ELD NOLOAD sections."""

import os
import shlex
import shutil
import subprocess
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[3]
BACKENDS = {"gnu": "__GCC_LINKER_CMD__", "lld": "__LLD_LINKER_CMD__", "eld": "__ELD_LINKER_CMD__"}


@pytest.fixture
def clang():
    command = os.environ.get("CLANG", shutil.which("clang"))
    if not command:
        pytest.skip("Clang is required for linker preprocessing")
    return command


def preprocess(clang, directory, backend, mode, body):
    stub = directory / "include/zephyr/kernel/mm.h"
    stub.parent.mkdir(parents=True, exist_ok=True)
    stub.write_text("/* The fixture supplies only the linker memory mode. */\n")
    source = directory / "layout.ld.S"
    source.write_text('#include <zephyr/linker/linker-tool.h>\n' + body)
    command = [
        clang,
        "-E",
        "-P",
        "-x",
        "c",
        "-D_LINKER",
        "-D_ASMLANGUAGE",
        "-DCONFIG_ARM",
        "-D" + BACKENDS[backend],
        "-I" + str(directory / "include"),
        "-I" + str(ROOT / "include"),
    ]
    if mode == "xip":
        command.append("-DCONFIG_XIP")
    if mode == "vm":
        command.append("-DK_MEM_IS_VM_KERNEL")
    result = subprocess.run(command + [str(source)], check=True, capture_output=True, text=True)
    return result.stdout


@pytest.mark.parametrize("mode", ["plain", "xip", "vm"])
@pytest.mark.parametrize(
    "options", ["", "(NOLOAD)", "(COPY)", "(TYPE = SHT_NOBITS)", "(READONLY(TYPE = SHT_PROGBITS))"]
)
def test_options_preserve_gnu_expansions(clang, tmp_path, mode, options):
    body = f"SECTION_DATA_PROLOGUE(probe, {options}, SUBALIGN(16))\n"
    actual = preprocess(clang, tmp_path, "eld", mode, body)
    reference = preprocess(clang, tmp_path, "gnu", mode, body)
    if mode == "xip" and options == "(NOLOAD)":
        reference = reference.replace("ALIGN_WITH_INPUT", "")
    assert actual.split() == reference.split()
    if mode == "xip":
        assert "SUBALIGN" not in actual  # Preserve the inherited XIP macro contract.


def linker(backend):
    if backend == "gnu":
        command = shlex.split(os.environ.get("ISR_METADATA_LD", "arm-none-eabi-ld"))
    elif backend == "lld":
        command = [os.environ.get("ISR_RISCV_LLD", shutil.which("ld.lld") or "ld.lld")]
    else:
        command = [os.environ.get("ELD", shutil.which("ld.eld") or "ld.eld")]
    if not shutil.which(command[0]):
        pytest.skip(f"Actual {backend} ELF linker is unavailable")
    return command


def lma(elf, section):
    segments = [
        segment
        for segment in elf.iter_segments()
        if segment["p_type"] == "PT_LOAD" and segment.section_in_segment(section)
    ]
    assert len(segments) == 1
    segment = segments[0]
    return segment["p_paddr"] + section["sh_addr"] - segment["p_vaddr"]


@pytest.mark.parametrize("backend", BACKENDS)
@pytest.mark.parametrize("mode", ["xip", "vm"])
def test_actual_noload_storage_preserves_copy_and_mmu_layout(clang, tmp_path, backend, mode):
    command = linker(backend)
    bss_alignment = 65536 if mode == "vm" else 256
    source = tmp_path / "entries.c"
    source.write_text(
        'int low __attribute__((section(".low"),aligned(4))) = 7;\n'
        'int high __attribute__((section(".high"),aligned(32))) = 9;\n'
        f'int zero __attribute__((aligned({bss_alignment})));\n'
        'char retained[24] __attribute__((section(".retained"),aligned(8))) = {0x55};\n'
        "void _start(void) { zero = low + high; }\n"
    )
    obj = tmp_path / "entries.o"
    subprocess.run(
        [
            clang,
            "--target=armv7m-none-eabi",
            "-mcpu=cortex-m3",
            "-mthumb",
            "-ffreestanding",
            "-fno-unwind-tables",
            "-c",
            str(source),
            "-o",
            str(obj),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    virtual = 0x80001000 if mode == "vm" else 0x20000004
    physical = 0x10000000 if mode == "vm" else virtual
    load = "PHYS" if mode == "vm" else "FLASH"
    script = preprocess(
        clang,
        tmp_path,
        backend,
        mode,
        f"MEMORY {{ FLASH(rx) : ORIGIN = 0, LENGTH = 256K\n"
        f"VIRT(rw) : ORIGIN = {virtual}, LENGTH = 256K\n"
        f"PHYS(rw) : ORIGIN = {physical}, LENGTH = 256K }}\n"
        "ENTRY(_start)\nSECTIONS { .text : { *(.text*) } > FLASH\n"
        "SECTION_DATA_PROLOGUE(.data_low,,) { *(.low) } GROUP_DATA_LINK_IN(VIRT," + load + ")\n"
        "SECTION_DATA_PROLOGUE(.data_high,,) { *(.high) } GROUP_DATA_LINK_IN(VIRT," + load + ")\n"
        ".info 0 (INFO) : { BYTE(1) }\n.last : { BYTE(0) } > FLASH\n"
        "SECTION_DATA_PROLOGUE(.bss,(NOLOAD),) { *(.bss*) } GROUP_NOLOAD_LINK_IN(VIRT,PHYS)\n"
        "SECTION_PROLOGUE(.noinit,(NOLOAD),) { *(.retained) } GROUP_NOLOAD_LINK_IN(VIRT,PHYS)\n"
        "/DISCARD/ : { *(.ARM.exidx*) *(.comment*) *(.ARM.attributes*) } }\n",
    )
    script_path = tmp_path / "layout.ld"
    script_path.write_text(script)
    image = tmp_path / "image.elf"
    subprocess.run(
        command + ["-N", "-T", str(script_path), str(obj), "-o", str(image)],
        check=True,
        capture_output=True,
        text=True,
    )
    with image.open("rb") as stream:
        elf = ELFFile(stream)
        low = elf.get_section_by_name(".data_low")
        high = elf.get_section_by_name(".data_high")
        bss = elf.get_section_by_name(".bss")
        noinit = elf.get_section_by_name(".noinit")
        assert int.from_bytes(low.data(), "little") == 7
        assert int.from_bytes(high.data(), "little") == 9
        delta = low["sh_addr"] - lma(elf, low)
        assert high["sh_addr"] - lma(elf, high) == delta
        assert bss["sh_addr"] % bss_alignment == 0
        assert noinit["sh_addr"] % 8 == 0
        assert bss["sh_type"] == noinit["sh_type"] == "SHT_NOBITS"
        assert bss["sh_size"] == 4 and noinit["sh_size"] == 24
        if mode == "vm":
            assert delta == virtual - physical
            assert bss["sh_addr"] == 0x80010000
            # NOBITS has no copied payload. Only GNU/ELD promise this LMA delta;
            # LLD still checks initialized-data copies and all NOLOAD VMA/storage rules.
            if backend != "lld":
                assert lma(elf, bss) == 0x1000F000
                assert noinit["sh_addr"] - lma(elf, noinit) == delta
        else:
            assert bss["sh_addr"] == lma(elf, bss)
            assert noinit["sh_addr"] == lma(elf, noinit)
