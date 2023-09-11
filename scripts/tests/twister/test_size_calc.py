#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for scl.py functions
"""

import mock
import os
import pytest
import sys

from contextlib import nullcontext

from twisterlib.size_calc import SizeCalculator


TESTDATA_1 = [
    ('qemu_x86-hello_world.elf', [], '', 20512, 83268, 0, 0,
     ['device_area', 'log_const_area', 'tbss', 'device_states', 'pagetables']),
    ('qemu_x86-hello_world.elf', ['pagetables', 'tbss'], '', 20512, 83268, 0, 0,
     ['device_area', 'log_const_area', 'device_states']),
    ('frdm_k64f-hello_world.elf', [], 'frdm_k64f-hello_world-build.log',
     13552, 4172, 1048576, 196608,
     ['device_area', 'device_states', 'log_const_area', 'tbss'])
]

@pytest.mark.parametrize(
    'filename, extra_sections, logfile_name,' \
    ' expected_used_rom, expected_used_ram,' \
    ' expected_available_rom, expected_available_ram,' \
    ' expected_unrecognised_sections',
    TESTDATA_1,
    ids=['qemu_x86 hello_world', 'qemu x86 hello_world with extra sections',
         'frdm_k64f hello_world']
)
def test_getters(
    test_data,
    filename,
    extra_sections,
    logfile_name,
    expected_used_rom,
    expected_used_ram,
    expected_available_rom,
    expected_available_ram,
    expected_unrecognised_sections
):
    elf_path = os.path.join(test_data, 'elfs', filename)
    logfile_path = os.path.join(test_data, 'elfs', logfile_name) \
                    if logfile_name else ''

    sc = SizeCalculator(elf_path, extra_sections,
                        buildlog_filepath=logfile_path)

    assert sc.get_used_ram() == expected_used_ram
    assert sc.get_used_rom() == expected_used_rom
    assert sc.get_available_ram() == expected_available_ram
    assert sc.get_available_rom() == expected_available_rom

    assert sorted(sc.unrecognized_sections()) == \
           sorted(expected_unrecognised_sections)


TESTDATA_2 = [
    (
        'qemu_x86-hello_world.elf',
        '',
        'SECTION NAME             VMA        LMA     SIZE  HEX SZ TYPE\n' \
        'text              0x00100000 0x00100000    16384 0x04000 ro     \n' \
        'initlevel         0x00104000 0x00104000       56 0x00038 rw     \n' \
        'device_area       0x00104038 0x00104038       80 0x00050 unknown\n' \
        'log_const_area    0x00104088 0x00104088       24 0x00018 unknown\n' \
        'tbss              0x001040a0 0x001040a0        4 0x00004 unknown\n' \
        'rodata            0x001040a0 0x001040a0     2680 0x00a78 ro     \n' \
        'bss               0x00104b20 0x00104b20    42968 0x0a7d8 alloc  \n' \
        'noinit            0x0010f2f8 0x0010f2f8    19788 0x04d4c alloc  \n' \
        'datas             0x00114060 0x00114060     1392 0x00570 rw     \n' \
        'device_states     0x001145d0 0x001145d0        8 0x00008 unknown\n' \
        'pagetables        0x001145d8 0x001145d8    23112 0x05a48 unknown\n' \
        'Totals: 20512 bytes (ROM), 83268 bytes (RAM)\n' \
    ),
    (
        'frdm_k64f-hello_world.elf',
        'frdm_k64f-hello_world-build.log',
        'Totals: 13552 bytes (ROM), 4172 bytes (RAM)\n' \
    )
]

@pytest.mark.parametrize(
    'filename, logfile_name, expected_report',
    TESTDATA_2,
    ids=['qemu_x86 hello_world', 'frdm_k64f hello_world']
)
def test_size_report(
    capfd,
    test_data,
    filename,
    logfile_name,
    expected_report
):
    elf_path = os.path.join(test_data, 'elfs', filename)
    logfile_path = os.path.join(test_data, 'elfs', logfile_name) \
                    if logfile_name else ''

    sc = SizeCalculator(elf_path, [], buildlog_filepath=logfile_path)

    sc.size_report()

    out, err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    assert expected_report in out


TESTDATA_3 = [
    ('dummy_build.log', True),
    ('not_a_buildlog_file.txt', False),
]

@pytest.mark.parametrize(
    'logfile_name, footprint',
    TESTDATA_3,
    ids=['valid', 'not valid']
)
def test_calculate_sizes(logfile_name, footprint):
    analyze_mock = mock.Mock()
    footprint_mock = mock.Mock()

    with mock.patch.object(SizeCalculator, '_analyze_elf_file', analyze_mock), \
         mock.patch.object(SizeCalculator, '_get_footprint_from_buildlog',
                           footprint_mock):
        SizeCalculator('dummy.elf', [], buildlog_filepath=logfile_name)

    analyze_mock.assert_called_once()

    if footprint:
        footprint_mock.assert_called_once()
    else:
        footprint_mock.assert_not_called()


TESTDATA_4 = [
    (b'\x7fELF', None, None),
    (b'\xEF\xBB\xBF\x24', 2, 'dummy.elf is not an ELF binary')
]

@pytest.mark.parametrize(
    'file_contents, exit_code, printout',
    TESTDATA_4,
    ids=['valid', 'not valid']
)
def test_check_elf_file(capfd, file_contents, exit_code, printout):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [])

    open_mock = mock.mock_open(read_data=file_contents)

    with mock.patch('builtins.open', open_mock), \
         pytest.raises(SystemExit) if exit_code else nullcontext() as sys_exit:
        sc._check_elf_file()

    open_mock.assert_called_once_with('dummy.elf', 'rb')

    if exit_code:
        assert str(sys_exit.value) == str(exit_code)

    if printout:
        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert printout in out


TESTDATA_5 = [
    ('CONFIG_XIP'.encode('utf-8'), None, True),
    (b'', None, False),
    (' no symbols '.encode('utf-8'), 2, None),
]

@pytest.mark.parametrize(
    'check_output_res, exit_code, expected_out',
    TESTDATA_5,
    ids=['xip', 'not xip', 'no symbols']
)
def test_check_is_xip(capfd, check_output_res, exit_code, expected_out):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [])

    with mock.patch('subprocess.check_output', return_value=check_output_res), \
         pytest.raises(SystemExit) if exit_code else nullcontext() as sys_exit:
        sc._check_is_xip()

    if exit_code:
        assert str(sys_exit.value) == str(exit_code)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert 'dummy.elf has no symbol information' in out
    else:
        assert sc.is_xip == expected_out


TESTDATA_6 = [
    (
"""dummy.elf:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 text          00004000  00100000  00100000  000000c0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 initlevel     00000038  00104000  00104000  000040c0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 device_area   00000050  00104038  00104038  000040f8  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 log_const_area 00000018  00104088  00104088  00004148  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 tbss          00000004  001040a0  001040a0  00004160  2**2
                  ALLOC, THREAD_LOCAL
  5 rodata        00000a78  001040a0  001040a0  00004160  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  6 bss           0000a7d8  00104b20  00104b20  00004bd8  2**5
                  ALLOC
  7 noinit        00004d4c  0010f2f8  0010f2f8  00004bd8  2**3
                  ALLOC
  8 datas         00000570  00114060  00114060  00004be0  2**5
                  CONTENTS, ALLOC, LOAD, DATA
  9 device_states 00000008  001145d0  001145d0  00005150  2**0
                  CONTENTS, ALLOC, LOAD, DATA
 10 pagetables    00005a40  001145e0  001145e0  00005160  2**5
                  CONTENTS, ALLOC, LOAD, DATA
 11 intList       00000050  ffff1000  ffff1000  0000aba0  2**2
                  CONTENTS, ALLOC, LOAD, DATA
 12 .comment      00000020  00000000  00000000  0000abf0  2**0
                  CONTENTS, READONLY
 13 .debug_aranges 00000da0  00000000  00000000  0000ac10  2**3
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 14 .debug_info   0002b4a0  00000000  00000000  0000b9b0  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 15 .debug_abbrev 00007442  00000000  00000000  00036e50  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 16 .debug_line   00013a2a  00000000  00000000  0003e292  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 17 .debug_frame  000029b4  00000000  00000000  00051cbc  2**2
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 18 .debug_str    000058cd  00000000  00000000  00054670  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 19 .debug_loc    00010f2e  00000000  00000000  00059f3d  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 20 .debug_ranges 000032c0  00000000  00000000  0006ae70  2**3
                  CONTENTS, READONLY, DEBUGGING, OCTETS
