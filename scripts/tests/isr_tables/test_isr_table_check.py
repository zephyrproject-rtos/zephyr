# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Reject bad linked layouts, stale aliases and changes to frozen addresses."""

import importlib.util
import json
import struct
import subprocess
import sys
from pathlib import Path

import pytest

SCRIPT = Path(__file__).resolve().parents[2] / "build" / "check_isr_tables.py"
SPEC = importlib.util.spec_from_file_location("check_isr_tables", SCRIPT)
CHECKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECKER)


def elf_fixture(
    path,
    bits=64,
    little=True,
    *,
    handler_delta=0,
    argument_delta=0,
    handler_size=4,
    generated=False,
    residue=None,
    placeholder=False,
    capacity=False,
    used_size=16,
    reserved_size=32,
    local_handler=False,
    lookup_name=None,
    generated_lookup=False,
    lookup_delta=0,
    text_budget=512,
    text_used=None,
    first_end_delta=0,
    omit_first_end=False,
    duplicate_first_end=False,
    empty_table=False,
    symbol_sections=None,
    table_data=None,
    table_nobits=False,
):
    """Write a minimal executable ELF using the published ELF field layout.

    Two typed ISR entries are merged into one output section, just as in a
    linked firmware. Linker aliases intentionally have st_size=0. Separate
    symbols identify a real function and parameter object for phase checks.
    """
    order = "<" if little else ">"
    pointer_bytes = bits // 8
    entry_bytes = 2 * pointer_bytes
    word = "I" if bits == 32 else "Q"
    table = struct.pack(order + word * 4, 0x3000, 0x2000, 0, 0x2000)
    if table_data is not None:
        table = table_data
    if empty_table:
        table = b""
    sections = [
        ("isr_table", 8 if table_nobits else 1, 3, 0x1000, table, len(table), pointer_bytes),
        (".text", 1, 6, 0x2000, bytes(32), 32, 4),
        (".objects", 1, 3, 0x3000, bytes(32), 32, pointer_bytes),
    ]
    # (name, value, size, binding+type, section-index)
    symbols = [
        ("_sw_isr_table", 0x1000, 0, 0x10, 1),
        ("_sw_isr_table_end", 0x1000 + len(table), 0, 0x10, 1),
        ("entry_first", 0x1000, 0, 0x10, 1),
        ("entry_second", 0x1000 + entry_bytes, 0, 0x10, 1),
        ("handler", 0x2000 + handler_delta, handler_size, 0x12, 2),
        ("argument", 0x3000 + argument_delta, pointer_bytes, 0x11, 3),
    ]
    if not empty_table:
        symbols.append(("entry_second_end", 0x1000 + 2 * entry_bytes, 0, 0x10, 1))
        if not omit_first_end:
            symbols.append(("entry_first_end", 0x1000 + entry_bytes + first_end_delta, 0, 0x10, 1))
        if duplicate_first_end:
            symbols.append(("entry_first_end", 0x1000 + entry_bytes + 1, 0, 0x10, 1))
    if generated:
        symbols.append(("z_object_gperf_find", 0x2010, 4, 0x12, 2))
    if residue:
        sections.append((residue, 1, 2, 0x6000, bytes(8), 8, 8))
    if placeholder:
        symbols.append(
            (
                placeholder if isinstance(placeholder, str) else "__isr_pre0_storage",
                0x3010,
                8,
                0x11,
                3,
            )
        )
    if local_handler:
        symbols.append(("handler", 0x2010, 4, 0x02, 2))
    if capacity:
        stack_index = len(sections) + 1
        sections.append(("priv_stacks_noinit", 8, 3, 0x4000, b"", reserved_size, 8))
        symbols.extend(
            [
                ("z_priv_stacks_ram_start", 0x4000, 0, 0x10, stack_index),
                ("z_priv_stacks_ram_end", 0x4000 + used_size, 0, 0x10, stack_index),
            ]
        )
        ro_index = len(sections) + 1
        sections.append((".rodata", 1, 2, 0x5000, bytes(32), 32, 8))
        symbols.extend(
            [
                ("_kobject_rodata_area_start", 0x5000, 0, 0x10, ro_index),
                ("_kobject_rodata_area_end", 0x5000 + used_size, 0, 0x10, ro_index),
            ]
        )
    if lookup_name:
        text_index = len(sections) + 1
        sections.append((".kobject_text", 1, 6, 0x7000, bytes(512), 512, 4))
        function_size = 104 if generated_lookup else 8
        actual_used = function_size if text_used is None else text_used
        symbols.extend(
            [
                ("_kobject_text_area_start", 0x7000, 0, 0x10, text_index),
                ("_kobject_text_area_end", 0x7000 + actual_used, 0, 0x10, text_index),
                ("CONFIG_KOBJECT_TEXT_AREA", text_budget, 0, 0x11, 0xFFF1),
                (
                    lookup_name,
                    0x7000 + lookup_delta,
                    function_size,
                    0x12 if generated_lookup else 0x22,
                    text_index,
                ),
            ]
        )
    if symbol_sections:
        symbols = [(*symbol[:4], symbol_sections.get(symbol[0], symbol[4])) for symbol in symbols]
    symbols.sort(key=lambda item: item[3] >> 4 != 0)
    first_global = 1 + sum(item[3] >> 4 == 0 for item in symbols)
    strings = b"\0" + b"".join(item[0].encode() + b"\0" for item in symbols)
    symbol_size = 16 if bits == 32 else 24
    symbol_data = bytearray(symbol_size)
    for symbol, value, size, info, section_index in symbols:
        offset = strings.index(symbol.encode() + b"\0")
        if bits == 32:
            record = struct.pack(order + "IIIBBH", offset, value, size, info, 0, section_index)
        else:
            record = struct.pack(order + "IBBHQQ", offset, info, 0, section_index, value, size)
        symbol_data.extend(record)
    symtab_index = len(sections) + 1
    strtab_index = symtab_index + 1
    sections.append((".symtab", 2, 0, 0, bytes(symbol_data), len(symbol_data), pointer_bytes))
    sections.append((".strtab", 3, 0, 0, strings, len(strings), 1))
    section_names = b"\0" + b"".join(s[0].encode() + b"\0" for s in sections) + b".shstrtab\0"
    sections.append((".shstrtab", 3, 0, 0, section_names, len(section_names), 1))
    header_size = 52 if bits == 32 else 64
    section_header_size = 40 if bits == 32 else 64
    contents = bytearray(header_size)
    offsets = []
    for _, kind, _, _, data, _, align in sections:
        contents.extend(bytes((-len(contents)) % align))
        offsets.append(len(contents))
        if kind != 8:
            contents.extend(data)
    contents.extend(bytes((-len(contents)) % pointer_bytes))
    headers_offset = len(contents)
    contents.extend(bytes(section_header_size))
    for index, (section, kind, flags, address, _, size, align) in enumerate(sections, 1):
        fmt = "IIIIIIIIII" if bits == 32 else "IIQQQQIIQQ"
        link = strtab_index if kind == 2 else 0
        info = first_global if kind == 2 else 0
        entsize = symbol_size if kind == 2 else 0
        contents.extend(
            struct.pack(
                order + fmt,
                section_names.index(section.encode() + b"\0"),
                kind,
                flags,
                address,
                offsets[index - 1],
                size,
                link,
                info,
                align,
                entsize,
            )
        )
    ident = b"\x7fELF" + bytes([1 if bits == 32 else 2, 1 if little else 2, 1]) + bytes(9)
    fmt = "HHIIIIIHHHHHH" if bits == 32 else "HHIQQQIHHHHHH"
    header = struct.pack(
        order + fmt,
        2,
        243 if bits == 32 else 183,
        1,
        0x2000,
        0,
        headers_offset,
        0,
        header_size,
        0,
        0,
        section_header_size,
        len(sections) + 1,
        len(sections),
    )
    contents[:header_size] = ident + header
    path.write_bytes(contents)
    return path


