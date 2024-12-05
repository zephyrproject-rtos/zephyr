# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import pytest
import textwrap

from unittest import mock
from pathlib import Path

from twisterlib.harness import Pytest
from twisterlib.testsuite import TestSuite
from twisterlib.testinstance import TestInstance
from twisterlib.platform import Platform


@pytest.fixture
def testinstance() -> TestInstance:
    testsuite = TestSuite('.', 'samples/hello', 'unit.test')
    testsuite.harness_config = {}
    testsuite.ignore_faults = False
    testsuite.sysbuild = False
    platform = Platform()

    testinstance = TestInstance(testsuite, platform, 'outdir')
    testinstance.handler = mock.Mock()
    testinstance.handler.options = mock.Mock()
    testinstance.handler.options.verbose = 1
    testinstance.handler.options.fixture = ['fixture1:option1', 'fixture2']
    testinstance.handler.options.pytest_args = None
    testinstance.handler.options.extra_test_args = []
    testinstance.handler.type_str = 'native'
    return testinstance


@pytest.mark.parametrize('device_type', ['native', 'qemu'])
def test_pytest_command(testinstance: TestInstance, device_type):
    pytest_harness = Pytest()
    pytest_harness.configure(testinstance)

    testinstance.handler.type_str = device_type
    ref_command = [
        'pytest',
        'samples/hello/pytest',
        f'--build-dir={testinstance.build_dir}',
        f'--junit-xml={testinstance.build_dir}/report.xml',
        f'--device-type={device_type}',
        '--twister-fixture=fixture1:option1',
        '--twister-fixture=fixture2'
    ]

    command = pytest_harness.generate_command()
    for c in ref_command:
        assert c in command


def test_pytest_command_dut_scope(testinstance: TestInstance):
    pytest_harness = Pytest()
    dut_scope = 'session'
    testinstance.testsuite.harness_config['pytest_dut_scope'] = dut_scope
    pytest_harness.configure(testinstance)
    command = pytest_harness.generate_command()
    assert f'--dut-scope={dut_scope}' in command


def test_pytest_command_extra_args(testinstance: TestInstance):
    pytest_harness = Pytest()
    pytest_args = ['-k test1', '-m mark1']
    testinstance.testsuite.harness_config['pytest_args'] = pytest_args
    pytest_harness.configure(testinstance)
    command = pytest_harness.generate_command()
    for c in pytest_args:
        assert c in command


def test_pytest_command_extra_test_args(testinstance: TestInstance):
    pytest_harness = Pytest()
    extra_test_args = ['-stop_at=3', '-no-rt']
    testinstance.handler.options.extra_test_args = extra_test_args
    pytest_harness.configure(testinstance)
    command = pytest_harness.generate_command()
    assert f'--extra-test-args={extra_test_args[0]} {extra_test_args[1]}' in command


def test_pytest_command_extra_args_in_options(testinstance: TestInstance):
    pytest_harness = Pytest()
    pytest_args_from_yaml = '--extra-option'
    pytest_args_from_cmd = ['-k', 'test_from_cmd']
    testinstance.testsuite.harness_config['pytest_args'] = [pytest_args_from_yaml]
    testinstance.handler.options.pytest_args = pytest_args_from_cmd
    pytest_harness.configure(testinstance)
    command = pytest_harness.generate_command()
    assert pytest_args_from_cmd[0] in command
    assert pytest_args_from_cmd[1] in command
    assert pytest_args_from_yaml in command
    assert command.index(pytest_args_from_yaml) < command.index(pytest_args_from_cmd[1])


@pytest.mark.parametrize(
    ('pytest_root', 'expected'),
    [
        (
            ['pytest/test_shell_help.py'],
            ['samples/hello/pytest/test_shell_help.py']
        ),
        (
            ['pytest/test_shell_help.py', 'pytest/test_shell_version.py', 'test_dir'],
            ['samples/hello/pytest/test_shell_help.py',
             'samples/hello/pytest/test_shell_version.py',
             'samples/hello/test_dir']
        ),
        (
            ['../shell/pytest/test_shell.py'],
            ['samples/shell/pytest/test_shell.py']
        ),
        (
            ['/tmp/test_temp.py'],
            ['/tmp/test_temp.py']
        ),
        (
            ['~/tmp/test_temp.py'],
            ['/home/joe/tmp/test_temp.py']
        ),
        (
            ['$ZEPHYR_BASE/samples/subsys/testsuite/pytest/shell/pytest'],
            ['/zephyr_base/samples/subsys/testsuite/pytest/shell/pytest']
        ),
        (
            ['pytest/test_shell_help.py::test_A', 'pytest/test_shell_help.py::test_B'],
            ['samples/hello/pytest/test_shell_help.py::test_A',
             'samples/hello/pytest/test_shell_help.py::test_B']
        ),
        (
            ['pytest/test_shell_help.py::test_A[param_a]'],
            ['samples/hello/pytest/test_shell_help.py::test_A[param_a]']
        )
    ],
    ids=[
        'one_file',
        'more_files',
        'relative_path',
        'absollute_path',
        'user_dir',
        'with_env_var',
        'subtests',
        'subtest_with_param'
    ]
)
def test_pytest_handle_source_list(testinstance: TestInstance, monkeypatch, pytest_root, expected):
    monkeypatch.setenv('ZEPHYR_BASE', '/zephyr_base')
    monkeypatch.setenv('HOME', '/home/joe')
    testinstance.testsuite.harness_config['pytest_root'] = pytest_root
    pytest_harness = Pytest()
    pytest_harness.configure(testinstance)
    command = pytest_harness.generate_command()
    for pytest_src in expected:
        assert pytest_src in command


