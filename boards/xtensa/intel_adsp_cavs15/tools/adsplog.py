#!/usr/bin/python3
#
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import sys
import time
import struct
import subprocess
import mmap

MAP_SIZE = 8192
SLOT_SIZE = 64

# Location of the log output window within the DSP BAR on the PCI
# device.  The hardware provides 4x 128k "windows" starting at 512kb
# in the BAR which the DSP software can map to 4k-aligned locations
# within its own address space.  By convention log output is an 8k
# region at window index 3.
WIN_OFFSET = 0x80000
WIN_ID = 3
WIN_SIZE = 0x20000
LOG_OFFSET = WIN_OFFSET + WIN_ID * WIN_SIZE

# Find me a way to do this detection as cleanly in python as shell, I
# dare you.
barfile = subprocess.Popen(["sh", "-c",
                            "echo -n "
                            "$(dirname "
                            "  $(fgrep PCI_ID=8086:5A98 "
                            "    /sys/bus/pci/devices/*/uevent))"
                            "/resource4"],
                           stdout=subprocess.PIPE).stdout.read()
fd = open(barfile)
mem = mmap.mmap(fd.fileno(), MAP_SIZE, offset=LOG_OFFSET,
                prot=mmap.PROT_READ)

# The mapping is an array of 64-byte "slots", each of which is
# prefixed by a magic number, which should be 0x55aa for log data,
# followed a 16 bit "ID" number, followed by a null-terminated string
# in the final 60 bytes.  The DSP firmware will write sequential IDs
# into the buffer starting from an ID of zero in the first slot, and
# wrapping at the end.  So the algorithm here is to find the smallest
# valid slot, print its data, and then enter a polling loop waiting
# for the next slot to be valid and have the correct next ID before
# printing that too.

# NOTE: unfortunately there's no easy way to detect a warm reset of
# the device, it will just jump back to the beginning and start
# writing there, where we aren't looking.  Really that level of
# robustness needs to be handled in the kernel.

next_slot = 0
next_id = 0xffff

for slot in range(int(MAP_SIZE / SLOT_SIZE)):
    off = slot * SLOT_SIZE
    (magic, sid) = struct.unpack("HH", mem[off:off+4])
    if magic == 0x55aa:
        if sid < next_id:
            next_slot = slot
            next_id = sid

while True:
    off = next_slot * SLOT_SIZE
    (magic, sid) = struct.unpack("HH", mem[off:off+4])
    if magic == 0x55aa and sid == next_id:
        # This dance because indexing large variable-length slices of
        # the mmap() array seems to produce garbage....
        msgbytes = []
        for i in range(4, SLOT_SIZE):
            b = mem[off+i]
            if b == 0:
                break
            msgbytes.append(b)
        msg = bytearray(len(msgbytes))
        for i, elem in enumerate(msgbytes):
            msg[i] = elem

        sys.stdout.write(msg.decode(encoding="utf-8", errors="ignore"))
        next_slot = int((next_slot + 1) % (MAP_SIZE / SLOT_SIZE))
        next_id += 1
    else:
        sys.stdout.flush()
        time.sleep(0.25)
