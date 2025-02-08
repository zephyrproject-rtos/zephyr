#!/usr/bin/env python
# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0
import ctypes
import mmap
import os
import re
import struct
import sys
import time
from glob import glob

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

FILE_MAGIC = 0xE463BE95

# Runtime mmap objects for each MAPPINGS entry
maps = {}


# Returns a string (e.g. "mt8195", "mt8186", "mt8188") if a supported
# adsp is detected, or None if not
def detect():
    compat = readfile(glob("/proc/device-tree/**/adsp@*/compatible", recursive=True)[0], "r")
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
    maps = {n: (regs[2 * i], regs[2 * i + 1]) for i, n in enumerate(rnames)}
    for i, ph in enumerate(struct.unpack(">II", readfile(path + "memory-region"))):
        for rmem in glob("/proc/device-tree/reserved-memory/*/"):
            phf = rmem + "phandle"
            if os.path.exists(phf) and struct.unpack(">I", readfile(phf))[0] == ph:
                (addr, sz) = struct.unpack(">QQ", readfile(rmem + "reg"))
                maps[f"dram{i}"] = (addr, sz)
                break
    return maps


# Register API for 8195
class MT8195:
    def __init__(self, maps):
        # Create a Regs object for the registers
        r = Regs(ctypes.addressof(ctypes.c_int.from_buffer(maps["cfg"])))
        r.ALTRESETVEC = 0x0004  # Xtensa boot address
        r.RESET_SW = 0x0024  # Xtensa halt/reset/boot control
        r.PDEBUGBUS0 = 0x000C  # Unclear, enabled by host, unused by SOF?
        r.SRAM_POOL_CON = 0x0930  # SRAM power control: low 4 bits (banks?) enable
        r.EMI_MAP_ADDR = 0x981C  # == host SRAM mapping - 0x40000000 (controls MMIO map?)
        r.freeze()
        self.cfg = r

    def logrange(self):
        return range(0x700000, 0x800000)

    def stop(self):
        self.cfg.RESET_SW |= 8  # Set RUNSTALL: halt CPU
        self.cfg.RESET_SW |= 3  # Set low two bits: "BRESET|DRESET"

    def start(self, boot_vector):
        self.stop()
        self.cfg.RESET_SW |= 0x10  # Enable "alternate reset" boot vector
        self.cfg.ALTRESETVEC = boot_vector
        self.cfg.RESET_SW &= ~3  # Release reset bits
        self.cfg.RESET_SW &= ~8  # Clear RUNSTALL: go!


# Register API for 8186/8188
class MT818x:
    def __init__(self, maps):
        # These have registers spread across two blocks
        cfg_base = ctypes.addressof(ctypes.c_int.from_buffer(maps["cfg"]))
        sec_base = ctypes.addressof(ctypes.c_int.from_buffer(maps["sec"]))
        self.cfg = Regs(cfg_base)
        self.cfg.SW_RSTN = 0x00
        self.cfg.IO_CONFIG = 0x0C
        self.cfg.freeze()
        self.sec = Regs(sec_base)
        self.sec.ALTVEC_C0 = 0x04
        self.sec.ALTVECSEL = 0x0C
        self.sec.freeze()

    def logrange(self):
        return range(0x700000, 0x800000)

    def stop(self):
        self.cfg.IO_CONFIG |= 1 << 31  # Set RUNSTALL to stop core
        time.sleep(0.1)
        self.cfg.SW_RSTN |= 0x11  # Assert reset: SW_RSTN_C0|SW_DBG_RSTN_C0

    # Note: 8186 and 8188 use different bits in ALTVECSEC, but
    # it's safe to write both to enable the alternate boot vector
    def start(self, boot_vector):
        self.cfg.IO_CONFIG |= 1 << 31  # Set RUNSTALL
        self.sec.ALTVEC_C0 = boot_vector
        self.sec.ALTVECSEL = 0x03  # Enable alternate vector
        self.cfg.SW_RSTN |= 0x00000011  # Assert reset
        self.cfg.SW_RSTN &= 0xFFFFFFEE  # Release reset
        self.cfg.IO_CONFIG &= 0x7FFFFFFF  # Clear RUNSTALL


