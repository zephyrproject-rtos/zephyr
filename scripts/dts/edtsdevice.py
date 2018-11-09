#
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

class EDTSDevice:
    def __init__(self, devices, dev_id):
        self.all = devices
        self.device = devices[dev_id]

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

    def get_reg_addr(self, index=0):
        return self.device['reg'][str(index)]['address']

    def get_reg_size(self, index=0):
        return self.device['reg'][str(index)]['size']

    def _find_reg_idx_by_name(self, name):
        regs = self.device['reg']
        for idx, reg in regs.items():
            if reg['name'] == name:
                return int(idx)
        return None

    def get_reg_addr_by_name(self, name):
        try:
            idx = self._find_reg_idx_by_name(name)
            return self.get_reg_addr(idx)
        except:
            return None

    def get_reg_size_by_name(self, name):
        try:
            idx = self._find_reg_idx_by_name(name)
            return self.get_reg_size(idx)
        except:
            return None

    def get_irq(self, index=0):
        return self.device['interrupts'][str(index)]['irq']

    def get_irq_prop(self, index=0, prop='irq'):
        return self.device['interrupts'][str(index)][prop]

    def _find_irq_idx_by_name(self, name):
        irqs = self.device['interrupts']
        for idx, irq in irqs.items():
            if irq['name'] == name:
                return int(idx)
        return None

    def get_irq_by_name(self, name):
        try:
            idx = self._find_irq_idx_by_name(name)
            return self.get_irq(idx)
        except:
            return None

    def get_irq_prop_by_name(self, name, prop):
        try:
            idx = self._find_irq_idx_by_name(name)
            return self.get_irq_prop(idx, prop)
        except:
            return None

    def get_irq_prop_types(self):
        irqs = self.device['interrupts']['0']
        return list(irqs.keys())

    ##
    #
    # @note: Unique id has the following format:
    #        <PARENT>_<COMPAT_PARENT>_<ADDRESS_NODE>_<COMPAT_NODE_ADDRESS>
    # @return device unique id string
    def get_unique_id(self):
        return self.get_property('unique_id')

    def get_parent(self, parent_type='bus'):
        parent_device_id = ''

        for comp in self.device['device-id'].split('/')[1:-1]:
            parent_device_id += '/' + comp

        return EDTSDevice(self.all, parent_device_id)
