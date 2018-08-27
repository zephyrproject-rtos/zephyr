#
# Copyright (c) 2017 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective
from extract.edts import *

##
# @brief Manage gpio-ranges directive.
#
# Handles:
# - gpio-ranges
#
# Generates in EDTS:
# - gpio-ranges/<index>/base                : base pin of gpio
# - gpio-ranges/<index>/npins               : number of pins
# - gpio-ranges/<index>/pin-controller      : device_id of pin controller
# - gpio-ranges/<index>/pin-controller-base : base pin of pin controller
#
class DTGpioRanges(DTDirective):

    def __init__(self):
        self._data = {}

    ##
    # @brief Extract GPIO ranges
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

        gpio_device_id = edts_device_id(node_address)
        gpio_range_index = 0
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
                # generate EDTS
                edts_insert_device_property(gpio_device_id,
                    'gpio-ranges/{}/pin-controller'.format(gpio_range_index),
                    edts_device_id(pin_controller_node_address))
                edts_insert_device_property(gpio_device_id,
                    'gpio-ranges/{}/pin-controller-base'.format(gpio_range_index),
                    int(pin_controller_pin_start))
                edts_insert_device_property(gpio_device_id,
                    'gpio-ranges/{}/base'.format(gpio_range_index),
                    int(gpio_pin_start))
                edts_insert_device_property(gpio_device_id,
                    'gpio-ranges/{}/npins'.format(gpio_range_index),
                    int(gpio_pin_count))

                gio_range_cell = 0
                gpio_range_index += 1

##
# @brief Management information for gpio-ranges.
gpioranges = DTGpioRanges()

