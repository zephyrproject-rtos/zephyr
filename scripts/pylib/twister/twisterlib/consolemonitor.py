# vim: set syntax=python ts=4 :
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Live console dashboard for a twister run (``--console-monitor``).

Renders a ``top``-style curses dashboard in the terminal running twister:
overall progress, in-flight jobs with their pipeline stage, a scrollable
and filterable instance table, and a per-instance detail view with the
stage timeline, failing test cases and log tails.

The UI runs in a thread of the twister process and observes the run
through :class:`LocalMonitorSource`, which reads the thread-safe state
maintained by ``twisterlib/runmonitor.py`` -- it is strictly an observer
and cannot influence the run.
"""

from __future__ import annotations

import contextlib
import json
import os
import threading
import time

# Counter line: (counter name, display label, color class).
COUNTER_FIELDS = (
    ('passed', 'passed', 'passed'),
    ('failed', 'failed', 'failed'),
    ('error', 'error', 'error'),
    ('notrun', 'built-only', 'not run'),
    ('skipped', 'skipped', 'skipped'),
    ('filtered_configs', 'filtered', 'filtered'),
)

FILTERS = ('all', 'active', 'failures', 'passed', 'queued', 'filtered')


def format_duration(seconds) -> str:
    """Human-friendly duration: 42s / 3m 10s / 2h 05m."""
    if seconds is None:
        return ''
    seconds = max(0, int(seconds))
    if seconds < 60:
        return f"{seconds}s"
    minutes, secs = divmod(seconds, 60)
    if minutes < 60:
        return f"{minutes}m {secs:02d}s"
    hours, minutes = divmod(minutes, 60)
    return f"{hours}h {minutes:02d}m"


def progress_bar(counters: dict, width: int) -> str:
    """Render an ASCII progress bar over the to-do total.

    Mirrors the semantics of the console output: the denominator excludes
    statically filtered instances.
    """
    total = max(0, counters.get('total', 0) - counters.get('filtered_static', 0))
    done = max(0, counters.get('done', 0) - counters.get('filtered_static', 0))
    width = max(10, width)
    if total == 0:
        return '[' + ' ' * width + ']  0/0'
    filled = round(width * min(done, total) / total)
    pct = int(100 * done / total)
    return f"[{'#' * filled}{'-' * (width - filled)}] {done}/{total} ({pct}%)"


def eta(counters: dict, elapsed: float) -> float | None:
    """Naive remaining-time estimate from average per-instance rate."""
    total = max(0, counters.get('total', 0) - counters.get('filtered_static', 0))
    done = max(0, counters.get('done', 0) - counters.get('filtered_static', 0))
    if done <= 5 or done >= total or elapsed <= 0:
        return None
    return elapsed * (total - done) / done


def match_filter(row: dict, name: str) -> bool:
    if name == 'active':
        return bool(row.get('current_op'))
    if name == 'failures':
        return row.get('status') in ('failed', 'error')
    if name == 'passed':
        return row.get('status') == 'passed'
    if name == 'queued':
        return row.get('status') == 'none' and not row.get('current_op')
    if name == 'filtered':
        return row.get('status') in ('filtered', 'skipped')
    return True


def sort_rows(rows: list[dict]) -> list[dict]:
    """Active first, then failures, then the rest, each by name."""
    order = {
        'failed': 1,
        'error': 1,
        'passed': 3,
        'not run': 4,
        'none': 5,
        'skipped': 6,
        'filtered': 7,
    }

    def key(row):
        primary = 0 if row.get('current_op') else order.get(row.get('status'), 5)
        return (primary, row.get('name', ''))

    return sorted(rows, key=key)


def status_cell(row: dict) -> tuple[str, str]:
    """(text, color-class) for a row's status column."""
    if row.get('current_op'):
        return row['current_op'], 'running'
    status = row.get('status') or 'none'
    label = {'none': 'queued', 'not run': 'built'}.get(status, status)
    return label, status


def failing_cases(detail: dict) -> list[dict]:
    """The test cases of an instance detail that need attention."""
    return [
        case
        for case in detail.get('testcases', [])
        if case.get('status') in ('failed', 'error', 'blocked')
    ]


