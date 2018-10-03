#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

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

            cell_yaml = yaml[get_compat(irq_parent)]
            l_cell_prefix = ['IRQ']

            for i in range(reduced[irq_parent]['props']['#interrupt-cells']):
                l_cell_name = [cell_yaml['#cells'][i].upper()]
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
