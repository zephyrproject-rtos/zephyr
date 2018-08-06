#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

##
# @file
# Code generation module for pinctrl.
#

import codegen

##
# @brief pin controller
#
class PinController(object):

    ## functions below are for pinmux hardware access
    _pinctrl_function_device_base = 32

    ## list of all pin controllers
    _pin_controllers = []

    ##
    # @brief Get list of compatible pin controllers
    #
    # @param compatible
    # @return list of compatible controllers
    #
    @classmethod
    def create_all_compatible(cls, compatible):
        # assure there is nothing left from last call
        cls._pin_controllers = []

        for device_id in codegen.edts().device_ids_by_compatible(compatible):
            controller = cls(device_id)
            cls._pin_controllers.append(controller)
        return cls._pin_controllers

    ##
    # @brief Get the maximum function count of all compatible pin controllers
    #
    # Includes the offset by PINCTRL_FUNCTION_DEVICE_BASE
    #
    # @return maximum function count
    #
    @classmethod
    def max_function_count(cls):
        max_count = 0
        for controller in cls._pin_controllers:
            count = controller.function_count()
            if count > max_count:
                max_count = count
        return max_count + cls._pinctrl_function_device_base

    ##
    # @brief Get the maximum state name count of all compatible pin controllers
    #
    # @return maximum state name count
    #
    @classmethod
    def max_state_name_count(cls):
        max_count = 0
        for controller in cls._pin_controllers:
            count = controller.state_name_count()
            if count > max_count:
                max_count = count
        return max_count

    ##
    # @brief Get the maximum state count of all compatible pin controllers
    #
    # @return maximum state count
    #
    @classmethod
    def max_state_count(cls):
        max_count = 0
        for controller in cls._pin_controllers:
            count = controller.state_count()
            if count > max_count:
                max_count = count
        return max_count

    ##
    # @brief Get the maximum pinctrl count of all compatible pin controllers
    #
    # @return maximum pinctrl count
    #
    @classmethod
    def max_pinctrl_count(cls):
        max_count = 0
        for controller in cls._pin_controllers:
            count = controller.pinctrl_count()
            if count > max_count:
                max_count = count
        return max_count

    # Get the maximum pin count of all compatible pin controllers
    @classmethod
    def max_pin_count(cls):
        max_count = 0
        for controller in cls._pin_controllers:
            count = controller.pin_count()
            if count > max_count:
                max_count = count
        return max_count

    def __init__(self, device_id):
        self._device_id = device_id
        self._state_name_data = []
        self._function_data = []
        self._state_data = []
        self._pinctrl_data = []
        self._gpio_range_data = []
        self._init_pinctrl()
        self._init_gpio_ranges()

    def _init_pinctrl(self):
        for device_id, device in codegen.edts()['devices'].items():
            if 'pinctrl' in device:
                for pinctrl_idx, pinctrl in device['pinctrl'].items():
                    pinctrl_name = pinctrl['name']
                    for pinconf_index in pinctrl['pinconf']:
                        if pinctrl['pinconf'][pinconf_index]['pin-controller'] != self._device_id:
                            # other pin controller
                            continue
                        # pinconf is for us
                        # assure state is available
                        state = dict()
                        if pinctrl_name not in self._state_name_data:
                            self._state_name_data.append(pinctrl_name)
                        state['state-name-id'] = self._state_name_data.index(pinctrl_name)
                        if device['label'] not in self._function_data:
                            self._function_data.append(device['label'])
                        state['function-id'] = self._function_data.index(device['label'])
                        if state not in self._state_data:
                            self._state_data.append(state)
                        # setup pin control data
                        name = None
                        pins = []
                        muxes = []
                        config = dict()
                        for pinconf_prop, pinconf_value in pinctrl['pinconf'][pinconf_index].items():
                            if pinconf_prop == 'name':
                                name = pinconf_value
                            elif pinconf_prop == 'pin-controller':
                                continue
                            elif pinconf_prop == 'pinmux':
                                for i in range(0, len(pinconf_value), 2):
                                    pins.append(pinconf_value[i])
                                    muxes.append(pinconf_value[i + 1])
                            elif pinconf_prop == 'pins':
                                for pin in pinconf_value:
                                    pins.append(int(pin))
                                    muxes.append(None)
                            elif pinconf_prop == 'group':
                                codegen.error("Pinctrl does not support 'group'.")
                            else:
                                config[pinconf_prop] = pinconf_value
                        for i, pin in enumerate(pins):
                            control = dict()
                            control['name'] = name
                            control['pin'] = pin
                            control['mux'] = muxes[i]
                            control['config'] = config
                            control['state-id'] = self._state_data.index(state)
                            self._pinctrl_data.append(control)


    def _init_gpio_ranges(self):
        for device_id, device in codegen.edts()['devices'].items():
            if 'gpio-ranges' in device:
                for gpio_range_idx, gpio_range in device['gpio-ranges'].items():
                    if gpio_range['pin-controller'] == self._device_id:
                        control = dict()
                        control['name'] = device['label']
                        control['base'] = gpio_range['pin-controller-base']
                        control['gpio-base-idx'] = gpio_range['base']
                        control['npins'] = gpio_range['npins']
                        self._gpio_range_data.append(control)


    def device_id(self):
        return self._device_id

    def function_data(self):
        return self._function_data

    def function_id(self, device_label):
        return self._function_data.index(device_label)

    ##
    # @brief Get the function count of a pin controller
    #
    # @return function count
    def function_count(self):
        return len(self._function_data)

    def state_name_data(self):
        return self._state_name_data

    def state_name_id(self, state_name):
        return self._state_name_data.index(state_name)

    def state_name(self, state_name_id):
        return self._state_name_data[state_name_id]

    ##
    # @brief Get the state name count of a pin controller
    #
    # @return state name count
    def state_name_count(self):
        return len(self._state_name_data)

    def state_data(self):
        return self._state_data

    ##
    # @brief Get the state count of a pin controller
    #
    # @return state count
    def state_count(self):
        return len(self._state_data)

    ##
    # @brief Get the state id
    #
    # @param device_label
    # @param state_name
    # @return state id
    def state_id(self, device_label, state_name):
        state_name_id = self.state_name_id(state_name)
        function_id = self.function_id(device_label)
        for state_id, state in enumerate(self._state_data):
            if state['state-name-id'] == state_name_id and state['function-id'] == function_id:
                return state_id
        return None

    def state_desc(self, state_id):
        state = self._state_data[state_id]
        state_name = self._state_name_data[state['state-name-id']]
        function = self._function_data[state['function-id']]
        return "{} {}".format(function, state_name)

    def pinctrl_data(self, state_name = None):
        if state_name is None:
            return self._pinctrl_data
        data = []
        state_name_id = self.state_name_id(state_name)
        for pinctrl in self._pinctrl_data:
            state = self._state_data[pinctrl['state-id']]
            if state['state-name-id'] == state_name_id:
                data.append(pinctrl)
        return data

    ##
    # @brief Get the pinctrl count of a pin controller
    #
    # @return pinctrl count
    def pinctrl_count(self):
        return len(self._pinctrl_data)

    def gpio_range_data(self):
        return self._gpio_range_data

    ##
    # @brief Get the gpio range of a pin controller
    #
    # @return gpio range count
    def gpio_range_count(self):
        return len(self._gpio_range_data)

    ##
    # @brief Get the the number of pins the pin controller controls
    #
    # @return pin count
    #
    def pin_count(self):
        pin_low = 9999999
        pin_high = 0
        for control in self._gpio_range_data:
            low = control['base']
            high = control['base'] + control['npins']
            if low < pin_low:
                pin_low = low
            if high > pin_high:
                pin_high = high
        pin_count = pin_high - pin_low
        if pin_count < 0:
            pin_count = 0
        return pin_count






