#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.edts import *
from extract.directive import DTDirective

##
# @brief Manage clocks related directives.
#
# Handles:
# - clocks
#
# Generates in EDTS:
#
# Clock consumer
# --------------
# - clocks/<index>/<cell-name> : <cell-value>
#                  (cell-name from cell-names of provider)
#
class DTClocks(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Insert clock cells into EDTS
    #
    # @param clock_consumer
    # @param yaml
    # @param clock_provider
    # @param clock_cells
    # @param property_path_templ "xxx/yyy/{}"
    #
    def _edts_insert_clock_cells(self, clock_consumer_node_address, yaml,
                                 clock_provider_node_address, clock_cells,
                                 property_path_templ):
        if len(clock_cells) == 0:
            return
        clock_consumer_device_id = edts_device_id(clock_consumer_node_address)
        clock_provider_device_id = edts_device_id(clock_provider_node_address)
        clock_provider = reduced[clock_provider_node_address]
        clock_provider_compat = get_compat(clock_provider_node_address)
        clock_provider_bindings = yaml[clock_provider_compat]
        clock_nr_cells = int(clock_provider['props'].get('#clock-cells', 0))
        clock_cells_names = clock_provider_bindings.get(
                    '#cells', ['ID', 'CELL1',  "CELL2", "CELL3"])
        for i, cell in enumerate(clock_cells):
            if i >= len(clock_cells_names):
                clock_cell_name = 'CELL{}'.format(i).lower()
            else:
                clock_cell_name = clock_cells_names[i].lower()
            edts_insert_device_property(clock_consumer_device_id,
                property_path_templ.format(clock_cell_name), cell)


    def _extract_consumer(self, node_address, yaml, clocks, names, def_label):

        clock_consumer_device_id = edts_device_id(node_address)
        clock_consumer = reduced[node_address]
        clock_consumer_compat = get_compat(node_address)
        clock_consumer_bindings = yaml[clock_consumer_compat]
        clock_consumer_label = self.get_node_label_string(node_address)

        clock_index = 0
        clock_cell_index = 0
        clock_nr_cells = 0
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
                clock_nr_cells = int(clock_provider['props'].get('#clock-cells', 0))
                clock_cells = []
            else:
                clock_cells.append(cell)
            clock_cell_index += 1
            if clock_cell_index > clock_nr_cells:

                 # generate EDTS
                edts_insert_device_property(clock_consumer_device_id,
                    'clocks/{}/provider'.format(clock_index),
                    edts_device_id(clock_provider_node_address))
                self._edts_insert_clock_cells(node_address, yaml,
                                        clock_provider_node_address,
                                        clock_cells,
                                        "clocks/{}/{{}}".format(clock_index))

                # generate defines
                prop_def = {}
                prop_alias = {}

                # legacy definitions for backwards compatibility
                # @todo remove legacy definitions

                # Legacy clocks definitions by extract_cells
                clock_provider_compat = get_compat(clock_provider_node_address)
                clock_provider_bindings = yaml[clock_provider_compat]
                clock_cells_string = clock_provider_bindings.get(
                    'cell_string', 'CLOCK')
                clock_cells_names = clock_provider_bindings.get(
                            '#cells', ['ID', 'CELL1',  "CELL2", "CELL3"])

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
                    # alias
                    if i < clock_nr_cells:
                        # clocks info for first clock
                        clock_alias_label = self.get_label_string([
                            clock_consumer_label, clock_cells_string,
                            clock_cell_name])
                        prop_alias[clock_alias_label] = clock_label
                # ----- legacy clocks definitions
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
                # ------ legacy end

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
