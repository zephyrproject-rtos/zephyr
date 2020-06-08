#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
import time
import logging
from ctypes import  c_uint16, addressof

from lib.driver import DiagDriver, Register
import lib.registers as regs_def
import lib.platforms as plat_def


class Device:

    def __init__(self):
        self.__opened = False

        self.drv = DiagDriver()
        self.dev_info = None

        self.hda_bar_mmap = None
        self.dsp_bar_mmap = None

        self.hda_gctl = None
        self.hda_gcap = None
        self.hda_ppctl = None
        self.hda_spibe = None

        self.dsp_ctl_sts = None
        self.dsp_hipci = None
        self.dsp_hipcie = None
        self.dsp_hipct = None

        self.fw_status = None
        self.fw_err_code = None

        self.ipc_len = None
        self.ipc_cmd = None

        self.allocated = []

    def close(self):
        if not self.__opened:
            logging.warning("Audio device not opened!!!")
            return
        self.__opened = False

    def open_device(self):
        logging.debug(">>> Device.open_device()")

        # Open device to get HDA BAR and DSP BAR
        self.dev_info = self.drv.open_device()

        # HDA MMAP
        self.hda_bar_mmap = self.drv.mmap(self.dev_info.hda_bar.base_p,
                                          self.dev_info.hda_bar.size)
        self.dev_info.hda_bar.base_v = addressof(self.hda_bar_mmap)
        # DSP MMAP
        self.dsp_bar_mmap = self.drv.mmap(self.dev_info.dsp_bar.base_p,
                                          self.dev_info.dsp_bar.size)
        self.dev_info.dsp_bar.base_v = addressof(self.dsp_bar_mmap)
        logging.debug(self.dev_info)

        # Registers from HDA
        self.hda_gctl = Register(self.hda_bar_mmap,
                                 regs_def.HDA_GR_GCTL)
        self.hda_gcap = Register(self.hda_bar_mmap,
                                 regs_def.HDA_GR_GCAP, c_uint16)
        self.hda_ppctl = Register(self.hda_bar_mmap,
                                  regs_def.HDA_PPC_PPCTL)
        self.hda_spibe = Register(self.hda_bar_mmap,
                                  regs_def.HDA_SPBF_SPBFCTL)
        # Registers from DSP
        self.dsp_ctl_sts = Register(self.dsp_bar_mmap,
                                    regs_def.ADSP_GR_ADSPCS)
        self.dsp_hipci = Register(self.dsp_bar_mmap,
                                  regs_def.ADSP_IPC_HIPCI)
        self.dsp_hipcie = Register(self.dsp_bar_mmap,
                                   regs_def.ADSP_IPC_HIPCIE)
        self.dsp_hipct = Register(self.dsp_bar_mmap,
                                  regs_def.ADSP_IPC_HIPCT)
        self.fw_status = Register(self.dsp_bar_mmap,
                                  plat_def.FW_STATUS)
        self.fw_err_code = Register(self.dsp_bar_mmap,
                                    plat_def.FW_ERR_CODE)
        self.ipc_len = Register(self.dsp_bar_mmap,
                                plat_def.FW_MBOX_UPLINK + plat_def.IPC_GLOBAL_LEN)
        self.ipc_cmd = Register(self.dsp_bar_mmap,
                                plat_def.FW_MBOX_UPLINK + plat_def.IPC_GLOBAL_CMD)

        self.__opened = True
        logging.debug("<<< Device.open_device()")

    def alloc_memory(self, size):
        logging.debug(">>> Device.alloc_memory()")
        buf = self.drv.alloc_mem(size)
        if buf.dma_addr_p == 0:
            raise RuntimeError("Could not allocate the memory")
        self.allocated.append(buf)
        logging.debug("<<< Device.alloc_memory()")
        return buf

    def free_memory(self, mem):
        logging.debug(">>> Device.free_memory()")
        if mem in self.allocated:
            ret = self.drv.free_mem(mem)
            if ret != 0:
                logging.error("Failed to free memory")
            self.allocated.remove(mem)
        else:
            logging.warning("Cannot find the memory from list")
        logging.debug("<<< Device.free_memory()")

    def power_cycle(self):
        logging.debug("Controller power down")
        self.hda_gctl.value = 0
        while self.hda_gctl.value != 0:
            time.sleep(0.1)
        logging.debug("   HDA_GCTL=%s" % self.hda_gctl)

        logging.debug("Controller power up")
        self.hda_gctl.value = 1
        while self.hda_gctl.value != 1:
            time.sleep(0.1)
        logging.debug("   HDA_GCTL=%s" % self.hda_gctl)

    def enable_proc_pipe_ctl(self):
        logging.debug("Enable processing pipe control")
        iss = ((self.hda_gcap.value & regs_def.HDA_GR_GCAP_ISS)
                >> regs_def.HDA_GR_GCAP_ISS_OFFSET)
        oss = ((self.hda_gcap.value & regs_def.HDA_GR_GCAP_OSS)
                >> regs_def.HDA_GR_GCAP_OSS_OFFSET)

        iss_mask = int("1" * iss, 2)
        oss_mask = int("1" * oss, 2)

        dma_mask = iss_mask + (oss_mask << iss)

        # Enable processing pipe
        self.hda_ppctl.value = self.hda_ppctl.value | 0x40000000 | dma_mask
        logging.debug("   HDA_PPCTL=%s" % self.hda_ppctl)

    def get_ipc_message(self):
        logging.info("Read IPC message from DSP")
        logging.info("IPC LEN: %s" % self.ipc_len)
        logging.info("IPC CMD: %s" % self.ipc_cmd)

    def core_reset_enter(self, core_mask):
        # Set Reset Bit for cores
        logging.debug("Enter core reset(mask=0x%08X)" % core_mask)

        reset = core_mask << regs_def.ADSP_GR_ADSPCS_CRST_OFFSET
        self._update_bits(self.dsp_ctl_sts, reset, reset)

        # Check core entered reset
        reg = self.dsp_ctl_sts.value
        if (reg & reset) != reset:
            raise RuntimeError("Reset enter failed: DSP_CTL_STS=%s core_maks=0x%08X"
                               % (self.dsp_ctl_sts, core_mask))
        logging.debug("   DSP_CTL_STS=%s" % self.dsp_ctl_sts)

    def core_reset_leave(self, core_mask):
        # Set Reset Bit for cores
        logging.debug("Leave core reset(mask=0x%08X)" % core_mask)

        leave = core_mask << regs_def.ADSP_GR_ADSPCS_CRST_OFFSET
        self._update_bits(self.dsp_ctl_sts, leave, 0)

        # Check core entered reset
        reg = self.dsp_ctl_sts.value
        if (reg & leave) != 0:
            raise RuntimeError("Reset leave failed: DSP_CTL_STS=%s core_maks=0x%08X"
                               % (self.dsp_ctl_sts, core_mask))
        logging.debug("   DSP_CTL_STS=%s" % self.dsp_ctl_sts)

    def core_stall_reset(self, core_mask):
        logging.debug("Stall core(mask=0x%08X)" % core_mask)
        stall = core_mask << regs_def.ADSP_GR_ADSPCS_CSTALL_OFFSET
        self._update_bits(self.dsp_ctl_sts, stall, stall)
        logging.debug("   DSP_CTL_STS=%s" % self.dsp_ctl_sts)
        self.core_reset_enter(core_mask)

    def core_run(self, core_mask):
        self.core_reset_leave(core_mask)

        logging.debug("Run/Unstall core(mask=0x%08X)" % core_mask)
        run = core_mask << regs_def.ADSP_GR_ADSPCS_CSTALL_OFFSET
        self._update_bits(self.dsp_ctl_sts, run, 0)
        logging.debug("   DSP_CTL_STS=%s" % self.dsp_ctl_sts)

    def core_power_down(self, core_mask):
        logging.debug("Power down core(mask=0x%08X)" % core_mask)
        mask = core_mask << regs_def.ADSP_GR_ADSPCS_SPA_OFFSET
        self._update_bits(self.dsp_ctl_sts, mask, 0)

        cnt = 0
        while cnt < 10:
            cpa = self.dsp_ctl_sts.value & regs_def.ADSP_GR_ADSPCS_CPA
            mask = (core_mask & 0) << regs_def.ADSP_GR_ADSPCS_CPA_OFFSET
            if cpa == mask:
                logging.debug("Confirmed match value: 0x%04X" % cpa)
                break
            time.sleep(0.01)
            cnt += 1

        if cnt == 10:
            logging.error("   DSP_CTL_STS=%s" % self.dsp_ctl_sts)
            raise RuntimeError("Failed to power down the core")
        logging.debug("   DSP_CTL_STS=%s" % self.dsp_ctl_sts)

    def core_power_up(self, core_mask):
        logging.debug("Power Up core(mask=0x%08X)" % core_mask)
        mask = core_mask << regs_def.ADSP_GR_ADSPCS_SPA_OFFSET
        self._update_bits(self.dsp_ctl_sts, mask, mask)

        cnt = 0
        while cnt < 10:
            cpa = self.dsp_ctl_sts.value & regs_def.ADSP_GR_ADSPCS_CPA
            mask = core_mask << regs_def.ADSP_GR_ADSPCS_CPA_OFFSET

            if cpa == mask:
                logging.debug("Confirmed match value: 0x%04X" % cpa)
                break
            time.sleep(0.01)
            cnt += 1

        if cnt == 10:
            logging.error("   DSP_CTL_STS=%s" % self.dsp_ctl_sts)
            raise RuntimeError("Failed to power up the core")

        logging.debug("   DSP_CTL_STS=%s" % self.dsp_ctl_sts)

    @staticmethod
    def _update_bits(reg, mask, value):

        old_val = reg.value
        new_val = (old_val & ~mask) | (value & mask)

        if old_val == new_val:
            return False

        reg.value = new_val
        return True
