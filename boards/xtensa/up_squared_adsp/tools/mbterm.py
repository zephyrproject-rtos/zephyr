#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

from time import sleep

from lib.device import Device
import lib.registers as regs

def mailbox_poll(dev):
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

    mailbox_poll(device)
