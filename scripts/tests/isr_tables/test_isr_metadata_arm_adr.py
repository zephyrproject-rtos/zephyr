# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Check linked Thumb ADR targets without treating REL immediates as addends."""

import importlib
import os
import shlex
import shutil
import subprocess

import pytest
import test_isr_metadata_finalize as finalizer_tests
from elftools.elf.descriptions import describe_reloc_type
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

tools = finalizer_tests.tools


@pytest.mark.parametrize("width,target_address", [("w", 0x10AC0), ("w", 0xF548), ("n", 0x10020)])
def test_thumb_adr_resolves_runtime_and_metadata_addend(
    tools, tmp_path, monkeypatch, width, target_address
):
    _, linker = tools
    compiler = shlex.split(os.environ.get("CLANG", "clang"))
    if not shutil.which(compiler[0]):
        pytest.skip("Clang's ARM integrated assembler is required for retained ADR relocations")
    monkeypatch.syspath_prepend(str(finalizer_tests.ROOT / "scripts/build"))
    finalizer = importlib.import_module("isr_metadata_finalize")
    source = tmp_path / "adr.c"
    # Clang widens an unresolved ADR.N. Describe the 16-bit ABI relocation
    # explicitly, as for the literal-load tests; the linker writes the actual
    # target. The initial immediate encodes the +16 addend with the PC bias.
    instruction = (
        ".inst.n 0xa003\\n.reloc .Ladr,R_ARM_THM_PC8,runtime_anchor"
        if width == "n"
        else "adr.w r0, runtime_anchor+16"
    )
    source.write_text(
        'const unsigned runtime_anchor[4] __attribute__((used,section(".anchor"))) = {1,2,3,4};\n'
        "void _start(void) __attribute__((naked,used));\n"
        'void _start(void) { __asm__ volatile("nop\\n.Ladr:\\n' + instruction + '\\nbx lr"); }\n'
        'const unsigned _iheader[17] __attribute__((used,section(".irq_info"))) = {3,68};\n'
        '__asm__(".globl z_isr_metadata_nonallocated\\n'
        '.equ z_isr_metadata_nonallocated,1");\n'
    )
    obj = tmp_path / "adr.o"
    subprocess.run(
        compiler
        + [
            "--target=arm-none-eabi",
            "-mcpu=cortex-m3",
            "-mthumb",
            "-Os",
            "-ffreestanding",
            "-c",
            str(source),
            "-o",
            str(obj),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    for case in ("runtime", "metadata_addend"):
        metadata = 0x70000000 if case == "runtime" else target_address
        script = tmp_path / f"{case}.ld"
        script.write_text(
            "ENTRY(_start)\nPHDRS { code PT_LOAD; data PT_LOAD; metadata PT_LOAD; }\nSECTIONS {\n"
            ".text 0x10000 : { *(.text*) } :code\n"
            f".anchor {target_address - 16} : {{ *(.anchor) }} :data\n"
            f".intList {metadata} : {{ *(.irq_info) }} :metadata\n"
            "/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) } }\n"
        )
        image = tmp_path / f"{case}.elf"
        subprocess.run(
            linker + ["--emit-relocs", "-T", str(script), str(obj), "-o", str(image)],
            check=True,
            capture_output=True,
            text=True,
        )
        with image.open("rb") as stream:
            elf = ELFFile(stream)
            matches = []
            for section in elf.iter_sections():
                if not isinstance(section, RelocationSection):
                    continue
                symbols = elf.get_section(section["sh_link"])
                for relocation in section.iter_relocations():
                    name = describe_reloc_type(relocation["r_info_type"], elf)
                    if name not in {"R_ARM_THM_ALU_PREL_11_0", "R_ARM_THM_PC8"}:
                        continue
                    symbol = symbols.get_symbol(relocation["r_info_sym"])
                    assert relocation["r_offset"] % 4 == 2
                    assert name == ("R_ARM_THM_ALU_PREL_11_0" if width == "w" else "R_ARM_THM_PC8")
                    assert (
                        finalizer.relocation_target(
                            elf, elf.get_section(section["sh_info"]), relocation, symbol
                        )
                        == target_address
                    )
                    matches.append(name)
            assert len(matches) == 1
        original = image.read_bytes()
        if case == "runtime":
            report = finalizer.finalize(image, tmp_path / f"{case}.json")
            assert (
                report["runtime_load_image_before_sha256"]
                == report["runtime_load_image_after_sha256"]
            )
        else:
            with pytest.raises(
                ValueError, match="Runtime relocation target refers to ISR metadata"
            ):
                finalizer.finalize(image, tmp_path / f"{case}.json")
            assert image.read_bytes() == original
