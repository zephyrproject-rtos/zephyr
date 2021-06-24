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
import logging

# Intel Audio DSP firmware loader.  No dependencies on anything
# outside this file beyond Python3 builtins.  Pass a signed rimage
# file as the single argument.

logging.basicConfig()
log = logging.getLogger("cavs-fw")
log.setLevel(logging.INFO)

FW_FILE = sys.argv[1]

PAGESZ = 4096
HUGEPAGESZ = 2 * 1024 * 1024
HUGEPAGE_FILE = "/dev/hugepages/cavs-fw-dma.tmp"

HDA_PPCTL__GPROCEN       = 1 << 30
HDA_SD_CTL__TRAFFIC_PRIO = 1 << 18
HDA_SD_CTL__START        = 1 << 1

def main():
    if os.system("lsmod | grep -q snd_sof_pci") == 0:
        log.warning("The Linux snd-sof-pci kernel module is loaded.  While this")
        log.warning("    loader will normally work in such circumstances, things")
        log.warning("    will get confused if the system tries to touch the hardware")
        log.warning("    simultaneously.  Operation is most reliable if it is")
        log.warning("    unloaded first.")

    # Make sure hugetlbfs is mounted (not there on chromeos)
    os.system("mount | grep -q hugetlbfs ||"
              + " (mkdir -p /dev/hugepages; "
              + "  mount -t hugetlbfs hugetlbfs /dev/hugepages)")

    with open(FW_FILE, "rb") as f:
        fw_bytes = f.read()

    (magic, sz) = struct.unpack("4sI", fw_bytes[0:8])
    if magic == b'XMan':
        log.info(f"Trimming {sz} bytes of extended manifest")
        fw_bytes = fw_bytes[sz:len(fw_bytes)]

    (hda, sd, dsp) = map_regs() # Device register mappings

    # Reset the HDA device
    log.info("Reset HDA device")
    hda.GCTL = 0
    while hda.GCTL & 1: pass
    hda.GCTL = 1
    while not hda.GCTL & 1: pass

    # Turn on HDA "global processing enable" first.  As documented,
    # this enables the audio DSP (vs. hardware HDA emulation).  But it
    # actually means "enable access to the ADSP registers in PCI BAR 4" (!)
    log.info("Enable HDA global processing")
    hda.PPCTL |= HDA_PPCTL__GPROCEN

    # Turn off the DSP CPUs (each byte of ADSPCS is a bitmask for each
    # of 1-8 DSP cores: lowest byte controls "stall", the second byte
    # engages "reset", the third controls power, and the highest byte
    # is the output state for "powered" to be read after a state
    # change.  Set stall and reset, and turn off power for everything:
    log.info(f"Powering down, ADSPCS = 0x{dsp.ADSPCS:x}")
    dsp.ADSPCS = 0xffff
    while dsp.ADSPCS & 0xff000000: pass
    log.info(f"Powered down, ADSPCS = 0x{dsp.ADSPCS:x}")

    # Configure our DMA stream to transfer the firmware image
    log.info(f"Configuring DMA output stream {hda_ostream_id}...")
    (buf_list_addr, num_bufs) = setup_dma_mem(fw_bytes)

    # Reset stream
    sd.CTL = 1
    while (sd.CTL & 1) == 0: pass
    sd.CTL = 0
    while (sd.CTL & 1) == 1: pass

    sd.CTL = (1 << 20) # Set stream ID to anything non-zero
    sd.BDPU = (buf_list_addr >> 32) & 0xffffffff
    sd.BDPL = buf_list_addr & 0xffffffff
    sd.CBL = len(fw_bytes)
    sd.LVI = num_bufs - 1

    # Enable "processing" on the output stream (send DMA to the DSP
    # and not the audio output hardware)
    hda.PPCTL |= (HDA_PPCTL__GPROCEN | (1 << hda_ostream_id))

    # SPIB ("Software Position In Buffer") is an Intel HDA extension
    # that puts a transfer boundary into the stream beyond which the
    # other side will not read.  The ROM wants to poll on a "buffer
    # full" bit on the other side that only works with this enabled.
    hda.SPBFCTL |= (1 << hda_ostream_id)
    hda.SD_SPIB = len(fw_bytes)

    # Power up all the cores on the DSP and wait for CPU0 to show that
    # it has power.  Leave stall and reset high for now
    log.info(f"Powering up DSP core #0, ADSPCS = 0x{dsp.ADSPCS:x}")
    dsp.ADSPCS = 0x01ffff
    while (dsp.ADSPCS & 0x01000000) == 0: pass
    log.info(f"Powered up {ncores(dsp)} cores, ADSPCS = 0x{dsp.ADSPCS:x}")

    # Send the DSP an IPC message to tell the device how to boot
    # ("PURGE_FW" means "load new code") and which DMA channel to use.
    # The high bit is the "BUSY" signal bit that latches a device
    # interrupt.
    #
    # Note: with cAVS 1.8+ the ROM receives the stream argument as an index
    # within the array of output streams (and we always use the first
    # one by construction).  But with 1.5 it's the HDA index, and
    # depends on the number of input streams on the device.
    stream_idx = hda_ostream_id if cavs15 else 0
    ipcval = (  (1 << 31)              # BUSY bit
              | (0x01 << 24)           # type = PURGE_FW
              | (1 << 14)              # purge_fw = 1
              | (stream_idx << 9)) # dma_id
    log.info(f"Sending PURGW_FW IPC, HIPCR = 0x{ipcval:x}")
    dsp.HIPCI = ipcval

    # Now start CPU #0 by dropping stall and reset
    log.info(f"Starting {ncores(dsp)} cores, ADSPCS = 0x{dsp.ADSPCS:x}")
    dsp.ADSPCS = 0x01fffe # Out of reset
    time.sleep(0.1)
    dsp.ADSPCS = 0x01fefe # Un-stall
    log.info(f"Started {ncores(dsp)} cores, ADSPCS = 0x{dsp.ADSPCS:x}")

    # Experimentation shows that these steps aren't actually required,
    # the ROM just charges ahead and initializes itself correctly even
    # if we don't wait for it.  Do them anyway for better visibility,
    # when requested.  Potentially remove later once this code is
    # mature.
    if log.level <= logging.INFO:
        # Wait for the ROM to boot and signal it's ready.  NOTE: This
        # short sleep seems to be needed; if we're banging on the
        # memory window during initial boot (before/while the window
        # control registers are configured?) the DSP hardware will
        # hang fairly reliably.
        time.sleep(0.1)
        log.info(f"Waiting for ROM init, FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")
        while (dsp.SRAM_FW_STATUS >> 24) != 5: pass
        log.info(f"ROM ready, FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")

        # Newer devices have an ACK bit we can check
        if not cavs15:
            log.info(f"Awaiting IPC acknowledgment, HIPCA 0x{dsp.HIPCA:x}")
            while not dsp.HIPCA & (1 << 31): pass
            dsp.HIPCA |= ~(1 << 31)

        # Wait for it to signal ROM_INIT_DONE
        log.info(f"Awaiting ROM init... FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")
        while (dsp.SRAM_FW_STATUS & 0x00ffffff) != 1: pass

    # It's ready, uncork the stream
    log.info(f"Starting DMA, FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")
    sd.CTL |= HDA_SD_CTL__START

    # The ROM sets a FW_ENTERED value of 5 into the bottom 28 bit
    # "state" field of FW_STATUS on entry to the app.  (Pedantry: this
    # is actually ephemeral and racy, because Zephyr is free to write
    # its own data once the app launches and we might miss it.
    # There's no standard "alive" signaling from the OS, which is
    # really what we want to wait for.  So give it one second and move
    # on).
    log.info(f"Waiting for load, FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")
    for _ in range(100):
        alive = dsp.SRAM_FW_STATUS & ((1 << 28) - 1) == 5
        if alive: break
        time.sleep(0.01)
    if alive:
        log.info("ROM reports firmware was entered")
    else:
        log.warning(f"Load failed?  FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")

    # Turn DMA off and reset the stream.  If this doesn't happen the
    # hardware continues streaming out of our now-stale page and has
    # been observed to glitch the next boot.
    sd.CTL = 1

    time.sleep(1)

    log.info(f"ADSPCS = 0x{dsp.ADSPCS:x}")
    log.info(f"Load complete, {ncores(dsp)} cores active")

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
    global cavs15
    cavs15 = id in [ "5a98", "1a98", "3198" ]
    log.info(f"Detected cAVS {'1.5' if cavs15 else '1.8+'} hardware")

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
    global hda_ostream_id
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

    return (hda, sd, dsp)

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
