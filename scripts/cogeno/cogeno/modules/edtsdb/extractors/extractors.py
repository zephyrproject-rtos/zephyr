#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import ExtractorsBase
from .clocks import DTClocks
from .compatible import DTCompatible
from .controller import DTController
from .default import DTDefault
from .dma import DTDMA
from .flash import DTFlash
from .gpio import DTGpio
from .heuristics import DTHeuristics
from .interrupts import DTInterrupts
from .pinctrl import DTPinCtrl
from .pwm import DTPWM
from .reg import DTReg

##
# @brief Pool of extractors for device tree data extraction.
#
class Extractors(ExtractorsBase):

    def __init__(self, edts, dts, bindings):
        super().__init__(edts, dts, bindings)
        # init all extractors
        self.clocks = DTClocks(self)
        self.compatible = DTCompatible(self)
        self.controller = DTController(self)
        self.default = DTDefault(self)
        self.dma = DTDMA(self)
        self.flash = DTFlash(self)
        self.gpio = DTGpio(self)
        self.heuristics = DTHeuristics(self)
        self.interrupts = DTInterrupts(self)
        self.pinctrl = DTPinCtrl(self)
        self.pwm = DTPWM(self)
        self.reg = DTReg(self)

    # convenience functions

    def extract_clocks(self, node_address, prop):
        self.clocks.extract(node_address, prop)

    def extract_compatible(self, node_address, prop):
        self.compatible.extract(node_address, prop)

    def extract_controller(self, node_address, prop):
        self.controller.extract(node_address, prop)

    def extract_default(self, node_address, prop):
        self.default.extract(node_address, prop)

    def extract_dma(self, node_address, prop):
        self.dma.extract(node_address, prop)

    def extract_flash(self, node_address, prop):
        self.flash.extract(node_address, prop)

    def extract_gpio(self, node_address, prop):
        self.gpio.extract(node_address, prop)

    def extract_heuristics(self, node_address, prop):
        self.heuristics.extract(node_address, prop)

    def extract_interrupts(self, node_address, prop):
        self.interrupts.extract(node_address, prop)

    def extract_pinctrl(self, node_address, prop):
        self.pinctrl.extract(node_address, prop)

    def extract_pwm(self, node_address, prop):
        self.pwm.extract(node_address, prop)

    def extract_reg(self, node_address, prop):
        self.reg.extract(node_address, prop)
