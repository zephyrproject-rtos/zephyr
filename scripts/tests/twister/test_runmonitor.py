#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the twister run monitor core (twisterlib.runmonitor).
"""

import json
import os
import queue
import time
from types import SimpleNamespace

import pytest
from twisterlib.runmonitor import (
    LOG_FILE_ALLOWLIST,
    SUMMARY_COUNTERS,
    MonitorState,
    RunMonitor,
    make_event,
    status_str,
)
from twisterlib.statuses import TwisterStatus


def mock_instance(name='native_sim/host/tests.dummy', outdir='/tmp/out'):
    platform, toolchain, *_ = name.split('/')
    return SimpleNamespace(
        name=name,
        platform=SimpleNamespace(name=platform),
        testsuite=SimpleNamespace(name=name.split('/', 2)[2]),
        toolchain=toolchain,
        status=TwisterStatus.NONE,
        reason=None,
        run=True,
        build_dir=os.path.join(outdir, name),
        testcases=[
            SimpleNamespace(
                name=f'{name}.case1',
                status=TwisterStatus.PASS,
                reason=None,
                duration=1.5,
            ),
        ],
        metrics={'used_ram': 2048, 'used_rom': 4096, 'unpicklable': object()},
        execution_time=2.5,
        build_time=10.0,
        retries=0,
    )


def test_status_str():
    assert status_str(TwisterStatus.PASS) == 'passed'
    assert status_str(TwisterStatus.NONE) == 'none'
    assert status_str(None) == 'none'
    assert status_str('failed') == 'failed'


def test_make_event_op_start():
    instance = mock_instance()
    evt = make_event('op_start', 'cmake', instance)
    assert evt['kind'] == 'op_start'
    assert evt['op'] == 'cmake'
    assert evt['name'] == instance.name
    assert evt['status'] == 'none'
    assert 'testcases' not in evt


def test_make_event_report_carries_results():
    instance = mock_instance()
    instance.status = TwisterStatus.PASS
    evt = make_event('op_done', 'report', instance, duration=0.1)
    assert evt['status'] == 'passed'
    assert evt['testcases'][0]['status'] == 'passed'
    assert evt['metrics'] == {'used_ram': 2048, 'used_rom': 4096}
    assert evt['execution_time'] == 2.5
    # events must be JSON serializable end to end
    json.dumps(evt)


def make_state(outdir):
    instance = mock_instance(outdir=str(outdir))
    state = MonitorState()
    state.init_plan({instance.name: instance}, str(outdir))
    return state, instance


def test_state_event_lifecycle(tmp_path):
    state, instance = make_state(tmp_path)
    row = state.rows[instance.name]
    assert row['status'] == 'none'

    state.apply_event(make_event('op_start', 'build', instance))
    assert row['current_op'] == 'build'
    assert row['ops'][-1]['end'] is None

    instance.status = TwisterStatus.PASS
    state.apply_event(make_event('op_done', 'build', instance, duration=3.0))
    assert row['current_op'] is None
    assert row['ops'][-1]['duration'] == 3.0
    assert row['status'] == 'passed'

    state.apply_event(make_event('op_done', 'report', instance, duration=0.1))
    assert row['testcases'][0]['name'] == f'{instance.name}.case1'

    # unknown instances must be ignored, not crash
    state.apply_event({'kind': 'op_start', 'op': 'build', 'name': 'bogus', 'ts': 0})


def test_state_json_snapshots(tmp_path):
    state, instance = make_state(tmp_path)
    results = SimpleNamespace(**{name: 0 for name in SUMMARY_COUNTERS})

    summary = json.loads(state.summary_json(results))
    assert summary['meta']['state'] == 'running'
    assert 'total' in summary['counters']

    rows = json.loads(state.instances_json())['instances']
    assert rows[0]['name'] == instance.name

    detail = json.loads(state.instance_json(instance.name))
    assert detail['name'] == instance.name
    assert detail['logs'] == []
    assert state.instance_json('bogus') is None

    state.finish()
    assert json.loads(state.summary_json(results))['meta']['state'] == 'finished'


def test_log_path_validation(tmp_path):
    state, instance = make_state(tmp_path)
    build_dir = tmp_path / instance.name
    build_dir.mkdir(parents=True)
    log = build_dir / 'build.log'
    log.write_text('hello build')

    assert state.log_path(instance.name, 'build.log') == str(log)
    # not in the allowlist
    assert state.log_path(instance.name, '../../secret') is None
    assert state.log_path(instance.name, 'zephyr.elf') is None
    # unknown instance / missing file
    assert state.log_path('bogus', 'build.log') is None
    assert state.log_path(instance.name, 'handler.log') is None


def test_run_monitor_drains_events(tmp_path):
    results = SimpleNamespace(**{name: 0 for name in SUMMARY_COUNTERS})
    mon = RunMonitor(queue.Queue(), results)
    instance = mock_instance(outdir=str(tmp_path))
    mon.state.init_plan({instance.name: instance}, str(tmp_path))
    assert mon.start()
    try:
        mon.event_queue.put(make_event('op_start', 'cmake', instance))
        deadline = time.time() + 5
        while time.time() < deadline:
            if mon.state.rows[instance.name]['current_op'] == 'cmake':
                break
            time.sleep(0.05)
        else:
            pytest.fail('event did not reach the monitor state')
        mon.finish()
        assert mon.state.meta['state'] == 'finished'
    finally:
        mon.stop()


def test_allowlist_is_logs_only():
    assert all(f.endswith('.log') for f in LOG_FILE_ALLOWLIST)


def test_console_ui_active():
    import twisterlib.runmonitor as runmonitor

    saved = runmonitor._ui_active_flag
    try:
        # no flag armed: monitoring is off
        runmonitor._ui_active_flag = None
        assert runmonitor.console_ui_active() is False
        # clearing without a flag must be a no-op, not a crash
        runmonitor.clear_ui_active_flag()

        # armed by the runner while the UI owns the terminal
        runmonitor.create_ui_active_flag()
        assert runmonitor.console_ui_active() is True
        # cleared the moment the UI exits
        runmonitor.clear_ui_active_flag()
        assert runmonitor.console_ui_active() is False
    finally:
        runmonitor._ui_active_flag = saved
