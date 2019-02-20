#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage pinctrl-x directive.
#
class DTPinCtrl(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Extract pinctrl information.
    #
    # @param node_path Path to node owning the pinctrl definition.
    # @param prop pinctrl-x key
    # @param def_label Define label string of client node owning the pinctrl
    #                  definition.
    #
    def extract(self, node_path, prop, def_label):

        pinconf = reduced[node_path]['props'][prop]

        prop_list = []
        if not isinstance(pinconf, list):
            prop_list.append(pinconf)
        else:
            prop_list = list(pinconf)

        def_prefix = def_label.split('_')

        prop_def = {}
        for p in prop_list:
            pin_node_path = phandles[p]
            pin_subnode = '/'.join(pin_node_path.split('/')[-1:])
            cell_yaml = get_binding(pin_node_path)
            cell_prefix = 'PINMUX'
            post_fix = []

            if cell_prefix is not None:
                post_fix.append(cell_prefix)

            for subnode in reduced:
                if pin_subnode in subnode and pin_node_path != subnode:
                    # found a subnode underneath the pinmux handle
                    pin_label = def_prefix + post_fix + subnode.split('/')[-2:]

                    for i, cells in enumerate(reduced[subnode]['props']):
                        key_label = list(pin_label) + \
                            [cell_yaml['#cells'][0]] + [str(i)]
                        func_label = key_label[:-2] + \
                            [cell_yaml['#cells'][1]] + [str(i)]
                        key_label = str_to_label('_'.join(key_label))
                        func_label = str_to_label('_'.join(func_label))

                        prop_def[key_label] = cells
                        prop_def[func_label] = \
                            reduced[subnode]['props'][cells]

        insert_defs(node_path, prop_def, {})

##
# @brief Management information for pinctrl-[x].
pinctrl = DTPinCtrl()
