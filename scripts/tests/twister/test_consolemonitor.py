#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the twister console dashboard (twisterlib.consolemonitor).
"""

import logging
import queue
import time
from types import SimpleNamespace

import pytest
from twisterlib.consolemonitor import (
    FILTERS,
    ConsoleUI,
    LocalMonitorSource,
    Snapshot,
    eta,
    failing_cases,
    format_duration,
    match_filter,
    progress_bar,
    sort_rows,
    status_cell,
)
from twisterlib.runmonitor import SUMMARY_COUNTERS, RunMonitor, make_event
from twisterlib.statuses import TwisterStatus


def test_format_duration():
    assert format_duration(None) == ''
    assert format_duration(42) == '42s'
    assert format_duration(190) == '3m 10s'
    assert format_duration(7500) == '2h 05m'


def test_progress_bar():
    counters = {'total': 10, 'done': 5, 'filtered_static': 0}
    bar = progress_bar(counters, 10)
    assert bar == '[#####-----] 5/10 (50%)'
    # statically filtered instances are excluded from the denominator
    counters = {'total': 12, 'done': 7, 'filtered_static': 2}
    assert '5/10' in progress_bar(counters, 10)
    assert '0/0' in progress_bar({}, 10)


def test_eta():
    counters = {'total': 100, 'done': 50, 'filtered_static': 0}
    assert eta(counters, 100.0) == pytest.approx(100.0)
    # too few samples or done: no estimate
    assert eta({'total': 100, 'done': 2}, 100.0) is None
    assert eta({'total': 100, 'done': 100}, 100.0) is None


def test_match_filter():
    running = {'status': 'none', 'current_op': 'build'}
    failed = {'status': 'failed', 'current_op': None}
    queued = {'status': 'none', 'current_op': None}
    assert match_filter(running, 'active')
    assert not match_filter(queued, 'active')
    assert match_filter(failed, 'failures')
    assert match_filter(queued, 'queued')
    assert all(match_filter(r, 'all') for r in (running, failed, queued))


def test_sort_rows_active_then_failures():
    rows = [
        {'name': 'c', 'status': 'passed', 'current_op': None},
        {'name': 'b', 'status': 'failed', 'current_op': None},
        {'name': 'a', 'status': 'none', 'current_op': 'run'},
    ]
    assert [r['name'] for r in sort_rows(rows)] == ['a', 'b', 'c']


def test_status_cell():
    assert status_cell({'current_op': 'cmake'}) == ('cmake', 'running')
    assert status_cell({'status': 'failed'}) == ('failed', 'failed')
    assert status_cell({'status': 'none'}) == ('queued', 'none')
    assert status_cell({'status': 'not run'}) == ('built', 'not run')


def test_failing_cases():
    detail = {
        'testcases': [
            {'name': 'a', 'status': 'passed'},
            {'name': 'b', 'status': 'failed', 'reason': 'assert'},
            {'name': 'c', 'status': 'blocked'},
            {'name': 'd', 'status': 'error'},
        ]
    }
    assert [c['name'] for c in failing_cases(detail)] == ['b', 'c', 'd']
    assert failing_cases({}) == []


def mock_instance(name, outdir):
    platform, toolchain, suite = name.split('/', 2)
    return SimpleNamespace(
        name=name,
        platform=SimpleNamespace(name=platform),
        testsuite=SimpleNamespace(name=suite),
        toolchain=toolchain,
        status=TwisterStatus.NONE,
        reason=None,
        run=True,
        build_dir=f'{outdir}/{name}',
        testcases=[],
        metrics={},
        execution_time=0,
        build_time=0,
        retries=0,
    )


@pytest.fixture
def local_monitor(tmp_path):
    """A RunMonitor with one instance, as --console-monitor uses it."""
    results = SimpleNamespace(**{name: 0 for name in SUMMARY_COUNTERS})
    mon = RunMonitor(queue.Queue(), results)
    instance = mock_instance('native_sim/host/tests.a', str(tmp_path))
    mon.state.init_plan({instance.name: instance}, str(tmp_path))
    assert mon.start()
    yield mon, instance
    mon.stop()


def test_local_source_reads_state_in_process(local_monitor, tmp_path):
    mon, instance = local_monitor
    source = LocalMonitorSource(mon)

    assert source.summary()['meta']['state'] == 'running'
    assert source.instances()[0]['name'] == instance.name
    assert source.instance(instance.name)['name'] == instance.name
    with pytest.raises(KeyError):
        source.instance('bogus')

    # events flow through the drain thread into the source
    mon.event_queue.put(make_event('op_start', 'build', instance))
    deadline = time.time() + 5
    while time.time() < deadline:
        if source.instances()[0]['current_op'] == 'build':
            break
        time.sleep(0.05)
    else:
        pytest.fail('event did not reach the local monitor state')


def test_local_source_log_tail(local_monitor, tmp_path):
    mon, instance = local_monitor
    source = LocalMonitorSource(mon)
    assert source.log_tail(instance.name, 'build.log') == ''

    build_dir = tmp_path / instance.name
    build_dir.mkdir(parents=True)
    (build_dir / 'build.log').write_text('line 1\nline 2\n')
    assert 'line 2' in source.log_tail(instance.name, 'build.log')
    assert source.log_tail(instance.name, 'build.log', tail=4).startswith('[... truncated ...]')
    # allowlist still applies in-process
    assert source.log_tail(instance.name, 'zephyr.elf') == ''


def test_snapshot_with_local_source(local_monitor):
    mon, instance = local_monitor
    instance.status = TwisterStatus.FAIL
    instance.reason = 'Timeout'
    mon.state.apply_event(make_event('op_done', 'report', instance, duration=0.1))

    snap = Snapshot()
    snap.poll(LocalMonitorSource(mon))
    assert snap.connected
    assert snap.state_label() == 'RUNNING'
    row = snap.rows[0]
    assert row['status'] == 'failed'
    assert row['reason'] == 'Timeout'
    assert match_filter(row, 'failures')

    mon.state.finish()
    snap.poll(LocalMonitorSource(mon))
    assert snap.state_label() == 'FINISHED'


def make_ui(rows):
    ui = ConsoleUI(client=SimpleNamespace(), interval=1.0)
    ui.snap.rows = rows
    return ui


def sample_rows():
    return [
        {'name': 'p1/t/a.pass', 'status': 'passed', 'current_op': None, 'reason': None},
        {'name': 'p1/t/b.fail', 'status': 'failed', 'current_op': None, 'reason': 'Timeout'},
        {'name': 'p2/t/c.fail', 'status': 'failed', 'current_op': None, 'reason': 'Exited'},
        {'name': 'p2/t/d.queued', 'status': 'none', 'current_op': None, 'reason': None},
    ]


def test_ui_vi_keys_move_selection():
    curses = pytest.importorskip('curses')
    ui = make_ui(sample_rows())
    assert ui.selected == 0
    ui.handle_key(ord('j'), curses, height=30)
    ui.handle_key(ord('j'), curses, height=30)
    assert ui.selected == 2
    ui.handle_key(ord('k'), curses, height=30)
    assert ui.selected == 1
    ui.handle_key(ord('G'), curses, height=30)
    assert ui.selected == len(ui.visible_rows()) - 1
    ui.handle_key(ord('g'), curses, height=30)
    assert ui.selected == 0


def test_ui_failures_hotkey_toggles_filter():
    curses = pytest.importorskip('curses')
    ui = make_ui(sample_rows())
    ui.handle_key(ord('f'), curses, height=30)
    assert FILTERS[ui.filter_idx] == 'failures'
    assert all(r['status'] in ('failed', 'error') for r in ui.visible_rows())
    assert len(ui.visible_rows()) == 2
    ui.handle_key(ord('f'), curses, height=30)
    assert FILTERS[ui.filter_idx] == 'all'


def test_ui_search_filters_rows():
    curses = pytest.importorskip('curses')
    ui = make_ui(sample_rows())
    ui.handle_key(ord('/'), curses, height=30)
    assert ui.search_mode
    for c in 'Timeout':
        ui.handle_key(ord(c), curses, height=30)
    ui.handle_key(ord('\n'), curses, height=30)
    assert not ui.search_mode
    assert ui.search == 'Timeout'
    # search matches name and reason, case-insensitively
    assert [r['name'] for r in ui.visible_rows()] == ['p1/t/b.fail']
    # backspace editing
    ui.handle_key(ord('/'), curses, height=30)
    ui.handle_key(curses.KEY_BACKSPACE, curses, height=30)
    assert ui.search == 'Timeou'
    # ESC cancels the search entirely
    ui.handle_key(27, curses, height=30)
    assert ui.search == ''
    assert len(ui.visible_rows()) == 4


def test_ui_q_ignored_while_searching():
    curses = pytest.importorskip('curses')
    ui = make_ui(sample_rows())
    ui.handle_key(ord('/'), curses, height=30)
    # 'q' is a search character here, not quit
    assert ui.handle_key(ord('q'), curses, height=30) is True
    assert ui.search == 'q'
    ui.handle_key(27, curses, height=30)
    assert ui.handle_key(ord('q'), curses, height=30) is False


def test_console_monitor_needs_tty():
    """--console-monitor is ignored in non-interactive environments."""
    from twisterlib.runner import TwisterRunner

    options = SimpleNamespace(console_monitor=True)
    runner = TwisterRunner({}, {}, SimpleNamespace(options=options))
    # pytest runs without a tty, so the guard must trip
    assert runner._console_monitor_wanted() is False
    assert runner._start_monitors() == (None, None, None)
    assert runner.event_queue is None


def test_mute_restore_console_logging(tmp_path):
    from twisterlib.log_helper import mute_console_logging, restore_console_logging

    logger = logging.getLogger('twister')
    saved = logger.handlers[:]
    try:
        logger.handlers.clear()
        stream = logging.StreamHandler()
        filehandler = logging.FileHandler(tmp_path / 'twister.log')
        logger.addHandler(stream)
        logger.addHandler(filehandler)

        muted = mute_console_logging()
        assert muted == [stream]
        assert logger.handlers == [filehandler]

        restore_console_logging(muted)
        assert stream in logger.handlers
        filehandler.close()
    finally:
        logger.handlers = saved
