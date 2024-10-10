#!/usr/bin/env python3
# Copyright(c) 2022 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
import os
import sys
import struct
import logging
import asyncio
import time
import subprocess
import ctypes
import mmap
import argparse
import pty

start_output = True

logging.basicConfig(level=logging.INFO)
log = logging.getLogger("cavs-fw")

PAGESZ = 4096
HUGEPAGESZ = 2 * 1024 * 1024
HUGEPAGE_FILE = "/dev/hugepages/cavs-fw-dma.tmp."

# SRAM windows. Base and stride varies depending on ADSP version
#
# Window 0 is the FW_STATUS area, and 4k after that the IPC "outbox"
# Window 1 is the IPC "inbox" (host-writable memory, just 384 bytes currently)
# Window 2 is used for debug slots (Zephyr shell is one user)
# Window 3 is winstream-formatted log output

WINDOW_BASE = 0x80000
WINDOW_STRIDE = 0x20000

WINDOW_BASE_ACE = 0x180000
WINDOW_STRIDE_ACE = 0x8000

DEBUG_SLOT_SIZE = 4096
DEBUG_SLOT_SHELL = 0
SHELL_RX_SIZE = 256
SHELL_MAX_VALID_SLOT_SIZE = 16777216

# pylint: disable=duplicate-code

# ADSPCS bits
CRST   = 0
CSTALL = 8
SPA    = 16
CPA    = 24

class HDAStream:
    # creates an hda stream with at 2 buffers of buf_len
    def __init__(self, stream_id: int):
        self.stream_id = stream_id
        self.base = hdamem + 0x0080 + (stream_id * 0x20)
        log.info(f"Mapping registers for hda stream {self.stream_id} at base {self.base:x}")

        self.hda = Regs(hdamem)
        self.hda.GCAP    = 0x0000
        self.hda.GCTL    = 0x0008
        self.hda.DPLBASE = 0x0070
        self.hda.DPUBASE = 0x0074
        self.hda.SPBFCH  = 0x0700
        self.hda.SPBFCTL = 0x0704
        self.hda.PPCH    = 0x0800
        self.hda.PPCTL   = 0x0804
        self.hda.PPSTS   = 0x0808
        self.hda.SPIB = 0x0708 + stream_id*0x08
        self.hda.freeze()

        self.regs = Regs(self.base)
        self.regs.CTL  = 0x00
        self.regs.STS  = 0x03
        self.regs.LPIB = 0x04
        self.regs.CBL  = 0x08
        self.regs.LVI  = 0x0c
        self.regs.FIFOW = 0x0e
        self.regs.FIFOS = 0x10
        self.regs.FMT = 0x12
        self.regs.FIFOL= 0x14
        self.regs.BDPL = 0x18
        self.regs.BDPU = 0x1c
        self.regs.freeze()

        self.dbg0 = Regs(hdamem + 0x0084 + (0x20*stream_id))
        self.dbg0.DPIB = 0x00
        self.dbg0.EFIFOS = 0x10
        self.dbg0.freeze()

        self.reset()

    def __del__(self):
        self.reset()

    def config(self, buf_len: int):
        log.info(f"Configuring stream {self.stream_id}")
        self.buf_len = buf_len
        log.info("Allocating huge page and setting up buffers")
        self.mem, self.hugef, self.buf_list_addr, self.pos_buf_addr, self.n_bufs = self.setup_buf(buf_len)

        log.info("Setting buffer list, length, and stream id and traffic priority bit")
        self.regs.CTL = ((self.stream_id & 0xFF) << 20) | (1 << 18) # must be set to something other than 0?
        self.regs.BDPU = (self.buf_list_addr >> 32) & 0xffffffff
        self.regs.BDPL = self.buf_list_addr & 0xffffffff
        self.regs.CBL = buf_len
        self.regs.LVI = self.n_bufs - 1
        self.mem.seek(0)
        self.debug()
        log.info(f"Configured stream {self.stream_id}")

    def write(self, data):

        bufl = min(len(data), self.buf_len)
        log.info(f"Writing data to stream {self.stream_id}, len {bufl}, SPBFCTL {self.hda.SPBFCTL:x}, SPIB {self.hda.SPIB}")
        self.mem[0:bufl] = data[0:bufl]
        self.mem[bufl:bufl+bufl] = data[0:bufl]
        self.hda.SPBFCTL |= (1 << self.stream_id)
        self.hda.SPIB += bufl
        log.info(f"Wrote data to stream {self.stream_id}, SPBFCTL {self.hda.SPBFCTL:x}, SPIB {self.hda.SPIB}")

    def start(self):
        log.info(f"Starting stream {self.stream_id}, CTL {self.regs.CTL:x}")
        self.regs.CTL |= 2
        log.info(f"Started stream {self.stream_id}, CTL {self.regs.CTL:x}")

    def stop(self):
        log.info(f"Stopping stream {self.stream_id}, CTL {self.regs.CTL:x}")
        self.regs.CTL &= 2
        time.sleep(0.1)
        self.regs.CTL |= 1
        log.info(f"Stopped stream {self.stream_id}, CTL {self.regs.CTL:x}")

    def setup_buf(self, buf_len: int):
        (mem, phys_addr, hugef) = map_phys_mem(self.stream_id)

        log.info(f"Mapped 2M huge page at 0x{phys_addr:x} for buf size ({buf_len})")

        # create two buffers in the page of buf_len and mark them
        # in a buffer descriptor list for the hardware to use
        buf0_len = buf_len
        buf1_len = buf_len
        bdl_off = buf0_len + buf1_len
        # bdl is 2 (64bits, 16 bytes) per entry, we have two
        mem[bdl_off:bdl_off + 32] = struct.pack("<QQQQ",
                                                phys_addr,
                                                buf0_len,
                                                phys_addr + buf0_len,
                                                buf1_len)
        dpib_off = bdl_off+32

        # ensure buffer is initialized, sanity
        for i in range(0, buf_len*2):
            mem[i] = 0

        log.info("Filled the buffer descriptor list (BDL) for DMA.")
        return (mem, hugef, phys_addr + bdl_off, phys_addr+dpib_off, 2)

    def debug(self):
        log.debug("HDA %d: PPROC %d, CTL 0x%x, LPIB 0x%x, BDPU 0x%x, BDPL 0x%x, CBL 0x%x, LVI 0x%x",
                 self.stream_id, (hda.PPCTL >> self.stream_id) & 1, self.regs.CTL, self.regs.LPIB, self.regs.BDPU,
                 self.regs.BDPL, self.regs.CBL, self.regs.LVI)
        log.debug("    FIFOW %d, FIFOS %d, FMT %x, FIFOL %d, DPIB %d, EFIFOS %d",
                 self.regs.FIFOW & 0x7, self.regs.FIFOS, self.regs.FMT, self.regs.FIFOL, self.dbg0.DPIB, self.dbg0.EFIFOS)
        log.debug("    status: FIFORDY %d, DESE %d, FIFOE %d, BCIS %d",
                 (self.regs.STS >> 5) & 1, (self.regs.STS >> 4) & 1, (self.regs.STS >> 3) & 1, (self.regs.STS >> 2) & 1)

    def reset(self):
        # Turn DMA off and reset the stream.  Clearing START first is a
        # noop per the spec, but absolutely required for stability.
        # Apparently the reset doesn't stop the stream, and the next load
        # starts before it's ready and kills the load (and often the DSP).
        # The sleep too is required, on at least one board (a fast
        # chromebook) putting the two writes next each other also hangs
        # the DSP!
        log.info(f"Resetting stream {self.stream_id}")
        self.debug()
        self.regs.CTL &= ~2 # clear START
        time.sleep(0.1)
        # set enter reset bit
        self.regs.CTL = 1
        while (self.regs.CTL & 1) == 0: pass
        # clear enter reset bit to exit reset
        self.regs.CTL = 0
        while (self.regs.CTL & 1) == 1: pass

        log.info(f"Disable SPIB and set position 0 of stream {self.stream_id}")
        self.hda.SPBFCTL = 0
        self.hda.SPIB = 0

        #log.info("Setting dma position buffer and enable it")
        #self.hda.DPUBASE = self.pos_buf_addr >> 32 & 0xffffffff
        #self.hda.DPLBASE = self.pos_buf_addr & 0xfffffff0 | 1

        log.info(f"Enabling dsp capture (PROCEN) of stream {self.stream_id}")
        self.hda.PPCTL |= (1 << self.stream_id)

        self.debug()
        log.info(f"Reset stream {self.stream_id}")

