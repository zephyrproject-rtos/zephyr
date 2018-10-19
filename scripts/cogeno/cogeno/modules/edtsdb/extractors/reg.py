#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage reg directive.
#
class DTReg(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)
        # Ignore properties that are covered by the extraction of
        extractors.ignore_properties.extend([ \
            # - 'reg'
            'reg-names', ])

    ##
    # @brief Extract reg directive info
    #
    # @param edts Extended device tree database
    # @param dts Device tree specification
    # @param bindings Device tree bindings
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):

        node = self.dts().reduced[node_address]
        node_compat = self.dts().get_compat(node_address)

        reg = self.dts().reduced[node_address]['props']['reg']
        if not isinstance(reg, list):
            reg = [reg, ]
        # Newer versions of dtc might have the reg property look like
        # reg = <1 2>, <3 4>;
        # So we need to flatten the list in that case
        if isinstance(reg[0], list):
            reg = [item for sublist in reg for item in sublist]

        (nr_address_cells, nr_size_cells) = self.dts().get_addr_size_cells(node_address)

        # generate EDTS
        device_id = self.edts_device_id(node_address)
        reg_index = 0
        reg_cell_index = 0
        reg_nr_cells = nr_address_cells + nr_size_cells
        reg_names = node['props'].get('reg-names', [])
        # if there is a single reg-name, we get a string, not a list back
        # turn it into a list so things are normalized
        if not isinstance(reg_names, list):
            reg_names = [reg_names, ]
        for cell in reg:
            if len(reg_names) > reg_index:
                reg_name = reg_names[reg_index]
            else:
                reg_name = str(reg_index)
            if reg_cell_index < nr_address_cells:
                cell += self.dts().translate_addr(cell, node_address,
                                                 nr_address_cells,
                                                 nr_size_cells)
                self.edts().insert_device_property(device_id,
                    'reg/{}/address/{}'.format(reg_name, reg_cell_index), cell)
            else:
                size_cell_index = reg_cell_index - nr_address_cells
                self.edts().insert_device_property(device_id,
                    'reg/{}/size/{}'.format(reg_name, size_cell_index), cell)
            reg_cell_index += 1
            if reg_cell_index >= reg_nr_cells:
                reg_cell_index = 0
                reg_index += 1