def manifest_fixture(tmp_path, bits=64):
    entry_bytes = bits // 4
    return {
        "version": 1,
        "tables": [
            {
                "section": "isr_table",
                "symbol": "_sw_isr_table",
                "end_symbol": "_sw_isr_table_end",
                "alignment": bits // 8,
                "size": 2 * entry_bytes,
                "writable": True,
                "inputs": [
                    {
                        "name": ".irq.first",
                        "offset": 0,
                        "size": entry_bytes,
                        "alignment": bits // 8,
                        "symbol": "entry_first",
                    },
                    {
                        "name": ".irq.second",
                        "offset": entry_bytes,
                        "size": entry_bytes,
                        "alignment": bits // 8,
                        "symbol": "entry_second",
                    },
                ],
            }
        ],
    }


def check(tmp_path, manifest, final=None, prelinked=None):
    manifest_path = tmp_path / "layout.json"
    manifest_path.write_text(json.dumps(manifest))
    final = final or elf_fixture(tmp_path / "final.elf")
    return CHECKER.run(final, manifest_path, prelinked)


@pytest.mark.parametrize("bits", [32, 64])
@pytest.mark.parametrize("little", [False, True])
def test_accepts_linker_aliases_with_zero_size(tmp_path, bits, little):
    final = elf_fixture(tmp_path / "final.elf", bits, little)
    code, report = check(tmp_path, manifest_fixture(tmp_path, bits), final)
    assert code == 0, report
    assert report["byte_order"] == ("little" if little else "big")


