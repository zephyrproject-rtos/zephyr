#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
import os
import fcntl
import struct
import mmap
import logging

from ctypes import cast, POINTER, c_uint8, c_uint32, c_uint16, c_uint64,\
                   addressof, byref

# diag_driver file
DIAG_DRV_PATH = "/dev/hda"

# Diag Driver Definition - sof-diagnostic-driver/ioctl.h

# IOCTL definition
CMD_OPEN_DEVICE  = 0x47
CMD_ALLOC_MEMORY = 0x3A
CMD_FREE_MEMORY  = 0x3B

CMD_OPEN_DEVICE_LEN = 40


class HdaBar:
    """ Data structure for HDA BAR information """

    def __init__(self, raw):
        self.base_p = 0
        self.base_v = 0
        self.size = 0
        (self.base_p, self.base_v, self.size) = struct.unpack('=QQL', raw)

    def __str__(self):
        return "   Base Physical Address: 0x%08X\n" \
               "   Base Virtual Address:  0x%08X\n" \
               "   Base Size:             0x%08X" \
                   % (self.base_p, self.base_v, self.size)


class HdaMemory:
    """ Data structure for HDA memory allocation """

    def __init__(self, size=0):
        self.dma_addr_p = 0
        self.dma_addr_v = 0
        self.size = size
        self.memmap = None

    def set_value(self, raw):
        (self.dma_addr_p,
         self.dma_addr_v,
         self.size) = struct.unpack('=QQL', raw)

    def get_value(self):
        data = bytearray(struct.pack('=QQL', self.dma_addr_p,
                                             self.dma_addr_v,
                                             self.size))
        return data

    def __str__(self):
        return "   DMA Physical Address: 0x%08X\n" \
               "   DMA Virtual  Address: 0x%08X\n" \
               "   DMA Size:             0x%08X"   \
               % (self.dma_addr_p, self.dma_addr_v, self.size)


class HdaHandle:
    """ Data structure for HDA handles """

    def __init__(self, raw):

        data = struct.unpack('20s20s', raw)
        self.hda_bar = HdaBar(data[0])
        self.dsp_bar = HdaBar(data[1])

    def __str__(self):
        output = (
            "HDA BAR:\n"
            "{hda}\n"
            "DSP BAR:\n"
            "{dsp}"
        ).format(
            hda = self.hda_bar, dsp = self.dsp_bar
        )
        return output


class DiagDriver:
    """ Interface for diag_driver """

    def __init__(self):
        self._handle = None
        self._mem_map_list = []
        self._buff_list = []

    def open_device(self):
        """
        Send CMD_OPEN_DEVICE and get HDA BAR and DSP BAR

        Returns:
            (handle)(obj:HdaHandle): HDA and DSP Bars objs
        """

        logging.debug(">>> DiagDriver.open_device()")

        # Allocate bytearry for HDABusTest_OpenDevice
        buf = bytearray(CMD_OPEN_DEVICE_LEN)

        logging.info("Open HDA device: %s" % DIAG_DRV_PATH)
        # Send CMD_OPEN_DEVICE
        with open(DIAG_DRV_PATH) as fd:
            fcntl.ioctl(fd, CMD_OPEN_DEVICE, buf)

        self._handle = HdaHandle(buf)

        logging.debug("<<< DiagDriver.open_device()")

        return self._handle

    def alloc_mem(self, size):
        """
        Send CMD_ALLOC_MEMORY to allocate DMA buffer

        Returns:
            hda_mem (obj:HDAMemory): Allocated DMA buffer information
        """

        logging.debug(">>> Diag_Driver.alloc_mem(size=0x%08X)" % size)

        hda_buf = HdaMemory(size)

        # Allocate bytearray for HDABusTestMem
        buf = hda_buf.get_value()

        # Send CMD_ALLOC_MEMORY
        with open(DIAG_DRV_PATH) as fd:
            fcntl.ioctl(fd, CMD_ALLOC_MEMORY, buf)

        hda_buf.set_value(buf)

        mem = self.mmap(hda_buf.dma_addr_p, hda_buf.size)
        hda_buf.memmap = mem
        hda_buf.dma_addr_v = addressof(mem)

        logging.debug("DMA Memory:\n%s" % hda_buf)

        # Append to buffer list for later clean up.
        self._buff_list.append(hda_buf)

        logging.debug("<<< Diag_Driver.alloc_mem()")

        return hda_buf

    def free_mem(self, hda_buf):
        """
        Send CMD_FREE_MEMORY to free the DMA buffer

        Params:
            had_mem (obj:HDAMemory): DMA buffer information to be freed

        Returns:
            0 for success, otherwise fail.
        """
        logging.debug(">>> Diag_Driver.free_mem()")

        if hda_buf not in self._buff_list:
            logging.error("Cannot find buffer from the list")
            raise ValueError("Cannot find buffer to free")

        logging.debug("Buffer to Free:\n%s" % hda_buf)

        buf = hda_buf.get_value()

        # Send CMD_FREE_MEMORY
        with open(DIAG_DRV_PATH) as fd:
            ret = fcntl.ioctl(fd, CMD_FREE_MEMORY, buf)

        self._buff_list.remove(hda_buf)

        logging.debug("<<< Diag_Driver.free_mem()")
        return ret

    def mmap(self, addr, length):
        """
        Setup mmap for HDA and DSP from /dev/mem

        Returns:
            (mem map,..)(uint32_t..): Array of uint32_t in mapped memory
        """

        logging.debug(">>> Diag_Driver.mmap(addr=0x%08X, length=0x%08X)"
                       % (addr, length))

        try:
            fd = os.open(DIAG_DRV_PATH, os.O_RDWR)
            mem_map = mmap.mmap(fd, length, offset=addr,
                                prot=mmap.PROT_READ | mmap.PROT_WRITE,
                                flags=mmap.MAP_SHARED)

            self._mem_map_list.append(mem_map)

            # Array of uint8
            mem = (c_uint8 * length).from_buffer(mem_map)
        finally:
            os.close(fd)

        logging.debug("<<< Diag_Driver.mmap()")

        return mem


class Register:
    def __init__(self, base_addr, offset, type=c_uint32):
        self._type = type
        self._obj = cast(byref(base_addr, offset), POINTER(type))

    def __str__(self):
        if self._type == c_uint8:
            return "0x%02X" % self.value
        elif self._type == c_uint16:
            return "0x%04X" % self.value
        elif self._type == c_uint32:
            return "0x%08X" % self.value
        elif self._type == c_uint64:
            return "0x%08X %08X" % (
                self.value >> 32,
                self.value & 0xFFFFFFFF
            )
        else:
            return "0x%08X (unknown type)" % self.value

    @property
    def value(self):
        return self._obj.contents.value
    @value.setter
    def value(self, value):
        self._obj[0] = value
