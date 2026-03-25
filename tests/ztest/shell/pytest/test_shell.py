# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
import pytest
from twister_harness import Shell

def test_run_all_shuffled(shell: Shell):
    lines = shell.exec_command('ztest shuffle -c 1 -s 1')
    output = '\n'.join(lines)
    assert "PASS - test_int_param" in output, f"Expected 'PASS - test_int_param' but got {lines}"
    assert "PASS - test_dummy" in output, f"Expected 'PASS - test_dummy' but got {lines}"
    assert "PASS - test_string_param" in output, f"Expected 'PASS - test_string_param' but got {lines}"

def test_run_testcase_once(shell: Shell):
    lines = shell.exec_command('ztest run-testcase ztest_shell::test_dummy')
    output = '\n'.join(lines)
    assert "PASS - test_dummy" in output, f"Expected 'PASS - test_dummy' but got {lines}"

def test_run_testcase_twice(shell: Shell):
    lines = shell.exec_command('ztest run-testcase ztest_shell::test_dummy -r 2')
    output = '\n'.join(lines)
    pass_ztest_shell_count = output.count("PASS - test_dummy")
    assert pass_ztest_shell_count == 2, f"Expected 2 occurrences of 'PASS - test_dummy' but found {pass_ztest_shell_count}"

def test_run_testsuite_once(shell: Shell):
    lines = shell.exec_command('ztest run-testsuite ztest_shell')
    output = '\n'.join(lines)
    assert "TESTSUITE ztest_shell succeeded" in output, f"Expected 'TESTSUITE ztest_shell succeeded' but got {lines}"

def test_run_testsuite_twice(shell: Shell):
    lines = shell.exec_command('ztest run-testsuite ztest_shell -r 2')
    output = '\n'.join(lines)
    pass_ztest_shell_count = output.count("TESTSUITE ztest_shell succeeded")
    assert pass_ztest_shell_count == 2, f"Expected 2 occurrences of 'TESTSUITE ztest_shell succeeded' but found {pass_ztest_shell_count}"

def test_run_testcase_with_int_parameter(shell: Shell):
    shell.exec_command('set_parameter 44')
    lines = shell.exec_command('ztest run-testcase ztest_shell::test_int_param')
    output = '\n'.join(lines)
    assert 'Passed integer: 44' in output, 'expected response not found'

@pytest.mark.parametrize("test_input", ["Testing", "Zephyr", "RTOS"])
def test_run_testcase_with_string_parameter(shell: Shell, test_input):
    shell.exec_command(f'set_parameter {test_input}')
    lines = shell.exec_command('ztest run-testcase ztest_shell::test_string_param')
    output = '\n'.join(lines)
    assert f'Passed string: {test_input}' in output, 'expected response not found'