@pytest.mark.parametrize("bits", [32, 64])
@pytest.mark.parametrize(
    "symbol",
    [
        "_sw_isr_table",
        "_sw_isr_table_end",
        "entry_first",
        "entry_first_end",
    ],
)
@pytest.mark.parametrize("section", [0xFFF1, 0xFFF2, 0xFF00])
def test_only_absolute_or_section_bound_aliases_have_linked_addresses(
    tmp_path, bits, symbol, section
):
    final = elf_fixture(tmp_path / "final.elf", bits, symbol_sections={symbol: section})
    code, report = check(tmp_path, manifest_fixture(tmp_path, bits), final)
    assert code == (0 if section == 0xFFF1 else 1), report
    if section != 0xFFF1:
        assert any(
            not item["passed"] and item["name"].endswith(".defined_section")
            for item in report["checks"]
        )


def test_logical_layout_name_need_not_be_the_elf_section_name(tmp_path):
    manifest = manifest_fixture(tmp_path)
    manifest["tables"][0]["section"] = "irq_vector_table"
    assert check(tmp_path, manifest)[0] == 0


def test_explicit_elf_section_must_contain_the_table(tmp_path):
    manifest = manifest_fixture(tmp_path)
    manifest["tables"][0]["elf_section"] = ".objects"
    code, report = check(tmp_path, manifest)
    assert code == 1
    assert any(
        item["name"] == "_sw_isr_table.symbol_section" and not item["passed"]
        for item in report["checks"]
    )


def test_rejects_swapped_offsets_even_when_total_size_is_right(tmp_path):
    manifest = manifest_fixture(tmp_path)
    inputs = manifest["tables"][0]["inputs"]
    inputs[0]["offset"], inputs[1]["offset"] = inputs[1]["offset"], inputs[0]["offset"]
    code, report = check(tmp_path, manifest)
    assert code == 1
    assert any(
        not item["passed"] and item["name"].endswith(".address") for item in report["checks"]
    )


def test_rejects_incorrect_table_capacity(tmp_path):
    manifest = manifest_fixture(tmp_path)
    manifest["tables"][0]["size"] += 16
    code, report = check(tmp_path, manifest)
    assert code == 1
    assert any(
        not item["passed"] and item["name"] == "_sw_isr_table.size" for item in report["checks"]
    )


def test_rejects_input_alias_to_another_object(tmp_path):
    manifest = manifest_fixture(tmp_path)
    manifest["tables"][0]["inputs"][0]["symbol"] = "argument"
    code, report = check(tmp_path, manifest)
    assert code == 1
    assert any(
        not item["passed"] and item["name"].endswith(".symbol_section") for item in report["checks"]
    )


def test_rejects_input_extending_past_reserved_table(tmp_path):
    manifest = manifest_fixture(tmp_path)
    manifest["tables"][0]["inputs"][1]["size"] += 16
    code, report = check(tmp_path, manifest)
    assert code == 1
    assert any(
        not item["passed"] and item["name"].endswith(".within_table") for item in report["checks"]
    )


