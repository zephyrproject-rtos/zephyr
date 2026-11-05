# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Check both TARGET1 initialization ABIs using compiler-generated constructors."""

import importlib
import os
import shlex
import shutil
import subprocess
from pathlib import Path

import pytest
from elftools.elf.descriptions import describe_reloc_type
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize("mode", ["abs", "rel"])
def test_target1_constructor_interpretations(tmp_path, monkeypatch, mode):
    compiler = shlex.split(os.environ.get("ISR_TARGET1_CC", "arm-none-eabi-gcc"))
    linker = shlex.split(os.environ.get("ISR_TARGET1_LD", "arm-none-eabi-ld"))
    if not shutil.which(compiler[0]) or not shutil.which(linker[0]):
        pytest.skip("Set ISR_TARGET1_CC/LD to an ARM compiler emitting TARGET1 constructors")
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    finalizer = importlib.import_module("isr_metadata_finalize")
    source = tmp_path / "constructor.c"
    source.write_text(
        "static volatile unsigned calls;\n"
        'void initialize(void) __attribute__((constructor,section(".text.constructor")));\n'
        "void initialize(void) { calls++; }\nvoid _start(void) { calls++; }\n"
        'const unsigned _iheader[17] __attribute__((used,section(".irq_info"))) = {3,68};\n'
        'const char records[8192] __attribute__((used,section(".intList"))) = {1};\n'
        '__asm__(".globl z_isr_metadata_nonallocated\\n'
        '.equ z_isr_metadata_nonallocated,1");\n'
    )
    obj = tmp_path / "constructor.o"
    subprocess.run(
        compiler + ["-ffreestanding", "-Os", "-c", str(source), "-o", str(obj)],
        check=True,
        capture_output=True,
        text=True,
    )
    with obj.open("rb") as stream:
        elf = ELFFile(stream)
        assert any(
            describe_reloc_type(relocation["r_info_type"], elf) == "R_ARM_TARGET1"
            for section in elf.iter_sections()
            if isinstance(section, RelocationSection)
            for relocation in section.iter_relocations()
        ), "Compiler did not emit TARGET1"
    for case in ("safe", "ambiguous", "direct"):
        # The alternate, unknown ABI can put a safe constructor address in
        # metadata. Refuse this ambiguity rather than guessing the linker mode.
        base = 0x70000000
        if case == "ambiguous":
            base = 0x30000 if mode == "abs" else 0xFFFF0000
        script = tmp_path / f"{case}.ld"
        script.write_text(
            "ENTRY(_start)\nPHDRS { code PT_LOAD; data PT_LOAD; metadata PT_LOAD; }\n"
            "SECTIONS { .text 0x10000 : { KEEP(*(.text.constructor)) *(.text*) } :code\n"
            ".init_array 0x20000 : { KEEP(*(.init_array*)) } :data\n"
            ".data 0x40000 : { *(.data*) *(.bss*) } :data\n"
            f".intList {base} : {{ *(.irq_info) *(.intList) }} :metadata\n"
            + ("initialize = ABSOLUTE(_iheader);\n" if case == "direct" else "")
            + "/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) } }\n"
        )
        image = tmp_path / f"{case}.elf"
        subprocess.run(
            linker
            + [f"--target1-{mode}", "--emit-relocs", "-T", str(script), str(obj), "-o", str(image)],
            check=True,
            capture_output=True,
            text=True,
        )
        original = image.read_bytes()
        if case == "safe":
            report = finalizer.finalize(image, tmp_path / f"{case}.json")
            assert (
                report["runtime_load_image_before_sha256"]
                == (report["runtime_load_image_after_sha256"])
            )
        else:
            message = "TARGET1.*ambiguous.*ELF unchanged" if case == "ambiguous" else "ISR metadata"
            with pytest.raises(ValueError, match=message):
                finalizer.finalize(image, tmp_path / f"{case}.json")
            assert image.read_bytes() == original
