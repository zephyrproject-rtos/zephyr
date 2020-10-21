#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Registers

######################################
# High Definition Audio Memory Space #
######################################

# HDA Register Blocks
HDA_GR_BASE   = 0x0000    # HDA Global Register Block
HDA_SD_BASE   = 0x0080    # HDA Stream Descriptor Register Block - index 0
HDA_SPBF_BASE = 0x0700    # HDA Software Position Based FIFO Capability Structure
HDA_PPC_BASE  = 0x0800    # HDA Processing Pipe Capabilities Structure

# Global Capabilities
HDA_GR_GCAP = HDA_GR_BASE + 0x0000
HDA_GR_GCAP_ISS = 0x0F00        # Number of Input Streams Supported
HDA_GR_GCAP_ISS_OFFSET  = 8     # Number of Input Streams Supported Offset
HDA_GR_GCAP_OSS = 0xF000        # Number of Output Streams Supported
HDA_GR_GCAP_OSS_OFFSET  = 12    # Number of Offset Streams Supported Offset

# Golbal Control
HDA_GR_GCTL = HDA_GR_BASE + 0x0008

# Stream Descriptor X Control/Status - only within the blcok
HDA_SD_CS = 0x0000
HDA_SD_CS_SRST = 0x000001       # Stream Reset
HDA_SD_CS_SRST_OFFSET = 0       # Stream Reset Offset
HDA_SD_CS_RUN  = 0x000002       # Stream Run
HDA_SD_CS_RUN_OFFSET  = 1       # Stream Run Offset
HDA_SD_CS_TP   = 0x040000       # Traffic Priority
HDA_SD_CS_TP_OFFSET   = 18      # Traffic Priority Offset
HDA_SD_CS_STRM = 0xF00000       # Stream Number
HDA_SD_CS_STRM_OFFSET = 20      # Stream Number Offset

# Stream Descriptor X Link Position in Buffer
HDA_SD_LPIB = 0x0004

# Stream Descriptor X Cyclic Buffer Length
HDA_SD_CBL = 0x0008

# Stream Descriptor X Last Valid Index
HDA_SD_LVI = 0x000C

# Stream Descriptor X FIFO Eviction Watermark
HDA_SD_FIFOW = 0x000E

# Stream Descriptor X FIFO Size
HDA_SD_FIFOS = 0x0010

# Stream Descriptor X Format
HDA_SD_FMT      = 0x0012
HDA_SD_FMT_BITS = 0x00F0        # Bit Rate
HDA_SD_FMT_BITS_OFFSET  = 4     # Bit Rate Offset

# Stream Descriptor X FIFO Limit
HDA_SD_FIFOL = 0x0014

# Stream Descriptor X Buffer Descriptor List Pointer Lower Base Address
HDA_SD_BDLPLBA = 0x0018

# Stream Descriptor X Buffer Descriptor List Pointer Upper Base Address
HDA_SD_BDLPUBA = 0x001C

# Stream Descriptor Size
HDA_SD_SIZE = 0x20


# Software Position Based FIFO Capability Control
HDA_SPBF_SPBFCTL = HDA_SPBF_BASE + 0x0004
HDA_SPBF_SPBFCTL_SPIBE = 0xFFFFFFFF     # Software Position in Buffer Enable
HDA_SPBF_SPBFCTL_SPIBE_OFFSET = 0       # Software Position in Buffer Enable Offset

# Stream Descriptor X Software Position in Buffer
HDA_SPBF_SD_BASE = HDA_SPBF_BASE + 0x0008
HDA_SPBF_SDSPIB  = 0x00

# Stream Descriptor X Max FIFO Size
HDA_SPBF_SDMAXFIFOS = 0x04

# Software Position Based FIFO Stream Descritpro Size
HDA_SPBF_SD_SIZE = 0x08

# Processing Pipe Control
HDA_PPC_PPCTL = HDA_PPC_BASE + 0x0004
HDA_PPC_PPCTL_PROCEN  = 0x3FFFFFFF      # Processing Enable
HDA_PPC_PPCTL_PROCEN_OFFSET = 0         # Processing Enable Offset
HDA_PPC_PPCTL_GPROCEN = 0x40000000      # Global Processing Enable
HDA_PPC_PPCTL_GPROCEN_OFFSET = 30       # Global Processing Enable Offset

