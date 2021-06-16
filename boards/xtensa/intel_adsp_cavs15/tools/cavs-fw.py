#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2021 Intel Corporation. All rights reserved.

import ctypes
import mmap
import os
import struct
import subprocess
import sys
import time

# Intel Audio DSP firmware loader.  No dependencies on anything
# outside this file beyond Python3 builtins.  Assumes the host system
# has a hugetlbs mounted at /dev/hugepages.  Confirmed to run out of
# the box on Ubuntu 18.04 and 20.04.  Run as root with the firmware
# file as the single argument.

FW_FILE = sys.argv[2] if sys.argv[1] == "-f" else sys.argv[1]

PAGESZ = 4096
HUGEPAGESZ = 2 * 1024 * 1024
HUGEPAGE_FILE = "/dev/hugepages/cavs-fw-dma.tmp"

HDA_PPCTL__GPROCEN       = 1 << 30
HDA_SD_CTL__TRAFFIC_PRIO = 1 << 18
HDA_SD_CTL__START        = 1 << 1

def main():
    with open(FW_FILE, "rb") as f:
        fw_bytes = f.read()

    (hda, sd, dsp) = map_regs() # Device register mappings

    # Turn on HDA "global processing enable" first, which actually
    # means "enable access to the ADSP registers in PCI BAR 4" (!)
    hda.PPCTL |= HDA_PPCTL__GPROCEN

    # Turn off the DSP CPUs (each byte of ADSPCS is a bitmask for each
    # of 1-8 DSP cores: lowest byte controls "stall", the second byte
    # engages "reset", the third controls power, and the highest byte
    # is the output state for "powered" to be read after a state
    # change.  Set stall and reset, and turn off power for everything:
    dsp.ADSPCS = 0xffff
    while dsp.ADSPCS & 0xff000000: pass

    # Reset the HDA device
    hda.GCTL = 0
    while hda.GCTL & 1: pass
    hda.GCTL = 1
    while not hda.GCTL & 1: pass

    # Power up (and clear stall and reset on) all the cores on the DSP
    # and wait for CPU0 to show that it has power
    dsp.ADSPCS = 0xff0000
    while (dsp.ADSPCS & 0x1000000) == 0: pass

    # Wait for the ROM to boot and signal it's ready.  This short
    # sleep seems to be needed; if we're banging on the memory window
    # during initial boot (before/while the window control registers
    # are configured?) the DSP hardware will hang fairly reliably.
    time.sleep(0.01)
    while (dsp.SRAM_FW_STATUS >> 24) != 5: pass

    # Send the DSP an IPC message to tell the device how to boot
    # ("PURGE_FW" means "load new code") and which DMA channel to use.
    # The high bit is the "BUSY" signal bit that latches a device
    # interrupt.
    dsp.HIPCI = (  (1 << 31)              # BUSY bit
                 | (0x01 << 24)           # type = PURGE_FW
                 | (1 << 14)              # purge_fw = 1
                 | (hda_ostream_id << 9)) # dma_id

    # Configure our DMA stream to transfer the firmware image
    (buf_list_addr, num_bufs) = setup_dma_mem(fw_bytes)
    sd.BDPU = (buf_list_addr >> 32) & 0xffffffff
    sd.BDPL = buf_list_addr & 0xffffffff
    sd.CBL = len(fw_bytes)
    sd.LVI = num_bufs - 1

    # Enable "processing" on the output stream (send DMA to the DSP
    # and not the audio output hardware)
    hda.PPCTL |= (HDA_PPCTL__GPROCEN | (1 << hda_ostream_id))

    # SPIB ("Software Position In Buffer") a Intel HDA extension that
    # puts a transfer boundary into the stream beyond which the other
    # side will not read.  The ROM wants to poll on a "buffer full"
    # bit on the other side that only works with this enabled.
    hda.SD_SPIB = len(fw_bytes)
    hda.SPBFCTL |= (1 << hda_ostream_id)

    # Uncork the stream
    sd.CTL |= HDA_SD_CTL__START

    # FIXME: The ROM sets a FW_ENTERED value of 5 into the bottom 28
    # bit "state" field of FW_STATUS on entry to the app.  But this is
    # actually ephemeral and racy, because Zephyr is free to write its
    # own data once the app launches and we might miss it.  There's no
    # standard "alive" signaling from the OS, which is really what we
    # want to wait for.  So give it one second and move on.
    for _ in range(100):
        alive = dsp.SRAM_FW_STATUS & ((1 << 28) - 1) == 5
        if alive: break
        time.sleep(0.01)
    if not alive:
        print(f"Load failed?  FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")

    # Turn DMA off and reset the stream.  If this doesn't happen the
    # hardware continues streaming out of our now-stale page and can
    # has been observed to glitch the next boot.
    sd.CTL &= ~HDA_SD_CTL__START
    sd.CTL |= 1

def map_regs():
    # List cribbed from kernel SOF driver.  Not all tested!
    for id in ["119a", "5a98", "1a98", "3198", "9dc8",
               "a348", "34C8", "38c8", "4dc8", "02c8",
               "06c8", "a3f0", "a0c8", "4b55", "4b58"]:
        p = runx(f"grep -il PCI_ID=8086:{id} /sys/bus/pci/devices/*/uevent")
        if p:
            pcidir = os.path.dirname(p)
            break

    hdamem = bar_map(pcidir, 0)

    # Standard HD Audio Registers
    hda = Regs(hdamem)
    hda.GCAP    = 0x0000
    hda.GCTL    = 0x0008
    hda.SPBFCTL = 0x0704
    hda.PPCTL   = 0x0804

    global hda_ostream_id
    hda_ostream_id = (hda.GCAP >> 8) & 0x0f # number of input streams
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
    dsp.HIPCI          = 0x00048
    dsp.SRAM_FW_STATUS = 0x80000 # Start of first SRAM window
    dsp.freeze()

    return (hda, sd, dsp)

def setup_dma_mem(fw_bytes):
    (mem, phys_addr) = map_phys_mem()
    mem[0:len(fw_bytes)] = fw_bytes

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
    return (phys_addr + bdl_off, 2)

global_mmaps = [] # protect mmap mappings from garbage collection!

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

    mem[0] = 0 # Fault the page in so it occupuies real memory!

    # Find the local process address of the mapping, then use that to
    # extract the physical address from the kernel's pagemap
    # interface.
    vaddr = ctypes.addressof(ctypes.c_int.from_buffer(mem))
    vpagenum = vaddr >> 12
    pagemap = open("/proc/self/pagemap", "rb")
    pagemap.seek(vpagenum * 8)
    pent = pagemap.read(8)

    # The PFN in a pagemap entry is the bottom 54 (?!) bits
    paddr = (struct.unpack("Q", pent)[0] & ((1 << 54) - 1)) * PAGESZ
    pagemap.close()

    return (mem, paddr)

# Maps a PCI BAR and returns the in-process address
def bar_map(pcidir, barnum):
    f = open(pcidir.decode() + "/resource" + str(barnum), "r+")
    mm = mmap.mmap(f.fileno(), os.fstat(f.fileno()).st_size)
    global_mmaps.append(mm)
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

if __name__ == "__main__":
    main()
