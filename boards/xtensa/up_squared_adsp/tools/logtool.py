#!/usr/bin/python3

#FILE = "/dev/shm/qemu-bridge-hp-sram-mem"
FILE = "/sys/kernel/debug/sof/etrace"
#OFFSET = 0x8000
OFFSET = 0x0
MAGIC = 0x55aa
SLOT_LEN = 64
SLOT_NUM = int(8192 / SLOT_LEN)

def read_id(f):
    buf = f.read(2)
    return int.from_bytes(buf, byteorder='little')

def read_magic(f):
    buf = f.read(2)
    return int.from_bytes(buf, byteorder='little')

def read_log_slot(f):
    magic = read_magic(f)

    if magic == MAGIC:
        id = read_id(f)
        slot = f.read(SLOT_LEN - 4)
        logstr = slot.decode(errors='replace').split('\r', 1)[0]
        print("id %d %s" % (id, logstr))

# Open a file
f = open(FILE, "rb")

for x in range(0, SLOT_NUM):
    f.seek(OFFSET + x * SLOT_LEN)
    read_log_slot(f)