# ADSP Register Blocks
ADSP_GR_BASE  = 0x0000      # ADSP General DSP Registers
ADSP_IPC_BASE = 0x0040      # ADSP IPC Register

# ADSP Control & Status
ADSP_GR_ADSPCS = ADSP_GR_BASE + 0x0004
ADSP_GR_ADSPCS_CRST = 0x000000FF        # Core Reset
ADSP_GR_ADSPCS_CRST_OFFSET = 0          # Core Reset Offset
ADSP_GR_ADSPCS_CSTALL = 0x0000FF00      # Core Run#/Stall
ADSP_GR_ADSPCS_CSTALL_OFFSET = 8        # Core Run#/Stall Offset
ADSP_GR_ADSPCS_SPA = 0x00FF0000         # Set Power Active
ADSP_GR_ADSPCS_SPA_OFFSET = 16          # Set Power Active Offset
ADSP_GR_ADSPCS_CPA = 0xFF000000         # Current Power Active
ADSP_GR_ADSPCS_CPA_OFFSET = 24          # Current Power Active Offset

# ADSP Interrupt Control
ADSP_GR_ADSPIC = ADSP_GR_BASE + 0x0008
ADSP_GR_ADSPIC_IPC = 0x00000001         # IPC Interrupt
ADSP_GR_ADSPIC_IPC_OFFSET = 0           # IPC Interrupt Offset
ADSP_GR_ADSPIC_CLDMA = 0x00000002       # Code Loader DMA Interrupt
ADSP_GR_ADSPIC_CLDMA_OFFSET = 1         # Code Loader DMA Interrupt Offset

# ADSP IPC DSP to Host
ADSP_IPC_HIPCT = ADSP_IPC_BASE + 0x0000
ADSP_IPC_HIPCT_BUSY = 0x80000000        # Busy
ADSP_IPC_HIPCT_BUSY_OFFSET = 31         # Busy Offset
ADSP_IPC_HIPCT_MSG = 0x7FFFFFFF         # Message
ADSP_IPC_HIPCT_MSG_OFFSET = 0           # Message Offset

# ADSP IPC DSP to Host Extension
ADSP_IPC_HIPCTE = ADSP_IPC_BASE + 0x0004
ADSP_IPC_HIPCTE_MSGEXT = 0x3FFFFFFF     # Message Extension
ADSP_IPC_HIPCTE_MSGEXT_OFFSET = 0       # Message Extension Offset

# ADSP IPC Host to DSP
ADSP_IPC_HIPCI = ADSP_IPC_BASE + 0x0008
ADSP_IPC_HIPCI_BUSY = 0x80000000        # Busy
ADSP_IPC_HIPCI_BUSY_OFFSET = 31         # Busy Offset
ADSP_IPC_HIPCI_MSG = 0x7FFFFFFF         # Message
ADSP_IPC_HIPCI_MSG_OFFSET = 0           # Message Offset

# ADSP IPC Host to DSP Extension
ADSP_IPC_HIPCIE = ADSP_IPC_BASE + 0x000C
ADSP_IPC_HIPCIE_ERR = 0x80000000        # Error
ADSP_IPC_HIPCIE_ERR_OFFSET = 31         # Error Offset
ADSP_IPC_HIPCIE_DONE = 0x40000000       # Done
ADSP_IPC_HIPCIE_DONE_OFFSET = 30        # Done Offset
ADSP_IPC_HIPCIE_MSGEXT = 0x3FFFFFFF     # Message Extension
ADSP_IPC_HIPCIE_MSGEXT_OFFSET = 0       # Message Extension Offset

# ADSP IPC Control
ADSP_IPC_HIPCCTL = ADSP_IPC_BASE + 0x0010
ADSP_IPC_HIPCCTL_IPCTBIE = 0x00000001   # IPC Target Busy Interrupt Enable
ADSP_IPC_HIPCCTL_IPCTBIE_OFFSET = 0     # IPC Target Busy Interrupt Enable Offset
ADSP_IPC_HIPCCTL_IPCIDIE = 0x00000002   # IPC Initiator Done Interrupt Enable
ADSP_IPC_HIPCCTL_IPCIDIE_OFFSET = 1     # IPC Initiator Done Interrupt Enable Offset
