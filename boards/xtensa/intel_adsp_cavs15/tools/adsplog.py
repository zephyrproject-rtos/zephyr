#!/usr/bin/python3
# Copyright (c) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import time
import mmap
import struct

# Log reader for the trace output buffer on a ADSP device.
#
# When run with no arguments, it will detect the device, dump the
# contents of the trace buffer and continue to poll for more output.
# The "--no-history" argument can be passed to suppress emission of the
# history, and emit only new output.  This can be useful for test
# integration where the user does not want to see any previous runs in
# the log output.
#
# The trace buffer is inside a shared memory region exposed by the
# audio PCI device as a BAR at index 4.  The hardware provides 4 128k
# "windows" starting at 512kb in the BAR which the DSP firmware can
# map to 4k-aligned locations within its own address space.  By
# protocol convention log output is an 8k region at window index 3.

MAP_SIZE = 8192
WIN_OFFSET = 0x80000
WIN_IDX = 3
WIN_SIZE = 0x20000
LOG_OFFSET = WIN_OFFSET + WIN_IDX * WIN_SIZE

mem = None
sys_devices = "/sys/bus/pci/devices"

for dev_addr in os.listdir(sys_devices):
    class_file = sys_devices + "/" + dev_addr + "/class"
    pciclass = open(class_file).read()

    vendor_file = sys_devices + "/" + dev_addr + "/vendor"
    pcivendor = open(vendor_file).read()

    if not "0x8086" in pcivendor:
        continue

    # Intel Multimedia audio controller
    #   0x040100 -> DSP is present
    #   0x040380 -> DSP is present but optional
    if "0x040100" in pciclass or "0x040380" in pciclass:
        barfile = sys_devices + "/" + dev_addr + "/resource4"

        fd = open(barfile)
        try:
            mem = mmap.mmap(fd.fileno(), MAP_SIZE, offset=LOG_OFFSET,
                            prot=mmap.PROT_READ)
        except OSError as ose:
            sys.stderr.write("""\
mmap failed! If CONFIG IO_STRICT_DEVMEM is set then you must unload the kernel driver.
""")
            raise ose
        break

if mem is None:
    sys.stderr.write("ERROR: No ADSP device found.\n")
    sys.exit(1)

# This SHOULD be just "mem[start:start+length]", but slicing an mmap
# array seems to be unreliable on one of my machines (python 3.6.9 on
# Ubuntu 18.04).  Read out bytes individually.
def read_mem(start, length):
    return b''.join(mem[x].to_bytes(1, 'little') for x in range(start, start + length))

def read_hdr():
    return struct.unpack("<IIII", read_mem(0, 16))

# Python implementation of the same algorithm in sys_winstream_read(),
# see there for details.
def winstream_read(last_seq):
    while True:
        (wlen, start, end, seq) = read_hdr()
        if seq == last_seq or start == end:
            return (seq, "")
        behind = seq - last_seq
        if behind > ((end - start) % wlen):
            return (seq, "")
        copy = (end - behind) % wlen
        suffix = min(behind, wlen - copy)
        result = read_mem(16 + copy, suffix)
        if suffix < behind:
            result += read_mem(16, behind - suffix)
        (wlen, start1, end, seq1) = read_hdr()
        if start1 == start and seq1 == seq:
            return (seq, result.decode("utf-8"))

# Choose our last_seq based on whether to dump the pre-existing buffer
(wlen, start, end, seq) = read_hdr()
last_seq = seq
if len(sys.argv) < 2 or sys.argv[1] != "--no-history":
    last_seq -= (end - start) % wlen

while True:
    time.sleep(0.1)
    (last_seq, output) = winstream_read(last_seq)
    if output:
        sys.stdout.write(output)
