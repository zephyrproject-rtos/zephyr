#!/usr/bin/env python
# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0
import ctypes
import sys
import mmap
import time
import struct
from glob import glob
import re

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

# Runtime mmap objects for each MAPPINGS entry
maps = {}

# Returns a string (e.g. "mt8195", "mt8186", "mt8188") if a supported
# adsp is detected, or None if not
def detect():
    compat = readfile(glob("/proc/device-tree/**/adsp@*/compatible",
                           recursive=True)[0], "r")
    m = re.match(r'.*(mt\d{4})-dsp', compat)
    if m:
        return m.group(1)

# Parse devicetree to find the MMIO mappings: there is an "adsp" node
# (in various locations) with an array of named "reg" mappings.  It
# also refers by reference to reserved-memory regions of system
# DRAM.  Those don't have names, call them dram0/1 (dram1 is the main
# region to which code is linked, dram0 is presumably a dma pool but
# unused by current firmware).  Returns a dict mapping name to a
# (addr, size) tuple.
def mappings():
    path = glob("/proc/device-tree/**/adsp@*/", recursive=True)[0]
    rnames = readfile(path + "reg-names", "r").split('\0')[:-1]
    regs = struct.unpack(f">{2 * len(rnames)}Q", readfile(path + "reg"))
    maps = { n : (regs[2*i], regs[2*i+1]) for i, n in enumerate(rnames) }
    for i, ph in enumerate(struct.unpack(">II", readfile(path + "memory-region"))):
        for rmem in glob("/proc/device-tree/reserved-memory/*/"):
            if struct.unpack(">I", readfile(rmem + "phandle"))[0] == ph:
                (addr, sz) = struct.unpack(">QQ", readfile(rmem + "reg"))
                maps[f"dram{i}"] = (addr, sz)
                break
    return maps

# Register API for 8195
class MT8195():
    def __init__(self, maps):
        # Create a Regs object for the registers
        r = Regs(ctypes.addressof(ctypes.c_int.from_buffer(maps["cfg"])))
        r.ALTRESETVEC   = 0x0004 # Xtensa boot address
        r.RESET_SW      = 0x0024 # Xtensa halt/reset/boot control
        r.PDEBUGBUS0    = 0x000c # Unclear, enabled by host, unused by SOF?
        r.SRAM_POOL_CON = 0x0930 # SRAM power control: low 4 bits (banks?) enable
        r.EMI_MAP_ADDR  = 0x981c # == host SRAM mapping - 0x40000000 (controls MMIO map?)
        r.freeze()
        self.cfg = r

    def stop(self):
        self.cfg.RESET_SW |= 8 # Set RUNSTALL: halt CPU
        self.cfg.RESET_SW |= 3 # Set low two bits: "BRESET|DRESET"

    def start(self, boot_vector):
        self.stop()
        self.cfg.RESET_SW |= 0x10          # Enable "alternate reset" boot vector
        self.cfg.ALTRESETVEC = boot_vector
        self.cfg.RESET_SW &= ~3            # Release reset bits
        self.cfg.RESET_SW &= ~8            # Clear RUNSTALL: go!

# Register API for 8186/8188
class MT818x():
    def __init__(self, maps):
        # These have registers spread across two blocks
        cfg_base = ctypes.addressof(ctypes.c_int.from_buffer(maps["cfg"]))
        sec_base = ctypes.addressof(ctypes.c_int.from_buffer(maps["sec"]))
        self.cfg = Regs(cfg_base)
        self.cfg.SW_RSTN = 0x00
        self.cfg.IO_CONFIG = 0x0c
        self.cfg.freeze()
        self.sec = Regs(sec_base)
        self.sec.ALTVEC_C0 = 0x04
        self.sec.ALTVECSEL = 0x0c
        self.sec.freeze()

    def stop(self):
        self.cfg.IO_CONFIG |= (1<<31) # Set RUNSTALL to stop core
        time.sleep(0.1)
        self.cfg.SW_RSTN |= 0x11    # Assert reset: SW_RSTN_C0|SW_DBG_RSTN_C0

    # Note: 8186 and 8188 use different bits in ALTVECSEC, but
    # it's safe to write both to enable the alternate boot vector
    def start(self, boot_vector):
        self.cfg.IO_CONFIG |= (1<<31)    # Set RUNSTALL
        self.sec.ALTVEC_C0 = boot_vector
        self.sec.ALTVECSEL = 0x03        # Enable alternate vector
        self.cfg.SW_RSTN |= 0x00000011   # Assert reset
        self.cfg.SW_RSTN &= 0xffffffee   # Release reset
        self.cfg.IO_CONFIG &= 0x7fffffff # Clear RUNSTALL

# Temporary logging protocol: watch the 1M null-terminated log
# stream at 0x60700000 -- the top of the linkable region of
# existing SOF firmware, before the heap.  Nothing uses this
# currently.  Will be replaced by winstream very soon.
def log():
    msg = b''
    dram = maps["dram1"]
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

def readfile(f, mode="rb"):
    return open(f, mode).read()

def le4(bstr):
    assert len(bstr) == 4
    return struct.unpack("<I", bstr)[0]

def main():
    dsp = detect()
    assert dsp

    # Probe devicetree for mappable memory regions
    mmio = mappings()

    # Open device and establish mappings
    devmem_fd = open("/dev/mem", "wb+")
    for mp in mmio:
        paddr = mmio[mp][0]
        mapsz = mmio[mp][1]
        mapsz = int((mapsz + 4095) / 4096) * 4096
        maps[mp] = mmap.mmap(devmem_fd.fileno(), mapsz, offset=paddr,
                             flags=mmap.MAP_SHARED, prot=mmap.PROT_WRITE|mmap.PROT_READ)

    if dsp == "mt8195":
        dev = MT8195(maps)
    elif dsp in ("mt8186", "mt8188"):
        dev = MT818x(maps)

    if sys.argv[1] == "load":
        dat = open(sys.argv[2], "rb").read()
        assert le4(dat[0:4])== FILE_MAGIC
        sram_len = le4(dat[4:8])
        boot_vector = le4(dat[8:12])
        sram = dat[12:12+sram_len]
        dram = dat[12 + sram_len:]
        assert len(sram) <= mmio["sram"][1]
        assert len(dram) <= mmio["dram1"][1]

        # Stop the device and write the regions.  Note that we don't
        # zero-fill SRAM, as that's been observed to reboot the host
        # (!!) on mt8186 when the writes near the end of the 512k
        # region.
        # pylint: disable=consider-using-enumerate
        for i in range(sram_len):
            maps["sram"][i] = sram[i]
        #for i in range(sram_len, mmio["sram"][1]):
        #    maps["sram"][i] = 0
        for i in range(len(dram)):
            maps["dram1"][i] = dram[i]
        for i in range(len(dram), mmio["dram1"][1]):
            maps["dram1"][i] = 0
        dev.start(boot_vector)
        log()

    elif sys.argv[1] == "log":
        log()

    elif sys.argv[1] == "dump":
        sz = mmio[sys.argv[2]][1]
        mm = maps[sys.argv[2]]
        sys.stdout.buffer.write(mm[0:sz])

    else:
        print(f"Usage: {sys.argv[0]} log | load <file>")

if __name__ == "__main__":
    main()
