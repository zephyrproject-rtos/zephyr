#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

"""Convert a circular CTF RAM dump into a chronological CTF trace.

The target stores fixed-size CTF packets in a circular RAM buffer.  A raw
memory dump is consequently in physical slot order, not time order.  This
tool validates each packet, orders valid packets by packet sequence number,
and emits a conventional CTF trace directory suitable for babeltrace2.
"""

import argparse
import shutil
import struct
import sys
from pathlib import Path


PACKET_MAGIC = 0xC1FC1FC1
# magic, timestamp_begin, timestamp_end, content_size, packet_size, sequence
PACKET_HEADER = struct.Struct("<IQQIIQ")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dump", type=Path, help="raw ram_tracing_circular memory dump")
    parser.add_argument("output", type=Path, help="output CTF trace directory")
    size_group = parser.add_mutually_exclusive_group(required=True)
    size_group.add_argument("--slot-size", type=int,
                            help="size in bytes of one CTF packet slot")
    size_group.add_argument("--packet-count", type=int,
                            help="number of equal-size packet slots in the dump")
    parser.add_argument("--metadata", type=Path,
                        default=Path(__file__).resolve().parents[2] /
                        "subsys/tracing/ctf/tsdl/metadata_circular",
                        help="CTF metadata to copy (default: metadata_circular)")
    parser.add_argument("--stream-name", default="channel0",
                        help="name for the generated CTF stream file")
    parser.add_argument("--force", action="store_true",
                        help="replace metadata and stream files if they exist")
    return parser.parse_args()


def packet_from_slot(slot, index, slot_size):
    magic, begin, end, content_bits, packet_bits, sequence = PACKET_HEADER.unpack_from(slot)
    if magic != PACKET_MAGIC:
        return None
    if packet_bits != slot_size * 8:
        return None
    if not PACKET_HEADER.size * 8 < content_bits <= packet_bits:
        return None
    if begin > end:
        return None
    return sequence, begin, end, index, slot


def main():
    args = parse_args()
    dump = args.dump.read_bytes()
    if len(dump) < PACKET_HEADER.size:
        sys.exit("dump is smaller than a CTF packet header")

    if args.packet_count is not None:
        if args.packet_count <= 0 or len(dump) % args.packet_count:
            sys.exit("dump size must be divisible by --packet-count")
        slot_size = len(dump) // args.packet_count
    else:
        slot_size = args.slot_size
        if slot_size < PACKET_HEADER.size or len(dump) % slot_size:
            sys.exit("--slot-size must fit a header and divide the dump size")

    packets = []
    for index, offset in enumerate(range(0, len(dump), slot_size)):
        packet = packet_from_slot(dump[offset:offset + slot_size], index, slot_size)
        if packet is not None:
            packets.append(packet)

    if not packets:
        sys.exit("no completed CTF packets found")
    if len({packet[0] for packet in packets}) != len(packets):
        sys.exit("duplicate packet sequence numbers found; dump is inconsistent")

    packets.sort(key=lambda packet: (packet[0], packet[1]))
    args.output.mkdir(parents=True, exist_ok=True)
    metadata_output = args.output / "metadata"
    stream_output = args.output / args.stream_name
    if not args.force and (metadata_output.exists() or stream_output.exists()):
        sys.exit("output metadata or stream already exists (use --force to replace it)")

    shutil.copyfile(args.metadata, metadata_output)
    with stream_output.open("wb") as stream:
        for _, _, _, _, packet in packets:
            stream.write(packet)

    first, last = packets[0], packets[-1]
    print(f"wrote {len(packets)} packets from slots {first[3]}..{last[3]} "
          f"(sequence {first[0]}..{last[0]}) to {args.output}")


if __name__ == "__main__":
    main()