def test_if_report_is_parsed(pytester, testinstance: TestInstance):
    test_file_content = textwrap.dedent("""
        def test_1():
            pass
        def test_2():
            pass
    """)
    test_file = pytester.path / 'test_valid.py'
    test_file.write_text(test_file_content)
    report_file = Path('report.xml')
    result = pytester.runpytest(
        str(test_file),
        f'--junit-xml={str(report_file)}'
    )
    result.assert_outcomes(passed=2)
    assert report_file.is_file()

    pytest_harness = Pytest()
    pytest_harness.configure(testinstance)
    pytest_harness.report_file = report_file

    pytest_harness._update_test_status()

    assert pytest_harness.status == "passed"
    assert testinstance.status == "passed"
    assert len(testinstance.testcases) == 2
    for tc in testinstance.testcases:
        assert tc.status == "passed"


def test_if_report_with_error(pytester, testinstance: TestInstance):
    test_file_content = textwrap.dedent("""
        def test_exp():
            raise Exception('Test error')
        def test_err():
            assert False
    """)
    test_file = pytester.path / 'test_error.py'
    test_file.write_text(test_file_content)
    report_file = pytester.path / 'report.xml'
    result = pytester.runpytest(
        str(test_file),
        f'--junit-xml={str(report_file)}'
    )
    result.assert_outcomes(failed=2)
    assert report_file.is_file()

    pytest_harness = Pytest()
    pytest_harness.configure(testinstance)
    pytest_harness.report_file = report_file

    pytest_harness._update_test_status()

    assert pytest_harness.status == "failed"
    assert testinstance.status == "failed"
    assert len(testinstance.testcases) == 2
    for tc in testinstance.testcases:
        assert tc.status == "failed"
        assert tc.output
        assert tc.reason
    assert testinstance.reason
    assert '2/2' in testinstance.reason


def test_if_report_with_skip(pytester, testinstance: TestInstance):
    test_file_content = textwrap.dedent("""
        import pytest
        @pytest.mark.skip('Test skipped')
        def test_skip_1():
            pass
        def test_skip_2():
            pytest.skip('Skipped on runtime')
    """)
    test_file = pytester.path / 'test_skip.py'
    test_file.write_text(test_file_content)
    report_file = pytester.path / 'report.xml'
    result = pytester.runpytest(
        str(test_file),
        f'--junit-xml={str(report_file)}'
    )
    result.assert_outcomes(skipped=2)
    assert report_file.is_file()

    pytest_harness = Pytest()
    pytest_harness.configure(testinstance)
    pytest_harness.report_file = report_file

    pytest_harness._update_test_status()

    assert pytest_harness.status == "skipped"
    assert testinstance.status == "skipped"
    assert len(testinstance.testcases) == 2
    for tc in testinstance.testcases:
        assert tc.status == "skipped"


def test_if_report_with_filter(pytester, testinstance: TestInstance):
    test_file_content = textwrap.dedent("""
        import pytest
        def test_A():
            pass
        def test_B():
            pass
    """)
    test_file = pytester.path / 'test_filter.py'
    test_file.write_text(test_file_content)
    report_file = pytester.path / 'report.xml'
    result = pytester.runpytest(
        str(test_file),
        '-k', 'test_B',
        f'--junit-xml={str(report_file)}'
    )
    result.assert_outcomes(passed=1)
    assert report_file.is_file()

    pytest_harness = Pytest()
    pytest_harness.configure(testinstance)
    pytest_harness.report_file = report_file
    pytest_harness._update_test_status()
    assert pytest_harness.status == "passed"
    assert testinstance.status == "passed"
    assert len(testinstance.testcases) == 1


def test_if_report_with_no_collected(pytester, testinstance: TestInstance):
    test_file_content = textwrap.dedent("""
        import pytest
        def test_A():
            pass
    """)
    test_file = pytester.path / 'test_filter.py'
    test_file.write_text(test_file_content)
    report_file = pytester.path / 'report.xml'
    result = pytester.runpytest(
        str(test_file),
        '-k', 'test_B',
        f'--junit-xml={str(report_file)}'
    )
    result.assert_outcomes(passed=0)
    assert report_file.is_file()

    pytest_harness = Pytest()
    pytest_harness.configure(testinstance)
    pytest_harness.report_file = report_file
    pytest_harness._update_test_status()
    assert pytest_harness.status == "skipped"
    assert testinstance.status == "skipped"