class MT8196:
    def __init__(self, maps):
        cfg_base = ctypes.addressof(ctypes.c_int.from_buffer(maps["cfg"]))
        sec_base = ctypes.addressof(ctypes.c_int.from_buffer(maps["sec"]))
        self.cfg = Regs(cfg_base)
        self.cfg.CFGREG_SW_RSTN = 0x0000
        self.cfg.MBOX_IRQ_EN = 0x009C
        self.cfg.HIFI_RUNSTALL = 0x0108
        self.cfg.freeze()
        self.sec = Regs(sec_base)
        self.sec.ALTVEC_C0 = 0x04
        self.sec.ALTVECSEL = 0x0C
        self.sec.freeze()

    def logrange(self):
        return range(0x580000, 0x600000)

    def stop(self):
        self.cfg.HIFI_RUNSTALL |= 0x1000
        self.cfg.CFGREG_SW_RSTN |= 0x11

    def start(self, boot_vector):
        self.sec.ALTVEC_C0 = 0
        self.sec.ALTVECSEL = 0
        self.sec.ALTVEC_C0 = boot_vector
        self.sec.ALTVECSEL = 1
        self.cfg.HIFI_RUNSTALL |= 0x1000
        self.cfg.MBOX_IRQ_EN |= 3
        self.cfg.CFGREG_SW_RSTN |= 0x11
        time.sleep(0.1)
        self.cfg.CFGREG_SW_RSTN &= ~0x11
        self.cfg.HIFI_RUNSTALL &= ~0x1000


# Temporary logging protocol: watch the 1M null-terminated log
# stream at 0x60700000 -- the top of the linkable region of
# existing SOF firmware, before the heap.  Nothing uses this
# currently.  Will be replaced by winstream very soon.
def old_log(dev):
    msg = b''
    dram = maps["dram1"]
    for i in dev.logrange():
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


# Wrapper class for winstream logging.  Instantiate with a single
# integer argument representing a local/in-process address for the
# shared winstream memory.  The memory mapped access is encapsulated
# with a Regs object for the fields and a ctypes array for the data
# area.  The lockless algorithm in read() matches the C version in
# upstream Zephyr, don't modify in isolation.  Note that on some
# platforms word access to the data array (by e.g. copying a slice
# into a bytes object or by calling memmove) produces bus errors
# (plausibly an alignment requirement on the fabric with the DSP
# memory, where arm64 python is happy doing unaligned loads?).  Access
# to the data bytes is done bytewise for safety.
class Winstream:
    def __init__(self, addr):
        r = Regs(addr)
        r.WLEN = 0x00
        r.START = 0x04
        r.END = 0x08
        r.SEQ = 0x0C
        r.freeze()
        # Sanity-check, the 32M size limit isn't a rule, but seems reasonable
        if r.WLEN > 0x2000000 or (r.START >= r.WLEN) or (r.END >= r.WLEN):
            raise RuntimeError("Invalid winstream")
        self.regs = r
        self.data = (ctypes.c_char * r.WLEN).from_address(addr + 16)
        self.msg = bytearray(r.WLEN)
        self.seq = 0

    def read(self):
        ws, msg, data = self.regs, self.msg, self.data
        last_seq = self.seq
        wlen = ws.WLEN
        while True:
            start, end, seq = ws.START, ws.END, ws.SEQ
            self.seq = seq
            if seq == last_seq or start == end:
                return ""
            behind = seq - last_seq
            if behind > ((end - start) % wlen):
                return ""
            copy = (end - behind) % wlen
            suffix = min(behind, wlen - copy)
            for i in range(suffix):
                msg[i] = data[copy + i][0]
            msglen = suffix
            l2 = behind - suffix
            if l2 > 0:
                for i in range(l2):
                    msg[msglen + i] = data[i][0]
                msglen += l2
            if start == ws.START and seq == ws.SEQ:
                return msg[0:msglen].decode("utf-8", "replace")


