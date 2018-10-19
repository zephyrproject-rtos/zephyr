#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

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
# Other nodes
# -----------
# - clock-frequency : clock frequency in Hz
#
class DTClocks(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)
        # Ignore properties that are covered by the extraction of
        extractors.ignore_properties.extend([ \
            # - 'clocks' for clock consumers
            'clock-names', 'clock-ranges',
            # - '#clock-cells' for clock providers
            #   ('clock-frequency' is an exemption)
            'clock-output-names', 'clock-indices',
            'clock-accuracy', 'oscillator',
            # - 'assigned-clocks' for clock consumer assigned clocks
            'assigned-clock-parents', 'assigned-clock-rates'])
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
            return "{}".format(clock_input_index)
        if not isinstance(clock_names, list):
            clock_names = [clock_names]
        if len(clock_names) <= clock_input_index:
            return "{}".format(clock_input_index)
        return clock_names[clock_input_index]

    ##
    # @brief Insert clock cells into EDTS
    #
    # @param clock_consumer_node_address
    # @param clock_provider_node_address
    # @param clock_cells
    # @param property_path_templ "xxx/yyy/{}"
    #
    def edts_insert_clock_cells(self, clock_consumer_node_address,
                                clock_provider_node_address, clock_cells,
                                property_path_templ):
        if len(clock_cells) == 0:
            return
        clock_consumer_device_id = self.edts_device_id(clock_consumer_node_address)
        clock_provider_device_id = self.edts_device_id(clock_provider_node_address)
        clock_provider = self.dts().reduced[clock_provider_node_address]
        clock_provider_compat = self.dts().get_compat(clock_provider_node_address)
        clock_provider_binding = self.bindings()[clock_provider_compat]
        clock_cells_names = clock_provider_binding.get('#cells', ['ID', ])
        cells_list = []
        for i, cell in enumerate(clock_cells):
            if i >= len(clock_cells_names):
                clock_cell_name = 'CELL{}'.format(i).lower()
            else:
                clock_cell_name = clock_cells_names[i].lower()
            self.edts().insert_device_property(clock_consumer_device_id,
                property_path_templ.format(clock_cell_name), cell)
            cells_list.append(cell)
        self.edts_insert_device_controller(clock_provider_node_address,
                                           clock_consumer_node_address,
                                           cells_list)


    def _extract_consumer(self, node_address, clocks):

        clock_consumer_device_id = self.edts_device_id(node_address)
        clock_consumer = self.dts().reduced[node_address]

        clock_index = 0
        clock_cell_index = 0
        clock_nr_cells = 0
        clock_provider_node_address = ''
        clock_provider = {}
        for cell in clocks:
            if clock_cell_index == 0:
                if cell not in  self.dts().phandles:
                    raise Exception(
                        ("Could not find the clock provider node {} for clocks"
                         " = {} in clock consumer node {}. Did you activate"
                         " the clock node?. Last clock provider: {}.")
                            .format(str(cell), str(clocks), node_address,
                                    str(clock_provider)))
                clock_provider_node_address = self.dts().phandles[cell]
                clock_provider = self.dts().reduced[clock_provider_node_address]
                clock_nr_cells = int(clock_provider['props'].get('#clock-cells', 0))
                clock_cells = []
            else:
                clock_cells.append(cell)
            clock_cell_index += 1
            if clock_cell_index > clock_nr_cells:
                # generate EDTS
                clock_name = self._clock_input_name(clock_consumer, clock_index)
                self.edts().insert_device_property(clock_consumer_device_id,
                    'clocks/{}/provider'.format(clock_name),
                    self.edts_device_id(clock_provider_node_address))
                for prop in ('clock-ranges',):
                    value = clock_consumer['props'].get(prop, None)
                    if value is not None:
                        self.edts().insert_device_property(clock_consumer_device_id,
                            'clocks/{}/{}'.format(clock_name, prop), value)
                self.edts_insert_clock_cells(node_address,
                    clock_provider_node_address, clock_cells,
                    "clocks/{}/{{}}".format(clock_name))

                clock_cell_index = 0
                clock_index += 1

    def _extract_assigned(self, node_address, prop_list):

        clock_consumer = self.dts().reduced[node_address]
        clocks = clock_consumer['props'].get('assigned-clocks', None)
        clock_parents = clock_consumer['props'].get('assigned-clock-parents', None)
        clock_rates = clock_consumer['props'].get('assigned-clock-rates', None)

        # generate EDTS
        clock_consumer_device_id = self.edts_device_id(node_address)
        clock_index = 0
        clock_parents_index = 0
        clock_cell_index = 0
        clock_nr_cells = 1 # [phandle, clock specifier] -> 1 specifier
        clock_provider_node_address = ''
        clock_provider = {}
        for cell in clocks:
            if clock_cell_index == 0:
                if cell not in  self.dts().phandles:
                    raise Exception(
                        ("Could not find the clock provider node {} for assigned-clocks"
                         " = {} in clock consumer node {}. Did you activate"
                         " the clock node?. Last clock provider: {}.")
                            .format(str(cell), str(clocks), node_address,
                                    str(clock_provider)))
                clock_provider_node_address = self.dts().phandles[cell]
                clock_provider = self.dts().reduced[clock_provider_node_address]
                clock_nr_cells = int(clock_provider['props'].get('#clock-cells', clock_nr_cells))
                clock_cells = []
            else:
                clock_cells.append(cell)
            clock_cell_index += 1
            if clock_cell_index > clock_nr_cells:
                # - assigned clock provider
                self.edts().insert_device_property(clock_consumer_device_id,
                    'assigned-clock/{}/provider'.format(clock_index),
                    self.edts_device_id(clock_provider_node_address))
                # - assigned clock provider output index
                self.edts_insert_clock_cells(node_address,
                    clock_provider_node_address, clock_cells,
                    'assigned-clock/{}/\{\}'.format(clock_index))
                # - assigned clock rate
                if len(clock_rates) > clock_index:
                    self.edts().insert_device_property(clock_consumer_device_id,
                        'assigned-clock/{}/rate'.format(clock_index),
                        clock_rates[clock_index])
                # - assigned clock parent
                if len(clock_parents) > clock_parents_index + 1:
                    clock_parent_node_address = self.dts().phandles[clock_parents[clock_parents_index]]
                    clock_parent = self.dts().reduced[clock_parent_node_address]
                    clock_parent_nr_cells = int(clock_parent['props'].get('#clock-cells', 0))
                    clock_parent_cells = clock_parents[
                        clock_parents_index + 1 :
                        clock_parents_index + 1 + clock_parent_nr_cells]
                    self.edts().insert_device_property(clock_consumer_device_id,
                        'assigned-clock/{}/parent/provider'.format(clock_index),
                        self.edts_device_id(clock_parent_node_address))
                    self.edts_insert_clock_cells(node_address,
                        clock_parent_node_address, clock_parent_cells,
                        'assigned-clock/{}/parent/\{\}'.format(clock_index))
                    clock_parents_index += clock_parent_nr_cells + 1

                clock_cell_index = 0
                clock_index += 1

    def _extract_provider(self, node_address, prop_list):

        clock_provider = self.dts().reduced[node_address]

        # generate EDTS
        clock_provider_device_id = self.edts_device_id(node_address)
        for output_name in self._clock_output_names(clock_provider):
            output_id = self._clock_output_id(clock_provider, output_name)
            self.edts().insert_device_property(clock_provider_device_id,
                'output-clock/{}/name'.format(output_id), output_name)
            for prop in ('clock-frequency', 'clock-accuracy', 'oscillator'):
                value = clock_provider['props'].get(prop, None)
                if value is not None:
                    self.edts().insert_device_property(clock_provider_device_id,
                        'output-clock/{}/{}'.format(output_id, prop), value)

    ##
    # @brief Extract clocks related directives
    #
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):

        properties = self.dts().reduced[node_address]['props'][prop]

        prop_list = []
        if not isinstance(properties, list):
            prop_list.append(properties)
        else:
            prop_list = list(properties)

        if prop == 'clocks':
            # indicator for clock consumers
            self._extract_consumer(node_address, prop_list)
        elif prop == '#clock-cells':
            # indicator for clock providers
            self._extract_provider(node_address, prop_list)
        elif prop in 'assigned-clocks':
            # indicator for assigned clocks
            self._extract_assigned(node_address, prop_list)
        elif prop == 'clock-frequency':
            # clock-frequency is used by nodes besides the
            # clock provider nodes (which is covered by '#clock-cells'.
            if not '#clock-cells' in self.dts().reduced[node_address]['props']:
                self.edts_insert_device_property(node_address, prop,
                                                 prop_list[0])
        else:
            raise Exception(
                "DTClocks.extract called with unexpected directive ({})."
                    .format(prop))
