# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Reject inconsistent target layouts before emitting ISR placement files."""

import importlib
import struct
from io import StringIO
from pathlib import Path

import pytest
from test_isr_local_layout import Configuration, metadata


@pytest.fixture
def parser_module(monkeypatch):
    monkeypatch.syspath_prepend(str(Path(__file__).resolve().parents[2] / "build"))
    return importlib.import_module("gen_isr_tables_parser_local")


def changed_metadata(module, config, **fields):
    data = bytearray(metadata(module, config, []))
    for name, value in fields.items():
        struct.pack_into(
            config.endian_prefix() + "I", data, module.METADATA_FIELDS.index(name) * 4, value
        )
    return bytes(data)


@pytest.mark.parametrize(
    "fields,message",
    [
        ({"version": 0}, "Unsupported ISR metadata version"),
        ({"header_size": 4}, "Invalid ISR metadata header size"),
        ({"header_size": 69}, "Invalid ISR metadata header size"),
        ({"table_size": 0}, "Invalid ISR table entry count"),
        ({"entry_alignment": 0}, "Invalid ISR metadata entry_alignment"),
        ({"entry_alignment": 3}, "Invalid ISR metadata entry_alignment"),
        ({"entry_size": 0}, "ISR entry size"),
        ({"entry_size": 17}, "ISR entry size"),
        ({"arg_size": 0}, "Invalid ISR arg field size"),
        ({"isr_size": 0}, "Invalid ISR isr field size"),
        ({"arg_offset": 12}, "ISR arg exceeds"),
        ({"isr_offset": 12}, "ISR isr exceeds"),
        ({"isr_offset": 4}, "fields overlap"),
    ],
)
def test_invalid_entry_header(parser_module, fields, message):
    config = Configuration()
    with pytest.raises(ValueError, match=message):
        parser_module.gen_isr_parser(
            changed_metadata(parser_module, config, **fields), config, None
        )


def test_truncated_header(parser_module):
    config = Configuration()
    with pytest.raises(ValueError, match="Truncated ISR metadata header"):
        parser_module.gen_isr_parser(metadata(parser_module, config, [])[:-1], config, None)


@pytest.mark.parametrize(
    "fields,message",
    [
        ({"shared_alignment": 0}, "Invalid ISR metadata shared_alignment"),
        ({"shared_alignment": 3}, "Invalid ISR metadata shared_alignment"),
        ({"shared_count_alignment": 0}, "Invalid ISR metadata shared_count_alignment"),
        ({"shared_count_alignment": 3}, "Invalid ISR metadata shared_count_alignment"),
        ({"shared_size": 0}, "Invalid shared ISR structure size"),
        ({"shared_size": 57}, "Invalid shared ISR structure size"),
        ({"shared_alignment": 4}, "Shared ISR alignment is smaller"),
        ({"shared_count_size": 0}, "Invalid shared ISR count field layout"),
        ({"shared_count_size": 7}, "Invalid shared ISR count field layout"),
        ({"shared_count_offset": 49}, "Invalid shared ISR count field layout"),
        ({"shared_count_offset": 40}, "Shared ISR count field overlaps"),
        ({"shared_count_offset": 56}, "Shared ISR count field overlaps"),
    ],
)
def test_invalid_shared_header(parser_module, fields, message):
    config = Configuration(shared=True)
    with pytest.raises(ValueError, match=message):
        parser_module.gen_isr_parser(
            changed_metadata(parser_module, config, **fields), config, None
        )


@pytest.mark.parametrize(
    "fields,message",
    [
        ({"vector_alignment": 0}, "Invalid ISR metadata vector_alignment"),
        ({"vector_alignment": 3}, "Invalid ISR metadata vector_alignment"),
        ({"vector_size": 0}, "Invalid ISR vector size"),
        ({"vector_size": 6}, "Invalid ISR vector size"),
    ],
)
def test_invalid_vector_header(parser_module, fields, message):
    config = Configuration(vector=True)
    with pytest.raises(ValueError, match=message):
        parser_module.gen_isr_parser(
            changed_metadata(parser_module, config, **fields), config, None
        )


