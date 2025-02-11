#!/usr/bin/env python
# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0
import ctypes
import sys
import mmap
import time
import struct

# MT8195 audio firmware load/debug gadget

# Note that the hardware handling here is only partial: in practice
# the audio DSP depends on clock and power well devices whose drivers
# live elsewhere in the kernel.  Those aren't duplicated here.  Make
# sure the DSP has been started by a working kernel driver first.
#
# See gen_img.py for docs on the image format itself.  The way this
# script works is to map the device memory regions and registers via
# /dev/mem and copy the two segments while resetting the DSP.
#
# In the kernel driver, the address/size values come from devicetree.
# But currently the MediaTek architecture is one kernel driver per SOC
# (i.e. the devicetree values in the kenrel source are tied to the
# specific SOC anyway), so it really doesn't matter and we hard-code
# the addresses for simplicity.
#
# (For future reference: in /proc/device-tree on current ChromeOS
# kernels, the host registers are a "cfg" platform resource on the
# "adsp@10803000" node.  The sram is likewise the "sram" resource on
# that device node, and the two dram areas are "memory-region"
# phandles pointing to "adsp_mem_region" and "adsp_dma_mem_region"
# nodes under "/reserved-memory").

FILE_MAGIC = 0xe463be95

MAPPINGS = { "regs" : (0x10803000,    0xa000),
             "sram" : (0x10840000,   0x40000),
             "dram" : (0x60000000, 0x1000000) }

# Runtime mmap objects for each MAPPINGS entry
maps = {}

def stop(cfg):
    cfg.RESET_SW |= 8 # Set RUNSTALL: halt CPU
    cfg.RESET_SW |= 3 # Set low two bits: "BRESET|DRESET"

def start(cfg, boot_vector):
    stop(cfg)

    cfg.RESET_SW |= 0x10          # Enable "alternate reset" boot vector
    cfg.ALTRESETVEC = boot_vector

    cfg.RESET_SW &= ~3  # Release reset bits
    cfg.RESET_SW &= ~8  # Clear RUNSTALL: go!

# Temporary logging protocol: watch the 1M null-terminated log
# stream at 0x60700000 -- the top of the linkable region of
# existing SOF firmware, before the heap.  Nothing uses this
# currently.  Will be replaced by winstream very soon.
def log():
    msg = b''
    dram = maps["dram"]
    for i in range(0x700000, 0x800000):
        x = dram[i]
        if x == 0:
            sys.stdout.buffer.write(msg)
            sys.stdout.buffer.flush()
            msg = b''
            while x == 0:
                time.sleep(0.1)
                x = dram[i]
        msg += x.to_bytes(1, "little")
    sys.stdout.buffer.write(msg)
    sys.stdout.buffer.flush()

def le4(bstr):
    assert len(bstr) == 4
    return struct.unpack("<I", bstr)[0]

def main():
    # Open device and establish mappings
    devmem_fd = open("/dev/mem", "wb+")
    for mp in MAPPINGS:
        paddr = MAPPINGS[mp][0]
        mapsz = MAPPINGS[mp][1]
        maps[mp] = mmap.mmap(devmem_fd.fileno(), mapsz, offset=paddr,
                             flags=mmap.MAP_SHARED, prot=mmap.PROT_WRITE|mmap.PROT_READ)

    # Create a Regs object for the registers
    cfg = Regs(ctypes.addressof(ctypes.c_int.from_buffer(maps["regs"])))
    cfg.ALTRESETVEC   = 0x0004 # Xtensa boot address
    cfg.RESET_SW      = 0x0024 # Xtensa halt/reset/boot control
    cfg.PDEBUGBUS0    = 0x000c # Unclear, enabled by host, unused by SOF?
    cfg.SRAM_POOL_CON = 0x0930 # SRAM power control: low 4 bits (banks?) enable
    cfg.EMI_MAP_ADDR  = 0x981c # == host SRAM mapping - 0x40000000 (controls MMIO map?)
    cfg.freeze()

    if sys.argv[1] == "load":
        dat = open(sys.argv[2], "rb").read()
        assert le4(dat[0:4])== FILE_MAGIC
        sram_len = le4(dat[4:8])
        boot_vector = le4(dat[8:12])
        sram = dat[12:12+sram_len]
        dram = dat[12 + sram_len:]
        assert len(sram) <= MAPPINGS["sram"][1]
        assert len(dram) <= MAPPINGS["dram"][1]
        for i in range(sram_len):
            maps["sram"][i] = sram[i]
        for i in range(sram_len, MAPPINGS["sram"][1]):
            maps["sram"][i] = 0
        for i in range(len(dram)):
            maps["dram"][i] = dram[i]
        for i in range(len(dram), MAPPINGS["dram"][1]):
            maps["dram"][i] = 0
        start(cfg, boot_vector)

    elif sys.argv[1] == "log":
        log()

    else:
        print(f"Usage: {sys.argv[0]} log | load <file>")

# (Cribbed from cavstool.py)
class Regs:
    def __init__(self, base_addr):
        vars(self)["base_addr"] = base_addr
        vars(self)["ptrs"] = {}
        vars(self)["frozen"] = False
    def freeze(self):
        vars(self)["frozen"] = True
    def __setattr__(self, name, val):
        if not self.frozen and name not in self.ptrs:
            addr = self.base_addr + val
            self.ptrs[name] = ctypes.c_uint32.from_address(addr)
        else:
            self.ptrs[name].value = val
    def __getattr__(self, name):
        return self.ptrs[name].value

if __name__ == "__main__":
    main()
