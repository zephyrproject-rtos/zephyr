#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

# NOTE: This file is part of the old device tree scripts, which will be removed
# later. They are kept to generate some legacy #defines via the
# --deprecated-only flag.
#
# The new scripts are gen_defines.py, edtlib.py, and dtlib.py.

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage interrupts directives.
#
class DTInterrupts(DTDirective):
    ##
    # @brief Extract interrupts
    #
    # @param node_path Path to node owning the
    #                  interrupts definition.
    # @param prop compatible property name
    # @param names (unused)
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_path, prop, names, def_label):
        if prop == "interrupts-extended":
            return
        vals = reduced[node_path]['props'][prop]
        if not isinstance(vals, list):
            vals = [vals]

        irq_parent = parent_irq_node(node_path)
        if not irq_parent:
            err(node_path + " has no interrupt-parent")

        l_base = [def_label]
        index = 0

        while vals:
            prop_def = {}
            prop_alias = {}
            l_idx = [str(index)]

            if names:
                name = [str_to_label(names.pop(0))]
            else:
                name = []

            cell_yaml = get_binding(irq_parent)
            l_cell_prefix = ['IRQ']

            for i in range(reduced[irq_parent]['props']['#interrupt-cells']):
                if "interrupt-cells" in cell_yaml:
                    cell_yaml_name = "interrupt-cells"
                else:
                    cell_yaml_name = "#cells"

                l_cell_name = [cell_yaml[cell_yaml_name][i].upper()]
                if l_cell_name == l_cell_prefix:
                    l_cell_name = []

                full_name = '_'.join(l_base + l_cell_prefix + l_idx + l_cell_name)
                prop_def[full_name] = vals.pop(0)
                add_compat_alias(node_path,
                                 '_'.join(l_cell_prefix + l_idx + l_cell_name),
                                 full_name, prop_alias)

                if name:
                    alias_list = l_base + l_cell_prefix + name + l_cell_name
                    prop_alias['_'.join(alias_list)] = full_name
                    add_compat_alias(node_path,
                                     '_'.join(l_cell_prefix + name + l_cell_name),
                                     full_name, prop_alias)

                if node_path in aliases:
                    add_prop_aliases(
                        node_path,
                        lambda alias:
                            '_'.join([str_to_label(alias)] +
                                     l_cell_prefix + l_idx + l_cell_name),
                        full_name,
                        prop_alias)

                    if name:
                        add_prop_aliases(
                            node_path,
                            lambda alias:
                                '_'.join([str_to_label(alias)] +
                                         l_cell_prefix + name + l_cell_name),
                            full_name,
                            prop_alias)
                    else:
                        add_prop_aliases(
                            node_path,
                            lambda alias:
                                '_'.join([str_to_label(alias)] +
                                         l_cell_prefix + name + l_cell_name),
                            full_name,
                            prop_alias, True)

            index += 1
            insert_defs(node_path, prop_def, prop_alias)


def parent_irq_node(node_path):
    while node_path:
        if 'interrupt-parent' in reduced[node_path]['props']:
            return phandles[reduced[node_path]['props']['interrupt-parent']]

        node_path = get_parent_path(node_path)

    return None


##
# @brief Management information for interrupts.
interrupts = DTInterrupts()