def adsp_is_ace():
    return ace15 or ace20 or ace30

def adsp_mem_window_config():
    if adsp_is_ace():
        base = WINDOW_BASE_ACE
        stride = WINDOW_STRIDE_ACE
    else:
        base = WINDOW_BASE
        stride = WINDOW_STRIDE

    return (base, stride)

def map_regs(log_only):
    p = runx(f"grep -iEl 'PCI_CLASS=40(10|38)0' /sys/bus/pci/devices/*/uevent")
    pcidir = os.path.dirname(p)

    # Platform/quirk detection.  ID lists cribbed from the SOF kernel driver
    global cavs25, ace15, ace20, ace30
    did = int(open(f"{pcidir}/device").read().rstrip(), 16)
    cavs25 = did in [ 0xa0c8, 0x43c8, 0x4b55, 0x4b58, 0x7ad0, 0x51c8 ]
    ace15 = did in [ 0x7e28 ]
    ace20 = did in [ 0xa828 ]
    ace30 = did in [ 0xe428 ]

    # Check sysfs for a loaded driver and remove it
    if os.path.exists(f"{pcidir}/driver"):
        mod = os.path.basename(os.readlink(f"{pcidir}/driver/module"))
        found_msg = f"Existing driver \"{mod}\" found"
        if log_only:
            log.info(found_msg)
        else:
            log.warning(found_msg + ", unloading module")
            runx(f"rmmod -f {mod}")
            # Disengage runtime power management so the kernel doesn't put it to sleep
            log.info(f"Forcing {pcidir}/power/control to always 'on'")
            with open(f"{pcidir}/power/control", "w") as ctrl:
                ctrl.write("on")

    # Make sure PCI memory space access and busmastering are enabled.
    # Also disable interrupts so as not to confuse the kernel.
    with open(f"{pcidir}/config", "wb+") as cfg:
        cfg.seek(4)
        cfg.write(b'\x06\x04')

    # Standard HD Audio Registers
    global hdamem
    (hdamem, _) = bar_map(pcidir, 0)
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
    sd.CBL  = 0x08
    sd.LVI  = 0x0c
    sd.BDPL = 0x18
    sd.BDPU = 0x1c
    sd.freeze()

    # Intel Audio DSP Registers
    global bar4_mmap
    global bar4_mem
    (bar4_mem, bar4_mmap) = bar_map(pcidir, 4)
    dsp = Regs(bar4_mem)
    if adsp_is_ace():
        dsp.HFDSSCS        = 0x1000
        dsp.HFPWRCTL       = 0x1d18 if ace15 or ace20 else 0x1d20
        dsp.HFPWRSTS       = 0x1d1c if ace15 or ace20 else 0x1d24
        dsp.DSP2CXCTL_PRIMARY = 0x178d04
        dsp.HFIPCXTDR      = 0x73200
        dsp.HFIPCXTDA      = 0x73204
        dsp.HFIPCXIDR      = 0x73210
        dsp.HFIPCXIDA      = 0x73214
        dsp.HFIPCXCTL      = 0x73228
        dsp.HFIPCXTDDY     = 0x73300
        dsp.HFIPCXIDDY     = 0x73380
        dsp.ROM_STATUS     = 0x163200 if ace15 else 0x160200
        dsp.SRAM_FW_STATUS = WINDOW_BASE_ACE
    else:
        dsp.ADSPCS         = 0x00004
        dsp.HIPCTDR        = 0x000c0
        dsp.HIPCTDA        = 0x000c4 # 1.8+ only
        dsp.HIPCTDD        = 0x000c8
        dsp.HIPCIDR        = 0x000d0
        dsp.HIPCIDA        = 0x000d4 # 1.8+ only
        dsp.HIPCIDD        = 0x000d8
        dsp.ROM_STATUS     = WINDOW_BASE # Start of first SRAM window
        dsp.SRAM_FW_STATUS = WINDOW_BASE
    dsp.freeze()

    return (hda, sd, dsp, hda_ostream_id)