""".encode('utf-8'),
        '',
        False,
        [{'load_addr': 1048576, 'name': 'text', 'recognized': True,
         'size': 16384, 'type': 'ro', 'virt_addr': 1048576},
        {'load_addr': 1064960, 'name': 'initlevel', 'recognized': True,
         'size': 56, 'type': 'rw', 'virt_addr': 1064960},
        {'load_addr': 1065016, 'name': 'device_area', 'recognized': False,
         'size': 80, 'type': 'unknown', 'virt_addr': 1065016},
        {'load_addr': 1065096, 'name': 'log_const_area', 'recognized': False,
         'size': 24, 'type': 'unknown', 'virt_addr': 1065096},
        {'load_addr': 1065120, 'name': 'tbss', 'recognized': False,
         'size': 4, 'type': 'unknown', 'virt_addr': 1065120},
        {'load_addr': 1065120, 'name': 'rodata', 'recognized': True,
         'size': 2680, 'type': 'ro', 'virt_addr': 1065120},
        {'load_addr': 1067808, 'name': 'bss', 'recognized': True,
         'size': 42968, 'type': 'alloc', 'virt_addr': 1067808},
        {'load_addr': 1110776, 'name': 'noinit', 'recognized': True,
         'size': 19788, 'type': 'alloc', 'virt_addr': 1110776},
        {'load_addr': 1130592, 'name': 'datas', 'recognized': True,
         'size': 1392, 'type': 'rw', 'virt_addr': 1130592},
        {'load_addr': 1131984, 'name': 'device_states', 'recognized': False,
         'size': 8, 'type': 'unknown', 'virt_addr': 1131984},
        {'load_addr': 1132000, 'name': 'pagetables', 'recognized': False,
         'size': 23104, 'type': 'unknown', 'virt_addr': 1132000},
        {'load_addr': 4294905856, 'name': 'intList', 'recognized': False,
         'size': 80, 'type': 'unknown', 'virt_addr': 4294905856}],
        64204,
        20512
    ),
    (
"""dummy.elf:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 text          00004000  00100000  00100000  000000c0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 initlevel     00000038  00104000  00104000  000040c0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 bss           0000a7d8  00104b20  00104b20  00004bd8  2**5
                  ALLOC
