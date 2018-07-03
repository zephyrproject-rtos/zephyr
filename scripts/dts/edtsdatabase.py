#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from collections.abc import Mapping
import json

##
# @brief Extended DTS database
#
# Database schema:
#
# _edts dict(
#   'devices':  dict(device-id :  device-struct),
#   'compatibles':  dict(compatible : sorted list(device-id)),
#   'device-types': dict(device-type : sorted list(device-id)),
#   ...
# )
#
# device-struct: dict(
#   'device-id' : device-id,
#   'device-type' : list(device-type) or device-type,
#   'compatible' : list(compatible) or compatible,
#   'label' : label,
#   property-name : property-value ...
# )
#
# Database types:
#
# device-id: <compatible>_<base address in hex>_<offset in hex>,
# compatible: any of ['st,stm32-spi-fifo', ...] - 'compatibe' from <binding>.yaml
# device-type: any of ['GPIO', 'SPI', 'CAN', ...] - 'id' from <binding>.yaml
# label: any of ['UART_0', 'SPI_11', ...] - label directive from DTS
#
class EDTSDatabase(Mapping):

    def __init__(self, *args, **kw):
        self._edts = dict(*args, **kw)
        # setup basic database schema
        for edts_key in ('devices', 'compatibles', 'device-types'):
            if not edts_key in self._edts:
                self._edts[edts_key] = dict()

    def __getitem__(self, key):
        return self._edts[key]

    def __iter__(self):
        return iter(self._edts)

    def __len__(self):
        return len(self._edts)

    def convert_string_to_label(self, s):
        # Transmute ,-@/ to _
        s = s.replace("-", "_")
        s = s.replace(",", "_")
        s = s.replace("@", "_")
        s = s.replace("/", "_")
        # Uppercase the string
        s = s.upper()
        return s

    def device_id(self, compatible, base_address, offset):
        # sanitize
        base_address = int(base_address)
        offset = int(offset)
        compatibe = self.convert_string_to_label(compatible.strip())
        return "{}_{:08X}_{:02X}".format(compatible, base_address, offset)

    def device_id_by_label(self, label):
        for device in self._edts['devices']:
            if label == device['label']:
                return device['device-id']

    def _update_device_compatible(self, device_id, compatible):
        if compatible not in self._edts['compatibles']:
            self._edts['compatibles'][compatible] = list()
        if device_id not in self._edts['compatibles'][compatible]:
            self._edts['compatibles'][compatible].append(device_id)
            self._edts['compatibles'][compatible].sort()

    def _update_device_type(self, device_id, device_type):
        if device_type not in self._edts['device-types']:
            self._edts['device-types'][device_type] = list()
        if device_id not in self._edts['device-types'][device_type]:
            self._edts['device-types'][device_type].append(device_id)
            self._edts['device-types'][device_type].sort()

    ##
    # @brief Insert property value for the device of the given device id.
    #
    # @param device_id
    # @param property_path Path of the property to access
    #                      (e.g. 'reg/0', 'interrupts/prio', 'label', ...)
    # @param property_value value
    #
    def insert_device_property(self, device_id, property_path, property_value):
        # special properties
        if property_path == 'compatible':
            self._update_device_compatible(device_id, property_value)
        elif property_path == 'device-type':
            self._update_device_type(device_id, property_value)

        # normal property management
        if device_id not in self._edts['devices']:
            self._edts['devices'][device_id] = dict()
            self._edts['devices'][device_id]['device-id'] = device_id
        if property_path == 'device-id':
            # already set
            return
        keys = property_path.strip("'").split('/')
        property_access_point = self._edts['devices'][device_id]
        for i in range(0, len(keys)):
            if i < len(keys) - 1:
                # there are remaining keys
                if keys[i] not in property_access_point:
                    property_access_point[keys[i]] = dict()
                property_access_point = property_access_point[key[i]]
            else:
                # we have to set the property value
                if keys[i] in property_access_point:
                    # There is already a value set
                    current_value = property_access_point[keys[i]]
                    if not isinstance(current_value, list):
                        current_value = [current_value, ]
                    if isinstance(property_value, list):
                        property_value = current_value.extend(property_value)
                    else:
                        property_value = current_value.append(property_value)
                property_access_point[keys[i]] = property_value

    ##
    #
    # @return list of devices that are compatible
    def compatible_devices(self, compatible):
        devices = list()
        for device_id in self._edts['compatibles'][compatible]:
            devices.append(self._edts['devices'][device_id])
        return devices

    ##
    #
    # @return list of device ids of activated devices that are compatible
    def compatible_devices_id(self, compatible):
        return self._edts['compatibles'].get(compatible, [])

    ##
    # @brief Get device tree property value for the device of the given device id.
    #
    #
    # @param device_id
    # @param property_path Path of the property to access
    #                      (e.g. 'reg/0', 'interrupts/prio', 'device_id', ...)
    # @return property value
    #
    def device_property(self, device_id, property_path, default="<unset>"):
        property_value = self._edts['devices'][device_id]
        for key in property_path.strip("'").split('/'):
            if isinstance(property_value, list):
                # TODO take into account prop with more than 1 elements of cell_size > 1
                if isinstance(property_value[0], dict):
                    property_value = property_value[0]
            try:
                property_value = property_value[str(key)]
            except TypeError:
                property_value = property_value[int(key)]
            except KeyError:
                    # we should have a dict
                    if isinstance(property_value, dict):
                        # look for key in labels
                        for x in range(0, len(property_value['labels'])):
                            if property_value['labels'][x] == key:
                                property_value = property_value['data'][x]
                                break
                    else:
                        return "Dict was expected here"
        if property_value is None:
            if default == "<unset>":
                default = \
                "Device tree property {} not available in {}[{}]".format(property_path, compatible, device_index)
            return default
        return property_value

    def load(self, file_path):
        with open(file_path, "r") as load_file:
            self._edts = json.load(load_file)

    def save(self, file_path):
        with open(file_path, "w") as save_file:
            json.dump(self._edts, save_file, indent = 4)

