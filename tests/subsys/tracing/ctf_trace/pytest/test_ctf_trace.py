# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Decode the CTF stream emitted by the ctf_trace application and assert that the
expected events are present with sane fields.

"""

import glob
import logging
import os
import re
import sys
import time

logger = logging.getLogger(__name__)

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "tracing"))
import trace_viewer as tv  # noqa: E402

METADATA = os.path.join(ZEPHYR_BASE, "subsys", "tracing", "ctf", "tsdl", "metadata")

# One representative event per object type the app exercises.
EXPECTED_EVENTS = [
    "thread_create",
    "semaphore_give_enter",
    "semaphore_take_exit",
    "mutex_lock_enter",
    "queue_init",
    "queue_append_enter",
    "queue_get_exit",
    "fifo_init_enter",
    "fifo_put_enter",
    "lifo_put_enter",
    "stack_push_enter",
    "heap_alloc_enter",
    "heap_alloc_exit",
    "zbus_chan_pub_enter",
    "zbus_chan_pub_exit",
    "zbus_obs_notify_enter",
    "zbus_obs_notify_exit",
    "zbus_sub_wait_exit",
    "zbus_sub_wait_msg_exit",
    "zbus_async_listener_enter",
    "zbus_async_listener_exit",
    "zbus_chan_claim_exit",
    "zbus_chan_read_exit",
    "zbus_chan_finish_exit",
]

ZBUS_IDS_RE = re.compile(
    r"ZBUS chan=(?P<chan>[0-9a-f]+) lis=(?P<lis>[0-9a-f]+) sub=(?P<sub>[0-9a-f]+) "
    r"msub=(?P<msub>[0-9a-f]+) alis=(?P<alis>[0-9a-f]+) busy=(?P<busy>-?[0-9]+)"
)


def _find_trace(build_dir):
    candidates = [os.path.join(str(build_dir), "channel0_0")]
    candidates += glob.glob(os.path.join(str(build_dir), "**", "channel0_0"), recursive=True)
    for c in candidates:
        if os.path.isfile(c) and os.path.getsize(c) > 0:
            return c
    return None


def test_ctf_trace(dut):
    # Wait for the app to finish emitting its tracepoints.
    lines = dut.readlines_until(regex=".*CTF TRACE DONE", timeout=30)
    zbus_ids = next((m for m in map(ZBUS_IDS_RE.search, lines) if m), None)
    assert zbus_ids, "zbus addresses not printed"

    build_dir = dut.device_config.app_build_dir or dut.device_config.build_dir

    trace = None
    for _ in range(25):
        trace = _find_trace(build_dir)
        if trace:
            break
        time.sleep(0.2)
    assert trace, f"CTF trace file (channel0_0) not found under {build_dir}"
    logger.info("decoding CTF trace %s (%d bytes)", trace, os.path.getsize(trace))

    assert os.path.isfile(METADATA), f"CTF metadata not found at {METADATA}"
    defs = tv.parse_metadata(METADATA)
    tr = tv.parse_trace(trace, defs, has_ts=True)

    names = [e.name for e in tr.events]
    seen = set(names)
    logger.info("decoded %d CTF events, %d distinct types", len(tr.events), len(seen))
    assert len(tr.events) > 0, "no CTF events decoded"

    missing = [e for e in EXPECTED_EVENTS if e not in seen]
    assert not missing, f"missing expected CTF events {missing}; decoded types: {sorted(seen)}"

    assert "queue_get_blocking" not in names, "K_NO_WAIT queue get emitted a blocking event"

    # Field sanity: queue_get_exit must carry the object id, timeout and return value.
    get_exit = next(e for e in tr.events if e.name == "queue_get_exit")
    for field in ("id", "timeout", "ret"):
        assert field in get_exit.fields, f"queue_get_exit missing field {field}: {get_exit.fields}"

    # Ordering sanity: the queue is initialised before it is read from.
    idx = {n: names.index(n) for n in ("queue_init", "queue_get_exit")}
    assert idx["queue_init"] < idx["queue_get_exit"], "queue_init must precede queue_get_exit"

    _check_zbus(tr.events, zbus_ids)

    logger.info("CTF trace validated: all %d expected event types present", len(EXPECTED_EVENTS))


def _check_zbus(events, ids):
    """Check the decoded zbus event fields against the values the app printed."""
    chan = int(ids["chan"], 16)
    obs = {name: int(ids[name], 16) for name in ("lis", "sub", "msub", "alis")}

    def first(name):
        return next(e.fields for e in events if e.name == name)

    pub_enter = first("zbus_chan_pub_enter")
    assert pub_enter["id"] == chan, f"zbus_chan_pub_enter: {pub_enter}"
    # K_MSEC(10), recorded in microseconds after rounding up to whole ticks.
    assert 10000 <= pub_enter["timeout"] < 11000, f"zbus_chan_pub_enter: {pub_enter}"
    pub_exit = first("zbus_chan_pub_exit")
    assert pub_exit["id"] == chan and pub_exit["ret"] == 0, f"zbus_chan_pub_exit: {pub_exit}"

    # Observers are notified in ZBUS_OBSERVERS() order.
    notified = [e.fields for e in events if e.name == "zbus_obs_notify_exit"]
    expected = [obs["lis"], obs["sub"], obs["msub"], obs["alis"]]
    assert [f["id"] for f in notified] == expected, f"zbus_obs_notify_exit: {notified}"
    for f in notified:
        assert f["chan"] == chan and f["ret"] == 0, f"zbus_obs_notify_exit: {f}"

    for name, sub in (("zbus_sub_wait_exit", "sub"), ("zbus_sub_wait_msg_exit", "msub")):
        f = first(name)
        assert (f["id"], f["chan"], f["ret"]) == (obs[sub], chan, 0), f"{name}: {f}"

    for name in ("zbus_async_listener_enter", "zbus_async_listener_exit"):
        f = first(name)
        assert (f["id"], f["chan"]) == (obs["alis"], chan), f"{name}: {f}"

    # A read of a claimed channel fails: checks the signed return value.
    read_exit = first("zbus_chan_read_exit")
    assert (read_exit["id"], read_exit["ret"]) == (chan, int(ids["busy"])), (
        f"zbus_chan_read_exit: {read_exit}"
    )
    assert int(ids["busy"]) < 0, f"zbus_chan_read of a claimed channel returned {ids['busy']}"