class Snapshot:
    """Latest data pulled from the monitor source."""

    def __init__(self):
        self.summary: dict | None = None
        self.rows: list[dict] = []
        self.connected = False
        self.ever_connected = False

    def poll(self, source) -> None:
        try:
            self.summary = source.summary()
            self.rows = source.instances()
            self.connected = True
            self.ever_connected = True
        except (OSError, ValueError, KeyError):
            self.connected = False

    def state_label(self) -> str:
        if self.connected:
            return (self.summary or {}).get('meta', {}).get('state', 'running').upper()
        return 'CONNECTING...'


class LocalMonitorSource:
    """Reads a RunMonitor's state in-process for the console dashboard.

    The MonitorState JSON methods provide the thread-safe snapshotting; the
    UI thread never touches the state's internals directly.
    """

    MAX_TAIL = 1024 * 1024

    def __init__(self, monitor):
        self.monitor = monitor
        self.base_url = monitor.state.meta.get('outdir', 'local run')

    def summary(self) -> dict:
        return json.loads(self.monitor.state.summary_json(self.monitor.results))

    def instances(self) -> list[dict]:
        return json.loads(self.monitor.state.instances_json())['instances']

    def instance(self, name: str) -> dict:
        payload = self.monitor.state.instance_json(name)
        if payload is None:
            raise KeyError(name)
        return json.loads(payload)

    def log_tail(self, name: str, fname: str, tail: int = 16 * 1024) -> str:
        path = self.monitor.state.log_path(name, fname)
        if path is None:
            return ''
        tail = max(1, min(tail, self.MAX_TAIL))
        size = os.path.getsize(path)
        with open(path, 'rb') as f:
            if size > tail:
                f.seek(size - tail)
            data = f.read(tail)
        if size > tail:
            data = b'[... truncated ...]\n' + data
        return data.decode(errors='replace')


