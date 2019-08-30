#!/usr/bin/python3

"""Logging reader tool"""

import argparse
import os
import sys

QEMU_ETRACE = "/dev/shm/qemu-bridge-hp-sram-mem"
SOF_ETRACE = "/sys/kernel/debug/sof/etrace"

QEMU_OFFSET = 0x8000
SOF_OFFSET = 0x0

MAGIC = 0x55aa
SLOT_LEN = 64
SLOT_NUM = int(8192 / SLOT_LEN)

def read_id(f):
    """Get id"""

    buf = f.read(2)
    return int.from_bytes(buf, byteorder='little')

def read_magic(f):
    """Get magic"""

    buf = f.read(2)
    return int.from_bytes(buf, byteorder='little')

def read_log_slot(f):
    """Read log slots"""

    magic = read_magic(f)

    if magic == MAGIC:
        id = read_id(f)
        slot = f.read(SLOT_LEN - 4)
        logstr = slot.decode(errors='replace').split('\r', 1)[0]
        print("id %d %s" % (id, logstr))

def read_logs(etrace, offset):
    """Read logs"""

    f = open(etrace, "rb")

    for x in range(0, SLOT_NUM):
        f.seek(offset + x * SLOT_LEN)
        read_log_slot(f)

def parse_args():
    """Parsing command line arguments"""

    parser = argparse.ArgumentParser(description='Logging tool')

    parser.add_argument('-e', '--etrace', choices=['sof', 'qemu'], default="sof")

    args = parser.parse_args()

    return args

def main():
    """Main"""

    args = parse_args()

    if os.geteuid() != 0:
        sys.exit("Please run this program as root / sudo")

    if args.etrace == 'sof':
        etrace = SOF_ETRACE
        offset = SOF_OFFSET
    else:
        etrace = QEMU_ETRACE
        offset = QEMU_OFFSET

    read_logs(etrace, offset)

if __name__ == "__main__":

    main()
