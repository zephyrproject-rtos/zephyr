#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective
from extract.edts import *

##
# @brief Manage interrupts directives.
#
class DTInterrupts(DTDirective):

    def __init__(self):
        pass

    def _find_parent_irq_node(self, node_address):
        address = ''

        for comp in node_address.split('/')[1:]:
            address += '/' + comp
            if 'interrupt-parent' in reduced[address]['props']:
                interrupt_parent = reduced[address]['props'].get(
                    'interrupt-parent')

        return phandles[interrupt_parent]

    ##
    # @brief Extract interrupts
    #
    # @param node_address Address of node owning the
    #                     interrupts definition.
    # @param yaml YAML definition for the owning node.
    # @param prop compatible property name
    # @param names (unused)
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, yaml, prop, names, def_label):

        node = reduced[node_address]

        try:
            props = list(node['props'].get(prop))
        except:
            props = [node['props'].get(prop)]

        irq_parent = self._find_parent_irq_node(node_address)
        irq_nr_cells = reduced[irq_parent]['props']['#interrupt-cells']
        irq_cell_yaml = yaml[get_compat(irq_parent)]
        irq_cell_names = irq_cell_yaml.get('#cells', [])
        irq_names = node['props'].get('interrupt-names', [])
        if not isinstance(irq_names, list):
            irq_names = [irq_names, ]

        # generate EDTS
        device_id = edts_device_id(node_address)
        irq_index = 0
        irq_cell_index = 0
        for cell in props:
            if len(irq_names) > irq_index:
                irq_name = irq_names[irq_index]
            else:
                irq_name = str(irq_index)
            if len(irq_cell_names) > irq_cell_index:
                irq_cell_name = irq_cell_names[irq_cell_index]
            else:
                irq_cell_name = str(irq_cell_index)
            edts_insert_device_property(device_id,
                'interrupts/{}/parent'.format(irq_name),
                edts_device_id(irq_parent))
            edts_insert_device_property(device_id,
                'interrupts/{}/{}'.format(irq_name, irq_cell_name),
                cell)
            irq_cell_index += 1
            if irq_cell_index >= irq_nr_cells:
                irq_cell_index = 0
                irq_index += 1

        # generate defines
        l_base = def_label.split('/')
        index = 0

        # Newer versions of dtc might have the interrupt propertly look like
        # interrupts = <1 2>, <3 4>;
        # So we need to flatten the list in that case
        if isinstance(props[0], list):
            props = [item for sublist in props for item in sublist]

        while props:
            prop_def = {}
            prop_alias = {}
            l_idx = [str(index)]

            try:
                name = [convert_string_to_label(names.pop(0))]
            except:
                name = []

            l_cell_prefix = ['IRQ']

            for i in range(irq_nr_cells):
                l_cell_name = [irq_cell_names[i].upper()]
                if l_cell_name == l_cell_prefix:
                    l_cell_name = []

                l_fqn = '_'.join(l_base + l_cell_prefix + l_idx + l_cell_name)
                prop_def[l_fqn] = props.pop(0)
                if len(name):
                    alias_list = l_base + l_cell_prefix + name + l_cell_name
                    prop_alias['_'.join(alias_list)] = l_fqn

                if node_address in aliases:
                    for i in aliases[node_address]:
                        alias_label = convert_string_to_label(i)
                        alias_list = [alias_label] + l_cell_prefix + name + l_cell_name
                        prop_alias['_'.join(alias_list)] = l_fqn

            index += 1
            insert_defs(node_address, prop_def, prop_alias)

##
# @brief Management information for interrupts.
interrupts = DTInterrupts()
