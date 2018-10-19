#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

import sys
from pathlib import Path
from collections import defaultdict
from copy import deepcopy

from .devicetree.phandles import DTSPHandlesMixin
from .devicetree.compatibles import DTSCompatiblesMixin
from .devicetree.aliases import DTSAliasesMixin
from .devicetree.chosen import DTSChosenMixin
from .devicetree.reduced import DTSReducedMixin
from .devicetree.parser import DTSParserMixin

##
# class that holds the information that was read out
# of the device tree specification
#
class DTS(DTSPHandlesMixin, DTSCompatiblesMixin, DTSAliasesMixin,
	  DTSChosenMixin, DTSReducedMixin, DTSParserMixin):

    regs_config = {
        'zephyr,flash' : 'CONFIG_FLASH',
        'zephyr,sram'  : 'CONFIG_SRAM',
        'zephyr,ccm'   : 'CONFIG_CCM'
    }

    name_config = {
        'zephyr,console'     : 'CONFIG_UART_CONSOLE_ON_DEV_NAME',
        'zephyr,bt-uart'     : 'CONFIG_BT_UART_ON_DEV_NAME',
        'zephyr,uart-pipe'   : 'CONFIG_UART_PIPE_ON_DEV_NAME',
        'zephyr,bt-mon-uart' : 'CONFIG_BT_MONITOR_ON_DEV_NAME',
        'zephyr,uart-mcumgr' : 'CONFIG_UART_MCUMGR_ON_DEV_NAME'
    }

    @staticmethod
    def find_node_by_path(nodes, path):
        d = nodes
        for k in path[1:].split('/'):
            d = d['children'][k]

        return d

    @staticmethod
    def get_parent_address(node_address):
        parent_address = ''

        for comp in node_address.split('/')[1:-1]:
            parent_address += '/' + comp

        return parent_address

    @staticmethod
    def node_top_address(node_address):
        address = node_address.split('/')[1]
        return address

    def __init__(self, dts_file_path = None):
        self._dts = {}
        self.compatibles = {}
        self.phandles = {}
        self.aliases = defaultdict(list)
        self.chosen = {}
        self.reduced = {}
        # load and parse DTS file
        if dts_file_path is not None:
            self.parse(dts_file_path)

    def find_parent_prop(self, node_address, prop):
        parent_address = get_parent_address(node_address)

        if prop in self.reduced[parent_address]['props']:
            parent_prop = self.reduced[parent_address]['props'].get(prop)
        else:
            raise Exception("Parent of node " + node_address +
                            " has no " + prop + " property")

        return parent_prop

    def get_node_compat(self, node_address):
        compat = None

        if 'props' in self.reduced[node_address]:
            compat = self.reduced[node_address]['props'].get('compatible', None)
        if isinstance(compat, list):
            compat = compat[0]

        return compat

    def get_parent_compat(self, node_address):
        parent_address = self.get_parent_address(node_address)
        if not (parent_address in self.reduced):
            # no parent - check whether there should be one
            if parent_address.count('/') > 1:
                # there should be a parent, maybe it is not activated
                raise Exception("Parent '{}' of node '{}' is not activated."
                                .format(parent_address, node_address))
            return None

        return self.get_node_compat(parent_address)


    def get_compats(self, node_address):
        compat = None

        if 'props' in self.reduced[node_address]:
            compat = self.reduced[node_address]['props'].get('compatible', None)
        if compat and not isinstance(compat, list):
            compat = [compat, ]

        return compat

    def get_compat(self, node_address):
        compat = None

        compat = self.get_node_compat(node_address)

        if compat == None:
            compat = self.get_parent_compat(node_address)

        return compat

    # Get the #{address,size}-cells for a given node
    def get_addr_size_cells(self, node_address):
        parent_addr = self.get_parent_address(node_address)
        if parent_addr == '':
            parent_addr = '/'

        # The DT spec says that if #address-cells is missing default to 2
        # if #size-cells is missing default to 1
        nr_addr = self.reduced[parent_addr]['props'].get('#address-cells', 2)
        nr_size = self.reduced[parent_addr]['props'].get('#size-cells', 1)

        return (nr_addr, nr_size)

    def translate_addr(self, addr, node_address, nr_addr_cells, nr_size_cells):

        try:
            ranges = deepcopy(self.find_parent_prop(node_address, 'ranges'))
            if type(ranges) is not list: ranges = [ ]
        except:
            return 0

        parent_address = self.get_parent_address(node_address)

        (nr_p_addr_cells, nr_p_size_cells) = self.get_addr_size_cells(parent_address)

        range_offset = 0
        while ranges:
            child_bus_addr = 0
            parent_bus_addr = 0
            range_len = 0
            for x in range(nr_addr_cells):
                val = ranges.pop(0) << (32 * (nr_addr_cells - x - 1))
                child_bus_addr += val
            for x in range(nr_p_addr_cells):
                val = ranges.pop(0) << (32 * (nr_p_addr_cells - x - 1))
                parent_bus_addr += val
            for x in range(nr_size_cells):
                range_len += ranges.pop(0) << (32 * (nr_size_cells - x - 1))
            # if we are outside of the range we don't need to translate
            if child_bus_addr <= addr <= (child_bus_addr + range_len):
                range_offset = parent_bus_addr - child_bus_addr
                break

        parent_range_offset = self.translate_addr(addr + range_offset,
                parent_address, nr_p_addr_cells, nr_p_size_cells)
        range_offset += parent_range_offset

        return range_offset

    def get_controller_type(self, node_address):
        controller_type = ''
        controller_props = self.reduced[node_address]['props']
        for prop in controller_props:
            if 'controller' in prop:
                controller_type = prop
                break
            if 'parent' in prop:
                controller_type = prop
                break
        if controller_type == '':
            raise Exception("Try to instantiate {} as a controller".format(node_address))
        return controller_type
