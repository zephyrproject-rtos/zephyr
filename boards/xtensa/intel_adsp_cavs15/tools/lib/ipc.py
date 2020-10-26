#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import logging
from ctypes import Structure, c_uint64, c_uint32, c_uint16, \
                   c_uint8, addressof, sizeof

import lib.platforms as plat_def

# Global Message - Generic
IPC_GLB_TYPE_OFFSET     = 28
IPC_GLB_TYPE_MASK       = (0xF << IPC_GLB_TYPE_OFFSET)

# Command Message
IPC_CMD_TYPE_OFFSET     = 16
IPC_CMD_TYPE_MASK       = (0xFFF << IPC_CMD_TYPE_OFFSET)

# Message Type
IPC_FW_READY            = 0x07 << IPC_GLB_TYPE_OFFSET

# Extended data types that can be appended onto end of IpCFwReady message
IpcExtData = {
    0: 'IPC_EXT_DMA_BUFFER',
    1: 'IPC_EXT_WINDOW'
}

# Extended Firmware Data
IpcRegion = {
    0: 'IPC_REGION_DOWNBOX',
    1: 'IPC_REGION_UPBOX',
    2: 'IPC_REGION_TRACE',
    3: 'IPC_REGION_DEBUG',
    4: 'IPC_REGION_STREAM',
    5: 'IPC_REGION_REGS',
    6: 'IPC_REGION_EXCEPTION'
}


class IpcHdr(Structure):
    """ Data structure for IPC Header """
    _pack_ = 1
    _fields_ = [
        ("size", c_uint32)
    ]

    def __str__(self):
        output = ("Size:   0x%08X" % self.size)

        return output


class IpcCmdHdr(Structure):
    """ Data structure for IPC Command Header """
    _pack_ = 1
    _fields_ = [
        ("size", c_uint32),
        ("cmd", c_uint32)
    ]

    def __str__(self):
        output = (
            "IPC Command Hdr:\n"
            "   Command:    {cmd:>10} (0x{cmd:0>8X})\n"
            "   Size:       {size:>10} (0x{size:0>4X})"
            ).format(
                size = self.size,
                cmd = self.cmd
            )
        return output


class IpcFwVersion(Structure):
    """ Data structure for IPC FwVersion message """
    _pack_ = 1
    _fields_ = [
        ("hdr", IpcHdr),
        ("major", c_uint16),
        ("minor", c_uint16),
        ("micro", c_uint16),
        ("build", c_uint16),
        ("date", c_uint8 * 12),
        ("time", c_uint8 * 10),
        ("tag", c_uint8 * 6),
        ("abi_version", c_uint32),
        ("reserved", c_uint32 * 4)
    ]

    def __str__(self):
        output = (
            "Firmware Version:\n"
            "   Major version:  {major: >5} (0x{major:0>4X})\n"
            "   Minor version:  {minor: >5} (0x{minor:0>4X})\n"
            "   Micro number:   {micro: >5} (0x{micro:0>4X})\n"
            "   Build number:   {build: >5} (0x{build:0>4X})\n"
            "   Date: {date: >24}\n"
            "   Time:  {time: >24}\n"
            "   Tag:  {tag: >24}\n"
            "   Abi version: {abi_version: >5} (0x{abi_version:0>4X})"
            ).format(
                major = self.major,
                minor = self.minor,
                micro = self.micro,
                build = self.build,
                date=''.join([chr(i) for i in list(self.date)]),
                time=''.join([chr(i) for i in list(self.time)]),
                tag=''.join([chr(i) for i in list(self.tag)]),
                abi_version = self.abi_version)
        return output


