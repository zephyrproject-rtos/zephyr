#
# Copyright (c) 2018 Linaro Limited
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

class EDTSDevice(object):

    ##
    # @brief Init Device
    #
    # @param edts EDTS database
    # @param device id Device ID within EDTS database
    def __init__(self, edts, device_id):
        if not (device_id in edts['devices']):
            raise Exception("Device with device id '{}' not available in EDTS"
                            .format(device_id))
        self._device_id = device_id
        self._edts = edts

    def get_device_id(self):
        return self._device_id

    ##
    # @brief Get devices that are children of this device.
    #
    # Returns a list of devices that are children of this device.
    #
    # @return List of devices, maybe empty.
    def get_children(self):
        child_devices = self.get_property('child-devices', None)
        if child_devices is None:
            return []
        children = []
        for i in child_devices:
            children.append(EDTSDevice(self._edts, child_devices[i]))
        return children

    ##
    # @brief Get parent device of this device.
    #
    # @return Parent device, maybe None.
    def get_parent(self):
        parent_device = self.get_property('parent-device', None)
        if parent_device is None:
            return None
        return EDTSDevice(self._edts, parent_device)

    ##
    # @brief Get device name
    #
    # Returns a unique name for the device. The name is sanitized to be used
    # in C code and allows easy identification of the device.
    #
    # Device name is generated from
    # - device compatible
    # - bus master address if the device is connected to a bus master
    # - device address
    # - parent device address if the device does not have a device address
    # - label if the parent also does not have a device address or there is
    #   no parent
    #
    # @return device name
    def get_name(self):
        device_name = self.get_property('compatible/0', None)
        if device_name is None:
            raise Exception("No compatible property for device id '{}'."
                            .format(device_id))

        bus_master_device_id = self.get_property('bus/master', None)
        if bus_master_device_id is not None:
            reg = self._edts.get_device_property(bus_master_device_id, 'reg')
            try:
                # use always the first key to get first address inserted into dict
                # because reg_index may be number or name
                # reg/<reg_index>/address/<address_index> : address
                for reg_index in reg:
                    for address_index in reg[reg_index]['address']:
                        bus = reg[reg_index]['address'][address_index]
                        device_name += '_' + hex(bus)[2:].zfill(8)
                        break
                    break
            except:
                # this device is missing the register directive
                raise Exception("No bus master register address property for device id '{}'."
                                .format(bus_master_device_id))

        reg = self.get_property('reg', None)
        if reg is None:
            # no reg property - take the reg property of the parent device
            parent_device_id = self.get_property('parent-device', None)
            if parent_device_id:
                parent_device = self._edts.get_device_by_device_id(parent_device_id)
                reg = parent_device.get_property('reg', None)
        device_address = None
        if reg is not None:
            try:
                # use always the first key to get first address inserted into dict
                # because reg_index may be number or name
                # reg/<reg_index>/address/<address_index> : address
                for reg_index in reg:
                    for address_index in reg[reg_index]['address']:
                        address = reg[reg_index]['address'][address_index]
                        device_address = hex(address)[2:].zfill(8)
                        break
                    break
            except:
                # this device is missing the register directive
                pass
        if device_address is None:
            # use label instead of address
            device_address = self.get_property('label', '<unknown>')
            # Warn about missing reg property
            print("device.py: No register address property for device id '{}'."
                  .format(self._device_id),
                  "Using '{}' instead".format(device_address.lower()))
        device_name += '_' + device_address

        device_name = device_name.replace("-", "_"). \
                                replace(",", "_"). \
                                replace(";", "_"). \
                                replace("@", "_"). \
                                replace("#", "_"). \
                                replace("&", "_"). \
                                replace("/", "_"). \
                                lower()
        return device_name

    ##
    # @brief Get device tree property value of this device.
    #
    # @param property_path Path of the property to access
    #                      (e.g. 'reg/0', 'interrupts/prio', 'device_id', ...)
    # @return property value
    #
    def get_property(self, property_path, default="<unset>"):
        property_value = self._edts['devices'][self._device_id]
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
                                .format(property_path, self._device_id)
            return default
        return property_value

    def select_property(self, *args, **kwargs):
        property_value = self._edts['devices'][self._device_id]
        for arg in args:
            arg = str(arg).strip("'")
            if arg == "FIRST":
                # take the first property
                path_elem = list(property_value.keys())[0]
                property_value = property_value[path_elem]
            else:
                for path_elem in arg.split('/'):
                    property_value = property_value[path_elem]
        return property_value

    def get_properties(self):
        return self._edts['devices'][self._device_id]

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
    # @brief Get the device tree properties flattened to property path : value.
    #
    # @param device_id
    # @param path_prefix
    # @return dictionary of property_path and property_value
    def get_properties_flattened(self, path_prefix = ""):
        flattened = dict()
        self._properties_flattened(self.get_properties(), '',
                                   flattened, path_prefix)
        return flattened
