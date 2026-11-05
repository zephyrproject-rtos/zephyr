# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Finalize real isolated metadata segments, preserving runtime relocations."""

import importlib
import os
import shlex
import shutil
import struct
import subprocess
from io import BytesIO
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[3]


@pytest.fixture
def finalizer(monkeypatch):
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    return importlib.import_module("isr_metadata_finalize")


@pytest.fixture
def tools():
    compiler = shlex.split(os.environ.get("ISR_METADATA_CC", "cc"))
    linker = shlex.split(os.environ.get("ISR_METADATA_LD", "ld"))
    if not shutil.which(compiler[0]) or not shutil.which(linker[0]):
        pytest.skip("An ELF compiler and linker are required")
    version = subprocess.run(linker + ["--version"], capture_output=True, text=True)
    if not any(name in version.stdout for name in ("GNU ld", "LLD")):
        pytest.skip("Set ISR_METADATA_CC and ISR_METADATA_LD to ELF tools")
    return compiler, linker


def build(directory, tools, *, defect=None, control=None):
    compiler, linker = tools
    declaration = "const" if defect != "writable" else ""
    marker = (
        ""
        if defect == "missing_policy"
        else (
            '__asm__(".globl z_isr_metadata_nonallocated\\n.equ z_isr_metadata_nonallocated,1");\n'
        )
    )
    external_reference = ""
    if defect == "runtime_reference":
        external_reference = 'const void *const forbidden = &_iheader;\n'
    if defect in {"absolute_reference", "absolute_addend_reference"}:
        external_reference = "extern const volatile uint32_t metadata_alias[];\n"
        if defect == "absolute_addend_reference":
            external_reference += (
                "const volatile uint32_t *const forbidden_alias = metadata_alias + 4;\n"
            )
        else:
            external_reference += "uint32_t read_metadata(void) { return metadata_alias[0]; }\n"
    if defect in {"veneer_reference", "veneer_addend_reference"}:
        target = "metadata_alias" if defect == "veneer_reference" else "argument+0x20000"
        external_reference += '__asm__(".pushsection .text\\nbl ' + target + '\\n.popsection");\n'
    if defect == "section_addend_reference":
        external_reference += (
            '__asm__(".pushsection .data.bad,\\"aw\\"\\n.long argument+0x10010\\n.popsection");\n'
        )
    if control:
        external_reference += (
            'const uint32_t vector_table[2] __attribute__((used,section(".runtime_vectors")))'
            " = {7,9};\nextern const volatile uint32_t vector_alias[];\n"
            "uint32_t read_vector(void) { return vector_alias[0]; }\n"
            '__asm__(".pushsection .data.scalar,\\"aw\\"\\n'
            '.long CONFIG_COUNT\\n.long scalar_offset\\n.popsection");\n'
        )
    source = directory / "image.c"
    source.write_text(
        "typedef unsigned int uint32_t;\n"
        "struct header { uint32_t version, header_size, fields[15]; };\n"
        + declaration
        + ' struct header _iheader __attribute__((used,section(".irq_info")))'
        " = {3,68,{0}};\n"
        'static const char records[8192] __attribute__((used,section(".intList"))) = {1};\n'
        + marker
        + external_reference
        + "volatile unsigned argument = 7;\n"
        "void handler(const void *p) { argument += p != 0; }\n"
        "struct entry { const void *arg; void (*isr)(const void *); };\n"
        'const struct entry entry __attribute__((used,section(".isr.entry")))'
        " = {(const void *)&argument, handler};\n"
        + (
            "extern const volatile struct entry entry_alias;\n"
            "uint32_t read_entry(void) { return entry_alias.arg != 0; }\n"
            if control
            else ""
        )
        + "void _start(void) { handler((const void *)&argument); }\n"
    )
    obj = directory / "image.o"
    subprocess.run(
        compiler
        + [
            "-c",
            "-Os",
            "-ffreestanding",
            "-fno-pic",
            "-fno-pie",
            "-fno-asynchronous-unwind-tables",
            "-fno-unwind-tables",
            "-o",
            str(obj),
            str(source),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    metadata_segment = "runtime_data" if defect == "mixed_load_segment" else "metadata"
    if defect == "no_load_segment":
        metadata_segment = "NONE"
    script = directory / "image.ld"
    zero_fill = "metadata_zero (NOLOAD) : { . += 16; } :metadata\n" if defect == "zero_fill" else ""
    # A separate debug VMA follows both fixture runtime memory regions, with
    # their boundary excluded. No runtime padding or addresses are rewritten.
    metadata_base = 0x30010 if control != "overlap" else 0
    if defect == "mixed_load_segment":
        metadata_base = 0x18000
    if defect in {"veneer_reference", "veneer_addend_reference"}:
        metadata_base = 0x5000010
    data_base = 0x20000
    if defect == "veneer_addend_reference":
        data_base, metadata_base = 0x4FF0000, 0x5010000
    aliases = ""
    if defect in {"absolute_reference", "absolute_addend_reference", "veneer_reference"}:
        addend = 16 if defect == "absolute_addend_reference" else 0
        aliases += f"metadata_alias = ABSOLUTE(_iheader) - {addend};\n"
    if control:
        aliases += (
            "vector_alias = ABSOLUTE(vector_table); entry_alias = ABSOLUTE(entry);\n"
            "CONFIG_COUNT = 17; scalar_offset = 0;\n"
        )
    script.write_text(
        "ENTRY(_start)\nPHDRS { runtime_code PT_LOAD; runtime_data PT_LOAD; metadata PT_LOAD; }\n"
        "SECTIONS {\n.vectors 0 : { *(.runtime_vectors) } :runtime_code\n"
        ".text 0x10000 : { *(.text*) } :runtime_code\n"
        ".MIPS.abiflags : { *(.MIPS.abiflags) } :runtime_code\n"
        ".reginfo : { *(.reginfo) } :runtime_code\n"
        f".intList {metadata_base} : {{ *(.irq_info) *(.intList) }} :{metadata_segment}\n"
        + zero_fill
        + f".data {data_base} : {{ *(.data*) *(.sdata*) *(.isr.entry) *(.rodata*) }} "
        ":runtime_data\n"
        ".bss : { *(.bss*) *(.sbss*) } :runtime_data\n"
        + aliases
        + "/DISCARD/ : { *(.eh_frame*) *(.ARM.exidx*) *(.ARM.extab*) *(.comment) *(.note*) }\n}\n"
    )
    image = directory / "image.elf"
    options = [] if defect == "no_relocations" else ["--emit-relocs"]
    if control == "overlap":
        options += ["--no-check-sections"]
    subprocess.run(
        linker + options + ["-T", str(script), str(obj), "-o", str(image)],
        check=True,
        capture_output=True,
        text=True,
    )
    return image


def test_dedicated_overlay_becomes_build_data_without_changing_runtime(finalizer, tools, tmp_path):
    path = build(tmp_path, tools)
    original = path.read_bytes()
    before = ELFFile(BytesIO(original))
    before_metadata = before.get_section_by_name(".intList").data()
    report = finalizer.finalize(path, tmp_path / "report.json")
    after = ELFFile(BytesIO(path.read_bytes()))
    assert report["runtime_load_image_before_sha256"] == report["runtime_load_image_after_sha256"]
    assert report["relocations_checked"] > 0
    assert report["detached_load_segments"]
    assert after.get_section_by_name(".intList").data() == before_metadata
    assert after.get_section_by_name(".intList")["sh_size"] > 2048
    assert not after.get_section_by_name(".intList")["sh_flags"] & 2
    for section in before.iter_sections():
        other = after.get_section_by_name(section.name)
        assert other.data() == section.data()
        if section.name != ".intList":
            assert other.header == section.header
    for name in ("argument", "handler", "entry", "_iheader"):
        assert before.get_section_by_name(".symtab").get_symbol_by_name(name)[0].entry == (
            after.get_section_by_name(".symtab").get_symbol_by_name(name)[0].entry
        )
    again = finalizer.finalize(path, tmp_path / "second-report.json")
    assert again["elf_before_sha256"] == again["elf_after_sha256"]


@pytest.mark.parametrize(
    "defect,message",
    [
        ("missing_policy", "absolute nonallocated"),
        ("runtime_reference", "Runtime relocation"),
        ("mixed_load_segment", "also contains"),
        ("no_load_segment", "no dedicated load segment"),
        ("no_relocations", "Retain executable relocations"),
        ("writable", "readonly record bytes"),
        ("zero_fill", "anonymous memory storage"),
        ("absolute_reference", "Runtime relocation"),
        ("absolute_addend_reference", "Runtime relocation"),
        ("section_addend_reference", "Runtime relocation"),
        ("veneer_reference", "Runtime relocation"),
        ("veneer_addend_reference", "runtime branch target|Runtime relocation"),
    ],
)
def test_unsafe_metadata_is_rejected_without_mutating_image(
    finalizer, tools, tmp_path, defect, message
):
    path = build(tmp_path, tools, defect=defect)
    original = path.read_bytes()
    with pytest.raises(ValueError, match=message):
        finalizer.finalize(path, tmp_path / "report.json")
    assert path.read_bytes() == original
    assert not (tmp_path / "report.json").exists()


def test_flash_zero_aliases_and_scalar_constants_remain_loadable(finalizer, tools, tmp_path):
    path = build(tmp_path, tools, control="flash0")
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = elf.get_section_by_name(".symtab")
        for name in ("vector_alias", "entry_alias", "CONFIG_COUNT", "scalar_offset"):
            assert symbols.get_symbol_by_name(name)[0]["st_shndx"] == "SHN_ABS"
        assert symbols.get_symbol_by_name("vector_alias")[0]["st_value"] == 0
    report = finalizer.finalize(path, tmp_path / "report.json")
    assert report["runtime_load_image_before_sha256"] == report["runtime_load_image_after_sha256"]


def test_metadata_virtual_overlap_is_rejected(finalizer, tools, tmp_path):
    path = build(tmp_path, tools, control="overlap")
    original = path.read_bytes()
    with pytest.raises(ValueError, match="virtual range overlaps runtime"):
        finalizer.finalize(path, tmp_path / "report.json")
    assert path.read_bytes() == original


def test_replacement_closes_the_temporary_handle(finalizer, tools, tmp_path, monkeypatch):
    path = build(tmp_path, tools)
    factory = finalizer.tempfile.NamedTemporaryFile
    replace = Path.replace
    handles = []

    def tracked_temp(*args, **kwargs):
        handle = factory(*args, **kwargs)
        handles.append(handle)
        return handle

    def replace_after_close(temporary, target):
        assert handles and all(handle.closed for handle in handles)
        return replace(temporary, target)

    monkeypatch.setattr(finalizer.tempfile, "NamedTemporaryFile", tracked_temp)
    monkeypatch.setattr(Path, "replace", replace_after_close)
    finalizer.finalize(path, tmp_path / "report.json")


def test_malformed_elf_load_mapping_is_rejected(finalizer, tools, tmp_path):
    """Corrupt a real ELF header to test validation, not to reproduce a toolchain bug."""
    path = build(tmp_path, tools)
    data = bytearray(path.read_bytes())
    elf = ELFFile(BytesIO(data))
    metadata = elf.get_section_by_name(".intList")
    index, segment = next(
        (index, segment)
        for index, segment in enumerate(elf.iter_segments())
        if segment["p_type"] == "PT_LOAD" and segment.section_in_segment(metadata)
    )
    offset = elf["e_phoff"] + index * elf["e_phentsize"] + (8 if elf.elfclass == 32 else 16)
    fmt = ("<" if elf.little_endian else ">") + ("I" if elf.elfclass == 32 else "Q")
    struct.pack_into(fmt, data, offset, segment["p_vaddr"] + metadata["sh_addralign"])
    path.write_bytes(data)
    with pytest.raises(ValueError, match="does not match its load mapping"):
        finalizer.finalize(path, tmp_path / "report.json")
    assert path.read_bytes() == data
