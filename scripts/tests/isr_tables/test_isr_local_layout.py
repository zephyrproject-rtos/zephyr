# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Check registration failures and placement across target pointer sizes."""

import importlib
import json
import os
import shutil
import struct
import subprocess
from dataclasses import asdict
from io import StringIO
from pathlib import Path
from types import SimpleNamespace

import pytest


@pytest.fixture
def parser_module(monkeypatch):
    monkeypatch.syspath_prepend(str(Path(__file__).resolve().parents[2] / "build"))
    return importlib.import_module("gen_isr_tables_parser_local")


class Configuration:
    sw_isr_array_name = "_sw_isr_table"
    shared_array_name = "z_shared_sw_isr_table"
    irq_vector_array_name = "_irq_vector_table"

    def __init__(self, *, shared=False, switch=False, vector=False, little=True):
        self.args = SimpleNamespace(sw_isr_table=True, vector_table=vector, big_endian=not little)
        self.shared = shared
        self.switch = switch
        self.little = little

    def endian_prefix(self):
        return "<" if self.little else ">"

    def check_sym(self, symbol):
        return symbol == "CONFIG_GEN_SW_ISR_TABLE_SWITCH" and self.switch

    def check_shared_interrupts(self):
        return self.shared

    def get_sym(self, symbol):
        return {
            "CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS": 3,
            "CONFIG_ARCH_SW_ISR_TABLE_ALIGN": 16,
            "CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN": 4,
        }[symbol]

    @staticmethod
    def get_swt_table_index(offset, irq):
        return irq - offset


def metadata(module, config, registrations, pointer_bytes=8):
    header = {
        "version": module.METADATA_VERSION,
        "header_size": 4 * len(module.METADATA_FIELDS),
        "table_size": 4,
        "offset": 16,
        "entry_size": pointer_bytes * 2,
        "entry_alignment": pointer_bytes,
        "arg_offset": 0,
        "arg_size": pointer_bytes,
        "isr_offset": pointer_bytes,
        "isr_size": pointer_bytes,
        "shared_size": pointer_bytes * 7,
        "shared_alignment": pointer_bytes,
        "shared_count_offset": pointer_bytes * 6,
        "shared_count_size": pointer_bytes,
        "shared_count_alignment": pointer_bytes,
        "vector_size": 4,
        "vector_alignment": 4,
    }
    prefix = config.endian_prefix()
    result = struct.pack(
        prefix + "I" * len(header), *(header[field] for field in module.METADATA_FIELDS)
    )
    for irq, flags, section in registrations:
        name = section.encode() + b"\0"
        size = (16 + len(name) + 3) & ~3
        result += struct.pack(prefix + "IiII", size, irq, flags, len(name))
        result += name + bytes(size - 16 - len(name))
    return result


@pytest.mark.parametrize("pointer_bytes,little", [(4, True), (4, False), (8, True), (8, False)])
def test_shared_entries_keep_each_original_section_once(parser_module, pointer_bytes, little):
    config = Configuration(shared=True, little=little)
    registrations = [(17, 0, ".irq.first"), (17, 0, ".irq.second"), (19, 0, ".irq.single")]
    parser = parser_module.gen_isr_parser(
        metadata(parser_module, config, registrations, pointer_bytes), config, None
    )
    tables = parser.layout_manifest()["tables"]
    inputs = [item for table in tables for item in table["inputs"]]
    for _, _, section in registrations:
        assert sum(item["name"] == section for item in inputs) == 1
    shared = next(table for table in tables if table["symbol"] == "z_shared_sw_isr_table")
    assert shared["size"] == 4 * 7 * pointer_bytes
    first = next(item for item in shared["inputs"] if item["name"] == ".irq.first")
    second = next(item for item in shared["inputs"] if item["name"] == ".irq.second")
    assert first["offset"] == 7 * pointer_bytes
    assert second["offset"] == 9 * pointer_bytes