@pytest.mark.parametrize(
    "residue,placeholder",
    [
        (".gnu.linkonce.sw_isr_table", False),
        (".gnu.linkonce.shared_sw_isr_table", False),
        (".isr_pre0.storage", False),
        (".text.isr_pre0_get_entry", False),
        (".isr_vector_owners", False),
        (".isr_vector_refs", False),
        (None, True),
        (None, "z_isr_pre0_sw"),
    ],
)
def test_rejects_obsolete_isr_storage(tmp_path, residue, placeholder):
    final = elf_fixture(tmp_path / "final.elf", residue=residue, placeholder=placeholder)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final)
    assert code == 1
    assert any(
        not item["passed"] and item["name"].startswith("final.no_") for item in report["checks"]
    )


def test_does_not_forbid_unrelated_linkonce_sections(tmp_path):
    final = elf_fixture(tmp_path / "final.elf", residue=".gnu.linkonce.unrelated")
    assert check(tmp_path, manifest_fixture(tmp_path), final)[0] == 0


def test_gperf_additions_and_used_ends_can_change_inside_reservations(tmp_path):
    prelinked = elf_fixture(tmp_path / "pre1.elf", capacity=True, used_size=32)
    final = elf_fixture(tmp_path / "final.elf", capacity=True, used_size=16, generated=True)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final, prelinked)
    assert code == 0, report
    assert "z_object_gperf_find" in report["stage"]["symbols_only_final"]


@pytest.mark.parametrize(
    "change,suffix",
    [
        ({"handler_delta": 4}, "stage.handler.value"),
        ({"argument_delta": 8}, "stage.argument.value"),
        ({"handler_size": 8}, "stage.handler.size"),
    ],
)
def test_rejects_movement_of_common_public_objects(tmp_path, change, suffix):
    prelinked = elf_fixture(tmp_path / "pre1.elf")
    final = elf_fixture(tmp_path / "final.elf", **change)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final, prelinked)
    assert code == 1
    assert any(item["name"] == suffix and not item["passed"] for item in report["checks"])


def test_local_symbol_does_not_hide_moved_public_function(tmp_path):
    prelinked = elf_fixture(tmp_path / "pre1.elf", local_handler=True)
    final = elf_fixture(tmp_path / "final.elf", local_handler=True, handler_delta=4)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final, prelinked)
    assert code == 1
    assert any(
        item["name"] == "stage.handler.value" and not item["passed"] for item in report["checks"]
    )


def test_boolean_is_not_a_manifest_version(tmp_path):
    manifest = manifest_fixture(tmp_path)
    manifest["version"] = True
    code, report = check(tmp_path, manifest)
    assert code == 2
    assert report["status"] == "input_error"


@pytest.mark.parametrize(
    "symbol",
    [
        "z_object_gperf_find",
        "z_object_gperf_wordlist_foreach",
        "k_object_find",
        "k_object_wordlist_foreach",
    ],
)
def test_known_weak_lookup_can_be_replaced_inside_text_reservation(tmp_path, symbol):
    prelinked = elf_fixture(tmp_path / "pre1.elf", lookup_name=symbol)
    final = elf_fixture(tmp_path / "final.elf", lookup_name=symbol, generated_lookup=True)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final, prelinked)
    assert code == 0, report
    assert any(item["name"].endswith(".reservation_stable") for item in report["checks"])


@pytest.mark.parametrize(
    "change,failed_check",
    [
        ({"lookup_delta": 512, "text_used": 616}, ".used_fits"),
        ({"lookup_delta": 4}, ".function_fits"),
        ({"text_budget": 256}, ".reservation_stable"),
        ({"text_budget": 1024}, ".reservation_within_section"),
    ],
)
def test_generated_lookup_must_fit_unchanged_text_budget(tmp_path, change, failed_check):
    symbol = "z_object_gperf_find"
    prelinked = elf_fixture(tmp_path / "pre1.elf", lookup_name=symbol)
    final = elf_fixture(tmp_path / "final.elf", lookup_name=symbol, generated_lookup=True, **change)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final, prelinked)
    assert code == 1
    assert any(
        item["name"].endswith(failed_check) and not item["passed"] for item in report["checks"]
    )