def setup_dma_mem(fw_bytes):
    (mem, phys_addr, _) = map_phys_mem(hda_ostream_id)
    mem[0:len(fw_bytes)] = fw_bytes

    log.info("Mapped 2M huge page at 0x%x to contain %d bytes of firmware"
          % (phys_addr, len(fw_bytes)))

    # HDA requires at least two buffers be defined, but we don't care about
    # boundaries because it's all a contiguous region. Place a vestigial
    # 128-byte (minimum size and alignment) buffer after the main one, and put
    # the 4-entry BDL list into the final 128 bytes of the page.
    buf0_len = HUGEPAGESZ - 2 * 128
    buf1_len = 128
    bdl_off = buf0_len + buf1_len
    mem[bdl_off:bdl_off + 32] = struct.pack("<QQQQ",
                                            phys_addr, buf0_len,
                                            phys_addr + buf0_len, buf1_len)
    log.info("Filled the buffer descriptor list (BDL) for DMA.")
    return (phys_addr + bdl_off, 2)

global_mmaps = [] # protect mmap mappings from garbage collection!

# Maps 2M of contiguous memory using a single page from hugetlbfs,
# then locates its physical address for use as a DMA buffer.
def map_phys_mem(stream_id):
    # Make sure hugetlbfs is mounted (not there on chromeos)
    os.system("mount | grep -q hugetlbfs ||"
              + " (mkdir -p /dev/hugepages; "
              + "  mount -t hugetlbfs hugetlbfs /dev/hugepages)")

    # Ensure the kernel has enough budget for one new page
    free = int(runx("awk '/HugePages_Free/ {print $2}' /proc/meminfo"))
    if free == 0:
        tot = 1 + int(runx("awk '/HugePages_Total/ {print $2}' /proc/meminfo"))
        os.system(f"echo {tot} > /proc/sys/vm/nr_hugepages")

    hugef_name = HUGEPAGE_FILE + str(stream_id)
    hugef = open(hugef_name, "w+")
    hugef.truncate(HUGEPAGESZ)
    mem = mmap.mmap(hugef.fileno(), HUGEPAGESZ)
    log.info("type of mem is %s", str(type(mem)))
    global_mmaps.append(mem)
    os.unlink(hugef_name)

    # Find the local process address of the mapping, then use that to extract
    # the physical address from the kernel's pagemap interface.  The physical
    # page frame number occupies the bottom bits of the entry.
    mem[0] = 0 # Fault the page in so it has an address!
    vaddr = ctypes.addressof(ctypes.c_int.from_buffer(mem))
    vpagenum = vaddr >> 12
    pagemap = open("/proc/self/pagemap", "rb")
    pagemap.seek(vpagenum * 8)
    pent = pagemap.read(8)
    paddr = (struct.unpack("Q", pent)[0] & ((1 << 55) - 1)) * PAGESZ
    pagemap.close()
    return (mem, paddr, hugef)

# Maps a PCI BAR and returns the in-process address
def bar_map(pcidir, barnum):
    f = open(pcidir + "/resource" + str(barnum), "r+")
    mm = mmap.mmap(f.fileno(), os.fstat(f.fileno()).st_size)
    global_mmaps.append(mm)
    log.info("Mapped PCI bar %d of length %d bytes."
             % (barnum, os.fstat(f.fileno()).st_size))
    return (ctypes.addressof(ctypes.c_int.from_buffer(mm)), mm)