""".encode('utf-8'),
        '',
        True,
        [{'load_addr': 1048576, 'name': 'text', 'recognized': True,
         'size': 16384, 'type': 'ro', 'virt_addr': 1048576},
        {'load_addr': 1064960, 'name': 'initlevel', 'recognized': True,
         'size': 56, 'type': 'rw', 'virt_addr': 1064960},
        {'load_addr': 1067808, 'name': 'bss', 'recognized': True,
         'size': 42968, 'type': 'alloc', 'virt_addr': 1067808}],
        43024,
        16440
    ),
    (
"""dummy.elf:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 text          00004000  00100000  00100000  000000c0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 dummyempty    00000000  00104000  00104000  000040c0  2**0
                  CONTENTS, READONLY
  2 dummy         00001000  00104000  00104000  000040c0  2**0
                  CONTENTS, READONLY
  3 dummyextra    00001000  00105000  00105000  000050c0  2**0
                  CONTENTS, READONLY
""".encode('utf-8'),
        'build.log',
        False,
        [{'load_addr': 1064960, 'name': 'dummy', 'recognized': False,
         'size': 4096, 'type': 'unknown', 'virt_addr': 1064960},
         {'load_addr': 1069056, 'name': 'dummyextra', 'recognized': True,
         'size': 4096, 'type': 'unknown', 'virt_addr': 1069056}],
        0,
        0
    ),
]

@pytest.mark.parametrize(
    'objdump, buildlog, is_xip, expected_sections, expected_ram, expected_rom',
    TESTDATA_6,
    ids=['valid', 'valid, xip', 'with buildlog, empty, extra']
)
def test_get_info_elf_sections(
    objdump,
    buildlog,
    is_xip,
    expected_sections,
    expected_ram,
    expected_rom
):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', ['dummyextra'],
                            buildlog_filepath=buildlog)

    with mock.patch('subprocess.check_output', return_value=objdump):
        sc._get_info_elf_sections()

    assert all(section in sc.sections for section in expected_sections)
    assert all(section in expected_sections for section in sc.sections)

    assert sc.used_ram == expected_ram
    assert sc.used_rom == expected_rom


def test_analyze_elf_file():
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [])

    sc._check_elf_file = mock.Mock()
    sc._check_is_xip = mock.Mock()
    sc._get_info_elf_sections = mock.Mock()

    sc._analyze_elf_file()

    sc._check_elf_file.assert_called_once()
    sc._check_is_xip.assert_called_once()
    sc._get_info_elf_sections.assert_called_once()


TESTDATA_7 = [
    (True, True, False, ['dummy\n', 'contents']),
    (False, True, True, []),
    (False, False, False, []),
]

@pytest.mark.parametrize(
    'exists, generate_warning, expect_logs, expected_content',
    TESTDATA_7,
    ids=['exists', 'doesn\'t exist, warning', 'doesn\'t exist, no warning']
)
def test_get_buildlog_file_content(
    caplog,
    exists,
    generate_warning,
    expect_logs,
    expected_content
):
    log = 'Incorrect path to build.log file to analyze footprints.' \
          ' Please check the path build.log'

    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [], buildlog_filepath='build.log')

    sc.generate_warning = generate_warning

    with mock.patch('os.path.exists', return_value=exists), \
         mock.patch('builtins.open',
                    mock.mock_open(read_data='dummy\ncontents')):
        content = sc._get_buildlog_file_content()

    if expect_logs:
        assert log in caplog.text
    else:
        assert log not in caplog.text

    assert content == expected_content


TESTDATA_8 = [
    ([], True, True, -1),
    (
"""[...]
[124/125] Building C object zephyr/CMakeFiles/zephyr_final.dir/isr_tables.c.obj
Memory region         Used Size  Region Size  %age Used
           FLASH:       13552 B         1 MB      1.29%
             RAM:        4172 B       192 KB      2.12%
           SRAML:          0 GB        64 KB      0.00%
        IDT_LIST:          0 GB         2 KB      0.00%
