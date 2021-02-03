#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
import math
import logging
from time import sleep
from ctypes import c_uint16, POINTER, cast, c_uint8, c_uint64

from lib.driver import Register
import lib.registers as regs_def
import lib.platforms as plat_def


class DmaBuf:
    """ Class for DMA buffer """

    def __init__(self, drv, size):
        self.drv = drv
        self.size = size

        # Allocated DMA buffer should be a multiplication of page size
        self.page_count = math.ceil(self.size / plat_def.DMA_PAGE_SIZE)
        logging.debug("Page Count: %d" % self.page_count)

        self.alloc_size = self.page_count * plat_def.DMA_PAGE_SIZE
        logging.debug("Allocate DMA Buffer: size=0x%08X alloc_size=0x%08X"
                      % (self.size, self.alloc_size))
        self.mem = self.drv.alloc_mem(self.alloc_size)
        self.addr_p = self.mem.dma_addr_p
        self.buf = cast(self.mem.dma_addr_v,
                        POINTER(c_uint8 * self.alloc_size)).contents

    def copy(self, data, size):
        """ Copying data to allocated DMA buffer """
        if size > self.alloc_size:
            raise ValueError("Not enough buffer. allocated: %d requested: %d"
                             % (self.alloc_size, size))
        logging.debug("Copying Data to DMA buffer")
        self.buf[:size] = data[:size]

    def free(self):
        if self.mem:
            self.drv.free_mem(self.mem)
            self.mem = None


class DmaBufDescList:
    """ Class DMA Buffer Descriptor List """

    def __init__(self, drv, fw_buf):
        self.drv = drv
        self.bd_count = fw_buf.page_count

        # Single Page for Buffer Descriptor List
        self.buf = DmaBuf(drv, plat_def.DMA_PAGE_SIZE)

        curr_ptr = 0
        # Map BDLE with data buffer
        logging.debug("Update Buffer Descriptor List:")
        for i in range(self.bd_count):
            bdle_addr = Register(self.buf.mem.memmap, (i * 16) + 0x00, c_uint64)
            bdle_len = Register(self.buf.mem.memmap, (i * 16) + 0x08)
            bdle_ioc = Register(self.buf.mem.memmap, (i * 16) + 0x0C)

            if fw_buf.alloc_size - curr_ptr > plat_def.DMA_PAGE_SIZE:
                bdle_len.value = plat_def.DMA_PAGE_SIZE
                bdle_ioc.value = 0
                bdle_addr.value = fw_buf.addr_p + curr_ptr
            else:
                bdle_len.value = fw_buf.alloc_size - curr_ptr
                bdle_ioc.value = 1
                bdle_addr.value = fw_buf.addr_p + curr_ptr

                logging.debug("   BDLE#%02d:  ADDR: %s  LEN: %s  IOC: %s"
                              % (i, bdle_addr, bdle_len, bdle_ioc))
                break
            curr_ptr += plat_def.DMA_PAGE_SIZE
            logging.debug("   BDLE#%02d:  ADDR: %s  LEN: %s  IOC: %s"
                          % (i, bdle_addr, bdle_len, bdle_ioc))

    def free(self):
        if self.buf:
            self.drv.free_mem(self.buf.mem)
            self.buf = None