# Syntactic sugar to make register block definition & use look nice.
# Instantiate from a base address, assign offsets to (uint32) named registers as
# fields, call freeze(), then the field acts as a direct alias for the register!
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
    return subprocess.check_output(cmd, shell=True).decode().rstrip()

def mask(bit):
    if cavs25:
        return 0b1 << bit

def load_firmware(fw_file):
    try:
        fw_bytes = open(fw_file, "rb").read()
    except Exception as e:
        log.error(f"Could not read firmware file: `{fw_file}'")
        log.error(e)
        sys.exit(1)

    (magic, sz) = struct.unpack("4sI", fw_bytes[0:8])
    if magic == b'XMan':
        log.info(f"Trimming {sz} bytes of extended manifest")
        fw_bytes = fw_bytes[sz:len(fw_bytes)]

    # This actually means "enable access to BAR4 registers"!
    hda.PPCTL |= (1 << 30) # GPROCEN, "global processing enable"

    log.info("Resetting HDA device")
    hda.GCTL = 0
    while hda.GCTL & 1: pass
    hda.GCTL = 1
    while not hda.GCTL & 1: pass

    log.info(f"Stalling and Resetting DSP cores, ADSPCS = 0x{dsp.ADSPCS:x}")
    dsp.ADSPCS |= mask(CSTALL)
    dsp.ADSPCS |= mask(CRST)
    while (dsp.ADSPCS & mask(CRST)) == 0: pass

    log.info(f"Powering down DSP cores, ADSPCS = 0x{dsp.ADSPCS:x}")
    dsp.ADSPCS &= ~mask(SPA)
    while dsp.ADSPCS & mask(CPA): pass

    log.info(f"Configuring HDA stream {hda_ostream_id} to transfer firmware image")
    (buf_list_addr, num_bufs) = setup_dma_mem(fw_bytes)
    sd.CTL = 1
    while (sd.CTL & 1) == 0: pass
    sd.CTL = 0
    while (sd.CTL & 1) == 1: pass
    sd.CTL = 1 << 20 # Set stream ID to anything non-zero
    sd.BDPU = (buf_list_addr >> 32) & 0xffffffff
    sd.BDPL = buf_list_addr & 0xffffffff
    sd.CBL = len(fw_bytes)
    sd.LVI = num_bufs - 1
    hda.PPCTL |= (1 << hda_ostream_id)

    # SPIB ("Software Position In Buffer") is an Intel HDA extension
    # that puts a transfer boundary into the stream beyond which the
    # other side will not read.  The ROM wants to poll on a "buffer
    # full" bit on the other side that only works with this enabled.
    hda.SPBFCTL |= (1 << hda_ostream_id)
    hda.SD_SPIB = len(fw_bytes)

    # Start DSP.  Host needs to provide power to all cores on 1.5
    # (which also starts them) and 1.8 (merely gates power, DSP also
    # has to set PWRCTL). On 2.5 where the DSP has full control,
    # and only core 0 is set.
    log.info(f"Starting DSP, ADSPCS = 0x{dsp.ADSPCS:x}")
    dsp.ADSPCS = mask(SPA)
    while (dsp.ADSPCS & mask(CPA)) == 0: pass

    log.info(f"Unresetting DSP cores, ADSPCS = 0x{dsp.ADSPCS:x}")
    dsp.ADSPCS &= ~mask(CRST)
    while (dsp.ADSPCS & 1) != 0: pass

    log.info(f"Running DSP cores, ADSPCS = 0x{dsp.ADSPCS:x}")
    dsp.ADSPCS &= ~mask(CSTALL)

    # Wait for the ROM to boot and signal it's ready.  This not so short
    # sleep seems to be needed; if we're banging on the memory window
    # during initial boot (before/while the window control registers
    # are configured?) the DSP hardware will hang fairly reliably.
    log.info(f"Wait for ROM startup, ADSPCS = 0x{dsp.ADSPCS:x}")
    time.sleep(1)
    while (dsp.SRAM_FW_STATUS >> 24) != 5: pass

    # Send the DSP an IPC message to tell the device how to boot.
    # Note: with cAVS 1.8+ the ROM receives the stream argument as an
    # index within the array of output streams (and we always use the
    # first one by construction).  But with 1.5 it's the HDA index,
    # and depends on the number of input streams on the device.
    stream_idx = 0
    ipcval = (  (1 << 31)            # BUSY bit
                | (0x01 << 24)       # type = PURGE_FW
                | (1 << 14)          # purge_fw = 1
                | (stream_idx << 9)) # dma_id
    log.info(f"Sending IPC command, HIPIDR = 0x{ipcval:x}")
    dsp.HIPCIDR = ipcval

    log.info(f"Starting DMA, FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")
    sd.CTL |= 2 # START flag

    wait_fw_entered(dsp, timeout_s=None)

    # Turn DMA off and reset the stream.  Clearing START first is a
    # noop per the spec, but absolutely required for stability.
    # Apparently the reset doesn't stop the stream, and the next load
    # starts before it's ready and kills the load (and often the DSP).
    # The sleep too is required, on at least one board (a fast
    # chromebook) putting the two writes next each other also hangs
    # the DSP!
    sd.CTL &= ~2 # clear START
    time.sleep(0.1)
    sd.CTL |= 1
    log.info(f"cAVS firmware load complete")

