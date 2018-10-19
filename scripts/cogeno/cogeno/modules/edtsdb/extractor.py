#
# Copyright (c) 2017, Linaro Limited
# Copyright (c) 2018, Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

import re
from copy import deepcopy

from .binder import Binder
from .dts import DTS
from .extractors.extractors import Extractors

##
# @brief ETDS Database extractor
#
# Methods for ETDS database extraction from DTS.
#
class EDTSExtractorMixin(object):
    __slots__ = []

    def extractors(self):
        return self._extractors

    def _extract_property(self, node_address, prop):
        extractors = self.extractors()

        if prop == 'reg':
            if 'partition@' in node_address:
                # reg in partition is covered by flash handling
                extractors.extract_flash(node_address, prop)
            else:
                extractors.extract_reg(node_address, prop)
        elif prop == 'interrupts' or prop == 'interrupts-extended':
            extractors.extract_interrupts(node_address, prop)
        elif prop == 'compatible':
            extractors.extract_compatible(node_address, prop)
        elif '-controller' in prop:
            extractors.extract_controller(node_address, prop)
        elif 'pinctrl-' in prop:
            extractors.extract_pinctrl(node_address, prop)
        elif prop.startswith(('gpio', )) or 'gpios' in prop:
            extractors.extract_gpio(node_address, prop)
        elif prop.startswith(('clock', '#clock', 'assigned-clock', 'oscillator')):
            extractors.extract_clocks(node_address, prop)
        elif prop.startswith(('dma', '#dma')):
            extractors.extract_dma(node_address, prop)
        elif prop.startswith(('pwm', '#pwm')):
            extractors.extract_pwm(node_address, prop)
        else:
            extractors.extract_default(node_address, prop)

    def _extract_node(self, root_node_address, sub_node_address,
                      sub_node_binding):
        dts = self.extractors().dts()
        bindings = self.extractors().bindings()
        extractors = self.extractors()

        node = dts.reduced[sub_node_address]
        node_compat = dts.get_compat(root_node_address)

        if node_compat not in bindings:
            return

        if sub_node_binding is None:
            node_binding = bindings[node_compat]
        else:
            node_binding = sub_node_binding

        # Do extra property definition based on heuristics
        extractors.extract_heuristics(sub_node_address, None)

        # check to see if we need to process the properties
        for binding_prop, binding_prop_value in node_binding['properties'].items():
            if 'properties' in binding_prop_value:
                for node_address in dts.reduced:
                    if root_node_address + '/' in node_address:
                        # subnode detected
                        self._extract_node(root_node_address, node_address,
                                           binding_prop_value)
            # We ignore 'generation' as we dot not create defines
            # and extract all properties that are:
            # - not marked by the extractors to be ignored and are
            # - in the binding
            for prop in node['props']:
                # if prop is in filter list - ignore it
                if prop in extractors.ignore_properties:
                    continue

                if re.match(binding_prop + '$', prop):
                    self._extract_property(sub_node_address, prop)

    def _extract_specification(self):
        dts = self.extractors().dts()
        bindings = self.extractors().bindings()
        extractors = self.extractors()

        # extract node info for devices
        for node_address in dts.reduced:
            node_compat = dts.get_compat(node_address)
            # node or parent node has a compat, it's a device
            if node_compat is not None and node_compat in bindings:
                self._extract_node(node_address, node_address, None)

        # extract chosen node info
        for k, v in dts.regs_config.items():
            if k in dts.chosen:
                extractors.default.edts_insert_chosen(k, dts.chosen[k])
                #extractors.extract_reg(dts.chosen[k], None, None, v, 1024)
        for k, v in dts.name_config.items():
            if k in dts.chosen:
                extractors.default.edts_insert_chosen(k, dts.chosen[k])

        node_address = dts.chosen.get('zephyr,flash', 'dummy-flash')
        extractors.extract_flash(node_address, 'zephyr,flash')
        node_address = dts.chosen.get('zephyr,code-partition', node_address)
        extractors.extract_flash(node_address, 'zephyr,code-partition')

    ##
    # @brief Extract DTS info to database
    #
    # @param dts_file_path DTS file
    # @param bindings_paths YAML file directories, we allow multiple
    #
    def extract(self, dts_file_path, bindings_paths):

        dts = DTS(dts_file_path)

        # Extract bindings of all compatibles
        bindings = Binder.bindings(dts.compatibles, bindings_paths)
        if bindings == {}:
            raise Exception("Missing Bindings information. Check YAML sources")

        # Create extractors
        self._extractors = Extractors(self, dts, bindings)

        # Extract device tree specification
        self._extract_specification()