class IpcFwReady(Structure):
    """ Data structure for IpcFwReady message """
    _pack_ = 1
    _fields_ = [
        ("hdr", IpcCmdHdr),
        ("dspbox_offset", c_uint32),
        ("hostbox_offset", c_uint32),
        ("dspbox_size", c_uint32),
        ("hostbox_size", c_uint32),
        ("version", IpcFwVersion),
        ("flags", c_uint64),
        ("reserved", c_uint32 * 4)
    ]

    def __str__(self):
        output = (
            "IPC Firmware Ready Message: (0x{cmd:0>8X}) (0x{size:0>8X})\n"
            "   DSP box offset: {dsp_offset: >5} (0x{dsp_offset:0>4X})\n"
            "   Host box offset:{host_offset: >5} (0x{host_offset:0>4X})\n"
            "   DSP box size:   {dsp_size: >5} (0x{dsp_size:0>4X})\n"
            "   Host box size:  {host_size: >5} (0x{host_size:0>4X})\n\n"
            "{version}\n\n"
            "Flags:"
            "   0x{flags:0>8X}"
            ).format(
                cmd = self.hdr.cmd,
                size = self.hdr.size,
                dsp_offset = self.dspbox_offset,
                host_offset = self.hostbox_offset,
                dsp_size = self.dspbox_size,
                host_size = self.hostbox_size,
                version = str(self.version),
                flags = self.flags
            )
        return output


class IpcExtDataHdr(Structure):
    """ Data structure for IPC extended data header """
    _pack_ = 1
    _fields_ = [
        ("hdr", IpcCmdHdr),
        ("type", c_uint32)
    ]


class IpcWindowElem(Structure):
    """ Data structure for Window Element message """
    _pack_ = 1
    _fields_ = [
        ("hdr", IpcHdr),
        ("type", c_uint32),
        ("id", c_uint32),
        ("flags", c_uint32),
        ("size", c_uint32),
        ("offset", c_uint32)
    ]

    def __str__(self):
        output = (
            "Window type: {type_str:>20} ({type:d})\n"
            "Window id:         {id: >5}\n"
            "Window flags:      {flags: >5} (0x{flags:0>4X})\n"
            "Window size:       {size: >5} (0x{size:0>4X})\n"
            "Window offset:     {offset: >5} (0x{offset:0>4X})\n"
            ).format(
                type_str = IpcRegion[self.type],
                type = self.type,
                id = self.id,
                flags = self.flags,
                size = self.size,
                offset = self.offset
            )
        return output


class IpcWindow(Structure):
    """ Data structure for extended data memory windows """

    _pack_ = 1
    _fields_ = [
        ("ext_hdr", IpcExtDataHdr),
        ("num_windows", c_uint32),
    ]

    def __str__(self):
        output = ("\n"
            "IPC Firmware Ready Extended Message: (0x{cmd:0>8X}) (0x{size:0>8X})\n"
            "Message Type:      {ext_type: >5d} ({ext_type_str})\n\n"
            "Number of Windows:    {num_windows: >2d}\n"
        ).format(
            cmd = self.ext_hdr.hdr.cmd,
            size = self.ext_hdr.hdr.size,
            ext_type = self.ext_hdr.type,
            ext_type_str = IpcExtData[self.ext_hdr.type],
            num_windows = int(self.num_windows)
        )
        return output


class Ipc:
    """ Class for IPC handle """

    def __init__(self, dev):
        self.drv = dev.drv
        self.dsp_base_p = dev.dev_info.dsp_bar.base_p

        # memory map MBOX UPLINK
        self.mmap = self.drv.mmap(self.dsp_base_p + (plat_def.FW_MBOX_UPLINK),
                                  plat_def.FW_MBOX_SIZE)
        self.mmap_addr = addressof(self.mmap)

    def read_fw_ready(self):
        """ FwReady message consist of two messages:
        1. FwReady
        2. ExtendedData
        """
        msg = IpcFwReady.from_address(self.mmap_addr)
        logging.info(str(msg))
        self.read_fw_ready_ext()

    def read_fw_ready_ext(self):
        addr_offset = self.mmap_addr + sizeof(IpcFwReady())
        msg = IpcWindow.from_address(addr_offset)
        logging.info(str(msg))

        # Extended Windows data type
        if msg.ext_hdr.type != 1:
            raise RuntimeError("Not Implemented: ext_hdr.type != 1")

        win_elem_list = []
        addr_offset += sizeof(IpcWindow())
        num_win = int(msg.num_windows)
        for _ in range(num_win):
            win_elem = IpcWindowElem.from_address(addr_offset)
            win_elem_list.append(win_elem)
            addr_offset += sizeof(IpcWindowElem())

        for elem in win_elem_list:
            logging.info(str(elem))