def load_firmware_ace(fw_file):
    try:
        fw_bytes = open(fw_file, "rb").read()
        # Resize fw_bytes for MTL
        if len(fw_bytes) < 512 * 1024:
            fw_bytes += b'\x00' * (512 * 1024 - len(fw_bytes))
    except Exception as e:
        log.error(f"Could not read firmware file: `{fw_file}'")
        log.error(e)
        sys.exit(1)

    (magic, sz) = struct.unpack("4sI", fw_bytes[0:8])
    if magic == b'$AE1':
        log.info(f"Trimming {sz} bytes of extended manifest")
        fw_bytes = fw_bytes[sz:len(fw_bytes)]

    # This actually means "enable access to BAR4 registers"!
    hda.PPCTL |= (1 << 30) # GPROCEN, "global processing enable"

    log.info("Resetting HDA device")
    hda.GCTL = 0
    while hda.GCTL & 1: pass
    hda.GCTL = 1
    while not hda.GCTL & 1: pass

    log.info("Turning of DSP subsystem")
    dsp.HFDSSCS &= ~(1 << 16) # clear SPA bit
    time.sleep(0.002)
    # wait for CPA bit clear
    while dsp.HFDSSCS & (1 << 24):
        log.info("Waiting for DSP subsystem power off")
        time.sleep(0.1)

    log.info("Turning on DSP subsystem")
    dsp.HFDSSCS |= (1 << 16) # set SPA bit
    time.sleep(0.002) # needed as the CPA bit may be unstable
    # wait for CPA bit
    while not dsp.HFDSSCS & (1 << 24):
        log.info("Waiting for DSP subsystem power on")
        time.sleep(0.1)

    log.info("Turning on Domain0")
    dsp.HFPWRCTL |= 0x1 # set SPA bit
    time.sleep(0.002) # needed as the CPA bit may be unstable
    # wait for CPA bit
    while not dsp.HFPWRSTS & 0x1:
        log.info("Waiting for DSP domain0 power on")
        time.sleep(0.1)

    log.info("Turning off Primary Core")
    dsp.DSP2CXCTL_PRIMARY &= ~(0x1) # clear SPA
    time.sleep(0.002) # wait for CPA settlement
    while dsp.DSP2CXCTL_PRIMARY & (1 << 8):
        log.info("Waiting for DSP primary core power off")
        time.sleep(0.1)

    log.info(f"Configuring HDA stream {hda_ostream_id} to transfer firmware image")
    (buf_list_addr, num_bufs) = setup_dma_mem(fw_bytes)
    sd.CTL = 1
    while (sd.CTL & 1) == 0: pass
    sd.CTL = 0
    while (sd.CTL & 1) == 1: pass
    sd.CTL |= (1 << 20) # Set stream ID to anything non-zero
    sd.BDPU = (buf_list_addr >> 32) & 0xffffffff
    sd.BDPL = buf_list_addr & 0xffffffff
    sd.CBL = len(fw_bytes)
    sd.LVI = num_bufs - 1
    hda.PPCTL |= (1 << hda_ostream_id)

    # SPIB ("Software Position In Buffer") is an Intel HDA extension
    # that puts a transfer boundary into the stream beyond which the
    # other side will not read.  The ROM wants to poll on a "buffer
    # full" bit on the other side that only works with this enabled.
    hda.SPBFCTL |= (1 << hda_ostream_id)
    hda.SD_SPIB = len(fw_bytes)


    # Send the DSP an IPC message to tell the device how to boot.
    # Note: with cAVS 1.8+ the ROM receives the stream argument as an
    # index within the array of output streams (and we always use the
    # first one by construction).  But with 1.5 it's the HDA index,
    # and depends on the number of input streams on the device.
    stream_idx = 0
    ipcval = (  (1 << 31)            # BUSY bit
                | (0x01 << 24)       # type = PURGE_FW
                | (1 << 14)          # purge_fw = 1
                | (stream_idx << 9)) # dma_id
    log.info(f"Sending IPC command, HFIPCXIDR = 0x{ipcval:x}")
    dsp.HFIPCXIDR = ipcval

    log.info("Turning on Primary Core")
    dsp.DSP2CXCTL_PRIMARY |= 0x1 # clear SPA
    time.sleep(0.002) # wait for CPA settlement
    while not dsp.DSP2CXCTL_PRIMARY & (1 << 8):
        log.info("Waiting for DSP primary core power on")
        time.sleep(0.1)

    log.info("Waiting for IPC acceptance")
    while dsp.HFIPCXIDR & (1 << 31):
        log.info("Waiting for IPC busy bit clear")
        time.sleep(0.1)

    log.info("ACK IPC")
    dsp.HFIPCXIDA |= (1 << 31)

    log.info(f"Starting DMA, FW_STATUS = 0x{dsp.ROM_STATUS:x}")
    sd.CTL |= 2 # START flag

    wait_fw_entered(dsp, timeout_s=None)

    # Turn DMA off and reset the stream.  Clearing START first is a
    # noop per the spec, but absolutely required for stability.
    # Apparently the reset doesn't stop the stream, and the next load
    # starts before it's ready and kills the load (and often the DSP).
    # The sleep too is required, on at least one board (a fast
    # chromebook) putting the two writes next each other also hangs
    # the DSP!
    sd.CTL &= ~2 # clear START
    time.sleep(0.1)
    sd.CTL |= 1
    log.info(f"ACE firmware load complete")