def test_disabled_layout_fields_may_be_zero(parser_module):
    config = Configuration()
    fields = dict.fromkeys(
        (name for name in parser_module.METADATA_FIELDS if name.startswith(("shared_", "vector_"))),
        0,
    )
    parser_module.gen_isr_parser(changed_metadata(parser_module, config, **fields), config, None)


@pytest.mark.parametrize("nonallocated", [False, True])
def test_metadata_policy_follows_verified_elf_policy(parser_module, nonallocated):
    config = Configuration()
    config.metadata_nonallocated = nonallocated
    parser = parser_module.gen_isr_parser(metadata(parser_module, config, []), config, None)
    assert parser.layout_manifest()["metadata_policy"] == (
        "nonallocated" if nonallocated else "absent"
    )


@pytest.mark.parametrize("little", [True, False])
def test_shared_padding_covers_target_structure(parser_module, little):
    config = Configuration(shared=True, little=little)
    config.swt_spurious_handler = "z_irq_spurious"
    parser = parser_module.gen_isr_parser(
        changed_metadata(parser_module, config, shared_count_offset=64, shared_size=80),
        config,
        None,
    )
    table = parser.tables["shared"]
    first = [item for item in table.inputs if item.offset < 80]
    assert [(item.offset, item.size) for item in first] == [
        (0, 16),
        (16, 16),
        (32, 16),
        (48, 16),
        (64, 8),
        (72, 8),
    ]
    assert table.size == 320
    source = StringIO()
    parser.write_source(source)
    assert "z_isr_pad_before_0[16]" in source.getvalue()
    assert "z_isr_pad_after_0[8]" in source.getvalue()


@pytest.mark.parametrize("alignment", [0, -1, 3])
def test_invalid_table_alignment(parser_module, alignment):
    layout = importlib.import_module("isr_table_layout")
    with pytest.raises(ValueError, match="Invalid ISR table alignment"):
        layout.Table("entries", "entries", alignment, 0).validate()


def test_negative_table_size(parser_module):
    layout = importlib.import_module("isr_table_layout")
    with pytest.raises(ValueError, match="Invalid ISR table size"):
        layout.Table("entries", "entries", 8, -1).validate()


def test_input_alignment_cannot_exceed_table_alignment(parser_module):
    layout = importlib.import_module("isr_table_layout")
    table = layout.Table(
        "entries", "entries", 4, 8, [layout.InputSection(".irq.first", 0, 8, 8, "first")]
    )
    with pytest.raises(ValueError, match="input alignment exceeds"):
        table.validate()


def test_table_start_alignment_does_not_require_trailing_padding(parser_module):
    layout = importlib.import_module("isr_table_layout")
    layout.Table(
        "entries", "entries", 64, 8, [layout.InputSection(".irq.first", 0, 8, 8, "first")]
    ).validate()
    layout.Table("entries", "entries", 8, 0).validate()


@pytest.mark.parametrize("little", [False, True])
@pytest.mark.parametrize("padding_size", [0, 4, 4096])
def test_trailing_zero_padding_preserves_registrations(parser_module, little, padding_size):
    config = Configuration(little=little)
    records = [(17, 0, ".irq.first"), (19, 0, ".irq.last")]
    original = metadata(parser_module, config, records)
    expected = parser_module.gen_isr_parser(original, config, None).layout_manifest()
    padded = parser_module.gen_isr_parser(original + bytes(padding_size), config, None)
    assert padded.layout_manifest() == expected


@pytest.mark.parametrize("little", [False, True])
def test_nonzero_bytes_after_zero_prefix_are_not_padding(parser_module, little):
    config = Configuration(little=little)
    original = metadata(parser_module, config, [(17, 0, ".irq.first")])
    with pytest.raises(ValueError, match="Invalid ISR registration size"):
        parser_module.gen_isr_parser(original + bytes(16) + b"\x01", config, None)


@pytest.mark.parametrize("little", [False, True])
def test_large_table_header_is_not_an_endianness_heuristic(parser_module, little):
    config = Configuration(little=little)
    # Decode only the authoritative target header, without building a large layout.
    parser = parser_module.gen_isr_parser.__new__(parser_module.gen_isr_parser)
    parser.config = config
    header, records = parser.read_metadata(
        changed_metadata(parser_module, config, table_size=65536)
    )
    assert header["table_size"] == 65536
    assert records == []