def test_other_gperf_named_functions_are_not_exempt(tmp_path):
    symbol = "z_object_gperf_unrelated"
    prelinked = elf_fixture(tmp_path / "pre1.elf", lookup_name=symbol)
    final = elf_fixture(tmp_path / "final.elf", lookup_name=symbol, generated_lookup=True)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final, prelinked)
    assert code == 1
    assert any(
        item["name"] == f"stage.{symbol}.size" and not item["passed"] for item in report["checks"]
    )


def test_rejects_used_storage_exceeding_prelinked_budget(tmp_path):
    prelinked = elf_fixture(tmp_path / "pre1.elf", capacity=True, used_size=32)
    final = elf_fixture(tmp_path / "final.elf", capacity=True, used_size=40)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final, prelinked)
    assert code == 1
    assert any(
        item["name"].endswith(".used_fits") and not item["passed"] for item in report["checks"]
    )


def test_rejects_change_to_reserved_section_extent(tmp_path):
    prelinked = elf_fixture(tmp_path / "pre1.elf", capacity=True, used_size=32)
    final = elf_fixture(tmp_path / "final.elf", capacity=True, reserved_size=40)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final, prelinked)
    assert code == 1
    assert any(
        item["name"].endswith(".reserved_size") and not item["passed"] for item in report["checks"]
    )


def test_missing_alias_is_a_verification_failure(tmp_path):
    manifest = manifest_fixture(tmp_path)
    manifest["tables"][0]["inputs"][0]["symbol"] = "missing_alias"
    code, report = check(tmp_path, manifest)
    assert code == 1
    assert report["errors"][0]["code"] == "symbol_resolution"


def test_cli_writes_report_without_traceback_for_bad_input(tmp_path):
    broken = tmp_path / "bad.elf"
    broken.write_bytes(b"not an ELF")
    manifest_path = tmp_path / "layout.json"
    manifest_path.write_text(json.dumps(manifest_fixture(tmp_path)))
    report_path = tmp_path / "report.json"
    result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            "--elf",
            str(broken),
            "--manifest",
            str(manifest_path),
            "--report",
            str(report_path),
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    assert result.returncode == 2
    assert "Traceback" not in result.stderr
    assert json.loads(report_path.read_text())["status"] == "input_error"
    assert "invalid_input" in result.stderr
    assert str(report_path) in result.stderr


def test_cli_reports_failed_check_and_report_path(tmp_path):
    final = elf_fixture(tmp_path / "final.elf")
    manifest = manifest_fixture(tmp_path)
    manifest["tables"][0]["size"] += 16
    manifest_path = tmp_path / "layout.json"
    manifest_path.write_text(json.dumps(manifest))
    report_path = tmp_path / "report.json"
    result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            "--elf",
            str(final),
            "--manifest",
            str(manifest_path),
            "--report",
            str(report_path),
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    assert result.returncode == 1
    assert "_sw_isr_table.size: actual=32, expected=48" in result.stderr
    assert str(report_path) in result.stderr
    assert len(result.stderr.splitlines()) <= CHECKER.DIAGNOSTIC_LIMIT + 2
    assert json.loads(report_path.read_text())["status"] == "fail"


def test_missing_input_used_end_is_a_verification_failure(tmp_path):
    final = elf_fixture(tmp_path / "final.elf", omit_first_end=True)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final)
    assert code == 1
    assert any(
        item["name"].endswith(".input..irq.first.end.unique_symbol") and not item["passed"]
        for item in report["checks"]
    )


@pytest.mark.parametrize("used_bytes", [0, 8])
def test_padding_cannot_hide_missing_or_short_input(tmp_path, used_bytes):
    # The section and table still contain all reserved bytes. Only the real
    # input's end marker distinguishes absent/short contents from padding.
    final = elf_fixture(tmp_path / "final.elf", first_end_delta=used_bytes - 16)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final)
    assert code == 1
    assert any(
        item["name"].endswith(".input..irq.first.used_size")
        and item["actual"] == used_bytes
        and not item["passed"]
        for item in report["checks"]
    )


def test_input_used_end_must_be_unique(tmp_path):
    final = elf_fixture(tmp_path / "final.elf", duplicate_first_end=True)
    code, report = check(tmp_path, manifest_fixture(tmp_path), final)
    assert code == 1
    assert any(
        item["name"].endswith(".input..irq.first.end.unique_symbol") and not item["passed"]
        for item in report["checks"]
    )


