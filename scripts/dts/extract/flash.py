#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

from extract.default import default
from extract.reg import reg

##
# @brief Manage flash directives.
#
class DTFlash(DTDirective):

    def __init__(self):
        # Node of the flash
        self._flash_node = None

    def extract_partition(self, node_address):
        prop_def = {}
        prop_alias = {}
        node = reduced[node_address]

        partition_name = node['props']['label']
        partition_sectors = node['props']['reg']

        label_prefix = ["FLASH_AREA", partition_name]
        label = self.get_label_string(label_prefix + ["LABEL",])
        prop_def[label] = '"{}"'.format(partition_name)

        label = self.get_label_string(label_prefix + ["READ_ONLY",])
        if node['props'].get('read-only'):
            prop_def[label] = 1
        else:
            prop_def[label] = 0

        index = 0
        while index < len(partition_sectors):
            sector_index = int(index/2)
            sector_start_offset = partition_sectors[index]
            sector_size = partition_sectors[index + 1]
            label = self.get_label_string(
                label_prefix + ["OFFSET", str(sector_index)])
            prop_def[label] = "{}".format(sector_start_offset)
            label = self.get_label_string(
                label_prefix + ["SIZE", str(sector_index)])
            prop_def[label] = "{}".format(sector_size)
            index += 2
        # alias sector 0
        label = self.get_label_string(label_prefix + ["OFFSET",])
        prop_alias[label] = self.get_label_string(
            label_prefix + ["OFFSET", '0'])
        label = self.get_label_string(label_prefix + ["SIZE",])
        prop_alias[label] = self.get_label_string(
            label_prefix + ["SIZE", '0'])

        insert_defs(node_address, prop_def, prop_alias)

    def _extract_flash(self, node_address, prop, def_label):
        load_defs = {
            'CONFIG_FLASH_BASE_ADDRESS': 0,
            'CONFIG_FLASH_SIZE': 0
        }

        if node_address == 'dummy-flash':
            # We will add addr/size of 0 for systems with no flash controller
            # This is what they already do in the Kconfig options anyway
            insert_defs(node_address, load_defs, {})
            self._flash_base_address = 0
            return

        self._flash_node = reduced[node_address]

        (nr_address_cells, nr_size_cells) = get_addr_size_cells(node_address)
        # if the nr_size_cells is 0, assume a SPI flash, need to look at parent
        # for addr/size info, and the second reg property (assume first is mmio
        # register for the controller itself)
        if (nr_size_cells == 0):
            node_address = get_parent_address(node_address)
            (nr_address_cells, nr_size_cells) = get_addr_size_cells(node_address)

        node_compat = get_compat(node_address)
        reg = reduced[node_address]['props']['reg']
        if type(reg) is not list: reg = [ reg, ]
        props = list(reg)

        # Newer versions of dtc might have the reg propertly look like
        # reg = <1 2>, <3 4>;
        # So we need to flatten the list in that case
        if isinstance(props[0], list):
            props = [item for sublist in props for item in sublist]

        # We assume the last reg property is the one we want
        while props:
            addr = 0
            size = 0

            for x in range(nr_address_cells):
                addr += props.pop(0) << (32 * (nr_address_cells - x - 1))
            for x in range(nr_size_cells):
                size += props.pop(0) << (32 * (nr_size_cells - x - 1))

        addr += translate_addr(addr, node_address,
                nr_address_cells, nr_size_cells)

        load_defs['CONFIG_FLASH_BASE_ADDRESS'] = hex(addr)
        load_defs['CONFIG_FLASH_SIZE'] = int(size / 1024)

        flash_props = ["label", "write-block-size", "erase-block-size"]
        for prop in flash_props:
            if prop in self._flash_node['props']:
                default.extract(node_address, prop, None, def_label)
        insert_defs(node_address, load_defs, {})

        #for address in reduced:
        #    if address.startswith(node_address) and 'partition@' in address:
        #        self._extract_partition(address, 'partition', None, def_label)

    def _extract_code_partition(self, node_address, prop, def_label):
        load_defs = {}

        if node_address == 'dummy-flash':
            node = None
        else:
            node = reduced[node_address]
            if self._flash_node is None:
                # No flash node scanned before-
                raise Exception(
                    "Code partition '{}' {} without flash definition."
                        .format(prop, node_address))

        if node and node is not self._flash_node:
            # only compute the load offset if the code partition
            # is not the same as the flash base address
            load_offset = node['props']['reg'][0]
            load_defs['CONFIG_FLASH_LOAD_OFFSET'] = load_offset
            load_size = node['props']['reg'][1]
            load_defs['CONFIG_FLASH_LOAD_SIZE'] = load_size
        else:
            load_defs['CONFIG_FLASH_LOAD_OFFSET'] = 0
            load_defs['CONFIG_FLASH_LOAD_SIZE'] = 0

        insert_defs(node_address, load_defs, {})

    ##
    # @brief Extract flash
    #
    # @param node_address Address of node owning the
    #                     flash definition.
    # @param prop compatible property name
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, prop, def_label):

        if prop == 'zephyr,flash':
            # indicator for flash
            self._extract_flash(node_address, prop, def_label)
        elif prop == 'zephyr,code-partition':
            # indicator for code_partition
            self._extract_code_partition(node_address, prop, def_label)
        else:
            raise Exception(
                "DTFlash.extract called with unexpected directive ({})."
                    .format(prop))
##
# @brief Management information for flash.
flash = DTFlash()
