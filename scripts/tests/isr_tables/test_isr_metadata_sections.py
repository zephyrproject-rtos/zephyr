# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Read real linked merged and separate build-record sections."""

import importlib
import struct
import subprocess
from pathlib import Path

import pytest
import test_isr_metadata_finalize as finalizer_tests
from elftools.elf.elffile import ELFFile

tools = finalizer_tests.tools
ROOT = Path(__file__).resolve().parents[3]


@pytest.fixture
def generator(monkeypatch):
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    return importlib.import_module("gen_isr_tables")


def build(directory, tools, *, split=True, marker=1, allocated=False, extra_header=False):
    compiler, linker = tools
    source = directory / "metadata.c"
    source.write_text(
        "#include <zephyr/isr_table_metadata.h>\n"
        'const struct z_isr_local_header _iheader __attribute__((used,section(".irq_info")))'
        " = {.version=Z_ISR_LOCAL_METADATA_VERSION,"
        ".header_size=sizeof(struct z_isr_local_header),.table_size=4};\n"
        'static const struct { struct z_isr_local_record header; char name[15]; }\n'
        'record __attribute__((used,aligned(Z_ISR_LOCAL_RECORD_ALIGNMENT),section(".intList"))) = '
        '{.header={32,17,0,15},.name=".irq.record.17"};\n'
        + (
            'const int extra __attribute__((used,section(".irq_info"))) = 9;\n'
            if extra_header
            else ""
        )
        + (
            f'__asm__(".globl z_isr_metadata_nonallocated\\n'
            f'.equ z_isr_metadata_nonallocated,{marker}");\n'
            if marker is not None
            else ""
        )
        + "void _start(void) {}\n"
    )
    obj = directory / "metadata.o"
    subprocess.run(
        compiler
        + [
            "-c",
            "-ffreestanding",
            "-fno-pie",
            "-fno-pic",
            "-fno-asynchronous-unwind-tables",
            "-fno-unwind-tables",
            "-I",
            str(ROOT / "include"),
            str(source),
            "-o",
            str(obj),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    placement = "" if allocated else "(INFO)"
    sections = (
        f".irq_info 0 {placement} : {{ *(.irq_info) }}\n"
        f".intList 0 {placement} : {{ *(.intList) }}\n"
        if split
        else f".intList 0 {placement} : {{ *(.irq_info) *(.intList) }}\n"
    )
    script = directory / "metadata.ld"
    script.write_text(
        "ENTRY(_start)\nSECTIONS { .text 0x10000 : { *(.text*) }\n"
        + sections
        + "/DISCARD/ : { *(.eh_frame*) *(.ARM.exidx*) *(.ARM.extab*) } }\n"
    )
    image = directory / "metadata.elf"
    # Separate debug sections intentionally share their independent offset 0.
    subprocess.run(
        linker + ["--no-check-sections", "-T", str(script), str(obj), "-o", str(image)],
        check=True,
        capture_output=True,
        text=True,
    )
    return image


@pytest.mark.parametrize("split", [False, True])
@pytest.mark.parametrize("marker", [None, 1])
def test_real_metadata_header_and_records(generator, tools, tmp_path, split, marker):
    path = build(tmp_path, tools, split=split, marker=marker)
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        data, prefix, nonallocated = generator.read_local_metadata(elf, [".intList", "intList"])
        order = "<" if elf.little_endian else ">"
        assert struct.unpack_from(order + "II", data) == (3, 68)
        assert len(data) == 68 + 32
        assert struct.unpack_from(order + "IiII", data, 68) == (32, 17, 0, 15)
        assert data[84:100] == b".irq.record.17".ljust(16, b"\0")
        assert prefix == ""
        assert nonallocated is (marker == 1)


@pytest.mark.parametrize(
    "options,message",
    [
        ({"marker": 0}, "Invalid ISR nonallocated-metadata capability"),
        ({"allocated": True}, "allocated or executable"),
        ({"extra_header": True}, "Separate ISR metadata header contains trailing data"),
    ],
)
def test_invalid_metadata_sections(generator, tools, tmp_path, options, message):
    path = build(tmp_path, tools, **options)
    with path.open("rb") as stream, pytest.raises(ValueError, match=message):
        generator.read_local_metadata(ELFFile(stream), [".intList", "intList"])