class ConsoleUI:
    """Full-screen curses dashboard. All curses use is contained here."""

    def __init__(self, client: LocalMonitorSource, interval: float):
        self.client = client
        self.interval = interval
        self.curses = None  # the curses module, bound in run()
        # Embedded mode: the UI runs inside the twister process itself
        # (--console-monitor); adjusts the key hints and quit semantics.
        self.embedded = False
        # External request to leave the UI (e.g. the run was interrupted).
        self.stop_requested = threading.Event()
        self.snap = Snapshot()
        self.filter_idx = 0
        self.scroll = 0
        self.selected = 0
        self.search = ''  # substring filter over instance name + reason
        self.search_mode = False  # '/' pressed: keys edit the search text
        self.detail: dict | None = None  # /api/instance payload when open
        self.detail_log: str | None = None  # current log file name
        self.detail_log_text = ''
        self.log_scroll = 0
        self.last_poll = 0.0

    # -- data ------------------------------------------------------------

    def visible_rows(self) -> list[dict]:
        name = FILTERS[self.filter_idx]
        rows = [r for r in self.snap.rows if match_filter(r, name)]
        if self.search:
            needle = self.search.lower()
            rows = [
                r
                for r in rows
                if needle in (r.get('name', '') + ' ' + (r.get('reason') or '')).lower()
            ]
        return sort_rows(rows)

    def poll_if_due(self) -> bool:
        if time.monotonic() - self.last_poll < self.interval:
            return False
        self.last_poll = time.monotonic()
        self.snap.poll(self.client)
        if self.detail is not None:
            self.refresh_detail()
        return True

    def refresh_detail(self) -> None:
        try:
            self.detail = self.client.instance(self.detail['name'])
            logs = [entry['file'] for entry in self.detail.get('logs', [])]
            if logs and self.detail_log not in logs:
                preferred = 'handler.log' if 'handler.log' in logs else logs[0]
                self.detail_log = preferred
            if self.detail_log:
                self.detail_log_text = self.client.log_tail(self.detail['name'], self.detail_log)
        except (OSError, ValueError, KeyError):
            pass

    def open_detail(self, row: dict) -> None:
        self.detail = {'name': row['name']}
        self.detail_log = None
        self.detail_log_text = ''
        self.log_scroll = 0
        self.refresh_detail()

    def cycle_log(self) -> None:
        logs = [entry['file'] for entry in (self.detail or {}).get('logs', [])]
        if not logs:
            return
        try:
            idx = logs.index(self.detail_log)
        except ValueError:
            idx = -1
        self.detail_log = logs[(idx + 1) % len(logs)]
        self.log_scroll = 0
        self.refresh_detail()

    # -- rendering -------------------------------------------------------

    COLOR_CLASSES = (
        'passed',
        'failed',
        'error',
        'not run',
        'skipped',
        'filtered',
        'running',
        'none',
        'header',
    )

    def init_colors(self, curses) -> dict:
        colors = dict.fromkeys(self.COLOR_CLASSES, 0)
        if not curses.has_colors():
            return colors
        curses.start_color()
        curses.use_default_colors()
        mapping = {
            'passed': curses.COLOR_GREEN,
            'failed': curses.COLOR_RED,
            'error': curses.COLOR_RED,
            'not run': curses.COLOR_CYAN,
            'skipped': curses.COLOR_YELLOW,
            'filtered': curses.COLOR_YELLOW,
            'running': curses.COLOR_MAGENTA,
            'none': -1,
            'header': curses.COLOR_BLUE,
        }
        for i, (cls, fg) in enumerate(mapping.items(), start=1):
            curses.init_pair(i, fg, -1)
            colors[cls] = curses.color_pair(i)
        return colors

    def run(self) -> int:
        import curses

        self.curses = curses
        return curses.wrapper(self._main, curses)

    def _addstr(self, win, y, x, text, attr=0):
        """addstr that clips to the window instead of raising.

        curses raises when writing the bottom-right cell or during a resize
        race; both are harmless for a redrawn-every-tick dashboard.
        """
        maxy, maxx = win.getmaxyx()
        if y < 0 or y >= maxy or x >= maxx - 1:
            return
        with contextlib.suppress(Exception):
            win.addstr(y, x, text[: maxx - x - 1], attr)

    def _reassert_terminal_modes(self, stdscr, curses) -> None:
        """Put the terminal back into curses mode if something disturbed it.

        Test runs can spawn tools that reset the tty (echo back on,
        canonical mode), which would make the UI stop responding to keys and
        echo raw escape sequences instead. Cheap to re-assert, so do it on
        every poll tick.
        """
        with contextlib.suppress(Exception):
            curses.noecho()
            curses.cbreak()
            stdscr.keypad(True)
            curses.curs_set(0)

    def _main(self, stdscr, curses) -> int:
        colors = self.init_colors(curses)
        curses.curs_set(0)
        stdscr.timeout(200)  # getch() tick; polling is time-based

        while not self.stop_requested.is_set():
            if self.poll_if_due():
                self._reassert_terminal_modes(stdscr, curses)
            stdscr.erase()
            height, width = stdscr.getmaxyx()
            self.draw_header(stdscr, curses, colors, width)
            if self.detail is not None:
                self.draw_detail(stdscr, colors, height, width)
            else:
                self.draw_table(stdscr, colors, height, width)
            self.draw_keybar(stdscr, curses, height, width)
            stdscr.refresh()

            key = stdscr.getch()
            if key == -1:
                continue
            if not self.handle_key(key, curses, height):
                return 0
        return 0

    def draw_header(self, win, curses, colors, width) -> None:
        summary = self.snap.summary or {}
        counters = summary.get('counters', {})
        meta = summary.get('meta', {})
        state = self.snap.state_label()
        elapsed = None
        if meta:
            elapsed = (meta.get('end') or summary.get('now', 0)) - meta.get('start', 0)
        head = f" twister monitor  {self.client.base_url}  [{state}]"
        if elapsed is not None:
            head += f"  elapsed {format_duration(elapsed)}"
            remaining = eta(counters, elapsed)
            if remaining is not None and meta.get('state') == 'running':
                head += f"  ~{format_duration(remaining)} left"
            if counters.get('iteration', 1) > 1:
                head += f"  iteration {counters['iteration']}"
        self._addstr(win, 0, 0, head.ljust(width), colors['header'] | curses.A_BOLD)

        self._addstr(win, 1, 1, progress_bar(counters, max(10, width - 30)))

        x = 1
        active = sum(1 for r in self.snap.rows if r.get('current_op'))
        for field, label, cls in COUNTER_FIELDS:
            text = f"{label}:{counters.get(field, 0)}  "
            self._addstr(win, 2, x, text, colors.get(cls, 0))
            x += len(text)
        self._addstr(win, 2, x, f"in-flight:{active}", colors['running'])

        tabs = '  '.join(
            f"[{f}]" if i == self.filter_idx else f" {f} " for i, f in enumerate(FILTERS)
        )
        if self.search_mode:
            tabs += f"   search: {self.search}_"
        elif self.search:
            tabs += f"   search: {self.search}"
        self._addstr(win, 3, 1, tabs, curses.A_BOLD)

    HEADER_LINES = 4
    KEYBAR_LINES = 1
    MAX_FAILING_CASES = 8

    def table_height(self, height) -> int:
        return max(1, height - self.HEADER_LINES - self.KEYBAR_LINES - 1)

    def draw_table(self, win, colors, height, width) -> None:
        rows = self.visible_rows()
        page = self.table_height(height)
        self.selected = max(0, min(self.selected, len(rows) - 1))
        if self.selected < self.scroll:
            self.scroll = self.selected
        elif self.selected >= self.scroll + page:
            self.scroll = self.selected - page + 1

        y = self.HEADER_LINES
        plat_w = 24
        stat_w = 15
        suite_w = max(20, width - plat_w - stat_w - 30)
        header = f" {'platform':<{plat_w}} {'suite':<{suite_w}} {'status':<{stat_w}} time"
        self._addstr(win, y, 0, header, colors['header'])
        for i, row in enumerate(rows[self.scroll : self.scroll + page]):
            idx = self.scroll + i
            label, cls = status_cell(row)
            timecol = (
                format_duration(row.get('execution_time')) if row.get('execution_time') else ''
            )
            reason = row.get('reason') or ''
            line = (
                f" {row.get('platform', ''):<{plat_w}.{plat_w}}"
                f" {row.get('suite', ''):<{suite_w}.{suite_w}}"
                f" {label:<{stat_w}.{stat_w}} {timecol:<8} {reason}"
            )
            attr = colors.get(cls, 0)
            if idx == self.selected:
                attr |= self.curses.A_REVERSE
            self._addstr(win, y + 1 + i, 0, line.ljust(width - 1), attr)
        if not rows:
            self._addstr(win, y + 2, 2, '(no instances match this filter)')

    def draw_detail(self, win, colors, height, width) -> None:
        detail = self.detail or {}
        y = self.HEADER_LINES
        label, cls = status_cell(detail)
        self._addstr(win, y, 1, detail.get('name', ''), colors['header'])
        line = f"status: {label}"
        if detail.get('reason'):
            line += f"  reason: {detail['reason']}"
        if detail.get('retries'):
            line += f"  retries: {detail['retries']}"
        self._addstr(win, y + 1, 1, line, colors.get(cls, 0))

        ops = detail.get('ops', [])
        stages = '  '.join(
            f"{o['op']}:" + (format_duration(o['duration']) if o.get('end') else '...') for o in ops
        )
        self._addstr(win, y + 2, 1, f"pipeline: {stages}" if stages else 'pipeline: (queued)')

        cases = detail.get('testcases', [])
        bad = failing_cases(detail)
        self._addstr(win, y + 3, 1, f"cases: {len(cases)} total, {len(bad)} failing")
        y += 4

        # List the failing test cases with their reasons, so a failure can
        # be understood without leaving the dashboard.
        for case in bad[: self.MAX_FAILING_CASES]:
            text = f"  {case['name']}"
            if case.get('reason'):
                text += f"  ({case['reason']})"
            self._addstr(win, y, 1, text, colors['failed'])
            y += 1
        if len(bad) > self.MAX_FAILING_CASES:
            self._addstr(
                win,
                y,
                1,
                f"  ... and {len(bad) - self.MAX_FAILING_CASES} more failing cases",
                colors['failed'],
            )
            y += 1

        logs = [entry['file'] for entry in detail.get('logs', [])]
        logline = f"log: {self.detail_log or '(none)'}"
        if len(logs) > 1:
            logline += f"  ({'  '.join(logs)} - press l to switch)"
        self._addstr(win, y, 1, logline, colors['header'])
        y += 1

        log_area = max(1, height - y - self.KEYBAR_LINES)
        lines = self.detail_log_text.splitlines()
        max_scroll = max(0, len(lines) - log_area)
        # log_scroll counts lines up from the end of the log; 0 means the
        # view is pinned to the tail and follows new output on refresh.
        self.log_scroll = max(0, min(self.log_scroll, max_scroll))
        start = max(0, len(lines) - log_area - self.log_scroll)
        for i, text in enumerate(lines[start : start + log_area]):
            self._addstr(win, y + i, 1, text)

    def draw_keybar(self, win, curses, height, width) -> None:
        if self.search_mode:
            keys = " type to filter   enter apply   esc clear   backspace delete"
            self._addstr(win, height - 1, 0, keys.ljust(width - 1), curses.A_REVERSE)
            return
        if self.embedded:
            finished = (self.snap.summary or {}).get('meta', {}).get('state') == 'finished'
            if finished:
                quit_hint = 'q continue (write reports and exit)'
            else:
                quit_hint = 'q leave monitor (run continues)   ctrl-c abort run'
        else:
            quit_hint = 'q quit'
        if self.detail is None:
            keys = (
                f" {quit_hint}   enter detail   f failures   / search"
                "   j/k or arrows select   tab filter   r refresh"
            )
        else:
            keys = (
                f" {quit_hint}   esc back   l switch log"
                "   j/k or arrows scroll   g/G top/end   r refresh"
            )
        self._addstr(win, height - 1, 0, keys.ljust(width - 1), curses.A_REVERSE)

    # -- input -----------------------------------------------------------

    def _handle_search_key(self, key, curses) -> None:
        """Edit the search text while '/' input mode is active."""
        if key in (curses.KEY_ENTER, ord('\n'), ord('\r')):
            self.search_mode = False
        elif key == 27:  # ESC: cancel the search entirely
            self.search = ''
            self.search_mode = False
        elif key in (curses.KEY_BACKSPACE, 127, 8):
            self.search = self.search[:-1]
        elif 32 <= key <= 126:
            self.search += chr(key)
        self.selected = self.scroll = 0

    def _select_failure_filter(self) -> None:
        """Toggle between the failures view and everything."""
        failures = FILTERS.index('failures')
        self.filter_idx = 0 if self.filter_idx == failures else failures
        self.selected = self.scroll = 0

    def handle_key(self, key, curses, height) -> bool:
        page = self.table_height(height)
        if self.search_mode and self.detail is None:
            self._handle_search_key(key, curses)
            return True
        if key in (ord('q'), ord('Q')):
            return False
        if key == ord('r'):
            self.last_poll = 0
        elif self.detail is None:
            if key == ord('\t'):
                self.filter_idx = (self.filter_idx + 1) % len(FILTERS)
                self.selected = self.scroll = 0
            elif key == ord('f'):
                self._select_failure_filter()
            elif key == ord('/'):
                self.search_mode = True
            elif key == 27 and self.search:  # ESC clears an applied search
                self.search = ''
                self.selected = self.scroll = 0
            elif key in (curses.KEY_UP, ord('k')):
                self.selected -= 1
            elif key in (curses.KEY_DOWN, ord('j')):
                self.selected += 1
            elif key == curses.KEY_PPAGE:
                self.selected -= page
            elif key == curses.KEY_NPAGE:
                self.selected += page
            elif key in (ord('g'), curses.KEY_HOME):
                self.selected = 0
            elif key in (ord('G'), curses.KEY_END):
                self.selected = len(self.visible_rows()) - 1
            elif key in (curses.KEY_ENTER, ord('\n'), ord('\r')):
                rows = self.visible_rows()
                if rows:
                    self.open_detail(rows[min(self.selected, len(rows) - 1)])
        else:
            if key == 27:  # ESC
                self.detail = None
            elif key == ord('l'):
                self.cycle_log()
            elif key in (curses.KEY_UP, ord('k')):
                self.log_scroll += 1
            elif key in (curses.KEY_DOWN, ord('j')):
                self.log_scroll -= 1
            elif key == curses.KEY_PPAGE:
                self.log_scroll += page
            elif key == curses.KEY_NPAGE:
                self.log_scroll -= page
            elif key in (ord('g'), curses.KEY_HOME):
                self.log_scroll = len(self.detail_log_text.splitlines())
            elif key in (ord('G'), curses.KEY_END):
                self.log_scroll = 0
        return True
