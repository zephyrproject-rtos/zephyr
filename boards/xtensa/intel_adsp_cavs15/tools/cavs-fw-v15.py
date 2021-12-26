#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2021 Intel Corporation. All rights reserved.

import struct
import sys
import time
from cavs_fw_common import *

# Intel Audio DSP firmware loader.  No dependencies on anything
# outside this file beyond Python3 builtins.  Assumes the host system
# has a hugetlbs mounted at /dev/hugepages.  Confirmed to run out of
# the box on Ubuntu 18.04 and 20.04.  Run as root with the firmware
# file as the single argument.

logging.basicConfig()
log = logging.getLogger("cavs-fw")
log.setLevel(logging.INFO)

FW_FILE = sys.argv[2] if sys.argv[1] == "-f" else sys.argv[1]

HDA_PPCTL__GPROCEN       = 1 << 30
HDA_SD_CTL__TRAFFIC_PRIO = 1 << 18
HDA_SD_CTL__START        = 1 << 1

def main():
    with open(FW_FILE, "rb") as f:
        fw_bytes = f.read()

    (magic, sz) = struct.unpack("4sI", fw_bytes[0:8])
    if magic == b'XMan':
        fw_bytes = fw_bytes[sz:len(fw_bytes)]

    (hda, sd, dsp, hda_ostream_id, cavs15) = map_regs() # Device register mappings
    log.info(f"Detected cAVS {'1.5' if cavs15 else '1.8+'} hardware")

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
    time.sleep(0.1)
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
        log.warning(f"Load failed?  FW_STATUS = 0x{dsp.SRAM_FW_STATUS:x}")

    # Turn DMA off and reset the stream.  If this doesn't happen the
    # hardware continues streaming out of our now-stale page and can
    # has been observed to glitch the next boot.
    sd.CTL &= ~HDA_SD_CTL__START
    sd.CTL |= 1

    time.sleep(1)

    log.info(f"ADSPCS = 0x{dsp.ADSPCS:x}")
    log.info(f"cAVS v15 firmware load complete, {ncores(dsp)} cores active")

if __name__ == "__main__":
    log.info("cAVS firmware loader v15")
    main()
