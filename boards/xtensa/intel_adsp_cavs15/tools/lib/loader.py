#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging
import array

from lib.stream_desc import StreamDescList
from lib.device import Device
from lib.ipc import Ipc
import lib.registers as regs_def
import lib.platforms as plat_def


class FirmwareStatus():
    """ Data structure for Firmware Status register """

    def __init__(self, value=None):
        self.value = None
        self.boot_state = None
        self.wait_state = None
        self.moudle = None
        self.error = None

        if value:
            self.set(value)

    def set(self, value):
        self.value = value
        self.boot_state = self.value & plat_def.FW_STATUS_BOOT_STATE
        self.wait_state = ((self.value & plat_def.FW_STATUS_WAIT_STATE)
                            >> plat_def.FW_STATUS_WAIT_STATE_OFFSET)
        self.moudle = ((self.value & plat_def.FW_STATUS_MODULE)
                        >> plat_def.FW_STATUS_MODULE_OFFSET)
        self.error = ((self.value & plat_def.FW_STATUS_ERROR)
                       >> plat_def.FW_STATUS_ERROR_OFFSET)

    def __str__(self):
        return "0x%08X" % self.value

    def print(self):
        output = ("Firmware Status Register (%s)\n"
                  "   Boot:   0x%06X (%s)\n"
                  "   Wait:   0x%02X (%s)\n"
                  "   Module: 0x%02X\n"
                  "   Error:  0x%02X" %
                  (self,
                   self.boot_state, plat_def.BOOT_STATUS_STR(self.boot_state),
                   self.wait_state, plat_def.WAIT_STATUS_STR(self.wait_state),
                   self.moudle, self.error))
        logging.info(output)

class FirmwareLoader():

    def __init__(self):
        self._init_done = False
        self.dma_id = None
        self.dev = None
        self.sdl = None

    def init(self, dma_id):
        if self._init_done:
            logging.warning("Already initialized! Skip init")
            return

        self.dma_id = dma_id
        self.dev = Device()
        self.dev.open_device()
        self.sdl = StreamDescList(self.dev)
        self.ipc = Ipc(self.dev)
        self._init_done = True

    def close(self):
        if not self._init_done:
            logging.warning("Not initialized! Skip closing.")
            return

        self.sdl.close()
        self.dev.close()
        self._init_done = False

    def reset_dsp(self):
        logging.debug(">>> FirmwareLoader.reset_dsp()")
        logging.debug("Recycling controller power...")
        self.dev.power_cycle()

        # This should be enabled prior to power down the cores.
        self.dev.enable_proc_pipe_ctl()

        logging.debug("Power down cores...")
        self.dev.core_stall_reset(plat_def.CORE_MASK)
        self.dev.core_power_down(plat_def.CORE_MASK)
        logging.debug("<<< FirmwareLoader.reset_dsp()")

    def boot_dsp(self):
        logging.debug(">>> FirmwareLoader.boot_dsp()")
        self.dev.enable_proc_pipe_ctl()
        self.sdl.reset_all()
        self.dev.core_power_up(0x1)
        self.dev.dsp_hipct.value = self.dev.dsp_hipct.value

        logging.debug("Purging pending FW request")
        boot_config = plat_def.FW_IPC_PURGE | regs_def.ADSP_IPC_HIPCI_BUSY
        boot_config = boot_config | ((self.dma_id - 7) << 9)
        self.dev.dsp_hipci.value = boot_config
        time.sleep(0.1)
        logging.debug("   DSP_HIPCI=%s" % self.dev.dsp_hipci)

        self.dev.core_power_up(plat_def.CORE_MASK)
        self.dev.core_run(plat_def.CORE_0)
        self.dev.core_run(plat_def.CORE_1)
        logging.debug("Wait for IPC DONE bit from ROM")
        while True:
            ipc_ack = self.dev.dsp_hipcie.value
            if (ipc_ack & (1 << regs_def.ADSP_IPC_HIPCIE_DONE_OFFSET)) != 0:
                break
            time.sleep(0.1)
        logging.debug("<<< FirmwareLoader.boot_dsp()")

    def check_fw_boot_status(self, expected):
        logging.debug(">>> FirmwareLoader.check_fw_boot_status(0x%08X)" % expected)
        raw_status = self.dev.fw_status.value
        FirmwareStatus(raw_status).print()

        if (raw_status & plat_def.FW_STATUS_ERROR) != 0:
            output = ("Firmware Status error:"
                      "   Status:     0x%08X\n"
                      "   Error Code  0x%08X" %
                      (raw_status, self.dev.fw_err_code.value))
            raise RuntimeError(output)
        status = raw_status & plat_def.FW_STATUS_BOOT_STATE
        logging.debug("<<< FirmwareLoader.check_fw_boot_status()")
        return status == expected

    def wait_for_fw_boot_status(self, boot_status):
        logging.debug("Waiting for FW Boot Status: 0x%08X (%s)"
                      % (boot_status,
                         plat_def.BOOT_STATUS_STR(boot_status)))

        for _ in range(10):
            reg = self.dev.fw_status.value
            bs = reg & plat_def.FW_STATUS_BOOT_STATE
            if bs == boot_status:
                logging.debug("Received Expected Boot Status")
                return True
            time.sleep(0.01)
        logging.error("Failed to receive expected status")
        return False

    def wait_for_fw_wait_status(self, wait_status):
        logging.debug("Waiting for FW Wait Status: 0x%08X (%s)"
                      % (wait_status,
                         plat_def.WAIT_STATUS_STR(wait_status)))
        for _ in range(10):
            reg = self.dev.fw_status.value
            ws = reg & plat_def.FW_STATUS_WAIT_STATE
            if ws == (wait_status << plat_def.FW_STATUS_WAIT_STATE_OFFSET):
                logging.debug("Received Expected Wait Status")
                return True
            time.sleep(0.01)
        logging.error("Failed to receive expected status")
        return False

    def load_firmware(self, fw_file):
        logging.debug(">>> FirmwareLoader.load_firmware()")
        with open(fw_file, "rb") as fd:
            data = array.array('B', fd.read())
            sd = self.sdl.get_sd(self.dma_id)
            sd.enable = True
            sd.alloc_memory(len(data))
            sd.buf.copy(data, len(data))
            sd.config()
            sd.set_stream_id(1)
            sd.set_traffic_priority(1)
            sd.set_bitrate(0x4)
            time.sleep(0.1)
            logging.debug("<<< FirmwareLoader.load_firmware()")
            return sd

    def download_firmware(self, fw_file):
        logging.debug(">>> FirmwareLoader.download_firmware(fw_file=%s)" % fw_file)

        # Load firmware to DMA buffer and update SD and SDL
        sd = self.load_firmware(fw_file)
        try:
            self.dev.hda_spibe.value |= (1 << self.dma_id)
            self.wait_for_fw_wait_status(plat_def.WAIT_STATUS_DMA_BUFFER_FULL)

            logging.info("Start firmware downloading...")
            sd.start()
            time.sleep(0.5)
            self.wait_for_fw_boot_status(plat_def.BOOT_STATUS_FW_ENTERED)
        finally:
            sd.pause()
            sd.reset()
            self.sdl.release_sd(sd.idx)
            self.dev.hda_spibe.value = 0

        logging.debug("<<< FirmwareLoader.download_firmware()")
