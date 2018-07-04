#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
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
# directives.
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

    def _extract_consumer(self, node_address, yaml, clocks, names, def_label):

        clock_consumer = reduced[node_address]
        clock_consumer_compat = get_compat(node_address)
        clock_consumer_bindings = yaml[clock_consumer_compat]
        clock_consumer_label = self.get_node_label_string(node_address)
        clock_consumer_clock_names = clock_consumer['props'].get('clock-names', None)

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

                clock_label_prefix = [clock_consumer_label, 'CLOCK', str(clock_index)]

                # - map clock consumer clock name to clock index
                clock_label = self.get_label_string([clock_consumer_label,
                    "CLOCK", "ID", self._clock_input_name(clock_consumer, clock_index)])
                prop_def[clock_label] = clock_index

                # - define clock provider device
                clock_label = self.get_label_string(clock_label_prefix + \
                                                    ["PROVIDER"])
                prop_def[clock_label] = clock_provider_label

                # - define clock provider output definition
                for i, cell in enumerate(clock_cells):
                    if i >= len(clock_cells_names):
                        clock_cell_name = 'CELL{}'.format(i)
                    else:
                        clock_cell_name = clock_cells_names[i]
                    clock_label = self.get_label_string(clock_label_prefix + \
                        [clock_cell_name,])
                    prop_def[clock_label] = str(cell)

                # ------ legacy start
                # legacy definitions for backwards compatibility
                # @todo remove legacy definitions

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
                # ------ legacy end

                insert_defs(node_address, prop_def, prop_alias)

                clock_cell_index = 0
                clock_index += 1
            # - count of clocks
            prop_def = {}
            clock_label = self.get_label_string([clock_consumer_label, \
                                                 'CLOCK', "COUNT"])
            prop_def[clock_label] = str(clock_index)
            insert_defs(node_address, prop_def, {})

    def _extract_assigned(self, node_address, yaml, clocks, names, def_label):

        clock_consumer = reduced[node_address]
        clock_consumer_label = self.get_node_label_string(node_address)
        clocks = clock_consumer['props'].get('assigned-clocks', None)
        clock_parents = clock_consumer['props'].get('assigned-clock-parents', None)
        clock_rates = clock_consumer['props'].get('assigned-clock-rates', None)

        clock_index = 0
        clock_cell_index = 0
        nr_clock_cells = 1 # [phandle, clock specifier] -> 1 specifier
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
                clock_provider_label = self.get_node_label_string( \
                                                clock_provider_node_address)
                clock_cells = []
            else:
                clock_cells.append(cell)
            clock_cell_index += 1
            if clock_cell_index > nr_clock_cells:
                # clock provider device - assigned clocks info
                ##############################################
                prop_def = {}
                prop_alias = {}

                clock_label_prefix = [clock_provider_label, 'CLOCK', 'OUTPUT',
                                      str(int(clock_cells[0])), 'ASSIGNED']

                # - define assigned clock rate
                if len(clock_rates) > clock_index:
                    clock_label = self.get_label_string(
                        clock_label_prefix + ["RATE"])
                    prop_def[clock_label] = clock_rates[clock_index]
                # - define assigned clock parent
                if len(clock_parents) > clock_index * 2 + 1:
                    clock_parent_node_address = phandles[clock_parents[clock_index * 2]]
                    clock_parent_label = self.get_node_label_string(clock_parent_node_address)
                    clock_parent_output_id = str(int(clock_parents[clock_index * 2 + 1]))
                    clock_label = self.get_label_string(
                        clock_label_prefix + ["PARENT", "CLOCK", "PROVIDER"])
                    prop_def[clock_label] = clock_parent_label
                    clock_label = self.get_label_string(
                        clock_label_prefix + ["PARENT", "CLOCK", "ID"])
                    prop_def[clock_label] = clock_parent_output_id
                # - define assigned consumer
                clock_label = self.get_label_string(
                    clock_label_prefix + ["CONSUMER"])
                prop_def[clock_label] = clock_consumer_label

                insert_defs(clock_provider_node_address, prop_def, prop_alias)

                clock_cell_index = 0
                clock_index += 1

    def _extract_provider(self, node_address, yaml, prop_list, names, def_label):

        clock_provider_def = {}
        clock_provider = reduced[node_address]
        clock_provider_label = self.get_node_label_string(node_address)

        # top node
        top_node_def = {}
        top_node_address = node_top_address(node_address)
        top_node_label = convert_string_to_label(top_node_address)
        # assure top node in clocks
        if not top_node_address in self._clocks:
            self._clocks[top_node_address] = {}

        output_count = 0
        for output_name in self._clock_output_names(clock_provider):
            output_id = self._clock_output_id(clock_provider, output_name)

            # - Clock output information is added to the clock provider node
            clock_label = self.get_label_string([clock_provider_label,
                "CLOCK", "OUTPUT", str(output_id), "NAME"])
            clock_provider_def[clock_label] = '"{}"'.format(output_name)
            clock_label = self.get_label_string([clock_provider_label,
                "CLOCK", "OUTPUT", "ID", output_name])
            clock_provider_def[clock_label] = str(output_id)
            if output_id >= output_count:
                output_count = output_id + 1

            # - Clock information is added to the top node
            if output_name in self._clocks[top_node_address]:
                raise Exception(
                    ("Clock output name {} used by multiple clock providers."
                     " Could not allocate to {}.")
                        .format(output_name, node_address))
            clock_index = len(self._clocks[top_node_address]) + 1
            self._clocks[top_node_address][output_name] = clock_index
            clock_label = self.get_label_string([top_node_label, "CLOCK",
                                        str(clock_index), "PROVIDER"])
            top_node_def[clock_label] = clock_provider_label
            clock_label = self.get_label_string([top_node_label, "CLOCK",
                                        str(clock_index), "OUTPUT", "ID"])
            top_node_def[clock_label] = str(output_id)
            clock_label = self.get_label_string([top_node_label, "CLOCK", "ID",
                                                output_name])
            top_node_def[clock_label] = str(clock_index)

        # clock provider clock count
        clock_label = self.get_label_string([clock_provider_label,
            "CLOCK", "OUTPUT", "COUNT"])
        clock_provider_def[clock_label] = str(output_count)
        insert_defs(node_address, clock_provider_def, {})
        # top node clock count
        clock_label = self.get_label_string([top_node_label, "CLOCK", "COUNT"])
        top_node_def[clock_label] = str(len(self._clocks))
        insert_defs(top_node_address, top_node_def, {})

    ##
    # @brief Extract clocks related directives
    #
    # Clock information is added to the top node
    #   index is free running
    # - <top node>_CLOCK_<index>_PROVIDER: <clock_provider_label>
    # - <top node>_CLOCK_<index>_OUTPUT_ID: <clock_provider_output_id>
    # - <top node>_CLOCK_ID_<clock-name> : <index>
    #   clock_name taken from provider clock-output-names directive
    # - <top node>_CLOCK_COUNT: <index> + 1
    #
    # Clock output information is added to the clock provider node
    #   index follows provider clock-output-names amended by clock-indices
    # - <clock_provider_label>_CLOCK_OUTPUT_<index>_NAME: <clock_name>
    #   clock_name taken from provider clock-output-names directive
    # - <clock_provider_label>_CLOCK_OUTPUT_ID_<clock_name> : <index>
    #   clock_name taken from provider clock-output-names directive
    # - <clock_provider_label>_CLOCK_OUTPUT_COUNT: <index> + 1
    #
    # Assigned clock information is added to the clock provider node.
    # - <clock_provider_label>_CLOCK_OUTPUT_<index>_ASSIGNED_PARENT_CLOCK_PROVIDER: <clock parent label>
    # - <clock_provider_label>_CLOCK_OUTPUT_<index>_ASSIGNED_PARENT_CLOCK_ID: <index>
    # - <clock_provider_label>_CLOCK_OUTPUT_<index>_ASSIGNED_RATE: <clock rate>
    # - <clock_provider_label>_CLOCK_OUTPUT_<index>_ASSIGNED_CONSUMER: <clock_consumer_label>
    #
    # Clock information is added to the clock consumer node
    #   index follows consumer clocks directive
    # - <clock_consumer_label>_CLOCK_<index>_<clock-cell-name>: <clock-cell-value>
    #   clock_cell_name taken from provider #cells binding
    # - <clock_consumer_label>_CLOCK_<index>_PROVIDER: <clock_provider_label>
    # - <clock_consumer_label>_CLOCK_ID_<clock_name>: <index>
    #   clock_name taken from consumer clock-names directive
    # - <clock_consumer_label>_CLOCK_COUNT: <index> + 1
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
        elif prop in ('clock-output-names', 'clock-indices'):
            # covered by _extract_provider
            pass
        elif prop in ('clock-frequency', 'clock-accuracy', 'oscillator'):
            # use default extraction
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