def fw_is_alive(dsp):
    return dsp.ROM_STATUS & ((1 << 28) - 1) == 5 # "FW_ENTERED"

def wait_fw_entered(dsp, timeout_s):
    log.info("Waiting %s for firmware handoff, ROM_STATUS = 0x%x",
             "forever" if timeout_s is None else f"{timeout_s} seconds",
             dsp.ROM_STATUS)
    hertz = 100
    attempts = None if timeout_s is None else timeout_s * hertz
    while True:
        alive = fw_is_alive(dsp)
        if alive:
            break
        if attempts is not None:
            attempts -= 1
            if attempts < 0:
                break
        time.sleep(1 / hertz)

    if not alive:
        log.warning("Load failed?  ROM_STATUS = 0x%x", dsp.ROM_STATUS)
    else:
        log.info("FW alive, ROM_STATUS = 0x%x", dsp.ROM_STATUS)

def winstream_offset():
    ( base, stride ) = adsp_mem_window_config()
    return base + stride * 3

# This SHOULD be just "mem[start:start+length]", but slicing an mmap
# array seems to be unreliable on one of my machines (python 3.6.9 on
# Ubuntu 18.04).  Read out bytes individually.
def win_read(base, start, length):
    try:
        return b''.join(bar4_mmap[base + x].to_bytes(1, 'little')
                        for x in range(start, start + length))
    except IndexError as ie:
        # A FW in a bad state may cause winstream garbage
        log.error("IndexError in bar4_mmap[%d + %d]", base, start)
        log.error("bar4_mmap.size()=%d", bar4_mmap.size())
        raise ie

def winstream_reg_hdr(base):
    hdr = Regs(bar4_mem + base)
    hdr.WLEN  = 0x00
    hdr.START = 0x04
    hdr.END   = 0x08
    hdr.SEQ   = 0x0c
    hdr.freeze()
    return hdr

def win_hdr(hdr):
    return ( hdr.WLEN, hdr.START, hdr.END, hdr.SEQ )

# Python implementation of the same algorithm in sys_winstream_read(),
# see there for details.
def winstream_read(base, last_seq):
    while True:
        hdr = winstream_reg_hdr(base)
        (wlen, start, end, seq) = win_hdr(hdr)
        if wlen > SHELL_MAX_VALID_SLOT_SIZE:
            log.debug("DSP powered off at winstream_read")
            return (seq, "")
        if wlen == 0:
            return (seq, "")
        if last_seq == 0:
            last_seq = seq if args.no_history else (seq - ((end - start) % wlen))
        if seq == last_seq or start == end:
            return (seq, "")
        behind = seq - last_seq
        if behind > ((end - start) % wlen):
            return (seq, "")
        copy = (end - behind) % wlen
        suffix = min(behind, wlen - copy)
        result = win_read(base, 16 + copy, suffix)
        if suffix < behind:
            result += win_read(base, 16, behind - suffix)
        (wlen, start1, end, seq1) = win_hdr(hdr)
        if start1 == start and seq1 == seq:
            # Best effort attempt at decoding, replacing unusable characters
            # Found to be useful when it really goes wrong
            return (seq, result.decode("utf-8", "replace"))

def idx_mod(wlen, idx):
    if idx >= wlen:
        return idx - wlen
    return idx

def idx_sub(wlen, a, b):
    return idx_mod(wlen, a + (wlen - b))

# Python implementation of the same algorithm in sys_winstream_write(),
# see there for details.
def winstream_write(base, msg):
    hdr = winstream_reg_hdr(base)
    (wlen, start, end, seq) = win_hdr(hdr)
    if wlen > SHELL_MAX_VALID_SLOT_SIZE:
        log.debug("DSP powered off at winstream_write")
        return
    if wlen == 0:
        return
    lenmsg = len(msg)
    lenmsg0 = lenmsg
    if len(msg) > wlen + 1:
        start = end
        lenmsg = wlen - 1
    lenmsg = min(lenmsg, wlen)
    if seq != 0:
        avail = (wlen - 1) - idx_sub(wlen, end, start)
        if lenmsg > avail:
            hdr.START = idx_mod(wlen, start + (lenmsg - avail))
    if lenmsg < lenmsg0:
        hdr.START = end
        drop = lenmsg0 - lenmsg
        msg = msg[drop : lenmsg - drop]
    suffix = min(lenmsg, wlen - end)
    for c in range(0, suffix):
        bar4_mmap[base + 16 + end + c] = msg[c]
    if lenmsg > suffix:
        for c in range(0, lenmsg - suffix):
            bar4_mmap[base + 16 + c] = msg[suffix + c]
    hdr.END = idx_mod(wlen, end + lenmsg)
    hdr.SEQ += lenmsg0

def debug_offset():
    ( base, stride ) = adsp_mem_window_config()
    return base + stride * 2

