#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage interrupts directives.
#
class DTInterrupts(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)
        # Ignore properties that are covered by the extraction of:
        extractors.ignore_properties.extend([ \
            # - 'interrupts' for interrupt consumers
            'interrupt-names',])

    def _find_parent_irq_node(self, node_address):
        dts = self.dts()
        address = ''

        for comp in node_address.split('/')[1:]:
            address += '/' + comp
            if 'interrupt-parent' in dts.reduced[address]['props']:
                interrupt_parent = dts.reduced[address]['props'].get(
                    'interrupt-parent')

        return dts.phandles[interrupt_parent]

    ##
    # @brief Extract interrupts
    #
    # @param node_address Address of node owning the
    #                     interrupts definition.
    # @param prop compatible property name
    #
    def extract(self, node_address, prop):
        dts = self.dts()
        bindings = self.bindings()
        edts = self.edts()

        node = dts.reduced[node_address]

        interrupts = node['props']['interrupts']
        if not isinstance(interrupts, list):
            interrupts = [interrupts, ]
        # Flatten the list in case
        if isinstance(interrupts[0], list):
            interrupts = [item for sublist in interrupts for item in sublist]

        irq_parent = self._find_parent_irq_node(node_address)
        irq_nr_cells = dts.reduced[irq_parent]['props']['#interrupt-cells']
        irq_cell_binding = bindings[dts.get_compat(irq_parent)]
        irq_cell_names = irq_cell_binding.get('#interrupt-cells',
                            irq_cell_binding.get('#cells', []))
        irq_names = node['props'].get('interrupt-names', [])
        if not isinstance(irq_names, list):
            irq_names = [irq_names, ]

        # generate EDTS
        device_id = self.edts_device_id(node_address)
        irq_index = 0
        irq_cell_index = 0
        for cell in interrupts:
            if len(irq_names) > irq_index:
                irq_name = irq_names[irq_index]
            else:
                irq_name = str(irq_index)
            if len(irq_cell_names) > irq_cell_index:
                irq_cell_name = irq_cell_names[irq_cell_index]
            else:
                irq_cell_name = str(irq_cell_index)
            edts.insert_device_property(device_id,
                'interrupts/{}/parent'.format(irq_name),
                self.edts_device_id(irq_parent))
            edts.insert_device_property(device_id,
                'interrupts/{}/{}'.format(irq_name, irq_cell_name),
                cell)
            if 'irq' in irq_cell_name:
                self.edts_insert_device_controller(irq_parent, node_address, cell)

            irq_cell_index += 1
            if irq_cell_index >= irq_nr_cells:
                irq_cell_index = 0
                irq_index += 1
