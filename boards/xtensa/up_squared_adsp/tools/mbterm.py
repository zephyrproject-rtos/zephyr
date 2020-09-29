#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

from time import sleep
from mmap import mmap, ACCESS_COPY
from ctypes import c_uint8

from lib.device import Device
import lib.registers as regs

MBOX = 0x91281000
LENGTH = 0x1000

with open("/dev/mem", "rb") as f:
    mem_map = mmap(f.fileno(), LENGTH, access=ACCESS_COPY, offset=MBOX)
    mem = (c_uint8 * LENGTH).from_buffer(mem_map)

def mailbox_poll_mem(dev):
    while True:
        if dev.dsp_hipct.value & regs.ADSP_IPC_HIPCT_BUSY:

            # Use only id for passing character
            line_len = dev.dsp_hipct.value & regs.ADSP_IPC_HIPCT_MSG

            # Mask out internal bits
            line_len &= 0x10FFFF

            if line_len:
                print(bytes(mem[:line_len]).decode())

            # Clear interrupt
            dev.dsp_hipct.value |= regs.ADSP_IPC_HIPCT_BUSY
        else:
            sleep(0.005)

# Character passed in mailbox ID field
def mailbox_poll_char(dev):
    while True:
        if dev.dsp_hipct.value & regs.ADSP_IPC_HIPCT_BUSY:

            # Use only id for passing character
            character = dev.dsp_hipct.value & regs.ADSP_IPC_HIPCT_MSG

            # Get readable character
            character &= 0x10FFFF

            print('{}'.format(chr(character)), end='')

            # Clear interrupt
            dev.dsp_hipct.value |= regs.ADSP_IPC_HIPCT_BUSY
        else:
            sleep(0.005)


if __name__ == "__main__":
    # Clear Done if needed
    #dev.dsp_hipct.value |= regs.ADSP_IPC_HIPCT_BUSY

    # Use Device library for controlling registers
    device = Device()
    device.open_device()

    mailbox_poll_mem(device)
