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
    platform = Platform()

    testinstance = TestInstance(testsuite, platform, 'outdir')
    testinstance.handler = mock.Mock()
    testinstance.handler.options = mock.Mock()
    testinstance.handler.options.verbose = 1
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
        f'--device-type={device_type}'
    ]

    command = pytest_harness.generate_command()
    for c in ref_command:
        assert c in command


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

    assert pytest_harness.state == "passed"
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

    assert pytest_harness.state == "failed"
    assert testinstance.status == "failed"
    assert len(testinstance.testcases) == 2
    for tc in testinstance.testcases:
        assert tc.status == "failed"
        assert tc.output
        assert tc.reason


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

    assert pytest_harness.state == "skipped"
    assert testinstance.status == "skipped"
    assert len(testinstance.testcases) == 2
    for tc in testinstance.testcases:
        assert tc.status == "skipped"
