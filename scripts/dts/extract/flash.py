#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective
from extract.edts import *

from extract.default import default
from extract.reg import reg

##
# @brief Manage flash directives.
#
class DTFlash(DTDirective):

    def __init__(self):
        # Node of the flash
        self._flash_node = None

    def _extract_partition(self, node_address, yaml, prop, names, def_label):
        node = reduced[node_address]
        partition_name = node['props']['label']
        partition_sectors = node['props']['reg']

        nr_address_cells = reduced['/']['props'].get('#address-cells')
        nr_size_cells = reduced['/']['props'].get('#size-cells')
        address = ''
        for comp in node_address.split('/')[1:-1]:
            address += '/' + comp
            nr_address_cells = reduced[address]['props'].get(
                '#address-cells', nr_address_cells)
            nr_size_cells = reduced[address]['props'].get('#size-cells', nr_size_cells)

        # generate EDTS
        sector_index = 0
        sector_cell_index = 0
        sector_nr_cells = nr_address_cells + nr_size_cells
        for cell in partition_sectors:
            if sector_cell_index < nr_address_cells:
                edts_insert_device_property(node_address,
                    'sector/{}/offset/{}'.format(sector_index, sector_cell_index),
                    cell)
            else:
                size_cell_index = sector_cell_index - nr_address_cells
                edts_insert_device_property(node_address,
                    'sector/{}/size/{}'.format(sector_index, size_cell_index),
                    cell)
            sector_cell_index += 1
            if sector_cell_index >= sector_nr_cells:
                sector_cell_index = 0
                sector_index += 1
        edts_insert_device_parent_device_property(node_address)

        # generate defines
        prop_def = {}
        prop_alias = {}

        label_prefix = ["FLASH_AREA", partition_name]
        label = self.get_label_string(label_prefix + ["LABEL",])
        prop_def[label] = '"{}"'.format(partition_name)

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

    def _extract_flash(self, node_address, yaml, prop, names, def_label):
        # generate defines
        load_defs = {}

        if node_address == 'dummy-flash':
            # We will add addr/size of 0 for systems with no flash controller
            # This is what they already do in the Kconfig options anyway
            load_defs = {
                'CONFIG_FLASH_BASE_ADDRESS': 0,
                'CONFIG_FLASH_SIZE': 0
            }
            insert_defs(node_address, load_defs, {})
            self._flash_base_address = 0
        else:
            self._flash_node = reduced[node_address]

            flash_props = ["label", "write-block-size", "erase-block-size"]
            for prop in flash_props:
                if prop in self._flash_node['props']:
                    default.extract(node_address, yaml, prop, None, def_label)
            insert_defs(node_address, load_defs, {})

        # generate EDTS
        # - keep behind define generation to overwrite values
        #   done by default.extract() generation
        device_id = edts_device_id(node_address)
        if node_address == 'dummy-flash':
            edts_insert_device_property(device_id, 'reg/0/address/0', "0")
            edts_insert_device_property(device_id, 'reg/0/size/0', "0")
        else:
            pass

    def _extract_code_partition(self, node_address, yaml, prop, names, def_label):
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
            load_offset = str(int(node['props']['reg'][0]) \
                              - int(self._flash_node['props']['reg'][0]))
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
    # @param yaml YAML definition for the owning node.
    # @param prop compatible property name
    # @param names (unused)
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, yaml, prop, names, def_label):

        if prop == 'zephyr,flash':
            # indicator for flash
            self._extract_flash(node_address, yaml, prop, names, def_label)
        elif prop == 'zephyr,code-partition':
            # indicator for code_partition
            self._extract_code_partition(node_address, yaml, prop, names, def_label)
        elif prop == 'reg':
            # indicator for partition
            self._extract_partition(node_address, yaml, prop, names, def_label)
        else:
            raise Exception(
                "DTFlash.extract called with unexpected directive ({})."
                    .format(prop))
##
# @brief Management information for flash.
flash = DTFlash()
