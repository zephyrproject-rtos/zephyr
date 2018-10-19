#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from ..dts import *
from ..binder import *

##
# @brief Base class for pool of extractors for device tree data extraction.
#
class ExtractorsBase(object):

    def __init__(self, edts, dts, bindings):
        self._edts = edts
        self._dts = dts
        self._bindings = bindings
        # Extractors
        self.clocks = None
        self.compatible = None
        self.controller = None
        self.default = None
        self.dma = None
        self.flash = None
        self.gpioranges = None
        self.gpios = None
        self.heuristics = None
        self.interrupts = None
        self.pinctrl = None
        self.pwm = None
        self.reg = None
        ##
        # Properties that the extractors want to be ignored
        # because they are unnecessary or are covered by the
        # extraction of other properties
        self.ignore_properties = []

    # convenience functions

    def dts(self):
        return self._dts

    def edts(self):
        return self._edts

    def bindings(self):
        return self._bindings

##
# @brief Base class for device tree directives extraction to edts.
#
class DTDirective(object):

    def __init__(self, extractors):
        ## The extractor pool this extractor belongs to
	    self._extractors = extractors

    def dts(self):
        return self._extractors._dts

    def edts(self):
        return self._extractors._edts

    def bindings(self):
        return self._extractors._bindings

    def extractors(self):
        return self._extractors

    ##
    # @brief Get EDTS device id associated to node address.
    #
    # @return ETDS device id
    def edts_device_id(self, node_address):
        return node_address


    def edts_insert_chosen(self, chosen, node_address):
        self.edts().insert_chosen(chosen, self.edts_device_id(node_address))

    ##
    # @brief Insert device property into EDTS
    #
    def edts_insert_device_property(self, node_address, property_path,
                                    property_value):

        device_id = self.edts_device_id(node_address)
        self.edts().insert_device_property(device_id, property_path, property_value)

    ##
    # @brief Insert device controller into EDTS
    #
    def edts_insert_device_controller(self, controller_address, node_address,
                                      line):
        device_id = self.edts_device_id(node_address)
        controller_id = self.edts_device_id(controller_address)
        controller_type = self.dts().get_controller_type(controller_address)
        self.edts().insert_device_controller(controller_type, controller_id,
                                            device_id, line)

    ##
    # @brief Extract directive information.
    #
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):
        pass