class StreamDesc:
    """ Class for Stream Descriptor """

    def __init__(self, idx, dev):
        self.idx = idx
        self.dev = dev
        self.used = False
        self.buf = None
        self.bdl = None

        offset = regs_def.HDA_SD_BASE + (regs_def.HDA_SD_SIZE * (idx))
        # Registers in SD
        self.ctl_sts = Register(self.dev.hda_bar_mmap,
                                offset + regs_def.HDA_SD_CS)
        self.lpib = Register(self.dev.hda_bar_mmap,
                             offset + regs_def.HDA_SD_LPIB)
        self.cbl = Register(self.dev.hda_bar_mmap,
                            offset + regs_def.HDA_SD_CBL)
        self.lvi = Register(self.dev.hda_bar_mmap,
                            offset + regs_def.HDA_SD_LVI, c_uint16)
        self.fifow = Register(self.dev.hda_bar_mmap,
                              offset + regs_def.HDA_SD_FIFOW, c_uint16)
        self.fifos = Register(self.dev.hda_bar_mmap,
                              offset + regs_def.HDA_SD_FIFOS, c_uint16)
        self.fmt = Register(self.dev.hda_bar_mmap,
                            offset + regs_def.HDA_SD_FMT, c_uint16)
        self.fifol = Register(self.dev.hda_bar_mmap,
                              offset + regs_def.HDA_SD_FIFOL)
        self.bdlplba = Register(self.dev.hda_bar_mmap,
                                offset + regs_def.HDA_SD_BDLPLBA)
        self.bdlpuba = Register(self.dev.hda_bar_mmap,
                                offset + regs_def.HDA_SD_BDLPUBA)

        # Register for SPIB
        offset = regs_def.HDA_SPBF_SD_BASE + (regs_def.HDA_SPBF_SD_SIZE * (idx))
        self.sdspib = Register(self.dev.hda_bar_mmap,
                               offset + regs_def.HDA_SPBF_SDSPIB)

    def free_memory(self):
        if self.buf is not None:
            self.buf.free()
            self.buf = None
        if self.bdl is not None:
            self.bdl.free()
            self.bdl = None

    def alloc_memory(self, size):
        self.free_memory()
        self.buf = DmaBuf(self.dev.drv, size)
        self.bdl = DmaBufDescList(self.dev.drv, self.buf)

    def config(self):
        self.bdlplba.value = self.bdl.buf.addr_p & 0xFFFFFFFF
        self.bdlpuba.value = self.bdl.buf.addr_p >> 32
        self.cbl.value = self.buf.size
        self.lvi.value = self.bdl.bd_count - 1
        self.sdspib.value = self.cbl.value

    def set_bitrate(self, bit_rate):
        logging.debug("SD#%02d: Set Bitrate: 0x%04X" % (self.idx, bit_rate))
        sdfmt = self.fmt.value
        sdfmt |= (bit_rate << 4)
        self.fmt.value = sdfmt
        logging.debug("SD#%02d: SD_FMT=%s" % (self.idx, self.fmt))

    def set_stream_id(self, stream_id):
        logging.debug("SD#%02d: Set Stream ID: 0x%04X" % (self.idx, stream_id))
        sd_ctl = self.ctl_sts.value
        sd_ctl &= ~(0xF << 20)
        sd_ctl |= (stream_id << 20)
        self.ctl_sts.value = sd_ctl
        logging.debug("SD#%02d: SD_CTL_STS=%s" % (self.idx, self.ctl_sts))

    def set_traffic_priority(self, value):
        logging.debug("SD#%02d: Set Traffic Priority(0x%02X)" % (self.idx, value))
        sd_ctl = self.ctl_sts.value
        sd_ctl |= (value << 18)
        self.ctl_sts.value = sd_ctl
        logging.debug("SD#%02d: SD_CTL_STS=%s" % (self.idx, self.ctl_sts))

    def start(self):
        """ Start DMA transfer """
        logging.debug("SD#%02d: Start DMA stream" % self.idx)
        self.ctl_sts.value = self.ctl_sts.value | (1 << 1)
        logging.debug("SD#%02d: SD_CTL_STS=%s" % (self.idx, self.ctl_sts))
        while self.ctl_sts.value & (1 << 1) == 0:
            sleep(0.001)

    def pause(self):
        logging.debug("SD#%02d: Pause DMA stream" % self.idx)
        self.ctl_sts.value = self.ctl_sts.value & ~(1 << 1)
        while self.ctl_sts.value & (1 << 1):
            sleep(0.001)

    def reset(self):
        logging.debug("SD#%02d: Reset DAM stream" % self.idx)
        self.pause()
        self.ctl_sts.value = self.ctl_sts.value | 1
        sleep(0.01)
        self.ctl_sts.value = self.ctl_sts.value & ~1
        sleep(0.01)


class StreamDescList:
    """ Class for DMA Stream Descriptor List """

    def __init__(self, dev):
        self.dev = dev
        self.sdl = []
        for i in range(plat_def.NUM_STREAMS):
            sd = StreamDesc(i, self.dev)
            self.sdl.append(sd)

    def close(self):
        for sd in self.sdl:
            if sd.used:
                self.release(sd)

    def reset_all(self):
        for sd in self.sdl:
            sd.reset()

    def get_sd(self, idx):
        sd = self.sdl[idx]
        if sd.used:
            raise ResourceWarning("IOB #%d already in use!" % idx)
        sd.used = True
        return sd

    def release(self, sd):
        return self.release_sd(sd.idx)

    def release_sd(self, idx, reset_hw=True):
        if not self.sdl[idx].used:
            logging.warning("SD#%d: Not used!!!" % idx)
        self.sdl[idx].used = False
        if reset_hw:
            self.sdl[idx].pause()
            self.sdl[idx].reset()
        self.sdl[idx].free_memory()
