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
        self._flash_area = {}

    def _extract_partition(self, node_address):
        prop_def = {}
        prop_alias = {}
        node = reduced[node_address]

        partition_name = node['props']['label']
        partition_sectors = node['props']['reg']

        # Build Index based partition IDs
        if node_address in self._flash_area:
            area_id = self._flash_area[node_address]["id"]
        else:
            area_id = len(self._flash_area)
            self._flash_area[node_address] = {'id': area_id }
        partition_idx = str(area_id)

        # Extract a per partition dev name, something like:
        # #define DT_FLASH_AREA_1_DEV             "FLASH_CTRL"

        # For now assume node_address is something like:
        # /flash-controller@4001E000/flash@0/partitions/partition@fc000
        # and we go up 3 levels to get the controller
        ctrl_addr = '/' + '/'.join(node_address.split('/')[1:-3])

        node = reduced[ctrl_addr]
        name = "\"{}\"".format(node['props']['label'])

        for area in self._flash_area.keys():
            if ctrl_addr in area:
                label = self.get_label_string(["DT_FLASH_AREA", partition_idx, "DEV"])
                prop_def[label] = name

        label = self.get_label_string(["DT_FLASH_AREA"] + [partition_name] + ["ID",])
        prop_def[label] = area_id

        label_prefix = ["DT_FLASH_AREA", partition_idx]

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

        label = self.get_label_string(["DT_FLASH_AREA", partition_idx, "LABEL"])
        prop_def[label] = self.get_label_string([partition_name,])

        prop_def["DT_FLASH_AREA_NUM"] = len(self._flash_area)

        insert_defs("DT_FLASH_AREA", prop_def, prop_alias)

    def _create_legacy_label(self, prop_alias, label):
        prop_alias[label.lstrip('DT_')] = label

    def extract_partition(self, node_address):
        prop_def = {}
        prop_alias = {}
        node = reduced[node_address]

        self._extract_partition(node_address)

        partition_name = node['props']['label']
        partition_sectors = node['props']['reg']

        label_prefix = ["DT_FLASH_AREA", partition_name]
        label = self.get_label_string(label_prefix + ["LABEL",])
        prop_def[label] = '"{}"'.format(partition_name)
        self._create_legacy_label(prop_alias, label)

        label = self.get_label_string(label_prefix + ["READ_ONLY",])
        prop_def[label] = 1 if 'read-only' in node['props'] else 0
        self._create_legacy_label(prop_alias, label)

        index = 0
        while index < len(partition_sectors):
            sector_index = int(index/2)
            sector_start_offset = partition_sectors[index]
            sector_size = partition_sectors[index + 1]
            label = self.get_label_string(
                label_prefix + ["OFFSET", str(sector_index)])
            prop_def[label] = "{}".format(sector_start_offset)
            self._create_legacy_label(prop_alias, label)
            label = self.get_label_string(
                label_prefix + ["SIZE", str(sector_index)])
            prop_def[label] = "{}".format(sector_size)
            self._create_legacy_label(prop_alias, label)
            index += 2
        # alias sector 0
        label = self.get_label_string(label_prefix + ["OFFSET",])
        prop_alias[label] = self.get_label_string(
            label_prefix + ["OFFSET", '0'])
        self._create_legacy_label(prop_alias, label)
        label = self.get_label_string(label_prefix + ["SIZE",])
        prop_alias[label] = self.get_label_string(
            label_prefix + ["SIZE", '0'])
        self._create_legacy_label(prop_alias, label)

        insert_defs(node_address, prop_def, prop_alias)

    def _extract_flash(self, node_address, prop, def_label):
        if node_address == 'dummy-flash':
            # We will add addr/size of 0 for systems with no flash controller
            # This is what they already do in the Kconfig options anyway
            insert_defs(node_address,
                        {'DT_FLASH_BASE_ADDRESS': 0, 'DT_FLASH_SIZE': 0},
                        {})
            self._flash_base_address = 0
            return

        self._flash_node = reduced[node_address]
        orig_node_addr = node_address

        (nr_address_cells, nr_size_cells) = get_addr_size_cells(node_address)
        # if the nr_size_cells is 0, assume a SPI flash, need to look at parent
        # for addr/size info, and the second reg property (assume first is mmio
        # register for the controller itself)
        is_spi_flash = False
        if nr_size_cells == 0:
            is_spi_flash = True
            node_address = get_parent_address(node_address)
            (nr_address_cells, nr_size_cells) = get_addr_size_cells(node_address)

        node_compat = get_compat(node_address)
        reg = reduced[node_address]['props']['reg']
        if type(reg) is not list: reg = [ reg, ]
        props = list(reg)

        num_reg_elem = len(props)/(nr_address_cells + nr_size_cells)

        # if we found a spi flash, but don't have mmio direct access support
        # which we determin by the spi controller node only have on reg element
        # (ie for the controller itself and no region for the MMIO flash access)
        if num_reg_elem == 1 and is_spi_flash:
            node_address = orig_node_addr
        else:
            # We assume the last reg property is the one we want
            while props:
                addr = 0
                size = 0

                for x in range(nr_address_cells):
                    addr += props.pop(0) << (32 * (nr_address_cells - x - 1))
                for x in range(nr_size_cells):
                    size += props.pop(0) << (32 * (nr_size_cells - x - 1))

            addr += translate_addr(addr, node_address, nr_address_cells,
                                   nr_size_cells)

            insert_defs(node_address,
                        {'DT_FLASH_BASE_ADDRESS': hex(addr),
                         'DT_FLASH_SIZE': size//1024},
                        {})

        for prop in 'write-block-size', 'erase-block-size':
            if prop in self._flash_node['props']:
                default.extract(node_address, prop, None, def_label)

                # Add an non-DT prefix alias for compatiability
                prop_alias = {}
                label_post = '_' + str_to_label(prop)
                prop_alias['FLASH' + label_post] = def_label + label_post
                insert_defs(node_address, {}, prop_alias)


    def _extract_code_partition(self, node_address, prop, def_label):
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
            load_size = node['props']['reg'][1]
        else:
            load_offset = 0
            load_size = 0

        insert_defs(node_address,
                    {'DT_CODE_PARTITION_OFFSET': load_offset,
                     'DT_CODE_PARTITION_SIZE': load_size},
                    {})

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
