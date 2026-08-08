# vim: set syntax=python ts=4 :
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Live run monitoring core for twister.

The runner's worker processes emit plain-dict events (one per pipeline op
start/completion) over a ``multiprocessing.Queue``; a drain thread in the
main process folds them into an in-memory :class:`MonitorState` -- one row
per test instance, initialized from the test plan so statically filtered
instances are visible too.

This module is transport-free: observers read the state in-process. The
``--console-monitor`` dashboard (``twisterlib/consolemonitor.py``) is one
such observer; serving the same state remotely can be layered on top.

Monitoring is strictly an observer: events are emitted with ``put_nowait``
and dropped on any queue error, so a monitor can never stall or fail the
run itself.
"""

from __future__ import annotations

import json
import logging
import multiprocessing
import os
import queue
import threading
import time

logger = logging.getLogger('twister')

# Shared flag telling the worker processes whether the --console-monitor UI
# currently owns the terminal. Created in the main process before the
# workers are forked (so they inherit the shared memory) and cleared the
# moment the UI exits, letting normal console behaviour (progress ticker,
# terminal resets) resume immediately when the user leaves the dashboard
# mid-run. Deliberately a module global rather than an options attribute:
# options travels inside pickled TestInstance objects through the manager
# queues, and multiprocessing.Value cannot be pickled that way.
_ui_active_flag = None

# ExecutionCounter attributes mirrored into the monitor summary.
SUMMARY_COUNTERS = (
    'total',
    'done',
    'iteration',
    'passed',
    'failed',
    'error',
    'skipped',
    'filtered_configs',
    'filtered_static',
    'filtered_runtime',
    'notrun',
    'warnings',
    'cases',
    'filtered_cases',
    'skipped_cases',
    'passed_cases',
    'notrun_cases',
    'failed_cases',
    'error_cases',
    'blocked_cases',
    'none_cases',
    'started_cases',
)

# Only these files from an instance's build directory are exposed to
# monitor log viewers.
LOG_FILE_ALLOWLIST = (
    'build.log',
    'handler.log',
    'device.log',
    'twister_harness.log',
    'valgrind.log',
)


def create_ui_active_flag():
    """Arm the UI-active flag; call in the main process before forking."""
    global _ui_active_flag
    _ui_active_flag = multiprocessing.Value('i', 1)
    return _ui_active_flag


def clear_ui_active_flag() -> None:
    """The UI exited: workers switch back to normal console behaviour."""
    if _ui_active_flag is not None:
        _ui_active_flag.value = 0


def console_ui_active() -> bool:
    """True while the --console-monitor UI owns the terminal."""
    flag = _ui_active_flag
    return flag is not None and flag.value == 1


def status_str(status) -> str:
    """Map a TwisterStatus (or raw string) to a JSON-friendly string.

    TwisterStatus is a str-mixin Enum, so TwisterStatus.NONE.value is the
    literal string 'None' -- normalize that (and empty/None) to 'none'.
    """
    value = getattr(status, 'value', status)
    return value if value and value != 'None' else 'none'


def make_event(kind: str, op: str, instance, **extra) -> dict:
    """Serialize an instance's pipeline transition into a plain-dict event.

    Runs in the worker process, so everything must reduce to picklable
    scalars here. The final 'report' op additionally carries the per-testcase
    results and metrics so the monitor state is complete without ever
    touching live TestInstance objects across processes.
    """
    evt = {
        'kind': kind,
        'op': op,
        'name': instance.name,
        'ts': time.time(),
        'status': status_str(instance.status),
        'reason': instance.reason,
    }
    evt.update(extra)
    if kind == 'op_done' and op == 'report':
        evt['testcases'] = [
            {
                'name': tc.name,
                'status': status_str(tc.status),
                'reason': tc.reason,
                'duration': tc.duration,
            }
            for tc in instance.testcases
        ]
        evt['metrics'] = {
            k: v for k, v in instance.metrics.items() if isinstance(v, (int, float, str))
        }
        evt['execution_time'] = instance.execution_time
        evt['build_time'] = instance.build_time
        evt['retries'] = instance.retries
    return evt


class MonitorState:
    """Thread-safe model of the run: one row per test instance plus metadata.

    Rows are created from the test plan before execution starts (so statically
    filtered/skipped instances are visible too) and updated exclusively by
    apply_event(); all reads serialize to JSON under the same lock.
    """

    def __init__(self):
        self._lock = threading.Lock()
        self.rows: dict[str, dict] = {}
        self.meta: dict = {
            'state': 'running',
            'start': time.time(),
            'end': None,
        }

    def init_plan(self, instances: dict, outdir: str) -> None:
        with self._lock:
            self.meta['outdir'] = outdir
            for name, instance in instances.items():
                self.rows[name] = {
                    'name': name,
                    'platform': instance.platform.name,
                    'suite': instance.testsuite.name,
                    'toolchain': instance.toolchain,
                    'status': status_str(instance.status),
                    'reason': instance.reason,
                    'run': instance.run,
                    'build_dir': os.path.relpath(instance.build_dir, outdir),
                    'current_op': None,
                    'op_since': None,
                    'ops': [],
                    'testcases': [],
                    'metrics': {},
                    'execution_time': 0,
                    'build_time': 0,
                    'retries': 0,
                    'updated': None,
                }

    def set_meta(self, **kwargs) -> None:
        with self._lock:
            self.meta.update(kwargs)

    def note_iteration(self, iteration: int) -> None:
        with self._lock:
            self.meta['iteration'] = iteration

    def finish(self) -> None:
        with self._lock:
            self.meta['state'] = 'finished'
            self.meta['end'] = time.time()

    def apply_event(self, evt: dict) -> None:
        with self._lock:
            row = self.rows.get(evt.get('name'))
            if row is None:
                return
            row['status'] = evt.get('status', row['status'])
            row['reason'] = evt.get('reason', row['reason'])
            row['updated'] = evt.get('ts')
            op = evt.get('op')
            if evt.get('kind') == 'op_start':
                row['current_op'] = op
                row['op_since'] = evt.get('ts')
                row['ops'].append(
                    {
                        'op': op,
                        'start': evt.get('ts'),
                        'end': None,
                        'duration': None,
                        'status': None,
                    }
                )
            elif evt.get('kind') == 'op_done':
                row['current_op'] = None
                row['op_since'] = None
                # Close the most recent open entry for this op.
                for entry in reversed(row['ops']):
                    if entry['op'] == op and entry['end'] is None:
                        entry['end'] = evt.get('ts')
                        entry['duration'] = evt.get('duration')
                        entry['status'] = evt.get('status')
                        break
                for key in (
                    'testcases',
                    'metrics',
                    'execution_time',
                    'build_time',
                    'retries',
                ):
                    if key in evt:
                        row[key] = evt[key]

    # Compact row: everything an instance table needs, without the
    # potentially large ops/testcases payloads.
    _COMPACT_FIELDS = (
        'name',
        'platform',
        'suite',
        'toolchain',
        'status',
        'reason',
        'run',
        'current_op',
        'op_since',
        'execution_time',
        'retries',
        'updated',
    )

    def instances_json(self) -> str:
        with self._lock:
            compact = [{k: row[k] for k in self._COMPACT_FIELDS} for row in self.rows.values()]
            return json.dumps({'now': time.time(), 'instances': compact})

    def instance_json(self, name: str) -> str | None:
        with self._lock:
            row = self.rows.get(name)
            if row is None:
                return None
            detail = dict(row)
            outdir = self.meta.get('outdir')
        # Filesystem scan outside the lock; build_dir may not exist yet.
        logs = []
        if outdir:
            build_dir = os.path.join(outdir, detail['build_dir'])
            for fname in LOG_FILE_ALLOWLIST:
                path = os.path.join(build_dir, fname)
                if os.path.isfile(path):
                    logs.append({'file': fname, 'size': os.path.getsize(path)})
        detail['logs'] = logs
        return json.dumps(detail)

    def summary_json(self, results) -> str:
        counters = {}
        if results is not None:
            counters = {name: getattr(results, name) for name in SUMMARY_COUNTERS}
        with self._lock:
            meta = dict(self.meta)
            active = sum(1 for row in self.rows.values() if row['current_op'])
        return json.dumps(
            {
                'now': time.time(),
                'meta': meta,
                'counters': counters,
                'active': active,
            }
        )

    def log_path(self, name: str, fname: str) -> str | None:
        """Resolve a validated log file path for an instance, or None."""
        if fname not in LOG_FILE_ALLOWLIST:
            return None
        with self._lock:
            row = self.rows.get(name)
            outdir = self.meta.get('outdir')
        if row is None or not outdir:
            return None
        path = os.path.join(outdir, row['build_dir'], fname)
        # Belt and braces: never follow a path outside the output directory.
        if not os.path.realpath(path).startswith(os.path.realpath(outdir) + os.sep):
            return None
        return path if os.path.isfile(path) else None


class RunMonitor:
    """Owns the monitor state and the event drain thread for one run."""

    def __init__(self, event_queue, results):
        self.event_queue = event_queue
        self.results = results
        self.state = MonitorState()
        self._stop = threading.Event()
        self._threads: list[threading.Thread] = []

    def start(self) -> bool:
        drain = threading.Thread(target=self._drain, name='twister-monitor-drain', daemon=True)
        self._threads = [drain]
        drain.start()
        return True

    def _drain(self):
        while not self._stop.is_set():
            try:
                evt = self.event_queue.get(timeout=0.5)
            except queue.Empty:
                continue
            except (EOFError, OSError):
                break
            self.state.apply_event(evt)

    def finish(self):
        """Mark the run finished; observers keep working until twister exits."""
        # Give the drain thread a moment to catch up with the tail of the
        # event queue before declaring the final state.
        deadline = time.time() + 2.0
        while time.time() < deadline:
            if self.event_queue.empty():
                break
            time.sleep(0.1)
        self.state.finish()

    def stop(self):
        self._stop.set()
