#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
from enum import Enum

# BXT

# CORE ID MASK
CORE_0 = 0x1
CORE_1 = 0x2
CORE_MASK = 0x3

# Number of Input Streams Supported (GCAP[11:8])
NUM_ISS = 6
# Number of Output Streams Supported (GCAP[15:12])
NUM_OSS = 7
# Total Number of Streams supported
NUM_STREAMS = NUM_ISS + NUM_OSS

# DMA Index for FW download
DMA_ID = 7

# DMA Page Size
DMA_PAGE_SIZE = 0x1000

# FW Registers in SRAM
FW_SRAM = 0x80000
FW_REGS = FW_SRAM + 0x00
FW_MBOX_UPLINK = FW_SRAM + 0x1000
FW_MBOX_DWLINK = FW_SRAM + 0x20000
FW_MBOX_SIZE   = 0x1000

# FW Status Register
FW_STATUS                   = FW_REGS + 0x0000
FW_STATUS_BOOT_STATE        = 0x00FFFFFF
FW_STATUS_BOOT_STATE_OFFSET = 0
FW_STATUS_WAIT_STATE        = 0x0F000000
FW_STATUS_WAIT_STATE_OFFSET = 24
FW_STATUS_MODULE            = 0x70000000
FW_STATUS_MODULE_OFFSET     = 28
FW_STATUS_ERROR             = 0x80000000
FW_STATUS_ERROR_OFFSET      = 31


class BOOT_STATUS(Enum):
    INIT        = 0
    INIT_DONE   = 1
    FW_ENTERED  = 5


def BOOT_STATUS_STR(status):
    try:
        e = BOOT_STATUS(status)
    except Exception:
        return "UNKNOWN"

    return e.name


# Boot Status
BOOT_STATUS_INIT        = 0x00
BOOT_STATUS_INIT_DONE   = 0x01
BOOT_STATUS_FW_ENTERED  = 0x05


class WAIT_STATUS(Enum):
    DMA_BUFFER_FULL = 5


def WAIT_STATUS_STR(status):
    try:
        e = WAIT_STATUS(status)
    except Exception:
        return "UNKNOWN"

    return e.name


# Wait Status
WAIT_STATUS_DMA_BUFFER_FULL = 0x05

# FW Error Status
FW_ERR_CODE = FW_SRAM + 0x0004

# IPC Purge FW message
FW_IPC_PURGE = 0x01004000

# IPC GLOBAL LENGTH register
IPC_GLOBAL_LEN = 0x00
IPC_GLOBAL_CMD = 0x04
