#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for scripts/tracing/ctf2perfetto.py.

A CTF trace is written from the tracing subsystem's TSDL metadata, exported,
and the resulting Trace Event Format JSON is checked lane by lane.
"""

import json

import ctf2perfetto
import pytest
import trace_viewer
from trace_builder import TraceBuilder

US = 1000  # one microsecond of CTF timestamp, in nanoseconds

# Kernel handles, as a native_sim trace would carry them.
MAIN = 0x20000100
WORKER = 0x20000200
SEM = 0x20000300
GPIO_PORT = 0x40000000

# Timestamps, in nanoseconds, of the trace built by sample_trace().
MAIN_IN = 1000 * US
ISR_ENTER = 1001 * US
ISR_EXIT = 1002 * US
SEM_TAKE = 1003 * US
SEM_TAKEN = 1004 * US
NAMED = 1005 * US
SYSCALL = 1006 * US
GPIO_SET = 1007 * US
MAIN_OUT = 1008 * US
WORKER_IN = 1009 * US
WORKER_SLEEP = 1010 * US
WORKER_OUT = 1011 * US
OPEN_END = 1012 * US  # last timestamp of the trace built by open_tail_trace().

# Perfetto tids the exporter hands to the first and second thread it sees.
MAIN_TID = ctf2perfetto.FIRST_THREAD_TID
WORKER_TID = ctf2perfetto.FIRST_THREAD_TID + 1


@pytest.fixture
def sample_trace(metadata_file, event_defs):
    """A trace file, next to its metadata, holding a short but realistic run.

    "main" is scheduled in, is interrupted, takes a semaphore, emits a user
    event, makes a syscall and drives a GPIO, then yields to "worker", which
    sleeps: one of every kind of record the exporter routes, in a schedule
    whose shape is easy to state.
    """
    tb = TraceBuilder(event_defs)
    tb.event(MAIN_IN, "thread_switched_in", thread_id=MAIN, name="main")
    tb.event(ISR_ENTER, "isr_enter")
    tb.event(ISR_EXIT, "isr_exit")
    tb.event(SEM_TAKE, "semaphore_take_enter", id=SEM, timeout=0)
    tb.event(SEM_TAKEN, "semaphore_take_exit", id=SEM, timeout=0, ret=0)
    tb.event(NAMED, "named_event", name="app_signal", arg0=7, arg1=8)
    tb.event(SYSCALL, "syscall_enter", id=42, name="k_sem_take")
    tb.event(GPIO_SET, "gpio_port_set_bits_raw_enter", port=GPIO_PORT, pins=3)
    tb.event(MAIN_OUT, "thread_switched_out", thread_id=MAIN, name="main")
    tb.event(WORKER_IN, "thread_switched_in", thread_id=WORKER, name="worker")
    tb.event(WORKER_SLEEP, "k_sleep_enter", timeout=5)
    tb.event(WORKER_OUT, "thread_switched_out", thread_id=WORKER, name="worker")

    path = metadata_file.parent / "channel0_0"
    path.write_bytes(tb.data)
    return path


@pytest.fixture
def events(sample_trace, event_defs):
    return ctf2perfetto.export(trace_viewer.parse_trace(str(sample_trace), event_defs))


@pytest.fixture
def open_tail_events(metadata_file, event_defs):
    """A trace whose last records end mid-run, with no trailing garbage.

    "worker" is switched in and an ISR is entered, and tracing stops before
    either is finished: every record is complete and valid, the thread and the
    ISR are simply still running when the trace ends. Their spans must extend
    to the last timestamp rather than vanish, and a trailing event naming no
    thread must stay on the lane that was running when it was recorded.
    """
    tb = TraceBuilder(event_defs)
    tb.event(MAIN_IN, "thread_switched_in", thread_id=MAIN, name="main")
    tb.event(ISR_ENTER, "isr_enter")
    tb.event(WORKER_IN, "thread_switched_in", thread_id=WORKER, name="worker")
    tb.event(OPEN_END, "k_sleep_enter", timeout=5)

    path = metadata_file.parent / "channel0_0"
    path.write_bytes(tb.data)
    return ctf2perfetto.export(trace_viewer.parse_trace(str(path), event_defs))


def test_every_lane_in_use_is_named(events):
    named = {e["tid"]: e["args"]["name"] for e in events if e["name"] == "thread_name"}
    assert named[MAIN_TID] == "main"
    assert named[WORKER_TID] == "worker"
    assert named[ctf2perfetto.ISR_LANE] == "ISR context"
    assert named[ctf2perfetto.SEMAPHORE_LANE] == "Semaphore"
    assert named[ctf2perfetto.GPIO_LANE] == "GPIO"
    assert ctf2perfetto.SOCKET_LANE not in named
    assert set(named) == {e["tid"] for e in events if e["ph"] != "M"}


def test_the_process_is_named(events):
    process = [e for e in events if e["name"] == "process_name"]
    assert [e["args"]["name"] for e in process] == [ctf2perfetto.PROCESS_NAME]
    assert "tid" not in process[0]


def test_spans_are_balanced(events):
    balance = {}
    for e in events:
        if e["ph"] in ("B", "E"):
            balance[e["tid"]] = balance.get(e["tid"], 0) + (1 if e["ph"] == "B" else -1)
            assert balance[e["tid"]] >= 0, "E event without matching B on same tid"
    assert all(v == 0 for v in balance.values()), "unbalanced B/E pairs"
    assert set(balance) == {MAIN_TID, WORKER_TID, ctf2perfetto.ISR_LANE}


def test_thread_tids_do_not_collide_with_lanes(events):
    tids = {e["tid"] for e in events if e["name"] == "thread_name"}
    threads = {t for t in tids if t >= ctf2perfetto.FIRST_THREAD_TID}
    assert threads.isdisjoint(set(ctf2perfetto.LANE_NAMES))


def test_kernel_objects_reach_their_lane(events):
    by_name = {e["name"]: e for e in events if e["ph"] == "i"}
    assert by_name["semaphore_take_enter"]["tid"] == ctf2perfetto.SEMAPHORE_LANE
    assert by_name["semaphore_take_enter"]["args"]["id"] == SEM
    assert by_name["semaphore_take_exit"]["tid"] == ctf2perfetto.SEMAPHORE_LANE
    assert by_name["gpio_port_set_bits_raw_enter"]["tid"] == ctf2perfetto.GPIO_LANE
    assert by_name["app_signal"]["tid"] == ctf2perfetto.CUSTOM_LANE
    assert by_name["app_signal"]["args"] == {"arg0": 7, "arg1": 8}


def test_enter_and_exit_keep_the_names_the_metadata_gives_them(events):
    instants = {e["name"] for e in events if e["ph"] == "i"}
    assert {"semaphore_take_enter", "semaphore_take_exit"} <= instants


def test_string_payloads_survive(events):
    by_name = {e["name"]: e for e in events if e["ph"] == "i"}
    assert by_name["syscall_enter"]["args"] == {"id": 42, "name": "k_sem_take"}


def test_event_without_a_thread_id_lands_on_the_running_thread(events):
    sleep = next(e for e in events if e["name"] == "k_sleep_enter")
    assert sleep["tid"] == WORKER_TID
    assert sleep["args"] == {"timeout": 5}


def test_span_boundaries_are_not_repeated_as_instants(events):
    instants = {e["name"] for e in events if e["ph"] == "i"}
    assert instants.isdisjoint({"thread_switched_in", "thread_switched_out", "isr_enter"})


def test_records_keep_the_time_they_happened(events):
    by_name = {e["name"]: e for e in events if e["ph"] == "i"}
    assert by_name["semaphore_take_enter"]["ts"] == SEM_TAKE / US
    assert by_name["app_signal"]["ts"] == NAMED / US

    isr = [e for e in events if e.get("tid") == ctf2perfetto.ISR_LANE and e["ph"] in ("B", "E")]
    assert [e["ts"] for e in isr] == [ISR_ENTER / US, ISR_EXIT / US]
    # The ISR lane holds no thread, so its spans say what they are.
    assert {e["name"] for e in isr} == {"isr_active"}

    main = [e for e in events if e.get("tid") == MAIN_TID and e["ph"] in ("B", "E")]
    assert [e["ts"] for e in main] == [MAIN_IN / US, MAIN_OUT / US]


def test_export_is_time_ordered(events):
    timed = [e for e in events if e["ph"] != "M"]
    assert [e["ts"] for e in timed] == sorted(e["ts"] for e in timed)
    assert all("ts" not in e for e in events if e["ph"] == "M")

    # Two records sharing a timestamp on one lane would be a zero length,
    # ambiguous slice, so equal ones are nudged apart, within their window.
    last = {}
    for e in timed:
        assert last.get(e["tid"], -1) < e["ts"], "two records share a timestamp on one lane"
        last[e["tid"]] = e["ts"]
    assert max(e["ts"] for e in timed) <= WORKER_OUT / US + 1


def test_end_of_trace_keeps_the_running_thread(open_tail_events):
    worker = [e for e in open_tail_events if e.get("tid") == WORKER_TID and e["ph"] in ("B", "E")]
    assert [e["ts"] for e in worker] == [WORKER_IN / US, OPEN_END / US]

    isr = [
        e
        for e in open_tail_events
        if e.get("tid") == ctf2perfetto.ISR_LANE and e["ph"] in ("B", "E")
    ]
    assert [e["ts"] for e in isr] == [ISR_ENTER / US, OPEN_END / US]
    assert {e["name"] for e in isr} == {"isr_active"}

    sleep = next(e for e in open_tail_events if e["name"] == "k_sleep_enter")
    assert sleep["tid"] == WORKER_TID


def test_main_writes_valid_json(sample_trace, tmp_path, capsys):
    out = tmp_path / "out.json"
    assert ctf2perfetto.main([str(sample_trace), "-o", str(out)]) == 0
    written = json.loads(out.read_text())
    assert written["displayTimeUnit"] == "ns"
    assert written["traceEvents"]
    assert "wrote" in capsys.readouterr().out


def test_missing_metadata_is_fatal(sample_trace, metadata_file, tmp_path):
    metadata_file.unlink()
    out = tmp_path / "out.json"
    with pytest.raises(SystemExit) as exc:
        ctf2perfetto.main([str(sample_trace), "-o", str(out)])
    assert "no TSDL metadata" in str(exc.value)
    assert not out.exists()


def test_undescribed_event_is_fatal(sample_trace, tmp_path):
    with open(sample_trace, "ab") as fh:
        fh.write(trace_viewer.HDR.pack(2000 * US, 0xBAD))

    out = tmp_path / "out.json"
    with pytest.raises(SystemExit) as exc:
        ctf2perfetto.main([str(sample_trace), "-o", str(out)])
    assert "0xbad" in str(exc.value).lower()
    assert not out.exists()


def test_trace_without_timestamps(metadata_file, event_defs, tmp_path):
    tb = TraceBuilder(event_defs, has_ts=False)
    tb.event(None, "thread_switched_in", thread_id=MAIN, name="main")
    tb.event(None, "semaphore_take_enter", id=SEM, timeout=0)
    tb.event(None, "thread_switched_out", thread_id=MAIN, name="main")
    path = metadata_file.parent / "channel0_0"
    path.write_bytes(tb.data)

    out = tmp_path / "out.json"
    assert ctf2perfetto.main([str(path), "--no-timestamp", "-o", str(out)]) == 0
    timed = [e for e in json.loads(out.read_text())["traceEvents"] if e["ph"] != "M"]
    assert [e["name"] for e in timed] == ["running", "semaphore_take_enter", "running"]
    assert [e["ts"] for e in timed] == sorted(e["ts"] for e in timed)


def test_undescribed_event_without_timestamps_is_fatal(metadata_file, event_defs, tmp_path):
    tb = TraceBuilder(event_defs, has_ts=False)
    tb.event(None, "thread_switched_in", thread_id=MAIN, name="main")
    path = metadata_file.parent / "channel0_0"
    path.write_bytes(tb.data + trace_viewer.HDR_NO_TS.pack(0xBAD))

    out = tmp_path / "out.json"
    with pytest.raises(SystemExit) as exc:
        ctf2perfetto.main([str(path), "--no-timestamp", "-o", str(out)])
    assert "0xbad" in str(exc.value).lower()
    assert not out.exists()


@pytest.mark.parametrize("kept", [6, 11], ids=["header", "payload"])
def test_trailing_partial_record_is_not_fatal(sample_trace, event_defs, tmp_path, kept):
    take = next(e for e in event_defs.values() if e.name == "semaphore_take_enter")
    record = trace_viewer.HDR.pack(2000 * US, take.eid) + b"\x00" * take.size
    with open(sample_trace, "ab") as fh:
        fh.write(record[:kept])

    out = tmp_path / "out.json"
    assert ctf2perfetto.main([str(sample_trace), "-o", str(out)]) == 0
    assert json.loads(out.read_text())["traceEvents"]
