#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to memory footprints.
"""

import importlib
import json
import mock
import os
import pytest
import sys
import re

# pylint: disable=no-name-in-module
from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock, clear_log_in_test
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestFootprint:
    # Log printed when entering delta calculations
    FOOTPRINT_LOG = 'running footprint_reports'

    # These warnings notify us that deltas were shown in log.
    # Coupled with the code under test.
    DELTA_WARNING_COMPARE = re.compile(
        r'Found [1-9]+[0-9]* footprint deltas to .*blackbox-out\.[0-9]+/twister.json as a baseline'
    )
    DELTA_WARNING_RUN = re.compile(r'Found [1-9]+[0-9]* footprint deltas to the last twister run')

    # Size report key we modify to control for deltas
    RAM_KEY = 'used_ram'
    DELTA_DETAIL = re.compile(RAM_KEY + r' \+[0-9]+, is now +[0-9]+ \+[0-9.]+%')

    @classmethod
    def setup_class(cls):
        apath = os.path.join(ZEPHYR_BASE, 'scripts', 'twister')
        cls.loader = importlib.machinery.SourceFileLoader('__main__', apath)
        cls.spec = importlib.util.spec_from_loader(cls.loader.name, cls.loader)
        cls.twister_module = importlib.util.module_from_spec(cls.spec)

    @classmethod
    def teardown_class(cls):
        pass

    @pytest.mark.parametrize(
        'old_ram_multiplier, expect_delta_log',
        [
            (0.75, True),
            (1.25, False),
        ],
        ids=['footprint increased', 'footprint reduced']
    )
    def test_compare_report(self, caplog, out_path, old_ram_multiplier, expect_delta_log):
        # First run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--enable-size-report'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Modify the older report so we can control the difference.
        # Note: if footprint tests take too long, replace first run with a prepared twister.json
        # That will increase test-to-code_under_test coupling, however.
        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)
            for ts in j['testsuites']:
                if 'reason' not in ts:
                    # We assume positive RAM usage.
                    ts[self.RAM_KEY] *= old_ram_multiplier
        with open(os.path.join(out_path, 'twister.json'), 'w') as f:
            f.write(json.dumps(j, indent=4))

        report_path = os.path.join(
            os.path.dirname(out_path),
            f'{os.path.basename(out_path)}.1',
            'twister.json'
        )

        # Second run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--compare-report', report_path] + \
               ['--show-footprint'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        assert self.FOOTPRINT_LOG in caplog.text

        if expect_delta_log:
            assert self.RAM_KEY in caplog.text
            assert re.search(self.DELTA_WARNING_COMPARE, caplog.text), \
                'Expected footprint deltas not logged.'
        else:
            assert self.RAM_KEY not in caplog.text
            assert not re.search(self.DELTA_WARNING_COMPARE, caplog.text), \
                'Unexpected footprint deltas logged.'

    def test_footprint_from_buildlog(self, out_path):
        # First run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               [] + \
               ['--enable-size-report'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Get values
        old_values = []
        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)
            for ts in j['testsuites']:
                if 'reason' not in ts:
                    assert self.RAM_KEY in ts
                    old_values += [ts[self.RAM_KEY]]

        # Second run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--footprint-from-buildlog'] + \
               ['--enable-size-report'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Get values
        new_values = []
        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)
            for ts in j['testsuites']:
                if 'reason' not in ts:
                    assert self.RAM_KEY in ts
                    new_values += [ts[self.RAM_KEY]]

        # There can be false positives if our calculations become too accurate.
        # Turn this test into a dummy (check only exit value) in such case.
        assert sorted(old_values) != sorted(new_values), \
            'Same values whether calculating size or using buildlog.'

    @pytest.mark.parametrize(
        'old_ram_multiplier, threshold, expect_delta_log',
        [
            (0.75, 95, False),
            (0.75, 25, True),
        ],
        ids=['footprint threshold not met', 'footprint threshold met']
    )
    def test_footprint_threshold(self, caplog, out_path, old_ram_multiplier,
                                 threshold, expect_delta_log):
        # First run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--enable-size-report'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Modify the older report so we can control the difference.
        # Note: if footprint tests take too long, replace first run with a prepared twister.json
        # That will increase test-to-code_under_test coupling, however.
        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)
            for ts in j['testsuites']:
                if 'reason' not in ts:
                    # We assume positive RAM usage.
                    ts[self.RAM_KEY] *= old_ram_multiplier
        with open(os.path.join(out_path, 'twister.json'), 'w') as f:
            f.write(json.dumps(j, indent=4))

        report_path = os.path.join(
            os.path.dirname(out_path),
            f'{os.path.basename(out_path)}.1',
            'twister.json'
        )

        # Second run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               [f'--footprint-threshold={threshold}'] + \
               ['--compare-report', report_path, '--show-footprint'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        assert self.FOOTPRINT_LOG in caplog.text

        if expect_delta_log:
            assert self.RAM_KEY in caplog.text
            assert re.search(self.DELTA_WARNING_COMPARE, caplog.text), \
                'Expected footprint deltas not logged.'
        else:
            assert self.RAM_KEY not in caplog.text
            assert not re.search(self.DELTA_WARNING_COMPARE, caplog.text), \
                'Unexpected footprint deltas logged.'

    @pytest.mark.parametrize(
        'flags, old_ram_multiplier, expect_delta_log',
        [
            ([], 0.75, False),
            (['--show-footprint'], 0.75, True),
        ],
        ids=['footprint reduced, no show', 'footprint reduced, show']
    )
    def test_show_footprint(self, caplog, out_path, flags, old_ram_multiplier, expect_delta_log):
        # First run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--enable-size-report'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Modify the older report so we can control the difference.
        # Note: if footprint tests take too long, replace first run with a prepared twister.json
        # That will increase test-to-code_under_test coupling, however.
        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)
            for ts in j['testsuites']:
                if 'reason' not in ts:
                    # We assume positive RAM usage.
                    ts[self.RAM_KEY] *= old_ram_multiplier
        with open(os.path.join(out_path, 'twister.json'), 'w') as f:
            f.write(json.dumps(j, indent=4))

        report_path = os.path.join(
            os.path.dirname(out_path),
            f'{os.path.basename(out_path)}.1',
            'twister.json'
        )

        # Second run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               flags + \
               ['--compare-report', report_path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]
        print(args)
        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        assert self.FOOTPRINT_LOG in caplog.text

        if expect_delta_log:
            assert self.RAM_KEY in caplog.text
            assert re.search(self.DELTA_DETAIL, caplog.text), \
                'Expected footprint delta not logged.'
            assert re.search(self.DELTA_WARNING_COMPARE, caplog.text), \
                'Expected footprint deltas not logged.'
        else:
            assert self.RAM_KEY not in caplog.text
            assert not re.search(self.DELTA_DETAIL, caplog.text), \
                'Expected footprint delta not logged.'
            assert re.search(self.DELTA_WARNING_COMPARE, caplog.text), \
                'Expected footprint deltas logged.'

    @pytest.mark.parametrize(
        'old_ram_multiplier, expect_delta_log',
        [
            (0.75, True),
            (1.25, False),
        ],
        ids=['footprint increased', 'footprint reduced']
    )
    def test_last_metrics(self, caplog, out_path, old_ram_multiplier, expect_delta_log):
        # First run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--enable-size-report'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Modify the older report so we can control the difference.
        # Note: if footprint tests take too long, replace first run with a prepared twister.json
        # That will increase test-to-code_under_test coupling, however.
        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)
            for ts in j['testsuites']:
                if 'reason' not in ts:
                    # We assume positive RAM usage.
                    ts[self.RAM_KEY] *= old_ram_multiplier
        with open(os.path.join(out_path, 'twister.json'), 'w') as f:
            f.write(json.dumps(j, indent=4))

        report_path = os.path.join(
            os.path.dirname(out_path),
            f'{os.path.basename(out_path)}.1',
            'twister.json'
        )

        # Second run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--last-metrics'] + \
               ['--show-footprint'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        assert self.FOOTPRINT_LOG in caplog.text

        if expect_delta_log:
            assert self.RAM_KEY in caplog.text
            assert re.search(self.DELTA_WARNING_RUN, caplog.text), \
                'Expected footprint deltas not logged.'
        else:
            assert self.RAM_KEY not in caplog.text
            assert not re.search(self.DELTA_WARNING_RUN, caplog.text), \
                'Unexpected footprint deltas logged.'

        second_logs = caplog.records
        caplog.clear()
        clear_log_in_test()

        # Third run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--compare-report', report_path] + \
               ['--show-footprint'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Since second run should use the same source as the third, we should compare them.
        delta_logs = [
            record.getMessage() for record in second_logs \
            if self.RAM_KEY in record.getMessage()
        ]
        assert all([log in caplog.text for log in delta_logs])

    @pytest.mark.parametrize(
        'old_ram_multiplier, expect_delta_log',
        [
            (0.75, True),
            (1.00, False),
            (1.25, True),
        ],
        ids=['footprint increased', 'footprint constant', 'footprint reduced']
    )
    def test_all_deltas(self, caplog, out_path, old_ram_multiplier, expect_delta_log):
        # First run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--enable-size-report'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Modify the older report so we can control the difference.
        # Note: if footprint tests take too long, replace first run with a prepared twister.json
        # That will increase test-to-code_under_test coupling, however.
        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)
            for ts in j['testsuites']:
                if 'reason' not in ts:
                    # We assume positive RAM usage.
                    ts[self.RAM_KEY] *= old_ram_multiplier
        with open(os.path.join(out_path, 'twister.json'), 'w') as f:
            f.write(json.dumps(j, indent=4))

        report_path = os.path.join(
            os.path.dirname(out_path),
            f'{os.path.basename(out_path)}.1',
            'twister.json'
        )

        # Second run
        test_platforms = ['intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--all-deltas'] + \
               ['--compare-report', report_path, '--show-footprint'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        assert self.FOOTPRINT_LOG in caplog.text

        if expect_delta_log:
            assert self.RAM_KEY in caplog.text
            assert re.search(self.DELTA_WARNING_COMPARE, caplog.text), \
                'Expected footprint deltas not logged.'
        else:
            assert self.RAM_KEY not in caplog.text
            assert not re.search(self.DELTA_WARNING_COMPARE, caplog.text), \
                'Unexpected footprint deltas logged.'
