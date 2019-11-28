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
# @brief Manage clocks related directives.
#
# Handles:
# - clocks
# directives.
#
class DTClocks(DTDirective):
    def _extract_consumer(self, node_path, clocks, def_label):
        clock_consumer_label = 'DT_' + node_label(node_path)

        clock_index = 0
        clock_cell_index = 0
        nr_clock_cells = 0
        clock_provider_node_path = ''
        clock_provider = {}
        for cell in clocks:
            if clock_cell_index == 0:
                if cell not in phandles:
                    raise Exception(
                        ("Could not find the clock provider node {} for clocks"
                         " = {} in clock consumer node {}. Did you activate"
                         " the clock node?. Last clock provider: {}.")
                            .format(str(cell), str(clocks), node_path,
                                    str(clock_provider)))
                clock_provider_node_path = phandles[cell]
                clock_provider = reduced[clock_provider_node_path]
                clock_provider_bindings = get_binding(
                                            clock_provider_node_path)
                nr_clock_cells = int(clock_provider['props'].get(
                                     '#clock-cells', 0))
                clock_cells_string = clock_provider_bindings.get(
                    'cell_string', 'CLOCK')

                if "clock-cells" in clock_provider_bindings:
                    clock_cells_names = clock_provider_bindings["clock-cells"]
                elif "#cells" in clock_provider_bindings:
                    clock_cells_names = clock_provider_bindings["#cells"]
                else:
                    clock_cells_names = ["ID", "CELL1",  "CELL2", "CELL3"]

                clock_cells = []
            else:
                clock_cells.append(cell)
            clock_cell_index += 1
            if clock_cell_index > nr_clock_cells or nr_clock_cells == 0:
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
                        add_compat_alias(node_path,
                                self.get_label_string(["",
                                    clock_cells_string, str(clock_index)]),
                                clock_label, prop_alias)
                    else:
                        clock_label = self.get_label_string([
                            clock_consumer_label, clock_cells_string,
                            clock_cell_name, str(clock_index)])
                        add_compat_alias(node_path,
                                self.get_label_string(["",
                                    clock_cells_string, clock_cell_name,
                                    str(clock_index)]),
                                clock_label, prop_alias)
                    prop_def[clock_label] = str(cell)
                    if clock_index == 0 and \
                        len(clocks) == (len(clock_cells) + 1):
                        index = ''
                    else:
                        index = str(clock_index)
                    if node_path in aliases:
                        if clock_cells_string == clock_cell_name:
                            add_prop_aliases(
                                node_path,
                                lambda alias:
                                    self.get_label_string([
                                        alias,
                                        clock_cells_string,
                                        index]),
                                clock_label,
                                prop_alias)
                        else:
                            add_prop_aliases(
                                node_path,
                                lambda alias:
                                    self.get_label_string([
                                        alias,
                                        clock_cells_string,
                                        clock_cell_name,
                                        index]),
                                clock_label,
                                prop_alias)
                    # alias
                    if i < nr_clock_cells:
                        # clocks info for first clock
                        clock_alias_label = self.get_label_string([
                            clock_consumer_label, clock_cells_string,
                            clock_cell_name])
                        prop_alias[clock_alias_label] = clock_label
                        add_compat_alias(node_path,
                                self.get_label_string(["",
                                    clock_cells_string, clock_cell_name]),
                                clock_label, prop_alias)


                # Legacy clocks definitions by extract_controller
                clock_provider_label_str = clock_provider['props'].get('label',
                                                                       None)
                if clock_provider_label_str is not None:
                    clock_cell_name = 'CLOCK_CONTROLLER'
                    if clock_index == 0 and \
                        len(clocks) == (len(clock_cells) + 1):
                        index = ''
                    else:
                        index = str(clock_index)
                    clock_label = self.get_label_string([clock_consumer_label,
                                                         clock_cell_name,
                                                         index])
                    add_compat_alias(node_path,
                            self.get_label_string(["", clock_cell_name, index]),
                            clock_label, prop_alias)
                    prop_def[clock_label] = '"' + clock_provider_label_str + '"'
                    if node_path in aliases:
                        add_prop_aliases(
                            node_path,
                            lambda alias:
                                self.get_label_string([
                                    alias,
                                    clock_cell_name,
                                    index]),
                            clock_label,
                            prop_alias)

                # If the provided clock has a fixed rate, extract its frequency
                # as a macro generated for the clock consumer.
                if clock_provider['props']['compatible'] == 'fixed-clock':
                    clock_prop_name = 'clock-frequency'
                    clock_prop_label = 'CLOCKS_CLOCK_FREQUENCY'
                    if clock_index == 0 and \
                        len(clocks) == (len(clock_cells) + 1):
                        index = ''
                    else:
                        index = str(clock_index)
                    clock_frequency_label = \
                        self.get_label_string([clock_consumer_label,
                                               clock_prop_label,
                                               index])

                    prop_def[clock_frequency_label] = \
                        clock_provider['props'][clock_prop_name]
                    add_compat_alias(
                        node_path,
                        self.get_label_string([clock_prop_label, index]),
                        clock_frequency_label,
                        prop_alias)
                    if node_path in aliases:
                        add_prop_aliases(
                            node_path,
                            lambda alias:
                                self.get_label_string([
                                    alias,
                                    clock_prop_label,
                                    index]),
                            clock_frequency_label,
                            prop_alias)

                insert_defs(node_path, prop_def, prop_alias)

                clock_cell_index = 0
                clock_index += 1

    ##
    # @brief Extract clocks related directives
    #
    # @param node_path Path to node owning the clockxxx definition.
    # @param prop clockxxx property name
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_path, prop, def_label):

        properties = reduced[node_path]['props'][prop]

        prop_list = []
        if not isinstance(properties, list):
            prop_list.append(properties)
        else:
            prop_list = list(properties)

        if prop == 'clocks':
            # indicator for clock consumers
            self._extract_consumer(node_path, prop_list, def_label)
        else:
            raise Exception(
                "DTClocks.extract called with unexpected directive ({})."
                    .format(prop))

##
# @brief Management information for clocks.
clocks = DTClocks()
