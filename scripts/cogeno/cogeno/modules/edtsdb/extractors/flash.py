#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage flash directives.
#
class DTFlash(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)
        # Node of the flash
        self._flash_node = None

    def _extract_partition(self, node_address, prop):
        dts = self.dts()

        node = dts.reduced[node_address]
        partition_name = node['props']['label']
        partition_sectors = node['props']['reg']

        nr_address_cells = dts.reduced['/']['props'].get('#address-cells')
        nr_size_cells = dts.reduced['/']['props'].get('#size-cells')
        address = ''
        for comp in node_address.split('/')[1:-1]:
            address += '/' + comp
            nr_address_cells = dts.reduced[address]['props'].get(
                '#address-cells', nr_address_cells)
            nr_size_cells = dts.reduced[address]['props'].get('#size-cells', nr_size_cells)

        # generate EDTS
        sector_index = 0
        sector_cell_index = 0
        sector_nr_cells = nr_address_cells + nr_size_cells
        for cell in partition_sectors:
            if sector_cell_index < nr_address_cells:
               self.edts_insert_device_property(node_address,
                    'sector/{}/offset/{}'.format(sector_index, sector_cell_index),
                    cell)
            else:
                size_cell_index = sector_cell_index - nr_address_cells
                self.edts_insert_device_property(node_address,
                    'sector/{}/size/{}'.format(sector_index, size_cell_index),
                    cell)
            sector_cell_index += 1
            if sector_cell_index >= sector_nr_cells:
                sector_cell_index = 0
                sector_index += 1

    def _extract_flash(self, node_address, prop):
        dts = self.dts()
        edts = self.edts()
        extractors = self.extractors()

        device_id = self.edts_device_id(node_address)

        if node_address == 'dummy-flash':
            # We will add addr/size of 0 for systems with no flash controller
            # This is what they already do in the Kconfig options anyway
            edts.insert_device_property(device_id, 'reg/0/address/0', "0")
            edts.insert_device_property(device_id, 'reg/0/size/0', "0")
            self._flash_node = None
            self._flash_base_address = 0
        else:
            self._flash_node = dts.reduced[node_address]

            flash_props = ["label", "write-block-size", "erase-block-size"]
            for prop in flash_props:
                if prop in self._flash_node['props']:
                    extractors.default.extract(node_address, prop,)

    def _extract_code_partition(self, node_address, prop):
        # do sanity check
        if node_address != 'dummy-flash':
            if self._flash_node is None:
                # No flash node scanned before-
                raise Exception(
                    "Code partition '{}' {} without flash definition."
                        .format(prop, node_address))

    ##
    # @brief Extract flash
    #
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):

        if prop == 'zephyr,flash':
            # indicator for flash
            self._extract_flash(node_address, prop)
        elif prop == 'zephyr,code-partition':
            # indicator for code_partition
            self._extract_code_partition(node_address, prop)
        elif prop == 'reg':
            # indicator for partition
            self._extract_partition(node_address, prop)
        else:
            raise Exception(
                "DTFlash.extract called with unexpected directive ({})."
                    .format(prop))
