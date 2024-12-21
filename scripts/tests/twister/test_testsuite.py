#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for testinstance class
"""

import mmap
import mock
import os
import pytest
import sys

from contextlib import nullcontext

ZEPHYR_BASE = os.getenv('ZEPHYR_BASE')
sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'scripts', 'pylib', 'twister'))

from twisterlib.statuses import TwisterStatus
from twisterlib.testsuite import (
    _find_src_dir_path,
    _get_search_area_boundary,
    find_c_files_in,
    scan_file,
    scan_testsuite_path,
    ScanPathResult,
    TestCase,
    TestSuite
)
from twisterlib.error import TwisterException, TwisterRuntimeError


TESTDATA_1 = [
    (
        ScanPathResult(
            ['a', 'b'],
            'Found a test that does not start with test_',
            False,
            False,
            True,
            ['feature_a', 'feature_b']
        ),
        ScanPathResult(
            ['a', 'b'],
            'Found a test that does not start with test_',
            False,
            False,
            True,
            ['feature_a', 'feature_b']
        ),
        True
    ),
#    (
#        ScanPathResult(),
#        ScanPathResult(),
#        True
#    ),
    (
        ScanPathResult(
            ['a', 'b'],
            'Found a test that does not start with test_',
            False,
            False,
            True,
            ['feature_a', 'feature_b']
        ),
        'I am not a ScanPathResult.',
        False
    ),
#    (
#        ScanPathResult(
#            ['a', 'b'],
#            'Found a test that does not start with test_',
#            False,
#            False,
#            True,
#            ['feature_a', 'feature_b']
#        ),
#        ScanPathResult(),
#        False
#    ),
]


@pytest.mark.parametrize(
    'original, provided, expected',
    TESTDATA_1,
    ids=[
        'identical',
#        'empties',
        'wrong type',
#        'different with empty'
    ]
)
def test_scanpathresults_dunders(original, provided, expected):
    result = original == provided

    assert result == expected

TESTDATA_2 = [
    (
        os.path.join('testsuites', 'tests', 'test_ztest.c'),
        ScanPathResult(
            warnings=None,
            matches=[
                'a',
                'c',
                'unit_a',
                'newline',
                'test_test_aa',
                'user',
                'last'
            ],
            has_registered_test_suites=False,
            has_run_registered_test_suites=False,
            has_test_main=False,
            ztest_suite_names = ['test_api']
        )
    ),
    (
        os.path.join('testsuites', 'tests', 'test_a', 'test_ztest_error.c'),
        ScanPathResult(
            warnings='Found a test that does not start with test_',
            matches=['1a', '1c', '2a', '2b'],
            has_registered_test_suites=False,
            has_run_registered_test_suites=False,
            has_test_main=True,
            ztest_suite_names = ['feature1', 'feature2']
        )
    ),
    (
        os.path.join('testsuites', 'tests', 'test_a', 'test_ztest_error_1.c'),
        ScanPathResult(
            warnings='found invalid #ifdef, #endif in ztest_test_suite()',
            matches=['unit_1a', 'unit_1b', 'Unit_1c'],
            has_registered_test_suites=False,
            has_run_registered_test_suites=False,
            has_test_main=False,
            ztest_suite_names = ['feature3']
        )
    ),
    (
        os.path.join(
            'testsuites',
            'tests',
            'test_d',
            'test_ztest_error_register_test_suite.c'
        ),
        ScanPathResult(
            warnings=None,
            matches=['unit_1a', 'unit_1b'],
            has_registered_test_suites=True,
            has_run_registered_test_suites=False,
            has_test_main=False,
            ztest_suite_names = ['feature4']
        )
    ),
    (
        os.path.join(
            'testsuites',
            'tests',
            'test_e',
            'test_ztest_new_suite.c'
        ),
        ScanPathResult(
            warnings=None,
            matches=['1a', '1b'],
            has_registered_test_suites=False,
            has_run_registered_test_suites=True,
            has_test_main=False,
            ztest_suite_names = ['feature5']
        )
    ),
#    (
#        os.path.join(
#            'testsuites',
#            'tests',
#            'test_e',
#            'test_ztest_no_suite.c'
#        ),
#        ScanPathResult(
#            warnings=None,
#            matches=None,
#            has_registered_test_suites=False,
#            has_run_registered_test_suites=False,
#            has_test_main=False,
#            ztest_suite_names = []
#        )
#    ),
]


@pytest.mark.parametrize(
    'test_file, expected',
    TESTDATA_2,
    ids=[
        'valid',
        'test not starting with test_',
        'invalid ifdef with test_main',
        'registered testsuite',
        'new testsuite with registered run',
#        'empty testsuite'
    ]
)
def test_scan_file(test_data, test_file, class_env, expected: ScanPathResult):
    """
    Testing scan_file method with different
    ztest files for warnings and results
    """

    result: ScanPathResult = scan_file(os.path.join(test_data, test_file))

    assert result == expected


# Generate testcases depending on available mmap attributes
TESTIDS_3 = []
TESTDATA_3 = []

try:
    TESTDATA_3.append(
        (
            'nt',
            {'access': mmap.ACCESS_READ}
        )
    )
    TESTIDS_3.append('windows')
except AttributeError:
    pass

try:
    TESTDATA_3.append(
        (
            'posix',
            {
                'flags': mmap.MAP_PRIVATE,
                'prot': mmap.PROT_READ,
                'offset': 0
            }
        )
    )
    TESTIDS_3.append('linux')
except AttributeError:
    pass


@pytest.mark.parametrize(
    'os_name, expected',
    TESTDATA_3,
    ids=TESTIDS_3
)
def test_scan_file_mmap(os_name, expected):
    class TestException(Exception):
        pass

    def assert_mmap(*args, **kwargs):
        assert expected.items() <= kwargs.items()

    # We do this to skip the rest of scan_file
    def raise_exception(*args, **kwargs):
        raise TestException('')

    with mock.patch('mmap.mmap', mock.Mock(side_effect=assert_mmap)), \
         mock.patch('builtins.open', mock.mock_open(read_data='dummy data')), \
         mock.patch('os.name', os_name), \
         mock.patch('contextlib.closing', raise_exception):
        try:
            scan_file('dummy/path')
        except TestException:
            assert True
            return

    assert False


TESTDATA_4 = [
    (
        ZEPHYR_BASE,
        '.',
        'test_c',
        'Tests should reference the category and subsystem' \
        ' with a dot as a separator.'
    ),
    (
        os.path.join(ZEPHYR_BASE, 'scripts', 'tests'),
        '.',
        '',
        'Tests should reference the category and subsystem' \
        ' with a dot as a separator.'),
]


@pytest.mark.parametrize(
    'testsuite_root, workdir, name, exception',
    TESTDATA_4
)
def test_get_unique_exception(testsuite_root, workdir, name, exception):
    """
    Test to check if tests reference the category and subsystem
    with a dot as a separator
    """

    with pytest.raises(TwisterException):
        unique = TestSuite(testsuite_root, workdir, name)
        assert unique == exception


TEST_DATA_REL_PATH = os.path.join(
    'scripts',
    'tests',
    'twister',
    'test_data',
    'testsuites'
)


TESTDATA_5 = [
    (
        os.path.join(ZEPHYR_BASE, TEST_DATA_REL_PATH),
        os.path.join(ZEPHYR_BASE, TEST_DATA_REL_PATH, 'tests', 'test_a'),
        os.path.join(
            os.sep,
            TEST_DATA_REL_PATH,
            'tests',
            'test_a',
            'test_a.check_1'
        ),
        os.path.join(
            os.sep,
            TEST_DATA_REL_PATH,
            'tests',
            'test_a',
            'test_a.check_1'
        ),
    ),
    (
        ZEPHYR_BASE,
        ZEPHYR_BASE,
        'test_a.check_1',
        'test_a.check_1'
    ),
    (
        ZEPHYR_BASE,
        os.path.join(
            ZEPHYR_BASE,
            TEST_DATA_REL_PATH,
            'test_b'
        ),
        os.path.join(os.sep, TEST_DATA_REL_PATH, 'test_b', 'test_b.check_1'),
        os.path.join(os.sep, TEST_DATA_REL_PATH, 'test_b', 'test_b.check_1')
    ),
    (
        os.path.join(ZEPHYR_BASE, 'scripts', 'tests'),
        os.path.join(ZEPHYR_BASE, 'scripts', 'tests'),
        'test_b.check_1',
        os.path.join('scripts', 'tests', 'test_b.check_1')
    ),
    (
        ZEPHYR_BASE,
        ZEPHYR_BASE,
        'test_a.check_1.check_2',
        'test_a.check_1.check_2'
    ),
]


@pytest.mark.parametrize(
    'testsuite_root, suite_path, name, expected',
    TESTDATA_5
)
def test_get_unique(testsuite_root, suite_path, name, expected):
    """
    Test to check if the unique name is given
    for each testsuite root and workdir
    """

    suite = TestSuite(testsuite_root, suite_path, name)
    assert suite.name == expected


TESTDATA_6 = [
    (
        b'/* dummy */\r\n    ztest_run_test_suite(feature)',
        [
            mock.Mock(
                start=mock.Mock(return_value=0),
                end=mock.Mock(return_value=0)
            )
        ],
        False,
        (0, 13)
    ),
    (
        b'ztest_register_test_suite(featureX, NULL, ztest_unit_test(test_a));',
        [
            mock.Mock(
                start=mock.Mock(return_value=0),
                end=mock.Mock(return_value=26)
            )
        ],
        True,
        (26, 67)
    ),
    (
        b'dummy text',
        [
            mock.Mock(
                start=mock.Mock(return_value=0),
                end=mock.Mock(return_value=0)
            )
        ],
        False,
        ValueError
    )
]

@pytest.mark.parametrize(
    'search_area, suite_regex_matches, is_registered_test_suite, expected',
    TESTDATA_6,
    ids=['run suite', 'registered suite', 'error']
)
def test_get_search_area_boundary(
    search_area,
    suite_regex_matches,
    is_registered_test_suite,
    expected
):
    with pytest.raises(expected) if \
     isinstance(expected, type) and issubclass(expected, Exception) \
     else nullcontext() as exception:
        result = _get_search_area_boundary(
            search_area,
            suite_regex_matches,
            is_registered_test_suite
        )

    if exception:
        assert str(exception.value) == 'can\'t find ztest_run_test_suite'
        return

    assert result == expected


TESTDATA_7 = [
    (True, [os.path.join('', 'home', 'user', 'dummy_path', 'dummy.c'),
            os.path.join('', 'home', 'user', 'dummy_path', 'dummy.cpp')]),
    (False, [])
]

@pytest.mark.parametrize(
    'isdir, expected',
    TESTDATA_7,
    ids=['valid', 'not a directory']
)
def test_find_c_files_in(isdir, expected):
    old_dir = os.path.join('', 'home', 'user', 'dummy_base_dir')
    new_path = os.path.join('', 'home', 'user', 'dummy_path')
    cur_dir = old_dir

    def mock_chdir(path, *args, **kwargs):
        nonlocal cur_dir
        cur_dir = path

    # We simulate such a structure:
    # <new_path>
    # ┣ dummy.c
    # ┣ wrong_dummy.h
    # ┗ dummy_dir
    #   ┣ dummy.cpp
    #   ┗ wrong_dummy.hpp
    # <old_dir>
    # ┗ wrong_dummy.c
    new_path_base = ['dummy.c', 'wrong_dummy.h']
    new_path_subs = ['dummy.cpp', 'wrong_dummy.hpp']
    old_dir_base = ['wrong_dummy.c']

    def format_tester(fmt):
        formats = [
            {'name': 'subdirs', 'fmt': '**/*.'},
            {'name': 'base', 'fmt': '*.'}
        ]

        for format in formats:
            if fmt.startswith(format['fmt']):
                return format['name'], fmt[len(format['fmt']):]

        raise ValueError('This test wasn\'t designed for those globs.'
                         ' Please fix the test before PR!')

    def mock_glob(fmt, *args, **kwargs):
        from_where, extension = format_tester(fmt)

        if cur_dir == old_dir:
            if from_where == 'subdirs':
                return []
            elif from_where == 'base':
                return list(filter(lambda fn: fn.endswith(extension),
                                   old_dir_base))
            else:
                return []
        if cur_dir == new_path:
            if from_where == 'subdirs':
                return list(filter(lambda fn: fn.endswith(extension),
                                   new_path_subs))
            elif from_where == 'base':
                return list(filter(lambda fn: fn.endswith(extension),
                                   new_path_base))
            else:
                return []

        raise ValueError('This test wasn\'t designed for those dirs.'
                         'Please fix the test before PR!')

    with mock.patch('os.path.isdir', return_value=isdir), \
         mock.patch('os.getcwd', return_value=cur_dir), \
         mock.patch('glob.glob', mock_glob), \
         mock.patch('os.chdir', side_effect=mock_chdir) as chdir_mock:
        filenames = find_c_files_in(new_path)

    assert sorted(filenames) == sorted(expected)

    assert chdir_mock.call_args is None or \
           chdir_mock.call_args == mock.call(old_dir)


TESTDATA_8 = [
    (
        os.path.join('dummy', 'path'),
        ['testsuite_file_1', 'testsuite_file_2'],
        ['src_dir_file_1', 'src_dir_file_2', 'src_dir_file_3'],
        {'src_dir_file_1': 1000, 'src_dir_file_2': 2000, 'src_dir_file_3': 0},
        {
            'testsuite_file_1': ScanPathResult(
                matches = ['test_a', 'b'],
                warnings = 'dummy warning',
                has_registered_test_suites = True,
                has_run_registered_test_suites = True,
                has_test_main = True,
                ztest_suite_names = ['feature_a']
            ),
            'testsuite_file_2': ValueError,
            'src_dir_file_1': ScanPathResult(
                matches = ['test_b', 'a'],
                warnings = None,
                has_registered_test_suites = True,
                has_run_registered_test_suites = True,
                has_test_main = True,
                ztest_suite_names = ['feature_b']
            ),
            'src_dir_file_2': ValueError,
            'src_dir_file_3': ValueError,
        },
        [
            'testsuite_file_2: can\'t find: dummy exception',
            'testsuite_file_1: dummy warning',
            'src_dir_file_2: error parsing source file: dummy exception',
        ],
        None,
        (['a', 'b', 'test_a', 'test_b'], ['feature_a', 'feature_b'])
    ),
    (
        os.path.join('dummy', 'path'),
        [],
        ['src_dir_file'],
        {'src_dir_file': 1000},
        {
            'src_dir_file': ScanPathResult(
                matches = ['test_b', 'a'],
                warnings = None,
                has_registered_test_suites = True,
                has_run_registered_test_suites = False,
                has_test_main = True,
                ztest_suite_names = ['feature_b']
            ),
        },
        [
            'Found call to \'ztest_register_test_suite()\'' \
            ' but no call to \'ztest_run_registered_test_suites()\''
        ],
        TwisterRuntimeError(
            'Found call to \'ztest_register_test_suite()\'' \
            ' but no call to \'ztest_run_registered_test_suites()\''
        ),
        None
    ),
    (
        os.path.join('dummy', 'path'),
        [],
        ['src_dir_file'],
        {'src_dir_file': 100},
        {
            'src_dir_file': ScanPathResult(
                matches = ['test_b', 'a'],
                warnings = 'dummy warning',
                has_registered_test_suites = True,
                has_run_registered_test_suites = True,
                has_test_main = True,
                ztest_suite_names = ['feature_b']
            ),
        },
        ['src_dir_file: dummy warning'],
        TwisterRuntimeError('src_dir_file: dummy warning'),
        None
    ),
]


@pytest.mark.parametrize(
    'testsuite_path, testsuite_glob, src_dir_glob, sizes, scanpathresults,' \
    ' expected_logs, expected_exception, expected',
    TESTDATA_8,
    ids=[
        'valid',
        'warning in src dir',
        'register with run error',
    ]
)
def test_scan_testsuite_path(
    caplog,
    testsuite_path,
    testsuite_glob,
    src_dir_glob,
    sizes,
    scanpathresults,
    expected_logs,
    expected_exception,
    expected
):
    src_dir_path = os.path.join(testsuite_path, 'src')

    def mock_fsdp(path, *args, **kwargs):
        return src_dir_path

    def mock_find(path, *args, **kwargs):
        if path == src_dir_path:
            return src_dir_glob
        elif path == testsuite_path:
            return testsuite_glob
        else:
            return []

    def mock_sf(filename, *args, **kwargs):
        if isinstance(scanpathresults[filename], type) and \
           issubclass(scanpathresults[filename], Exception):
            raise scanpathresults[filename]('dummy exception')
        return scanpathresults[filename]

    def mock_stat(filename, *args, **kwargs):
        result = mock.Mock()
        type(result).st_size = sizes[filename]

        return result

    with mock.patch('twisterlib.testsuite._find_src_dir_path', mock_fsdp), \
         mock.patch('twisterlib.testsuite.find_c_files_in', mock_find), \
         mock.patch('twisterlib.testsuite.scan_file', mock_sf), \
         mock.patch('os.stat', mock_stat), \
         pytest.raises(type(expected_exception)) if \
          expected_exception else nullcontext() as exception:
        result = scan_testsuite_path(testsuite_path)

    assert all(
        [expected_log in " ".join(caplog.text.split()) \
         for expected_log in expected_logs]
    )

    if expected_exception:
        assert str(expected_exception) == str(exception.value)
        return

    assert len(result[0]) == len(expected[0])
    assert all(
        [expected_subcase in result[0] for expected_subcase in expected[0]]
    )
    assert len(result[1]) == len(expected[1])
    assert all(
        [expected_subcase in result[1] for expected_subcase in expected[1]]
    )


TESTDATA_9 = [
    ('dummy/path', 'dummy/path/src', 'dummy/path/src'),
    ('dummy/path', 'dummy/src', 'dummy/src'),
    ('dummy/path', 'another/path', '')
]


@pytest.mark.parametrize(
    'test_dir_path, isdir_path, expected',
    TESTDATA_9,
    ids=['src here', 'src in parent', 'no path']
)
def test_find_src_dir_path(test_dir_path, isdir_path, expected):
    def mock_isdir(path, *args, **kwargs):
        return os.path.normpath(path) == isdir_path

    with mock.patch('os.path.isdir', mock_isdir):
        result = _find_src_dir_path(test_dir_path)

    assert os.path.normpath(result) == expected or result == expected


TEST_DATA_REL_PATH = os.path.join(
    'scripts',
    'tests',
    'twister',
    'test_data',
    'testsuites'
)


TESTDATA_10 = [
    (
        ZEPHYR_BASE,
        ZEPHYR_BASE,
        'test_a.check_1',
        {
            'testcases': ['testcase1', 'testcase2']
        },
        ['subcase1', 'subcase2'],
        ['testsuite_a', 'testsuite_b'],
        [
            ('test_a.check_1.testcase1', False),
            ('test_a.check_1.testcase2', False)
        ],
    ),
    (
        ZEPHYR_BASE,
        ZEPHYR_BASE,
        'test_a.check_1',
        {},
        ['subcase_repeat', 'subcase_repeat', 'subcase_alone'],
        ['testsuite_a'],
        [
            ('test_a.check_1.subcase_repeat', False),
            ('test_a.check_1.subcase_alone', False)
        ],
    ),
    (
        ZEPHYR_BASE,
        ZEPHYR_BASE,
        'test_a.check_1',
        {},
        [],
        ['testsuite_a', 'testsuite_a'],
        [
            ('test_a.check_1', True)
        ],
    ),
]


@pytest.mark.parametrize(
    'testsuite_root, suite_path, name, data,' \
    ' parsed_subcases, suite_names, expected',
    TESTDATA_10,
    ids=['data', 'subcases', 'empty']
)
def test_testsuite_add_subcases(
    testsuite_root,
    suite_path,
    name,
    data,
    parsed_subcases,
    suite_names,
    expected
):
    """
    Test to check if the unique name is given
    for each testsuite root and workdir
    """

    suite = TestSuite(testsuite_root, suite_path, name)
    suite.add_subcases(data, parsed_subcases, suite_names)

    assert sorted(suite.ztest_suite_names) == sorted(suite_names)

    assert len(suite.testcases) == len(expected)
    for testcase in suite.testcases:
        for expected_value in expected:
            if expected_value[0] == testcase.name and \
               expected_value[1] == testcase.freeform:
                break
        else:
            assert False


TESTDATA_11 = [
#    (
#        ZEPHYR_BASE,
#        ZEPHYR_BASE,
#        'test_a.check_1',
#        {
#            'testcases': ['testcase1', 'testcase2']
#        },
#        [],
#    ),
    (
        ZEPHYR_BASE,
        ZEPHYR_BASE,
        'test_a.check_1',
        {
            'testcases': ['testcase1', 'testcase2'],
            'harness': 'console',
            'harness_config': { 'dummy': 'config' }
        },
        [
            ('harness', 'console'),
            ('harness_config', { 'dummy': 'config' })
        ],
    ),
#    (
#        ZEPHYR_BASE,
#        ZEPHYR_BASE,
#        'test_a.check_1',
#        {
#            'harness': 'console'
#        },
#        Exception,
#    )
]


@pytest.mark.parametrize(
    'testsuite_root, suite_path, name, data, expected',
    TESTDATA_11,
    ids=[
#        'no harness',
        'proper harness',
#        'harness error'
    ]
)
def test_testsuite_load(
    testsuite_root,
    suite_path,
    name,
    data,
    expected
):
    suite = TestSuite(testsuite_root, suite_path, name)

    with pytest.raises(expected) if \
     isinstance(expected, type) and issubclass(expected, Exception) \
     else nullcontext() as exception:
        suite.load(data)

    if exception:
        assert str(exception.value) == 'Harness config error: console harness' \
                                 ' defined without a configuration.'
        return

    for attr_name, value in expected:
        assert getattr(suite, attr_name) == value


def test_testcase_dunders():
    case_lesser = TestCase(name='A lesser name')
    case_greater = TestCase(name='a greater name')
    case_greater.status = TwisterStatus.FAIL

    assert case_lesser < case_greater
    assert str(case_greater) == 'a greater name'
    assert repr(case_greater) == f'<TestCase a greater name with {str(TwisterStatus.FAIL)}>'


TESTDATA_11 = [
        (
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites',
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites/tests/test_a',
            'test_a.check_1',
            'test_a.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE,
            'test_a.check_1',
            'test_a.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites/test_b',
            'test_b.check_1',
            'test_b.check_1'
        ),
        (
            os.path.join(ZEPHYR_BASE, 'scripts/tests'),
            os.path.join(ZEPHYR_BASE, 'scripts/tests'),
            'test_b.check_1',
            'test_b.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE,
            'test_a.check_1.check_2',
            'test_a.check_1.check_2'
        ),
]
@pytest.mark.parametrize("testsuite_root, suite_path, name, expected", TESTDATA_11)
def test_get_no_detailed_test_id(testsuite_root, suite_path, name, expected):
    '''Test to check if the name without path is given for each testsuite'''
    suite = TestSuite(testsuite_root, suite_path, name, detailed_test_id=False)
    print(suite.name)
    assert suite.name == expected