def test_empty_table_does_not_require_input_markers(tmp_path):
    final = elf_fixture(tmp_path / "final.elf", empty_table=True)
    manifest = manifest_fixture(tmp_path)
    manifest["tables"][0]["inputs"] = []
    manifest["tables"][0]["size"] = 0
    code, report = check(tmp_path, manifest, final)
    assert code == 0, report


@pytest.mark.parametrize("bits,little", [(32, True), (32, False), (64, True), (64, False)])
def test_rejects_uninitialized_isr_table(tmp_path, bits, little):
    final = elf_fixture(tmp_path / "nobits.elf", bits, little, table_nobits=True)
    code, report = check(tmp_path, manifest_fixture(tmp_path, bits), final)
    assert code == 1, report
    assert any(
        not item["passed"] and item["name"].endswith(".file_backed") for item in report["checks"]
    )


@pytest.mark.parametrize("bits,little", [(32, True), (32, False), (64, True), (64, False)])
@pytest.mark.parametrize(
    "count,second_arg,accepted",
    [
        (0, 0x3000, True),
        (1, 0x3000, True),
        (2, 0x3000, False),
        (2, 0x3004, True),
        (3, 0x3004, False),
    ],
)
def test_shared_registration_pairs(tmp_path, bits, little, count, second_arg, accepted):
    width = bits // 8
    word = "I" if bits == 32 else "Q"
    data = struct.pack(
        ("<" if little else ">") + word * 5, 0x3000, 0x2000, second_arg, 0x2000, count
    )
    final = elf_fixture(tmp_path / "shared.elf", bits, little, table_data=data)
    manifest = manifest_fixture(tmp_path, bits)
    manifest["tables"][0].update(section="shared_sw_isr_table", size=len(data))
    manifest["metadata"] = {
        "entry_size": width * 2,
        "arg_offset": 0,
        "arg_size": width,
        "isr_offset": width,
        "isr_size": width,
        "shared_size": len(data),
        "shared_count_offset": width * 4,
        "shared_count_size": width,
    }
    code, report = check(tmp_path, manifest, final)
    assert code == (0 if accepted else 1), report
    if not accepted:
        assert any(
            not item["passed"]
            and item["name"].endswith(".unique_pair" if count == 2 else ".count_fits")
            for item in report["checks"]
        )
    # Duplicate clients in pre1 must not be hidden by a valid final image.
    if count == 2 and not accepted:
        valid_data = struct.pack(
            ("<" if little else ">") + word * 5, 0x3000, 0x2000, 0x3004, 0x2000, count
        )
        valid = elf_fixture(tmp_path / "valid-final.elf", bits, little, table_data=valid_data)
        code, report = check(tmp_path, manifest, valid, final)
        assert code == 1, report
        assert any(
            not item["passed"]
            and item["name"].startswith("prelinked.shared.")
            and item["name"].endswith(".unique_pair")
            for item in report["checks"]
        )


@pytest.mark.parametrize("bits,little", [(32, True), (32, False), (64, True), (64, False)])
@pytest.mark.parametrize("duplicate", [False, True])
def test_shared_target_abi_offsets_and_padding(tmp_path, bits, little, duplicate):
    width = bits // 8
    word = "I" if bits == 32 else "Q"
    # ISR-first entries, padding before the count and trailing structure padding.
    data = struct.pack(
        ("<" if little else ">") + word * 7,
        0x2000,
        0x3000,
        0x2000,
        0x3000 if duplicate else 0x3004,
        0xDEAD,
        2,
        0xBEEF,
    )
    image = elf_fixture(tmp_path / "padded-shared.elf", bits, little, table_data=data)
    manifest = manifest_fixture(tmp_path, bits)
    manifest["tables"][0].update(section="shared_sw_isr_table", size=len(data))
    manifest["metadata"] = {
        "entry_size": width * 2,
        "arg_offset": width,
        "arg_size": width,
        "isr_offset": 0,
        "isr_size": width,
        "shared_size": len(data),
        "shared_count_offset": width * 5,
        "shared_count_size": width,
    }
    code, report = check(tmp_path, manifest, image)
    assert code == (1 if duplicate else 0), report
    if duplicate:
        assert any(
            not item["passed"] and item["name"].endswith(".unique_pair")
            for item in report["checks"]
        )
