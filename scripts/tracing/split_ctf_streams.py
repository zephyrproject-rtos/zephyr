#!/usr/bin/env python3
#
# Copyright (c) 2026 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Split a Zephyr CTF capture into one stream file per CPU.

A backend that drives a single link - a UART, USB, or a RAM buffer read out
with a debugger - carries the packets of every CPU interleaved on that one
link. babeltrace2 wants one file per stream, so the packets have to be sorted
by the cpu_id in their packet context before the trace can be read.

Each packet is self describing: the header carries a magic number and the
context carries the packet size, so this needs no knowledge of the events
themselves and does not have to be kept in step with the event metadata.

Usage:

    ./scripts/tracing/split_ctf_streams.py -i channel0_0 -o ctf/
    cp subsys/tracing/ctf/tsdl/metadata ctf/
    babeltrace2 ctf/
"""

import argparse
import pathlib
import struct
import sys

CTF_MAGIC = 0xC1FC1FC1

# struct packet_header { uint32_t magic; uint32_t stream_id; }
HEADER = struct.Struct("<II")
# struct packet_context { uint64 timestamp_begin; uint64 timestamp_end;
#                         uint32 content_size; uint32 packet_size;
#                         uint32 events_discarded; uint8 cpu_id; }
CONTEXT = struct.Struct("<QQIIIB")
PREFIX_SIZE = HEADER.size + CONTEXT.size


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument("-i", "--input", required=True,
                        help="raw CTF capture holding interleaved packets")
    parser.add_argument("-o", "--output", required=True,
                        help="directory to write the per-CPU stream files into")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="report every packet as it is split out")
    return parser.parse_args()


def resync(data, pos):
    """Find the next packet magic at or after pos, or None."""
    want = struct.pack("<I", CTF_MAGIC)
    found = data.find(want, pos)
    return None if found < 0 else found


def main():
    args = parse_args()

    data = pathlib.Path(args.input).read_bytes()
    outdir = pathlib.Path(args.output)
    outdir.mkdir(parents=True, exist_ok=True)

    streams = {}
    pos = 0
    packets = 0
    resyncs = 0
    truncated = 0
    discarded_seen = {}

    while pos + PREFIX_SIZE <= len(data):
        magic, _stream_id = HEADER.unpack_from(data, pos)
        if magic != CTF_MAGIC:
            # A lossy transport can drop bytes; scan forward for the next
            # packet rather than giving up on the whole capture.
            nxt = resync(data, pos + 1)
            if nxt is None:
                break
            resyncs += 1
            pos = nxt
            continue

        (_ts_begin, _ts_end, _content_bits,
         packet_bits, discarded, cpu_id) = CONTEXT.unpack_from(data, pos + HEADER.size)

        packet_size = packet_bits // 8
        if packet_size < PREFIX_SIZE:
            # Not a packet we can trust; treat the magic as a coincidence.
            nxt = resync(data, pos + 1)
            if nxt is None:
                break
            resyncs += 1
            pos = nxt
            continue

        if pos + packet_size > len(data):
            # Capture stopped mid-packet.
            truncated = len(data) - pos
            break

        if cpu_id not in streams:
            streams[cpu_id] = (outdir / f"channel0_{cpu_id}").open("wb")
        streams[cpu_id].write(data[pos:pos + packet_size])

        discarded_seen[cpu_id] = discarded
        packets += 1
        if args.verbose:
            print(f"packet {packets}: cpu {cpu_id}, {packet_size} bytes, "
                  f"{_content_bits // 8} bytes of events")

        pos += packet_size

    for f in streams.values():
        f.close()

    if not streams:
        sys.exit(f"No CTF packets found in {args.input}. Was the capture made "
                 "with a CTF-format tracing build?")

    print(f"{packets} packets -> {len(streams)} stream(s) in {outdir}")
    for cpu_id in sorted(streams):
        print(f"  channel0_{cpu_id}: cpu {cpu_id}, "
              f"{discarded_seen[cpu_id]} event(s) discarded on target")
    if resyncs:
        print(f"WARNING: resynchronized {resyncs} time(s); the capture lost bytes")
    if truncated:
        print(f"WARNING: dropped a trailing partial packet of {truncated} bytes")


if __name__ == "__main__":
    main()