def test_switch_packs_only_registered_slots(parser_module):
    config = Configuration(switch=True)
    parser = parser_module.gen_isr_parser(
        metadata(parser_module, config, [(19, 0, ".irq.last")]), config, None
    )
    table = parser.layout_manifest()["tables"][0]
    assert table["size"] == 16
    assert table["inputs"] == [
        {"name": ".irq.last", "offset": 0, "size": 16, "alignment": 8, "symbol": "z_isr_entry_0x3"}
    ]


@pytest.mark.parametrize(
    "registrations,shared,message",
    [
        ([(16, 0, ".irq.same"), (17, 0, ".irq.same")], False, "Repeated ISR"),
        ([(16, 0, ".irq.a"), (16, 0, ".irq.b")], False, "Too many handlers"),
        ([(16, 0, f".irq.{index}") for index in range(4)], True, "Too many handlers"),
        ([(15, 0, ".irq.before")], False, "outside the software table"),
        ([(20, 0, ".irq.after")], False, "outside the software table"),
        ([(16, 1, ".irq.direct")], False, "without an IRQ vector table"),
    ],
)
def test_invalid_registrations_fail_before_emitting_layout(
    parser_module, registrations, shared, message
):
    config = Configuration(shared=shared)
    with pytest.raises(ValueError, match=message):
        parser_module.gen_isr_parser(metadata(parser_module, config, registrations), config, None)


def test_truncated_record_is_not_silently_dropped(parser_module):
    config = Configuration()
    data = metadata(parser_module, config, [(16, 0, ".irq.incomplete")])
    with pytest.raises(ValueError, match="Truncated ISR registration name"):
        parser_module.gen_isr_parser(data[:-4], config, None)


def test_gnu_link_resolves_original_entry_relocations(parser_module, tmp_path):
    compiler = shutil.which(os.environ.get("ISR_AARCH64_CC", "aarch64-elf-gcc"))
    linker = shutil.which(os.environ.get("ISR_AARCH64_LD", "aarch64-elf-ld"))
    if compiler is None or linker is None:
        pytest.skip("AArch64 GNU cross compiler and linker are required")
    layout = importlib.import_module("isr_table_layout")
    checker = importlib.import_module("check_isr_tables")
    source = tmp_path / "entries.c"
    source.write_text("""
unsigned argument = 7;
void handler(const void *arg) { (void)arg; }
struct entry { const void *arg; void (*isr)(const void *); };
__attribute__((section(".irq.first"), used))
struct entry first = { &argument, handler };
__attribute__((section(".irq.second"), used))
struct entry second = { 0, handler };
""")
    object_path = tmp_path / "entries.o"
    subprocess.run(
        [
            compiler,
            "-c",
            "-ffreestanding",
            "-fno-asynchronous-unwind-tables",
            "-fno-unwind-tables",
            "-o",
            str(object_path),
            str(source),
        ],
        check=True,
    )
    table = layout.Table(
        "sw_isr_table",
        "_sw_isr_table",
        8,
        32,
        [
            layout.InputSection(".irq.first", 0, 16, 8, "entry_first"),
            layout.InputSection(".irq.second", 16, 16, 8, "entry_second"),
        ],
    )
    body = StringIO()
    table.write_ld(body)
    script = tmp_path / "layout.ld"
    script.write_text(
        "SECTIONS { .text 0x1000 : { *(.text*) } "
        ".data 0x2000 : { *(.data*) } sw_isr_table 0x3000 : {\n" + body.getvalue() + "\n} }\n"
    )
    final = tmp_path / "final.elf"
    subprocess.run(
        [linker, "-T", str(script), "-e", "handler", "-o", str(final), str(object_path)], check=True
    )
    manifest = tmp_path / "layout.json"
    manifest.write_text(
        json.dumps(
            {
                "version": 1,
                "tables": [
                    asdict(table) | {"end_symbol": table.end_symbol, "writable": True},
                ],
            }
        )
    )
    code, report = checker.run(final, manifest)
    assert code == 0, report
    with final.open("rb") as stream:
        image = checker.Image(stream)
        actual = struct.unpack("<QQQQ", image.sections["sw_isr_table"].data())
        assert actual == (
            image.unique("argument")["value"],
            image.unique("handler")["value"],
            0,
            image.unique("handler")["value"],
        )
