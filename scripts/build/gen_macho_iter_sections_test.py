#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import gen_macho_iter_sections as sut
import pytest


def test_build_mapping_is_deterministic_and_fits_macho_limit():
    names = ["ztest_unit_test", "ztest_expected_result_entry"]

    mapping = sut.build_mapping(names)

    assert mapping == sut.build_mapping(list(reversed(names)))
    assert all(len(short_name) <= 16 for short_name in mapping.values())
    assert all(short_name.isascii() for short_name in mapping.values())


def test_build_mapping_rejects_duplicate_logical_names():
    with pytest.raises(ValueError, match="duplicate logical section"):
        sut.build_mapping(["ztest_unit_test", "ztest_unit_test"])


def test_build_mapping_rejects_short_name_collisions():
    with pytest.raises(ValueError, match="short section name collision"):
        sut.build_mapping(["first", "second"], shorten_fn=lambda _name: "zcollision")


def test_write_outputs_contains_compiler_and_alias_metadata(tmp_path):
    names = ["ztest_unit_test", "unused"]
    mapping = sut.build_mapping(names)
    header = tmp_path / "macho_sections.h"
    aliases = tmp_path / "macho_sections.c"

    sut.write_outputs(mapping, header, aliases, ["ztest_unit_test"])

    short_name = mapping["ztest_unit_test"]
    assert f"#define Z_MACHO_SEC__ztest_unit_test {short_name}" in header.read_text()
    assert f"#define Z_MACHO_SEC_ztest_unit_test {short_name}" in header.read_text()
    assert f'__DATA${short_name}' in aliases.read_text()
    assert mapping["unused"] not in aliases.read_text()


def test_parse_source_names_reads_iterable_section_calls(tmp_path):
    source = tmp_path / "sections.cmake"
    source.write_text(
        "zephyr_iterable_section(NAME first)\n"
        "zephyr_iterable_section(\n  NAME second\n)\n"
        "Z_LINK_ITERABLE(third)\n"
    )

    assert sut.parse_source_names([source]) == ["first", "second", "third"]


def test_parse_source_names_reads_compiler_section_helpers(tmp_path):
    source = tmp_path / "sections.h"
    source.write_text(
        "STRUCT_SECTION_ITERABLE(ztest_unit_test, test)\n"
        "STRUCT_SECTION_ITERABLE(_static_thread_data, test)\n"
        "__in_section(_log_strings, static, 0)\n"
        "__in_section(__static_thread_data, static, 0)\n"
        "#define _NOINIT_SECTION_NAME noinit\n"
    )

    assert sut.parse_source_names([source]) == [
        "ztest_unit_test",
        "_static_thread_data",
        "log_strings",
        "_static_thread_data",
        "noinit",
    ]


def test_parse_linker_script_names_reads_boundary_symbols(tmp_path):
    linker_script = tmp_path / "sections.ld"
    linker_script.write_text("_ztest_unit_test_list_start = .;\n_ztest_unit_test_list_end = .;\n")

    assert sut.parse_linker_script_names([linker_script]) == ["ztest_unit_test"]


def test_parse_alias_names_reads_boundary_references(tmp_path):
    source = tmp_path / "references.h"
    source.write_text(
        "STRUCT_SECTION_FOREACH(device, dev) {}\n"
        "TYPE_SECTION_START(log_const)\n"
        "TYPE_SECTION_END_EXTERN(struct log_source_dynamic_data, log_dynamic)\n"
        "extern struct ztest_unit_test _ztest_unit_test_list_start[];\n"
    )

    assert sut.parse_alias_names([source]) == [
        "device",
        "log_const",
        "log_dynamic",
        "ztest_unit_test",
    ]
