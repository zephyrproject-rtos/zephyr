#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2021 Intel Corporation. All rights reserved.

import ctypes
import mmap
import os
import struct
import subprocess
import time
import logging

logging.basicConfig()
log = logging.getLogger("cavs-fw")
log.setLevel(logging.INFO)

global_mmaps = [] # protect mmap mappings from garbage collection!

PAGESZ = 4096
HUGEPAGESZ = 2 * 1024 * 1024
HUGEPAGE_FILE = "/dev/hugepages/cavs-fw-dma.tmp"

# Count of active/running cores
def ncores(dsp):
    return bin(dsp.ADSPCS >> 24).count("1")

def map_regs():
    # List cribbed from kernel SOF driver.  Not all tested!
    for id in ["119a", "5a98", "1a98", "3198", "9dc8",
               "a348", "34C8", "38c8", "4dc8", "02c8",
               "06c8", "a3f0", "a0c8", "4b55", "4b58"]:
        p = runx(f"grep -il PCI_ID=8086:{id} /sys/bus/pci/devices/*/uevent")
        if p:
            pcidir = os.path.dirname(p)
            break

    # Detect hardware version, this matters in a few spots
    cavs15 = id in [ "5a98", "1a98", "3198" ]

    # Disengage runtime power management so the kernel doesn't put it to sleep
    with open(pcidir + b"/power/control", "w") as ctrl:
        ctrl.write("on")

    # Make sure PCI memory space access and busmastering are enabled.
    # Also disable interrupts so as not to confuse the kernel.
    with open(pcidir + b"/config", "wb+") as cfg:
        cfg.seek(4)
        cfg.write(b'\x06\x04')

    time.sleep(0.1)

    hdamem = bar_map(pcidir, 0)

    # Standard HD Audio Registers
    hda = Regs(hdamem)
    hda.GCAP    = 0x0000
    hda.GCTL    = 0x0008
    hda.SPBFCTL = 0x0704
    hda.PPCTL   = 0x0804

    # Find the ID of the first output stream
    hda_ostream_id = (hda.GCAP >> 8) & 0x0f # number of input streams
    log.info(f"Selected output stream {hda_ostream_id} (GCAP = 0x{hda.GCAP:x})")
    hda.SD_SPIB = 0x0708 + (8 * hda_ostream_id)

    hda.freeze()

    # Standard HD Audio Stream Descriptor
    sd = Regs(hdamem + 0x0080 + (hda_ostream_id * 0x20))
    sd.CTL  = 0x00
    sd.LPIB = 0x04
    sd.CBL  = 0x08
    sd.LVI  = 0x0c
    sd.FMT  = 0x12
    sd.BDPL = 0x18
    sd.BDPU = 0x1c
    sd.freeze()

    # Intel Audio DSP Registers
    dsp = Regs(bar_map(pcidir, 4))
    dsp.ADSPCS         = 0x00004
    if cavs15:
        dsp.HIPCI      = 0x00048 # original name of the register...
    else:
        dsp.HIPCI      = 0x000d0 # ...now named "HIPCR" per 1.8+ docs
        dsp.HIPCA      = 0x000d4
    dsp.SRAM_FW_STATUS = 0x80000 # Start of first SRAM window
    dsp.freeze()

    return (hda, sd, dsp, hda_ostream_id, cavs15)

def setup_dma_mem(fw_bytes):
    (mem, phys_addr) = map_phys_mem()
    mem[0:len(fw_bytes)] = fw_bytes

    log.info("Mapped 2M huge page at 0x%x to contain %d bytes of firmware"
          % (phys_addr, len(fw_bytes)))

    # HDA requires at least two buffers be defined, but we don't care
    # about boundaries because it's all a contiguous region. Place a
    # vestigial 128-byte (minimum size and alignment) buffer after the
    # main one, and put the 4-entry BDL list into the final 128 bytes
    # of the page.
    buf0_len = HUGEPAGESZ - 2 * 128
    buf1_len = 128
    bdl_off = buf0_len + buf1_len
    mem[bdl_off:bdl_off + 32] = struct.pack("<QQQQ",
                                            phys_addr, buf0_len,
                                            phys_addr + buf0_len, buf1_len)
    log.info("Filled the buffer descriptor list (BDL) for DMA.")
    return (phys_addr + bdl_off, 2)

# Maps 2M of contiguous memory using a single page from hugetlbfs,
# then locates its physical address for use as a DMA buffer.
def map_phys_mem():
    # Ensure the kernel has enough budget for one new page
    free = int(runx("awk '/HugePages_Free/ {print $2}' /proc/meminfo"))
    if free == 0:
        tot = 1 + int(runx("awk '/HugePages_Total/ {print $2}' /proc/meminfo"))
        os.system(f"echo {tot} > /proc/sys/vm/nr_hugepages")

    hugef = open(HUGEPAGE_FILE, "w+")
    hugef.truncate(HUGEPAGESZ)
    mem = mmap.mmap(hugef.fileno(), HUGEPAGESZ)
    global_mmaps.append(mem)
    os.unlink(HUGEPAGE_FILE)

    # Find the local process address of the mapping, then use that to
    # extract the physical address from the kernel's pagemap
    # interface.  The physical page frame number occupies the bottom
    # bits of the entry.
    mem[0] = 0 # Fault the page in so it has an address!
    vaddr = ctypes.addressof(ctypes.c_int.from_buffer(mem))
    vpagenum = vaddr >> 12
    pagemap = open("/proc/self/pagemap", "rb")
    pagemap.seek(vpagenum * 8)
    pent = pagemap.read(8)
    paddr = (struct.unpack("Q", pent)[0] & ((1 << 55) - 1)) * PAGESZ
    pagemap.close()
    log.info("Obtained the physical address of the mapped huge page.")

    return (mem, paddr)

# Maps a PCI BAR and returns the in-process address
def bar_map(pcidir, barnum):
    f = open(pcidir.decode() + "/resource" + str(barnum), "r+")
    mm = mmap.mmap(f.fileno(), os.fstat(f.fileno()).st_size)
    global_mmaps.append(mm)
    log.info("Mapped PCI bar %d of length %d bytes." % (barnum, os.fstat(f.fileno()).st_size))
    return ctypes.addressof(ctypes.c_int.from_buffer(mm))

# Syntactic sugar to make register block definition & use look nice.
# Instantiate from a base address, assign offsets to (uint32) named
# registers as fields, call freeze(), then the field acts as a direct
# alias for the register!
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

def runx(cmd):
    return subprocess.Popen(["sh", "-c", cmd],
                            stdout=subprocess.PIPE).stdout.read()