[125/125] Linking C executable zephyr/zephyr.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:       13552 B         1 MB      1.29%
             RAM:        4172 B       192 KB      2.12%
           SRAML:          0 GB        64 KB      0.00%
        IDT_LIST:          0 GB         2 KB      0.00%
""".splitlines(),
        False,
        False,
        5
    ),
    (
"""[...]
[124/125] Building C object zephyr/CMakeFiles/zephyr_final.dir/isr_tables.c.obj
[125/125] Linking C executable zephyr/zephyr.elf
""".splitlines(),
        False,
        True,
        -1
    ),
]

@pytest.mark.parametrize(
    'file_content, log_empty, log_missing, expected_result',
    TESTDATA_8,
    ids=['empty', 'valid', 'Memory region missing']
)
def test_find_offset_of_last_pattern_occurrence(
    caplog,
    file_content,
    log_empty,
    log_missing,
    expected_result
):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [], buildlog_filepath='build.log')

    result = sc._find_offset_of_last_pattern_occurrence(file_content)

    if log_empty:
        assert 'Build.log file is empty' in caplog.text
    else:
        assert 'Build.log file is empty' not in caplog.text

    if log_missing:
        assert 'Information about memory footprint for this' \
               ' test configuration is not found.' \
               ' Please check file build.log.' in caplog.text
    else:
        assert 'Information about memory footprint for this' \
               ' test configuration is not found.' \
               ' Please check file build.log.' not in caplog.text

    assert result == expected_result


TESTDATA_9 = [
    ([], -1, []),
    # The last line can be kept because it will be later discarded
    # by _unify_prefixes_on_all_values.
    (['0\n', '1\n', '2\n', '3\n', '4\n'], -1, ['4\n']),
    (['0\n', '1\n', '2\n', '3\n', '4\n'], 6, ['4\n']),
    (['0\n', '1\n', '2\n', '3\n', '4\n'], 4, ['1\n', '2\n', '3\n', '4\n']),
]

@pytest.mark.parametrize(
    'file_content, start_offset, expected_result',
    TESTDATA_9,
    ids=['empty', 'under start', 'over start', 'valid']
)
def test_get_lines_with_footprint(file_content, start_offset, expected_result):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [], buildlog_filepath='build.log')

    result = sc._get_lines_with_footprint(start_offset, file_content)

    assert result == expected_result


TESTDATA_10 = [
    ([], []),
    (['           FLASH:       13552 B         1 MB      1.29%\n'],
     ['           FLASH:       13552 B         1 MB      1.29']),
]

@pytest.mark.parametrize(
    'text_lines, expected_lines',
    TESTDATA_10,
    ids=['empty', 'valid']
)
def test_clear_whitespaces_from_lines(text_lines, expected_lines):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [], buildlog_filepath='build.log')

    lines = sc._clear_whitespaces_from_lines(text_lines)

    assert lines == expected_lines


TESTDATA_11 = [
    ([], [[]]),
    (
        ['           FLASH:       13552 B         1 MB      1.29',
         '             RAM:        4172 B       192 KB      2.12'],
        [['FLASH', '13552 B', '1 MB', '1.29'],
         ['RAM', '4172 B', '192 KB', '2.12']]
    ),
]

@pytest.mark.parametrize(
    'text_lines, expected_result',
    TESTDATA_11,
    ids=['empty', 'valid']
)
def test_divide_test_lines_into_columns(text_lines, expected_result):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [])

    result = sc._divide_text_lines_into_columns(text_lines)

    assert result == expected_result


TESTDATA_12 = [
    ([[]], True, True, [[]]),
    ([['Too'], ['short']], False, False, [[]]),
    (
        [['Memory region', 'Used Size', 'Region Size', '%age Used'],
         ['FLASH', '13552 B', '1 MB', '1.29'],
         ['RAM', '4172 B', '192 KB', '2.12'],
         ['ROM', '172 MB', '1 GB', '90.12']],
        True,
        False,
        [['Memory region', 'Used Size', 'Region Size', '%age Used'],
         ['FLASH', '13552 B', '1048576 B', '1.29'],
         ['RAM', '4172 B', '196608 B', '2.12'],
         ['ROM', '180355072 B', '1073741824 B', '90.12']]
    ),
]

@pytest.mark.parametrize(
    'data_lines, generate_warning, expect_incomplete_log, expected_result',
    TESTDATA_12,
    ids=['empty, generate warning', 'too short', 'valid']
)
def test_unify_prefixes_on_all_values(
    caplog,
    data_lines,
    generate_warning,
    expect_incomplete_log,
    expected_result
):
    converter_dict = {
        '1 MB': '1048576 B',
        '1 GB': '1073741824 B',
        '192 KB': '196608 B',
        '172 MB': '180355072 B',
    }

    def mock_converter(value, *args, **kwargs):
        if value in converter_dict:
            return converter_dict[value]
        return value

    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [], buildlog_filepath='build.log')

    sc._binary_prefix_converter = mock.Mock(side_effect=mock_converter)
    sc.generate_warning = generate_warning

    result = sc._unify_prefixes_on_all_values(data_lines)

    if expect_incomplete_log:
        assert 'Incomplete information about memory footprint.' \
               ' Please check file build.log' in caplog.text
    else:
        assert 'Incomplete information about memory footprint.' \
               ' Please check file build.log' not in caplog.text

    assert result == expected_result


TESTDATA_13 = [
    ('2.22 \n', '2.22'),
    ('1234567890 B', '1234567890'),
    ('180 KB', '184320'),
    ('25 MB', '26214400'),
    ('1 GB', '1073741824'),
]

@pytest.mark.parametrize(
    'value, expected_result',
    TESTDATA_13,
    ids=['invalid', 'B', 'KB', 'MB', 'GB']
)
def test_binary_prefix_converter(value, expected_result):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [])

    result = sc._binary_prefix_converter(value)

    assert result == expected_result


TESTDATA_14 = [
    (4, True),
    (-1, False),
]

@pytest.mark.parametrize(
    'start, is_valid',
    TESTDATA_14,
    ids=['valid', 'not valid']
)
def test_create_data_table(start, is_valid):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [])

    sc._get_buildlog_file_content = mock.Mock()
    sc._find_offset_of_last_pattern_occurrence = mock.Mock(return_value=start)
    sc._get_lines_with_footprint = mock.Mock()
    sc._clear_whitespaces_from_lines = mock.Mock()
    sc._divide_text_lines_into_columns = mock.Mock()
    sc._unify_prefixes_on_all_values = mock.Mock()

    result = sc._create_data_table()

    sc._get_buildlog_file_content.assert_called_once()
    sc._find_offset_of_last_pattern_occurrence.assert_called_once()

    if is_valid:
        sc._get_lines_with_footprint.assert_called_once()
        sc._clear_whitespaces_from_lines.assert_called_once()
        sc._divide_text_lines_into_columns.assert_called_once()
        sc._unify_prefixes_on_all_values.assert_called_once()
    else:
        sc._get_lines_with_footprint.assert_not_called()
        sc._clear_whitespaces_from_lines.assert_not_called()
        sc._divide_text_lines_into_columns.assert_not_called()
        sc._unify_prefixes_on_all_values.assert_not_called()
        assert result == [[]]


TESTDATA_15 = [
    ([[]], False, [], 0, 0, 0, 0),
    (None, True, ['Missing information about memory footprint.' \
                  ' Check file build.log.'], 0, 0, 0, 0),
    ([[], [0, 100, 200], [0, 300, 400]], True, [], 300, 100, 400, 200),
]

@pytest.mark.parametrize(
    'data_from_file, generate_warning, expected_logs,' \
    ' expected_used_ram, expected_used_rom,' \
    ' expected_available_ram, expected_available_rom',
    TESTDATA_15,
    ids=['empty data table', 'no data table + warning', 'valid']
)
def test_get_footprint_from_buildlog(
    caplog,
    data_from_file,
    generate_warning,
    expected_logs,
    expected_used_ram,
    expected_used_rom,
    expected_available_ram,
    expected_available_rom
):
    with mock.patch.object(SizeCalculator, '_calculate_sizes', mock.Mock()):
        sc = SizeCalculator('dummy.elf', [], buildlog_filepath='build.log')
    sc._create_data_table = mock.Mock(return_value=data_from_file)

    sc._get_footprint_from_buildlog()

    assert all([log in caplog.text for log in caplog.text])

    assert sc.used_ram == expected_used_ram
    assert sc.used_rom == expected_used_rom
    assert sc.available_ram == expected_available_ram
    assert sc.available_rom == expected_available_rom