# Locates a winstream descriptor in the firmware via its 96-bit magic
# number and returns the address and size fields it finds there.
def find_winstream(maps):
    magic = b'\x74\x5f\x6a\xd0\x79\xe2\x4f\x00\xcd\xb8\xbd\xf9'
    for m in maps:
        if "dram" in m:
            # Some python versions produce bus errors (!) on the
            # hardware when finding a 12 byte substring (maybe a SIMD
            # load that the hardware doesn't like?).  Do it in two
            # chunks.
            magoff = maps[m].find(magic[0:8])
            if magoff >= 0:
                magoff = maps[m].find(magic[8:], magoff) - 8
            if magoff >= 0:
                addr = le4(maps[m][magoff + 12 : magoff + 16])
                return addr
    raise RuntimeError("Cannot find winstream descriptor in firmware runtime")


def winstream_localaddr(globaddr, mmio, maps):
    for m in mmio:
        off = globaddr - mmio[m][0]
        if 0 <= off < mmio[m][1]:
            return ctypes.addressof(ctypes.c_int.from_buffer(maps[m])) + off
    raise RuntimeError("Winstream address not inside DSP memory")


def winstream_log(mmio, maps):
    physaddr = find_winstream(maps)
    regsbase = winstream_localaddr(physaddr, mmio, maps)
    ws = Winstream(regsbase)
    while True:
        msg = ws.read()
        if msg:
            sys.stdout.write(msg)
            sys.stdout.flush()
        else:
            time.sleep(0.1)


def main():
    dsp = detect()
    assert dsp

    # Probe devicetree for mappable memory regions
    mmio = mappings()

    # Open device and establish mappings
    with open("/dev/mem", "wb+") as devmem_fd:
        for mp in mmio:
            paddr = mmio[mp][0]
            mapsz = mmio[mp][1]
            mapsz = int((mapsz + 4095) / 4096) * 4096
            maps[mp] = mmap.mmap(
                devmem_fd.fileno(),
                mapsz,
                offset=paddr,
                flags=mmap.MAP_SHARED,
                prot=mmap.PROT_WRITE | mmap.PROT_READ,
            )

    if dsp == "mt8195":
        dev = MT8195(maps)
    elif dsp in ("mt8186", "mt8188"):
        dev = MT818x(maps)
    elif dsp == "mt8196":
        dev = MT8196(maps)

    if sys.argv[1] == "load":
        dat = None
        with open(sys.argv[2], "rb") as f:
            dat = f.read()
        assert le4(dat[0:4]) == FILE_MAGIC
        sram_len = le4(dat[4:8])
        boot_vector = le4(dat[8:12])
        sram = dat[12 : 12 + sram_len]
        dram = dat[12 + sram_len :]
        assert len(sram) <= mmio["sram"][1]
        assert len(dram) <= mmio["dram1"][1]

        # Stop the device and write the regions.  Note that we don't
        # zero-fill SRAM, as that's been observed to reboot the host
        # (!!) on mt8186 when the writes near the end of the 512k
        # region.
        # pylint: disable=consider-using-enumerate
        for i in range(sram_len):
            maps["sram"][i] = sram[i]
        # for i in range(sram_len, mmio["sram"][1]):
        #    maps["sram"][i] = 0
        for i in range(len(dram)):
            maps["dram1"][i] = dram[i]
        for i in range(len(dram), mmio["dram1"][1]):
            maps["dram1"][i] = 0
        dev.start(boot_vector)
        winstream_log(mmio, maps)

    elif sys.argv[1] == "log":
        winstream_log(mmio, maps)

    elif sys.argv[1] == "oldlog":
        old_log(dev)

    elif sys.argv[1] == "mem":
        print("Memory Regions:")
        for m in mmio:
            print(f"  {m}: {mmio[m][1]} @ 0x{mmio[m][0]:08x}")

    elif sys.argv[1] == "dump":
        sz = mmio[sys.argv[2]][1]
        mm = maps[sys.argv[2]]
        sys.stdout.buffer.write(mm[0:sz])

    else:
        print(f"Usage: {sys.argv[0]} log | load <file>")


if __name__ == "__main__":
    main()
