#
# Copyright (c) 2017 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage gpio-ranges directive.
#
class DTGpioRanges(DTDirective):

    def __init__(self):
        self._data = {}

    ##
    # @brief Extract GPIO ranges
    #
    # GPIO range information is added to the GPIO port node
    # - <gpio_label>_GPIO_RANGE_<index>_<property>: <property value>
    #
    # and to the pin controller node that controls the GPIO port pins
    # - <pin_controller_label>_GPIO_RANGE_<index>_<property>: <property value>
    #
    # @param node_address Address of node owning the gpio-ranges definition.
    # @param yaml YAML definition for the owning node.
    # @param prop gpio-ranges property name
    # @param names (unused)
    # @param def_label Define label string of node owning the gio-ranges definition.
    #
    def extract(self, node_address, yaml, prop, names, def_label):

        # gpio-ranges definition
        gpio_ranges = reduced[node_address]['props'][prop]

        gpio_range_cells = 3
        prop_list = []
        if not isinstance(gpio_ranges, list):
            prop_list.append(gpio_ranges)
        else:
            prop_list = list(gpio_ranges)

        gpio_label = self.get_node_label_string(node_address)

        gpio_range_cell = 0
        gpio_pin_start = 0
        gpio_pin_count = 0
        pin_controller_node_address = ''
        pin_controller_pin_start = 0
        for p in prop_list:
            if gpio_range_cell == 0:
                pin_controller_node_address = phandles[p]
            elif gpio_range_cell == 1:
                gpio_pin_start = p
            elif gpio_range_cell == 2:
                pin_controller_pin_start = p
            elif gpio_range_cell == 3:
                gpio_pin_count = p
            if gpio_range_cell < gpio_range_cells:
                gpio_range_cell += 1
            else:
                # pin controller device - gpio range info
                #########################################
                pin_controller_label = self.get_node_label_string( \
                                                pin_controller_node_address)
                # Get the index for the gpio range info
                # of the specific pin controller
                index_label = self.get_label_string( \
                            [pin_controller_label, "GPIO", "RANGE", "COUNT"])
                index = self._data.get(index_label, 0)
                self._data[index_label] = index + 1

                prop_def = {}
                range_label_prefix = [pin_controller_label, \
                                       "GPIO", "RANGE", str(index)]
                # - define label of GPIO device
                range_label = self.get_label_string(range_label_prefix + ["CLIENT"])
                prop_def[range_label] = gpio_label
                # - gpio pin number of base pin of GPIO port
                range_label = self.get_label_string(range_label_prefix + \
                                               ["CLIENT", "BASE"])
                prop_def[range_label] = str(gpio_pin_start)
                # - pin controller pin number of base pin of GPIO port
                range_label = self.get_label_string(range_label_prefix + \
                                               ["BASE"])
                prop_def[range_label] = str(pin_controller_pin_start)
                # - number of pins of GPIO port in range
                #   (consecutive numbers assumed)
                range_label = self.get_label_string(range_label_prefix + ["NPINS"])
                prop_def[range_label] = str(gpio_pin_count)
                # -count of ranges
                prop_def[index_label] = str(self._data[index_label])

                insert_defs(pin_controller_node_address, prop_def, {})

                # gpio device - gpio range info
                ###############################
                index_label = self.get_label_string([gpio_label, \
                                                "GPIO", "RANGE", "COUNT"])
                index = self._data.get(index_label, 0)
                self._data[index_label] = index + 1

                prop_def = {}
                range_label_prefix = [gpio_label, "GPIO", "RANGE", str(index)]
                # - define label of pin controller device
                range_label = self.get_label_string(range_label_prefix + \
                                               ["CONTROLLER"])
                prop_def[range_label] = pin_controller_label
                # - gpio pin number of base pin of GPIO port
                range_label = self.get_label_string(range_label_prefix + ["BASE"])
                prop_def[range_label] = str(gpio_pin_start)
                # - pin controller pin number of base pin of GPIO port
                range_label = self.get_label_string(range_label_prefix + \
                                               ["CONTROLLER", "BASE"])
                prop_def[range_label] = str(pin_controller_pin_start)
                # - number of pins of GPIO port in range
                #   (consecutive numbers assumed)
                range_label = self.get_label_string(range_label_prefix + ["NPINS"])
                prop_def[range_label] = str(gpio_pin_count)
                # -count of ranges
                prop_def[index_label] = str(self._data[index_label])

                insert_defs(node_address, prop_def, {})

                gio_range_cell = 0

##
# @brief Management information for gpio-ranges.
gpioranges = DTGpioRanges()

