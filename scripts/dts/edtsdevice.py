#
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

class EDTSDevice:
    def __init__(self, device):
        self.device = device

    ##
    # @brief Get device tree property value for the device of the given device id.
    #
    # @param property_path Path of the property to access
    #                      (e.g. 'reg/0', 'interrupts/prio', 'device_id', ...)
    # @return property value
    #
    def get_property(self, property_path, default="<unset>"):
        device_id = self.device['device-id']
        property_value = self.device
        property_path_elems = property_path.strip("'").split('/')
        for elem_index, key in enumerate(property_path_elems):
            if isinstance(property_value, dict):
                property_value = property_value.get(key, None)
            elif isinstance(property_value, list):
                if int(key) < len(property_value):
                    property_value = property_value[int(key)]
                else:
                    property_value = None
            else:
                property_value = None
        if property_value is None:
            if default == "<unset>":
                default = "Device tree property {} not available in {}" \
                                .format(property_path, device_id)
            return default
        return property_value

    def _properties_flattened(self, properties, path, flattened, path_prefix):
        if isinstance(properties, dict):
            for prop_name in properties:
                super_path = "{}/{}".format(path, prop_name).strip('/')
                self._properties_flattened(properties[prop_name],
                                                  super_path, flattened,
                                                  path_prefix)
        elif isinstance(properties, list):
            for i, prop in enumerate(properties):
                super_path = "{}/{}".format(path, i).strip('/')
                self._properties_flattened(prop, super_path, flattened,
                                                  path_prefix)
        else:
            flattened[path_prefix + path] = properties

    ##
    ##
    # @brief Get the device tree properties for the device of the given device id.
    #
    # @param device_id
    # @param path_prefix
    # @return dictionary of property_path and property_value
    def properties_flattened(self, path_prefix = ""):
        flattened = dict()
        self._properties_flattened(self.device, '', flattened, path_prefix)
        return flattened
