#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the build.log side of size_calc.py
"""

from unittest import mock

import pytest
from twisterlib.size_calc import SizeCalculator

HEADER = 'Memory region         Used Size  Region Size  %age Used'
FLASH = '           FLASH:       12345 B       256 KB      4.71%'
RAM = '             RAM:        4096 B        64 KB      6.25%'
IDT_LIST = '        IDT_LIST:           0 GB         2 KB      0.00%'

REPORT = [HEADER, FLASH, RAM, IDT_LIST]
PREAMBLE = [
    '-- Configuring done',
    '[1/2] Building C object zephyr/CMakeFiles/zephyr.dir/main.c.obj',
    '[2/2] Linking C executable zephyr/zephyr.elf',
]


def make_log(tmp_path, lines):
    log = tmp_path / 'build.log'
    log.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    return log


def calculator(tmp_path, lines):
    """A SizeCalculator that read the given build log and no ELF.

    _calculate_sizes() always analyses the ELF, to report unrecognised
    sections, but nothing here is about the ELF.
    """
    log = make_log(tmp_path, lines)
    with mock.patch.object(SizeCalculator, '_analyze_elf_file'):
        return SizeCalculator('dummy.elf', [], str(log), generate_warning=False)


def bare():
    """A SizeCalculator whose string handling can be called on its own."""
    calc = SizeCalculator.__new__(SizeCalculator)
    calc.buildlog_filename = 'build.log'
    calc.generate_warning = False
    return calc


class TestFindingTheReport:
    def test_the_offset_counts_back_from_the_end(self):
        content = [ln + '\n' for ln in PREAMBLE + REPORT]

        offset = bare()._find_offset_of_last_pattern_occurrence(content)

        assert content[len(content) - offset] == HEADER + '\n'

    def test_the_last_report_is_the_one_found(self):
        earlier = [ln.replace('12345', '99999') for ln in REPORT]
        content = [ln + '\n' for ln in PREAMBLE + earlier + [''] + REPORT]

        offset = bare()._find_offset_of_last_pattern_occurrence(content)

        assert content[len(content) - offset :] == [ln + '\n' for ln in REPORT]

    def test_the_marker_has_to_start_the_line(self):
        """A line that only mentions the table is not the table."""
        note = '-- Note: the Memory region table above is advisory'
        content = [ln + '\n' for ln in PREAMBLE + REPORT + [note]]

        offset = bare()._find_offset_of_last_pattern_occurrence(content)

        assert content[len(content) - offset] == HEADER + '\n'

    def test_no_report_at_all(self):
        content = [ln + '\n' for ln in PREAMBLE]

        assert bare()._find_offset_of_last_pattern_occurrence(content) == -1

    def test_an_empty_file(self):
        assert bare()._find_offset_of_last_pattern_occurrence([]) == -1


class TestTakingTheLines:
    def test_the_four_lines_of_the_report(self):
        content = [ln + '\n' for ln in PREAMBLE + REPORT]

        lines = bare()._get_lines_with_footprint(len(REPORT), content)

        assert lines == [ln + '\n' for ln in REPORT]

    def test_a_report_that_runs_off_the_end(self):
        """Only three lines are left, so only three come back."""
        content = [ln + '\n' for ln in PREAMBLE + REPORT[:3]]

        lines = bare()._get_lines_with_footprint(3, content)

        assert lines == [ln + '\n' for ln in REPORT[:3]]

    def test_an_offset_that_is_the_whole_file(self):
        """The largest offset that still points at the first line.

        One more than this is past the front of the file and falls back to
        the last line instead, so the two sides of that comparison are a
        whole report apart rather than the same line.
        """
        content = [ln + '\n' for ln in REPORT]

        assert bare()._get_lines_with_footprint(len(content), content) == content

    def test_an_offset_that_makes_no_sense_falls_back_to_the_last_line(self):
        content = [ln + '\n' for ln in PREAMBLE]

        assert bare()._get_lines_with_footprint(0, content) == [content[-1]]
        assert bare()._get_lines_with_footprint(999, content) == [content[-1]]

    def test_an_empty_file(self):
        assert bare()._get_lines_with_footprint(4, []) == []


class TestSplittingTheTable:
    def test_the_newline_and_the_percent_sign_go(self):
        assert bare()._clear_whitespaces_from_lines([FLASH + '\n']) == [FLASH[:-1]]

    def test_nothing_to_clear(self):
        assert bare()._clear_whitespaces_from_lines([]) == []

    def test_columns_are_split_on_runs_of_spaces(self):
        rows = bare()._divide_text_lines_into_columns(
            bare()._clear_whitespaces_from_lines([HEADER + '\n', FLASH + '\n'])
        )

        assert rows == [
            ['Memory region', 'Used Size', 'Region Size', '%age Used'],
            ['FLASH', '12345 B', '256 KB', '4.71'],
        ]

    def test_no_lines_give_one_empty_row(self):
        assert bare()._divide_text_lines_into_columns([]) == [[]]


TESTDATA_PREFIX = [
    ('12345 B', '12345'),
    ('256 KB', '262144'),
    ('2 MB', '2097152'),
    ('1 GB', '1073741824'),
    ('0 GB', '0'),
    # Columns that are not a size come back as they are, only right-stripped.
    ('FLASH', 'FLASH'),
    ('4.71', '4.71'),
    ('Region Size ', 'Region Size'),
]


@pytest.mark.parametrize('value, expected', TESTDATA_PREFIX)
def test_binary_prefix_converter(value, expected):
    assert bare()._binary_prefix_converter(value) == expected


class TestUnifyingThePrefixes:
    def test_the_first_row_is_skipped_whatever_is_in_it(self):
        """Row zero is the column headings, so the converter never sees it.

        A real heading comes back unchanged either way, being nothing the
        converter would rewrite -- so a heading shaped like a value is what
        makes the skip observable at all.
        """
        rows = [
            ['Memory region', '1 KB'],
            ['FLASH', '1 KB'],
            ['RAM', '1 KB'],
            ['IDT_LIST', '1 KB'],
        ]

        out = bare()._unify_prefixes_on_all_values(rows)

        assert out[0] == ['Memory region', '1 KB']
        assert out[1:] == [['FLASH', '1024'], ['RAM', '1024'], ['IDT_LIST', '1024']]

    def test_the_header_row_is_left_alone(self):
        rows = [
            ['Memory region', 'Used Size', 'Region Size', '%age Used'],
            ['FLASH', '12345 B', '256 KB', '4.71'],
            ['RAM', '4096 B', '64 KB', '6.25'],
            ['IDT_LIST', '0 GB', '2 KB', '0.00'],
        ]

        out = bare()._unify_prefixes_on_all_values(rows)

        assert out[0] == ['Memory region', 'Used Size', 'Region Size', '%age Used']
        assert out[1] == ['FLASH', '12345', '262144', '4.71']
        assert out[2] == ['RAM', '4096', '65536', '6.25']

    def test_a_table_of_the_wrong_height_is_thrown_away(self):
        """Four rows are expected: a header and three memory regions."""
        rows = [['Memory region'], ['FLASH', '1 B', '2 KB', '0.1']]

        assert bare()._unify_prefixes_on_all_values(rows) == [[]]


class TestFootprintFromBuildLog:
    def test_an_ordinary_report(self, tmp_path):
        calc = calculator(tmp_path, PREAMBLE + REPORT)

        assert calc.get_used_rom() == 12345
        assert calc.get_available_rom() == 256 * 1024
        assert calc.get_used_ram() == 4096
        assert calc.get_available_ram() == 64 * 1024

    def test_the_last_report_in_the_file_is_the_one_used(self, tmp_path):
        earlier = [ln.replace('12345', '99999') for ln in REPORT]
        calc = calculator(tmp_path, PREAMBLE + earlier + [''] + REPORT)

        assert calc.get_used_rom() == 12345

    def test_regions_after_the_third_are_not_read(self, tmp_path):
        extra = '            ITCM:         512 B        16 KB      3.13%'
        calc = calculator(tmp_path, PREAMBLE + REPORT + [extra])

        assert calc.get_used_rom() == 12345
        assert calc.get_used_ram() == 4096

    def test_a_log_with_no_report_gives_zeros(self, tmp_path):
        calc = calculator(tmp_path, PREAMBLE)

        assert calc.get_used_rom() == 0
        assert calc.get_used_ram() == 0
        assert calc.get_available_rom() == 0
        assert calc.get_available_ram() == 0

    def test_a_build_log_that_is_not_there_gives_zeros(self, tmp_path):
        with mock.patch.object(SizeCalculator, '_analyze_elf_file'):
            calc = SizeCalculator(
                'dummy.elf', [], str(tmp_path / 'build.log'), generate_warning=False
            )

        assert calc.get_used_rom() == 0
        assert calc.get_used_ram() == 0

    def test_a_log_that_is_not_named_build_log_is_not_read(self, tmp_path):
        """The footprint is only taken from a file called build.log."""
        other = tmp_path / 'other.log'
        other.write_text('\n'.join(PREAMBLE + REPORT) + '\n', encoding='utf-8')

        with mock.patch.object(SizeCalculator, '_analyze_elf_file'):
            calc = SizeCalculator('dummy.elf', [], str(other), generate_warning=False)

        assert calc.get_used_rom() == 0
