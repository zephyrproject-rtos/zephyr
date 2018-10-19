#
# Copyright (c) 2017 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage gpio related directive.
#
# Handles:
# - gpios
# - gpio-ranges
#
# Generates in EDTS:
# - gpios/<index>/controller                : device_id of gpio controller
# - gpios/<index>/<cell-name> : <cell-value>
#                  (cell-name from cell-names of gpio controller)
# - gpio-ranges/<index>/base                : base pin of gpio
# - gpio-ranges/<index>/npins               : number of pins
# - gpio-ranges/<index>/pin-controller      : device_id of pin controller
# - gpio-ranges/<index>/pin-controller-base : base pin of pin controller
#
class DTGpio(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)

    ##
    # @brief Insert gpio cells into EDTS
    #
    # @param gpio_client_node_address
    # @param gpio_node_address
    # @param gpio_cells
    # @param property_path_templ "xxx/yyy/{}"
    #
    def edts_insert_gpio_cells(self, gpio_client_node_address,
                                gpio_node_address, gpio_cells,
                                property_path_templ):
        if len(gpio_cells) == 0:
            return
        gpio_client_device_id = self.edts_device_id(gpio_client_node_address)
        gpio_device_id = self.edts_device_id(gpio_node_address)
        gpio = self.dts().reduced[gpio_node_address]
        gpio_compat = self.dts().get_compat(gpio_node_address)
        gpio_binding = self.bindings()[gpio_compat]
        gpio_cells_names = gpio_binding.get('#cells', ['ID', ])
        cells_list = []
        for i, cell in enumerate(gpio_cells):
            if i >= len(gpio_cells_names):
                gpio_cell_name = 'CELL{}'.format(i).lower()
            else:
                gpio_cell_name = gpio_cells_names[i].lower()
            self.edts().insert_device_property(gpio_client_device_id,
                property_path_templ.format(gpio_cell_name), cell)
            cells_list.append(cell)

    ##
    # @brief Extract GPIO ranges
    #
    # @param node_address Address of node issuing the directive.
    # @param prop property name
    # @param prop_list list of property values
    #
    def _extract_gpios(self, node_address, prop, prop_list):

        gpio_client_device_id = self.edts_device_id(node_address)

        gpio_nr_cells = 0
        gpios_index = 0
        gpios_cell_index = 0
        for p in prop_list:
            if gpios_cell_index == 0:
                gpio_node_address = self.dts().phandles[p]
                gpio = self.dts().reduced[gpio_node_address]
                gpio_nr_cells = int(gpio['props'].get('#gpio-cells', 2))
                gpios_cells = []
            else:
                gpios_cells.append(p)
            gpios_cell_index += 1
            if gpios_cell_index > gpio_nr_cells:
                # generate EDTS
                self.edts_insert_device_controller(gpio_node_address, node_address,
                                                   gpios_cells[0])
                self.edts().insert_device_property(gpio_client_device_id,
                    '{}/{}/controller'.format(prop, gpios_index),
                    self.edts_device_id(gpio_node_address))
                self.edts_insert_gpio_cells(node_address, gpio_node_address,
                                            gpios_cells,
                                            "{}/{}/{{}}".format(prop, gpios_index))
                gpios_index += 1
                gpios_cell_index = 0

    ##
    # @brief Extract GPIO ranges
    #
    # @param node_address Address of node issuing the directive.
    # @param prop_list list of property values
    #
    def _extract_gpio_ranges(self, node_address, prop_list):

        gpio_range_cells = 3

        gpio_device_id = self.edts_device_id(node_address)
        gpio_range_index = 0
        gpio_range_cell = 0
        gpio_pin_start = 0
        gpio_pin_count = 0
        pin_controller_node_address = ''
        pin_controller_pin_start = 0
        for p in prop_list:
            if gpio_range_cell == 0:
                pin_controller_node_address = self.dts().phandles[p]
            elif gpio_range_cell == 1:
                gpio_pin_start = p
            elif gpio_range_cell == 2:
                pin_controller_pin_start = p
            elif gpio_range_cell == 3:
                gpio_pin_count = p
            if gpio_range_cell < gpio_range_cells:
                gpio_range_cell += 1
            else:
                # generate EDTS
                self.edts().insert_device_property(gpio_device_id,
                    'gpio-ranges/{}/pin-controller'.format(gpio_range_index),
                    self.edts_device_id(pin_controller_node_address))
                self.edts().insert_device_property(gpio_device_id,
                    'gpio-ranges/{}/pin-controller-base'.format(gpio_range_index),
                    int(pin_controller_pin_start))
                self.edts().insert_device_property(gpio_device_id,
                    'gpio-ranges/{}/base'.format(gpio_range_index),
                    int(gpio_pin_start))
                self.edts().insert_device_property(gpio_device_id,
                    'gpio-ranges/{}/npins'.format(gpio_range_index),
                    int(gpio_pin_count))

                gio_range_cell = 0
                gpio_range_index += 1

    ##
    # @brief Extract gpio related directives
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
        # Newer versions of dtc might have the property look like
        # <1 2>, <3 4>;
        # So we need to flatten the list in that case
        if isinstance(prop_list[0], list):
            prop_list = [item for sublist in prop_list for item in sublist]

        if prop == 'gpio-ranges':
            # indicator for gpio
            self._extract_gpio_ranges(node_address, prop_list)
        elif 'gpios' in prop:
            # indicator for gpio client
            self._extract_gpios(node_address, prop, prop_list)
        else:
            raise Exception(
                "DTGpio.extract called with unexpected directive ({})."
                    .format(prop))

