# Copyright (c) 2025 Realtek Semiconductor, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Runner for flashing with rtsflash."""

import subprocess
import sys
import time

import usb.core
import usb.util

from runners.core import RunnerCaps, ZephyrBinaryRunner

RTS_STANDARD_REQUEST = 0x0

RTS_STD_GET_STATUS = 0x1A
RTS_STD_SET_STATUS = 0x1B

RTS_VENDOR_REQUEST = 0x40
RTS_VREQ_XMEM_WRITE = 0x40
RTS_VREQ_XMEM_READ = 0x41


class RtsUsb:
    def __init__(self, device=None):
        if not device:
            idvendor = 0x0BDA
            idproduct = 0x5817
        else:
            idvendor = int(device.split(':', 1)[0], 16)
            idproduct = int(device.split(':', 1)[1], 16)
        dev = usb.core.find(idVendor=idvendor, idProduct=idproduct)
        if not dev:
            raise RuntimeError("device not found")
        self.dev = dev
        self.mode = ""
        self.get_mode()

    def __del__(self):
        usb.util.dispose_resources(self.dev)

    def enable_download(self):
        if self.mode == "ROM":
            status = self.dev.ctrl_transfer(
                RTS_STANDARD_REQUEST | 0x80, RTS_STD_GET_STATUS, 0, 0, 1
            )
            if int.from_bytes(status, byteorder="little") == 0:
                self.dev.ctrl_transfer(RTS_STANDARD_REQUEST, RTS_STD_SET_STATUS, 1, 0, 0)

    def xmem_write(self, addr, data):
        setup_value = addr & 0xFFFF
        setup_index = (addr >> 16) & 0xFFFF
        self.dev.ctrl_transfer(
            RTS_VENDOR_REQUEST, RTS_VREQ_XMEM_WRITE, setup_value, setup_index, data
        )

    def xmem_read(self, addr):
        setup_value = addr & 0xFFFF
        setup_index = (addr >> 16) & 0xFFFF
        ret = self.dev.ctrl_transfer(
            RTS_VENDOR_REQUEST | 0x80, RTS_VREQ_XMEM_READ, setup_value, setup_index, 4
        )
        return int.from_bytes(ret, byteorder="little")

    def get_mode(self):
        init_vector = self.xmem_read(0x401E2090)
        if init_vector == 0:
            self.mode = "ROM"
        elif init_vector == 0x01800000:
            self.mode = "IAP"
        else:
            raise ValueError("Unknown work mode")


class RtsflashBinaryRunner(ZephyrBinaryRunner):
    """Runner front-end for rtsflash."""

    def __init__(self, cfg, dev_id, alt, img, exe='dfu-util'):
        super().__init__(cfg)
        self.dev_id = dev_id
        self.alt = alt
        self.img = img
        self.cmd = [exe, f'-d {dev_id}']
        try:
            self.list_pattern = f', alt={int(self.alt)},'
        except ValueError:
            self.list_pattern = f', name="{self.alt}",'
        self.reset = False

    @classmethod
    def name(cls):
        return "rtsflash"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"flash"}, dev_id=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return 'USB VID:PID of the connected device.'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--alt", required=True, help="interface alternate setting number or name"
        )
        parser.add_argument("--pid", dest='dev_id', help=cls.dev_id_help())
        parser.add_argument("--img", help="binary to flash, default is --bin-file")
        parser.add_argument(
            '--dfu-util', default='dfu-util', help='dfu-util executable; defaults to "dfu-util"'
        )

    @classmethod
    def do_create(cls, cfg, args):
        if args.img is None:
            args.img = cfg.bin_file

        ret = RtsflashBinaryRunner(cfg, args.dev_id, args.alt, args.img, exe=args.dfu_util)

        ret.ensure_device()
        return ret

    def ensure_device(self):
        if not self.find_device():
            self.reset = True
            print('Please reset your board to switch to DFU mode...')
            while not self.find_device():
                time.sleep(0.1)

    def find_device(self):
        cmd = list(self.cmd) + ['-l']
        output = self.check_output(cmd)
        output = output.decode(sys.getdefaultencoding())
        return self.list_pattern in output

    def do_run(self, command, **kwargs):
        if not self.dev_id:
            raise RuntimeError(
                'Please specify a USB VID:PID with the -i/--dev-id or --pid command-line switch.'
            )

        self.require(self.cmd[0])
        self.ensure_output('bin')

        if not self.find_device():
            raise RuntimeError('device not found')

        fpusb = RtsUsb(self.dev_id)

        fpusb.enable_download()

        try:
            self.check_call(['dfu-suffix', '-c', self.img])
        except subprocess.CalledProcessError:
            self.call(['dfu-suffix', '-a', self.img])

        cmd = list(self.cmd)
        cmd.extend(['-a', self.alt, '-D', self.img])
        self.check_call(cmd)

        if self.reset:
            print('Now reset your board again to switch back to runtime mode.')
