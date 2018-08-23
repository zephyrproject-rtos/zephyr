#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage clocks related directives.
#
# Handles:
# - clocks
# directives.
#
class DTClocks(DTDirective):

    def __init__(self):
        pass

    def _extract_consumer(self, node_address, yaml, clocks, names, def_label):

        clock_consumer = reduced[node_address]
        clock_consumer_compat = get_compat(node_address)
        clock_consumer_bindings = yaml[clock_consumer_compat]
        clock_consumer_label = self.get_node_label_string(node_address)

        clock_index = 0
        clock_cell_index = 0
        nr_clock_cells = 0
        clock_provider_node_address = ''
        clock_provider = {}
        for cell in clocks:
            if clock_cell_index == 0:
                if cell not in phandles:
                    raise Exception(
                        ("Could not find the clock provider node {} for clocks"
                         " = {} in clock consumer node {}. Did you activate"
                         " the clock node?. Last clock provider: {}.")
                            .format(str(cell), str(clocks), node_address,
                                    str(clock_provider)))
                clock_provider_node_address = phandles[cell]
                clock_provider = reduced[clock_provider_node_address]
                clock_provider_compat = get_compat(clock_provider_node_address)
                clock_provider_bindings = yaml[clock_provider_compat]
                clock_provider_label = self.get_node_label_string( \
                                                clock_provider_node_address)
                nr_clock_cells = int(clock_provider['props'].get(
                                     '#clock-cells', 0))
                clock_cells_string = clock_provider_bindings.get(
                    'cell_string', 'CLOCK')
                clock_cells_names = clock_provider_bindings.get(
                    '#cells', ['ID', 'CELL1',  "CELL2", "CELL3"])
                clock_cells = []
            else:
                clock_cells.append(cell)
            clock_cell_index += 1
            if clock_cell_index > nr_clock_cells:
                # clock consumer device - clocks info
                #####################################
                prop_def = {}
                prop_alias = {}

                # Legacy clocks definitions by extract_cells
                for i, cell in enumerate(clock_cells):
                    if i >= len(clock_cells_names):
                        clock_cell_name = 'CELL{}'.format(i)
                    else:
                        clock_cell_name = clock_cells_names[i]
                    if clock_cells_string == clock_cell_name:
                        clock_label = self.get_label_string([
                            clock_consumer_label, clock_cells_string,
                            str(clock_index)])
                    else:
                        clock_label = self.get_label_string([
                            clock_consumer_label, clock_cells_string,
                            clock_cell_name, str(clock_index)])
                    prop_def[clock_label] = str(cell)
                    if clock_index == 0 and \
                        len(clocks) == (len(clock_cells) + 1):
                        index = ''
                    else:
                        index = str(clock_index)
                    if node_address in aliases:
                        for alias in aliases[node_address]:
                            if clock_cells_string == clock_cell_name:
                                clock_alias_label = self.get_label_string([
                                    alias, clock_cells_string, index])
                            else:
                                clock_alias_label = self.get_label_string([
                                    alias, clock_cells_string,
                                    clock_cell_name, index])
                            prop_alias[clock_alias_label] = clock_label
                    # alias
                    if i < nr_clock_cells:
                        # clocks info for first clock
                        clock_alias_label = self.get_label_string([
                            clock_consumer_label, clock_cells_string,
                            clock_cell_name])
                        prop_alias[clock_alias_label] = clock_label
                # Legacy clocks definitions by extract_controller
                clock_provider_label_str = clock_provider['props'].get('label',
                                                                       None)
                if clock_provider_label_str is not None:
                    try:
                        generation = clock_consumer_bindings['properties'][
                            'clocks']['generation']
                    except:
                        generation = ''
                    if 'use-prop-name' in generation:
                        clock_cell_name = 'CLOCKS_CONTROLLER'
                    else:
                        clock_cell_name = 'CLOCK_CONTROLLER'
                    if clock_index == 0 and \
                        len(clocks) == (len(clock_cells) + 1):
                        index = ''
                    else:
                        index = str(clock_index)
                    clock_label = self.get_label_string([clock_consumer_label,
                                                         clock_cell_name,
                                                         index])
                    prop_def[clock_label] = '"' + clock_provider_label_str + '"'
                    if node_address in aliases:
                        for alias in aliases[node_address]:
                            clock_alias_label = self.get_label_string([
                                alias, clock_cell_name, index])
                            prop_alias[clock_alias_label] = clock_label

                insert_defs(node_address, prop_def, prop_alias)

                clock_cell_index = 0
                clock_index += 1

    ##
    # @brief Extract clocks related directives
    #
    # @param node_address Address of node owning the clockxxx definition.
    # @param yaml YAML definition for the owning node.
    # @param prop clockxxx property name
    # @param names (unused)
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_address, yaml, prop, names, def_label):

        properties = reduced[node_address]['props'][prop]

        prop_list = []
        if not isinstance(properties, list):
            prop_list.append(properties)
        else:
            prop_list = list(properties)

        if prop == 'clocks':
            # indicator for clock consumers
            self._extract_consumer(node_address, yaml, prop_list, names, def_label)
        else:
            raise Exception(
                "DTClocks.extract called with unexpected directive ({})."
                    .format(prop))

##
# @brief Management information for clocks.
clocks = DTClocks()
