#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.edts import *
from extract.directive import DTDirective
from extract.default import default

##
# @brief Manage clocks related directives.
#
# Handles:
# - clocks
# - clock-names
# - clock-output-names
# - clock-indices
# - clock-ranges
# - clock-frequency
# - clock-accuracy
# - oscillator
# - assigned-clocks
# - assigned-clock-parents
# - assigned-clock-rates
#
# Generates in EDTS:
#
# Clock provider
# --------------
# - clock-output/<index>/name : clock output name
# - clock-output/<index>/clock-frequency : fixed clock output frequency in Hz
# - clock-output/<index>/clock-accuracy : accuracy of clock in ppb (parts per billion).
# - clock-output/<index>/oscillator :  True
#
# Clock consumer
# --------------
# - clocks/<index>/name : name of clock input
# - clocks/<index>/provider : device id of clock provider
# - clocks/<index>/clock-ranges : True
# - clocks/<index>/<cell-name> : <cell-value>
#                  (cell-name from cell-names of provider)
# - assigned-clocks/<index>/provider : device id of provider of assigned clock
# - assigned-clocks/<index>/rate : selected rate of assigned clock in Hz
# - assigned-clocks/<index>/<cell-name> : <cell-value>
#                           (cell-name from cell-names of provider)
# - assigned-clocks/<index>/parent/provider : provider of parent clock of assigned clock
# - assigned-clocks/<index>/parent/<cell-name> : <cell-value>
#                                  (cell-name from cell-names of provider)
#
class DTClocks(DTDirective):

    def __init__(self):
        ##
        # Dictionary of all clocks
        # primary key is the top node
        # secondary key is is the clock name,
        # value is the clock id
        self._clocks = {}

    ##
    # @brief Get output clock names of clock provider
    #
    def _clock_output_names(self, clock_provider):
        names = clock_provider['props'].get('clock-output-names', [])
        if len(names) == 0:
            names = [clock_provider['props'].get('label', '<clock name unknown>')]
        elif not isinstance(names, list):
            names = [names]
        return names

    ##
    # @brief Get clock id of output clock of clock provider
    #
    # @param clock_provider
    # @param clock_output_name
    # @return clock id
    #
    def _clock_output_id(self, clock_provider, clock_output_name):
        clock_output_names = self._clock_output_names(clock_provider)
        clock_indices = clock_provider['props'].get('clock-indices', None)
        if clock_indices:
            if len(clock_output_names) != len(clock_indices):
                raise Exception(
                    ("clock-output-names count ({}) does not match"
                    " clock-indices count ({}) in clock provider {}.")
                        .format(len(clock_output_names), len(clock_indices),
                                str(clock_provider['name'])))
        for i, name in enumerate(clock_output_names):
            if name == clock_output_name:
                if clock_indices:
                    return clock_indices[i]
                return i
        return None

    ##
    # @brief Get clock name of output clock of clock provider
    #
    # @param clock_provider
    # @param clock_output_name
    # @return clock id
    #
    def _clock_output_name(self, clock_provider, clock_output_id):
        clock_output_names = self._clock_output_names(clock_provider)
        clock_indices = clock_provider['props'].get('clock-indices', None)
        if clock_indices:
            for i, clock_id in enumerate(clock_indices):
                if clock_id == clock_output_id:
                    clock_output_id = i
                    break
        return clock_output_names[clock_output_id]

    ##
    # @brief Get clock name of input clock of clock consumer
    #
    def _clock_input_name(self, clock_consumer, clock_input_index):
        clock_names = clock_consumer['props'].get('clock-names', None)
        if clock_names is None:
            return "clk{}".format(clock_input_index)
        if not isinstance(clock_names, list):
            clock_names = [clock_names]
        if len(clock_names) <= clock_input_index:
            return "clk{}".format(clock_input_index)
        return clock_names[clock_input_index]

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
        clock_consumer_clock_names = clock_consumer['props'].get('clock-names', None)

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
                    'clocks/{}/name'.format(clock_index),
                    self._clock_input_name(clock_consumer, clock_index))
                edts_insert_device_property(clock_consumer_device_id,
                    'clocks/{}/provider'.format(clock_index),
                    edts_device_id(clock_provider_node_address))
                for prop in ('clock-ranges',):
                    value = clock_consumer['props'].get(prop, None)
                    if value is not None:
                        edts_insert_device_property(clock_consumer_device_id,
                            'clocks/{}/{}'.format(clock_index, prop), value)
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

    def _extract_assigned(self, node_address, yaml, clocks, names, def_label):

        clock_consumer = reduced[node_address]
        clocks = clock_consumer['props'].get('assigned-clocks', None)
        clock_parents = clock_consumer['props'].get('assigned-clock-parents', None)
        clock_rates = clock_consumer['props'].get('assigned-clock-rates', None)

        # generate EDTS
        clock_consumer_device_id = edts_device_id(node_address)
        clock_index = 0
        clock_parents_index = 0
        clock_cell_index = 0
        clock_nr_cells = 1 # [phandle, clock specifier] -> 1 specifier
        clock_provider_node_address = ''
        clock_provider = {}
        for cell in clocks:
            if clock_cell_index == 0:
                if cell not in phandles:
                    raise Exception(
                        ("Could not find the clock provider node {} for assigned-clocks"
                         " = {} in clock consumer node {}. Did you activate"
                         " the clock node?. Last clock provider: {}.")
                            .format(str(cell), str(clocks), node_address,
                                    str(clock_provider)))
                clock_provider_node_address = phandles[cell]
                clock_provider = reduced[clock_provider_node_address]
                clock_nr_cells = int(clock_provider['props'].get('#clock-cells', clock_nr_cells))
                clock_cells = []
            else:
                clock_cells.append(cell)
            clock_cell_index += 1
            if clock_cell_index > clock_nr_cells:
                # - assigned clock provider
                edts_insert_device_property(clock_consumer_device_id,
                    'assigned-clock/{}/provider'.format(clock_index),
                    edts_device_id(clock_provider_node_address))
                # - assigned clock provider output index
                self._edts_insert_clock_cells(node_address, yaml,
                    clock_provider_node_address, clock_cells,
                    'assigned-clock/{}/\{\}'.format(clock_index))
                # - assigned clock rate
                if len(clock_rates) > clock_index:
                    edts_insert_device_property(clock_consumer_device_id,
                        'assigned-clock/{}/rate'.format(clock_index),
                        clock_rates[clock_index])
                # - assigned clock parent
                if len(clock_parents) > clock_parents_index + 1:
                    clock_parent_node_address = phandles[clock_parents[clock_parents_index]]
                    clock_parent = reduced[clock_parent_node_address]
                    clock_parent_nr_cells = int(clock_parent['props'].get('#clock-cells', 0))
                    clock_parent_cells = clock_parents[
                        clock_parents_index + 1 :
                        clock_parents_index + 1 + clock_parent_nr_cells]
                    edts_insert_device_property(clock_consumer_device_id,
                        'assigned-clock/{}/parent/provider'.format(clock_index),
                        edts_device_id(clock_parent_node_address))
                    self._edts_insert_clock_cells(node_address, yaml,
                        clock_parent_node_address, clock_parent_cells,
                        'assigned-clock/{}/parent/\{\}'.format(clock_index))
                    clock_parents_index += clock_parent_nr_cells + 1

                clock_cell_index = 0
                clock_index += 1

    def _extract_provider(self, node_address, yaml, prop_list, names, def_label):

        clock_provider = reduced[node_address]

        # generate EDTS
        clock_provider_device_id = edts_device_id(node_address)
        for output_name in self._clock_output_names(clock_provider):
            output_id = self._clock_output_id(clock_provider, output_name)
            edts_insert_device_property(clock_provider_device_id,
                'output-clock/{}/name'.format(output_id), output_name)
            for prop in ('clock-frequency', 'clock-accuracy', 'oscillator'):
                value = clock_provider['props'].get(prop, None)
                if value is not None:
                    edts_insert_device_property(clock_provider_device_id,
                        'output-clock/{}/{}'.format(output_id, prop), value)

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
        elif prop in ('clock-names', 'clock-ranges'):
            # covered by _extract_consumer
            pass
        elif prop == '#clock-cells':
            # indicator for clock providers
            self._extract_provider(node_address, yaml, prop_list, names, def_label)
        elif prop in ('clock-output-names', 'clock-indices', 'clock-frequency',
                      'clock-accuracy', 'oscillator'):
            # covered by _extract_provider
            # legacy definitions for backwards compatibility
            if prop in ('clock-frequency',):
                default.extract(node_address, yaml, prop, names, def_label)
        elif prop in 'assigned-clocks':
            # indicator for assigned clocks
            self._extract_assigned(node_address, yaml, prop_list, names, def_label)
        elif prop in ('assigned-clock-parents', 'assigned-clock-rates'):
            # covered by _extract_assigned
            pass
        else:
            raise Exception(
                "DTClocks.extract called with unexpected directive ({})."
                    .format(prop))

##
# @brief Management information for clocks.
clocks = DTClocks()