def debug_slot_offset(num):
    return debug_offset() + DEBUG_SLOT_SIZE * (1 + num)

def debug_slot_offset_by_type(the_type, timeout_s=0.2):
    ADSP_DW_SLOT_COUNT=15
    hertz = 100
    attempts = timeout_s * hertz
    while attempts > 0:
        data = win_read(debug_offset(), 0, ADSP_DW_SLOT_COUNT * 3 * 4)
        for i in range(ADSP_DW_SLOT_COUNT):
            start_index = i * (3 * 4)
            end_index = (i + 1) * (3 * 4)
            desc = data[start_index:end_index]
            resource_id, type_id, vma = struct.unpack('<III', desc)
            if type_id == the_type:
                log.info("found desc %u resource_id 0x%08x type_id 0x%08x vma 0x%08x",
                         i, resource_id, type_id, vma)
                return debug_slot_offset(i)
        log.debug("not found, %u attempts left", attempts)
        attempts -= 1
        time.sleep(1 / hertz)
    return None

def shell_base_offset():
    return debug_offset() + DEBUG_SLOT_SIZE * (1 + DEBUG_SLOT_SHELL)

def read_from_shell_memwindow_winstream(last_seq):
    offset = shell_base_offset() + SHELL_RX_SIZE
    (last_seq, output) = winstream_read(offset, last_seq)
    if output:
        os.write(shell_client_port, output.encode("utf-8"))
    return last_seq

def write_to_shell_memwindow_winstream():
    msg = os.read(shell_client_port, 1)
    if len(msg) > 0:
        winstream_write(shell_base_offset(), msg)

def create_shell_pty():
    global shell_client_port
    (shell_client_port, user_port) = pty.openpty()
    name = os.ttyname(user_port)
    log.info(f"shell PTY at: {name}")
    asyncio.get_event_loop().add_reader(shell_client_port, write_to_shell_memwindow_winstream)

async def ipc_delay_done():
    await asyncio.sleep(0.1)
    if adsp_is_ace():
        dsp.HFIPCXTDA = ~(1<<31) & dsp.HFIPCXTDA # Signal done
    else:
        dsp.HIPCTDA = 1<<31

def inbox_offset():
    ( base, stride ) = adsp_mem_window_config()
    return base + stride

def outbox_offset():
    ( base, _ ) = adsp_mem_window_config()
    return base + 4096

ipc_timestamp = 0

# Super-simple command language, driven by the test code on the DSP
def ipc_command(data, ext_data):
    send_msg = False
    done = True
    log.debug ("ipc data %d, ext_data %x", data, ext_data)
    if data == 0: # noop, with synchronous DONE
        pass
    elif data == 1: # async command: signal DONE after a delay (on 1.8+)
        done = False
        asyncio.ensure_future(ipc_delay_done())
    elif data == 2: # echo back ext_data as a message command
        send_msg = True
    elif data == 3: # set ADSPCS
        dsp.ADSPCS = ext_data
    elif data == 4: # echo back microseconds since last timestamp command
        global ipc_timestamp
        t = round(time.time() * 1e6)
        ext_data = t - ipc_timestamp
        ipc_timestamp = t
        send_msg = True
    elif data == 5: # copy word at outbox[ext_data >> 16] to inbox[ext_data & 0xffff]
        src = outbox_offset() + 4 * (ext_data >> 16)
        dst = inbox_offset() + 4 * (ext_data & 0xffff)
        for i in range(4):
            bar4_mmap[dst + i] = bar4_mmap[src + i]
    elif data == 6: # HDA RESET (init if not exists)
        stream_id = ext_data & 0xff
        if stream_id in hda_streams:
            hda_streams[stream_id].reset()
        else:
            hda_str = HDAStream(stream_id)
            hda_streams[stream_id] = hda_str
    elif data == 7: # HDA CONFIG
        stream_id = ext_data & 0xFF
        buf_len = ext_data >> 8 & 0xFFFF
        hda_str = hda_streams[stream_id]
        hda_str.config(buf_len)
    elif data == 8: # HDA START
        stream_id = ext_data & 0xFF
        hda_streams[stream_id].start()
        hda_streams[stream_id].mem.seek(0)

    elif data == 9: # HDA STOP
        stream_id = ext_data & 0xFF
        hda_streams[stream_id].stop()
    elif data == 10: # HDA VALIDATE
        stream_id = ext_data & 0xFF
        hda_str = hda_streams[stream_id]
        hda_str.debug()
        is_ramp_data = True
        hda_str.mem.seek(0)
        for (i, val) in enumerate(hda_str.mem.read(256)):
            if i != val:
                is_ramp_data = False
            # log.info("stream[%d][%d]: %d", stream_id, i, val) # debug helper
        log.info("Is ramp data? " + str(is_ramp_data))
        ext_data = int(is_ramp_data)
        log.info(f"Ext data to send back on ramp status {ext_data}")
        send_msg = True
    elif data == 11: # HDA HOST OUT SEND
        stream_id = ext_data & 0xff
        buf = bytearray(256)
        for i in range(0, 256):
            buf[i] = i
        hda_streams[stream_id].write(buf)
    elif data == 12: # HDA PRINT
        stream_id = ext_data & 0xFF
        buf_len = ext_data >> 8 & 0xFFFF
        hda_str = hda_streams[stream_id]
        # check for wrap here
        pos = hda_str.mem.tell()
        read_lens = [buf_len, 0]
        if pos + buf_len >= hda_str.buf_len*2:
            read_lens[0] = hda_str.buf_len*2 - pos
            read_lens[1] = buf_len - read_lens[0]
        # validate the read lens
        assert (read_lens[0] + pos) <= (hda_str.buf_len*2)
        assert read_lens[0] % 128 == 0
        assert read_lens[1] % 128 == 0
        buf_data0 = hda_str.mem.read(read_lens[0])
        hda_msg0 = buf_data0.decode("utf-8", "replace")
        sys.stdout.write(hda_msg0)
        if read_lens[1] != 0:
            hda_str.mem.seek(0)
            buf_data1 = hda_str.mem.read(read_lens[1])
            hda_msg1 = buf_data1.decode("utf-8", "replace")
            sys.stdout.write(hda_msg1)
        pos = hda_str.mem.tell()
        sys.stdout.flush()
    else:
        log.warning(f"cavstool: Unrecognized IPC command 0x{data:x} ext 0x{ext_data:x}")
        if not fw_is_alive(dsp):
            if args.log_only:
                log.info("DSP power seems off")
                wait_fw_entered(dsp, timeout_s=None)
            else:
                log.warning("DSP power seems off?!")
                time.sleep(2) # potential spam reduction

            return

    if adsp_is_ace():
        dsp.HFIPCXTDR = 1<<31 # Ack local interrupt, also signals DONE on v1.5
        if done:
            dsp.HFIPCXTDA = ~(1<<31) & dsp.HFIPCXTDA # Signal done
        if send_msg:
            log.debug("ipc: sending msg 0x%08x" % ext_data)
            dsp.HFIPCXIDDY = ext_data
            dsp.HFIPCXIDR = (1<<31) | ext_data
    else:
        dsp.HIPCTDR = 1<<31 # Ack local interrupt, also signals DONE on v1.5
        if done:
            dsp.HIPCTDA = 1<<31 # Signal done
        if send_msg:
            dsp.HIPCIDD = ext_data
            dsp.HIPCIDR = (1<<31) | ext_data

