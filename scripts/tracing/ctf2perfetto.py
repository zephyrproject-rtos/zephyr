#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""
Export a Zephyr CTF trace to the Chrome/Perfetto Trace Event Format.

Reads a Common Trace Format (CTF) binary stream produced by Zephyr's tracing
subsystem (for example the ``tracing.bin`` dumped by ``samples/subsys/tracing``
when built with the CTF format) and writes the JSON Trace Event Format read by
`Perfetto <https://ui.perfetto.dev>`_ and Chrome's ``chrome://tracing``.

The CTF decoder and the running-thread and ISR segmentation come from
:zephyr_file:`scripts/tracing/trace_viewer.py`, so the export shows the same
schedule as the viewer and needs nothing but Python 3.

As babeltrace2 does, the TSDL ``metadata`` is read from next to the trace
unless ``--metadata`` points elsewhere. Only the metadata written by the build
that produced a trace describes it, so a missing one, or one that does not
describe a record of the trace, is an error rather than a partial export.

Usage::

    ./ctf2perfetto.py build/tracing.bin -o trace.json
    ./ctf2perfetto.py build/tracing.bin --metadata path/to/metadata
"""

import argparse
import bisect
import json
import os
import sys

from trace_viewer import (
    HDR,
    HDR_NO_TS,
    ISR_ENTER,
    ISR_EXIT,
    ISR_EXIT_TO_SCHEDULER,
    THREAD_SWITCHED_IN,
    THREAD_SWITCHED_OUT,
    TraceReader,
    parse_metadata,
    safe_open,
)

# Perfetto lanes ("threads") for the subsystems that are not Zephyr threads.
DEFAULT_LANE = 0
ISR_LANE = 1
MUTEX_LANE = 2
SEMAPHORE_LANE = 3
TIMER_LANE = 4
NET_LANE = 5
SOCKET_LANE = 6
GPIO_LANE = 7
CUSTOM_LANE = 8

LANE_NAMES = {
    DEFAULT_LANE: "general",
    ISR_LANE: "ISR context",
    MUTEX_LANE: "Mutex",
    SEMAPHORE_LANE: "Semaphore",
    TIMER_LANE: "Timer",
    NET_LANE: "Net",
    SOCKET_LANE: "Socket",
    GPIO_LANE: "GPIO",
    CUSTOM_LANE: "Custom",
}

# Kernel-object events are routed to a lane by the first matching substring.
KEYWORD_LANES = (
    ("semaphore", SEMAPHORE_LANE),
    ("mutex", MUTEX_LANE),
    ("timer", TIMER_LANE),
    ("gpio", GPIO_LANE),
    ("net", NET_LANE),
    ("socket", SOCKET_LANE),
)

# Thread handles are pointers, Perfetto tids are dense: number the threads from
# here so they never collide with the lanes above.
FIRST_THREAD_TID = 1000

# Perfetto groups lanes under a process; a Zephyr trace only ever has the one.
PID = 0
PROCESS_NAME = "Zephyr"

# The records the running and ISR spans are built from. Drawing them as instants
# as well would mark every span boundary twice.
SPAN_EVENTS = frozenset(
    {THREAD_SWITCHED_IN, THREAD_SWITCHED_OUT, ISR_ENTER, ISR_EXIT, ISR_EXIT_TO_SCHEDULER}
)


def load_event_defs(binary_path, override):
    """Return the {event id: EventDef} table describing ``binary_path``.

    A CTF record carries no length, so metadata that disagrees about one
    event's payload does not merely mislabel that record: the decode desyncs
    and everything after it is lost. Only the ``metadata`` the build wrote next
    to the trace describes it, so there is nothing to fall back to.
    """
    path = override or os.path.join(os.path.dirname(os.path.abspath(binary_path)), "metadata")
    if not os.path.exists(path):
        sys.exit(
            f"error: no TSDL metadata at {path}\n"
            "       pass --metadata <path> to the metadata written by the build "
            "that produced this trace"
        )
    defs = parse_metadata(path)
    if not defs:
        sys.exit(f"error: no event definitions in {path}")
    return defs, path


def check_fully_decoded(raw, tr, defs, hdr, meta_path):
    """Stop the export if decoding ended on a record the metadata is missing.

    Each decoded record has a known length, so their total is the offset where
    the decoder stopped. A complete header left there whose event id is unknown
    means the metadata does not describe this trace; anything shorter, or a
    known id, is the record a running application was still writing.
    """
    consumed = sum(hdr.size + defs[ev.eid].size for ev in tr.events)
    if len(raw) - consumed < hdr.size:
        return
    eid = hdr.unpack_from(raw, consumed)[-1]
    if eid in defs:
        return
    sys.exit(
        f"error: event id 0x{eid:x} at offset {consumed} is not described by {meta_path}\n"
        "       decoding cannot continue past it; use the metadata written by the "
        "build that produced this trace"
    )


def closed_spans(tr):
    """Return the running and ISR spans, including the ones still open.

    The decoder draws a span when it reads the event that ends it, so a thread
    still on the CPU, or an ISR not yet left, when tracing stopped is missing
    from ``tr.segments`` entirely. That is the normal way a trace ends, and
    what it was doing at the end is usually the reason it was taken, so close
    those at the last recorded timestamp instead of dropping them.
    """
    segments, isr_spans = list(tr.segments), list(tr.isr_spans)
    running = isr_start = None
    depth = 0
    for ev in tr.events:
        if ev.eid == THREAD_SWITCHED_IN:
            running = (ev.ts, ev.fields.get("thread_id"))
        elif ev.eid == THREAD_SWITCHED_OUT:
            running = None
        elif ev.eid == ISR_ENTER:
            if depth == 0:
                isr_start = ev.ts
            depth += 1
        elif ev.eid in (ISR_EXIT, ISR_EXIT_TO_SCHEDULER):
            # Nested ISRs collapse into one span, as they do in the viewer.
            depth = max(depth - 1, 0)
            if depth == 0:
                isr_start = None

    # A reader that closed them itself appended exactly these, so appending
    # again would draw every trailing span twice.
    if running and running[1] is not None and tr.t1 > running[0]:
        tail = (running[0], tr.t1, running[1])
        if tail not in segments[-1:]:
            segments.append(tail)
    if isr_start is not None and tr.t1 > isr_start:
        tail = (isr_start, tr.t1)
        if tail not in isr_spans[-1:]:
            isr_spans.append(tail)
    return segments, isr_spans


def name_lanes(records, tid_map, threads):
    """Return the phase 'M' records naming the process and the lanes in use.

    Only the lanes the export actually wrote to are named: a trace with no
    networking should not open an empty "Socket" lane in the UI.
    """
    names = dict(LANE_NAMES)
    for handle, tid in tid_map.items():
        names[tid] = threads.get(handle, {}).get("name") or f"0x{handle:x}"
    meta = [{"name": "process_name", "ph": "M", "pid": PID, "args": {"name": PROCESS_NAME}}]
    for tid in sorted({rec["tid"] for rec in records}):
        meta.append(
            {"name": "thread_name", "ph": "M", "pid": PID, "tid": tid, "args": {"name": names[tid]}}
        )
    return meta


class _Timeline:
    """Collect Trace Event records and hand them back in timestamp order.

    Records are queued grouped by kind, so they are far out of time order until
    emit() sorts them. Perfetto sorts by ``ts`` too, but a 'B' and an 'E'
    sharing a timestamp on one lane are ambiguous, and CTF events routinely
    carry the same recorded time; equal timestamps are therefore nudged apart
    per lane, which keeps each lane's order without moving a record away from
    where it happened.
    """

    NUDGE_US = 1e-3

    def __init__(self):
        self._recs = []

    def add(self, name, ts, ph, tid, args=None):
        """Queue one record, ``ts`` being the CTF timestamp in nanoseconds."""
        self._recs.append(
            (
                ts,
                len(self._recs),
                {"name": name, "ph": ph, "pid": PID, "tid": tid, "args": args or {}},
            )
        )

    def span(self, start, end, tid, name="running"):
        """Queue the 'B'/'E' pair for one span of the ``tid`` lane."""
        self.add(name, start, "B", tid)
        self.add(name, end, "E", tid)

    def emit(self):
        """Return the queued records, sorted, with their ``ts`` filled in."""
        out = []
        last = {}
        for ts, _, rec in sorted(self._recs, key=lambda r: (r[0], r[1])):
            us = ts / 1000.0
            prev = last.get(rec["tid"])
            if prev is not None and us <= prev:
                us = prev + self.NUDGE_US
            last[rec["tid"]] = us
            rec["ts"] = us
            out.append(rec)
        return out


def export(tr):
    """Convert a decoded trace into a list of Perfetto Trace Event records."""
    tl = _Timeline()
    tid_map = {}
    segments, isr_spans = closed_spans(tr)
    starts = [start for start, _, _ in segments]

    def real_tid(thread_id):
        return tid_map.setdefault(int(thread_id), FIRST_THREAD_TID + len(tid_map))

    def running_lane(ts):
        """Lane of the thread on the CPU at ``ts``, for events naming none."""
        i = bisect.bisect_right(starts, ts) - 1
        if i >= 0 and ts <= segments[i][1]:
            return real_tid(segments[i][2])
        return DEFAULT_LANE

    for start, end, tid in segments:
        tl.span(start, end, real_tid(tid))

    for start, end in isr_spans:
        tl.span(start, end, ISR_LANE, "isr_active")

    for ev in tr.events:
        if ev.eid in SPAN_EVENTS:
            continue

        name = ev.name
        fields = ev.fields or {}

        if name == "named_event":
            nm = fields.get("name")
            if nm:
                payload = {k: v for k, v in fields.items() if k != "name"}
                tl.add(str(nm), ev.ts, "i", CUSTOM_LANE, payload)
            continue

        for keyword, lane in KEYWORD_LANES:
            if keyword in name:
                tl.add(name, ev.ts, "i", lane, fields)
                break
        else:
            thread_id = fields.get("thread_id")
            tid = running_lane(ev.ts) if thread_id is None else real_tid(thread_id)
            tl.add(name, ev.ts, "i", tid, fields)

    records = tl.emit()
    return name_lanes(records, tid_map, tr.threads) + records


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Export a Zephyr CTF trace to the Chrome/Perfetto Trace Event Format",
        allow_abbrev=False,
    )
    ap.add_argument("binary", help="CTF trace binary (e.g. build/tracing.bin or ctf/data)")
    ap.add_argument("--metadata", help="path to TSDL metadata file")
    ap.add_argument("-o", "--output", default="out.json", help="output JSON filename")
    ap.add_argument(
        "--no-timestamp",
        action="store_true",
        help="trace was built without CONFIG_TRACING_CTF_TIMESTAMP",
    )
    args = ap.parse_args(argv)

    defs, meta_path = load_event_defs(args.binary, args.metadata)
    hdr = HDR_NO_TS if args.no_timestamp else HDR

    reader = TraceReader(defs, has_ts=not args.no_timestamp)
    with safe_open(args.binary, "rb") as fh:
        raw = fh.read()
    reader.feed(raw)
    tr = reader.tr
    check_fully_decoded(raw, tr, defs, hdr, meta_path)
    if not tr.events:
        sys.exit("error: no events decoded; is this a CTF trace? try --no-timestamp")

    events = export(tr)
    with open(args.output, "w") as f:
        json.dump({"traceEvents": events, "displayTimeUnit": "ns"}, f)

    print(f"wrote {len(events)} trace event(s) and {len(tr.threads)} thread(s) to {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
