#!/usr/bin/python3
#
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import time
import mmap

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
#
# The 8k window is treated as an array of 64-byte "slots", each of
# which is prefixed by a magic number, which should be 0x55aa for log
# data, followed a 16 bit "ID" number, followed by a null-terminated
# string in the final 60 bytes (or 60 non-null bytes of log data).
# The DSP firmware will write sequential IDs into the buffer starting
# from an ID of zero in the first slot, and wrapping at the end.

MAP_SIZE = 8192
SLOT_SIZE = 64
NUM_SLOTS = int(MAP_SIZE / SLOT_SIZE)
SLOT_MAGIC = 0x55aa
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
        mem = mmap.mmap(fd.fileno(), MAP_SIZE, offset=LOG_OFFSET,
                    prot=mmap.PROT_READ)
        break

if mem is None:
    sys.stderr.write("ERROR: No ADSP device found.\n")
    sys.exit(1)

# Returns a tuple of (id, msg) if the slot is valid, or (-1, "") if
# the slot does not contain firmware trace data
def read_slot(slot, mem):
    off = slot * SLOT_SIZE

    magic = (mem[off + 1] << 8) | mem[off]
    sid = (mem[off + 3] << 8) | mem[off + 2]

    if magic != SLOT_MAGIC:
        return (-1, "")

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

    return (sid, msg.decode(encoding="utf-8", errors="ignore"))

def read_hist(start_slot):
    id0, msg = read_slot(start_slot, mem)

    # An invalid slot zero means no data has ever been placed in the
    # trace buffer, which is likely a system reset condition.  Back
    # off for one second, because continuing to read the buffer has
    # been observed to hang the flash process (which I think can only
    # be a hardware bug).
    if start_slot == 0 and id0 < 0:
        sys.stdout.write("===\n=== [ADSP Device Reset]\n===\n")
        sys.stdout.flush()
        time.sleep(1)
        return (0, 0, "")

    # Start at zero and read forward to get the last data in the
    # buffer.  We are always guaranteed that slot zero will contain
    # valid data if any slot contains valid data.
    last_id = id0
    final_slot = start_slot
    for i in range(start_slot + 1, NUM_SLOTS):
        id, s = read_slot(i, mem)
        if id != ((last_id + 1) & 0xffff):
            break
        msg += s
        final_slot = i
        last_id = id

    final_id = last_id

    # Now read backwards from the end to get the prefix blocks from
    # the last wraparound
    last_id = id0
    for i in range(NUM_SLOTS - 1, final_slot, -1):
        id, s = read_slot(i, mem)
        if id < 0:
            break

        # Race protection: the other side might have clobbered the
        # data after we read the ID, make sure it hasn't changed.
        id_check = read_slot(i, mem)[0]
        if id_check != id:
            break

        if ((id + 1) & 0xffff) == last_id:
            msg = s + msg
        last_id = id

    # If we were unable to read forward from slot zero, but could read
    # backward, then this is a wrapped buffer being currently updated
    # into slot zero.  See comment below.
    if final_slot == start_slot and last_id != id0:
        return None

    return ((final_slot + 1) % NUM_SLOTS, (final_id + 1) & 0xffff, msg)

# Returns a tuple containing the next slot to expect data in, the ID
# that slot should hold, and the full string history of trace data
# from the buffer.  Start with slot zero (which is always part of the
# current string if there is any data at all) and scan forward and
# back to find the maximum extent.
def trace_history():
    # This loop is a race protection for the situation where the
    # buffer has wrapped and new data is currently being placed into
    # slot zero.  In those circumstances, slot zero will have a valid
    # magic number but its sequence ID will not correlate with the
    # previous and next slots.
    ret = None
    while ret is None:
        ret = read_hist(0)
        if ret is None:
            ret = read_hist(1)
    return ret


# Loop, reading the next slot if it has new data.  Otherwise check the
# full buffer and see if history is discontiguous (i.e. what is in the
# buffer should be a proper suffix of what we have stored).  If it
# doesn't match, then just print it (it's a reboot or a ring buffer
# overrun).  If nothing has changed, then sleep a bit and keep
# polling.
def main():
    next_slot, next_id, last_hist = trace_history()

    # We only have one command line argument, to suppress the history
    # dump at the start (so CI runs don't see e.g. a previous device
    # state containing logs from another test, and get confused)
    if len(sys.argv) < 2 or sys.argv[1] != "--no-history":
        sys.stdout.write(last_hist)

    while True:
        id, smsg = read_slot(next_slot, mem)

        if id == next_id:
            next_slot = int((next_slot + 1) % NUM_SLOTS)
            next_id = (id + 1) & 0xffff
            last_hist += smsg
            sys.stdout.write(smsg)
        else:
            slot2, id2, msg2 = trace_history()

            # Device reset:
            if slot2 == 0 and id2 == 0 and msg2 == "":
                next_id = 1
                next_slot = slot2
                last_hist = ""

            if not last_hist.endswith(msg2):
                # On a mismatch, go back and check one last time to
                # address the race where a new slot wasn't present
                # just JUST THEN but is NOW.
                id3, s3 = read_slot(next_slot, mem)
                if id3 == next_id:
                    next_slot = int((next_slot + 1) % NUM_SLOTS)
                    next_id = (next_id + 1) & 0xffff
                    last_hist += s3
                    sys.stdout.write(s3)
                    continue

                # Otherwise it represents discontiguous data, either a
                # reset of an overrun, just dump what we have and
                # start over.
                next_slot = slot2
                last_hist = msg2
                sys.stdout.write(msg2)
            else:
                sys.stdout.flush()
                time.sleep(0.10)

if __name__ == "__main__":
    main()