def handle_ipc():
    if adsp_is_ace():
        if dsp.HFIPCXIDA & 0x80000000:
            log.debug("ipc: Ack DSP reply with IDA_DONE")
            dsp.HFIPCXIDA = 1<<31 # must ACK any DONE interrupts that arrive!
        if dsp.HFIPCXTDR & 0x80000000:
            ipc_command(dsp.HFIPCXTDR & ~0x80000000, dsp.HFIPCXTDDY)
        return

    if dsp.HIPCIDA & 0x80000000:
        dsp.HIPCIDA = 1<<31 # must ACK any DONE interrupts that arrive!
    if dsp.HIPCTDR & 0x80000000:
        ipc_command(dsp.HIPCTDR & ~0x80000000, dsp.HIPCTDD)

async def main():
    #TODO this bit me, remove the globals, write a little FirmwareLoader class or something to contain.
    global hda, sd, dsp, hda_ostream_id, hda_streams

    try:
        (hda, sd, dsp, hda_ostream_id) = map_regs(args.log_only)
    except Exception as e:
        log.error("Could not map device in sysfs; run as root?")
        log.error(e)
        sys.exit(1)

    log.info(f"Detected cAVS 1.8+ hardware")

    if args.log_only:
        wait_fw_entered(dsp, timeout_s=None)
    else:
        if not args.fw_file:
            log.error("Firmware file argument missing")
            sys.exit(1)

        if adsp_is_ace():
            load_firmware_ace(args.fw_file)
        else:
            load_firmware(args.fw_file)
        time.sleep(0.1)

        if not args.quiet:
            sys.stdout.write("--\n")

    if args.shell_pty:
        create_shell_pty()

    hda_streams = dict()

    last_seq = 0
    last_seq_shell = 0
    while start_output is True:
        await asyncio.sleep(0.03)
        if args.shell_pty:
            last_seq_shell = read_from_shell_memwindow_winstream(last_seq_shell)
        (last_seq, output) = winstream_read(winstream_offset(), last_seq)
        if output:
            sys.stdout.write(output)
            sys.stdout.flush()
        if not args.log_only:
            handle_ipc()

def args_parse():
    global args
    ap = argparse.ArgumentParser(description="DSP loader/logger tool", allow_abbrev=False)
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="No loader output, just DSP logging")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="More loader output, DEBUG logging level")
    ap.add_argument("-l", "--log-only", action="store_true",
                    help="Don't load firmware, just show log output")
    ap.add_argument("-p", "--shell-pty", action="store_true",
                    help="Create a Zephyr shell pty if enabled in firmware")
    ap.add_argument("-n", "--no-history", action="store_true",
                    help="No current log buffer at start, just new output")
    ap.add_argument("fw_file", nargs="?", help="Firmware file")

    args = ap.parse_args()

    if args.quiet:
        log.setLevel(logging.WARN)
    elif args.verbose:
        log.setLevel(logging.DEBUG)

if __name__ == "__main__":
    args_parse()
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        start_output = False
