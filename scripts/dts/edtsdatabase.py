#!/usr/bin/env python3
#
# Copyright (c) 2018 Bobby Noelte
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

from pathlib import Path
from collections.abc import Mapping
import argparse
import json
import pprint
from edtsdevice import EDTSDevice

##
# @brief ETDS Database consumer
#
# Methods for ETDS database usage.
#
class EDTSConsumerMixin(object):
    __slots__ = []

    ##
    # @brief Get device ids of all activated compatible devices.
    #
    # @param compatibles compatible(s)
    # @return list of device ids of activated devices that are compatible
    def get_device_ids_by_compatible(self, compatibles):
        device_ids = dict()
        if not isinstance(compatibles, list):
            compatibles = [compatibles, ]
        for compatible in compatibles:
            for device_id in self._edts['compatibles'].get(compatible, []):
                device_ids[device_id] = 1
        return list(device_ids.keys())

    ##
    # @brief Get device id of activated device with given label.
    #
    # @return device id
    def get_device_id_by_label(self, label):
        for device_id, device in self._edts['devices'].items():
            if label == device.get('label', None):
                return device_id
        return None

    ##
    # @brief Get the device dict matching a device_id.
    #
    # @param device_id
    # @return (dict)device
    def get_device_by_device_id(self, device_id):
        try:
            return EDTSDevice(self._edts['devices'][device_id])
        except:
            return None

    def load(self, file_path):
        with open(file_path, "r") as load_file:
            self._edts = json.load(load_file)
##
# @brief ETDS Database provider
#
# Methods for ETDS database creation.
#
class EDTSProviderMixin(object):
    __slots__ = []

    def _update_device_compatible(self, device_id, compatible):
        if compatible not in self._edts['compatibles']:
            self._edts['compatibles'][compatible] = list()
        if device_id not in self._edts['compatibles'][compatible]:
            self._edts['compatibles'][compatible].append(device_id)
            self._edts['compatibles'][compatible].sort()

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
        if property_path.startswith('compatible'):
            self._update_device_compatible(device_id, property_value)

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
                property_access_point = property_access_point[keys[i]]
            else:
                # we have to set the property value
                if keys[i] in property_access_point and \
                   property_access_point[keys[i]] != property_value:
                   # Property is already set and we're overwiting with a new
                   # different value. Tigger an error
                    raise Exception("Overwriting edts cell {} with different value\n \
                                     Initial value: {} \n \
                                     New value: {}".format(property_access_point,
                                     property_access_point[keys[i]],
                                     property_value
                                     ))
                property_access_point[keys[i]] = property_value


    def save(self, file_path):
        with open(file_path, "w") as save_file:
            json.dump(self._edts, save_file, indent = 4, sort_keys=True)

##
# @brief Extended DTS database
#
# Database schema:
#
# _edts dict(
#   'devices':  dict(device-id :  device-struct),
#   'compatibles':  dict(compatible : sorted list(device-id)),
#   ...
# )
#
# device-struct dict(
#   'device-id' : device-id,
#   'compatible' : list(compatible) or compatible,
#   'label' : label,
#   property-name : property-value ...
# )
#
# Database types:
#
# device-id: opaque id for a device (do not use for other purposes),
# compatible: any of ['st,stm32-spi-fifo', ...] - 'compatibe' from <binding>.yaml
# label: any of ['UART_0', 'SPI_11', ...] - label directive from DTS
#
class EDTSDatabase(EDTSConsumerMixin, EDTSProviderMixin, Mapping):

    def __init__(self, *args, **kw):
        self._edts = dict(*args, **kw)
        # setup basic database schema
        for edts_key in ('devices', 'compatibles'):
            if not edts_key in self._edts:
                self._edts[edts_key] = dict()

    def __getitem__(self, key):
        return self._edts[key]

    def __iter__(self):
        return iter(self._edts)

    def __len__(self):
        return len(self._edts)

    def parse_arguments(self):
        rdh = argparse.RawDescriptionHelpFormatter
        parser = argparse.ArgumentParser(description=__doc__, formatter_class=rdh)

        parser.add_argument("-e", "--edts", nargs=1, required=True,
                            help="edts json file")

        return parser.parse_args()

    def main(self):
        args = self.parse_arguments()
        edts_file = Path(args.edts[0])
        if not edts_file.is_file():
            raise self._get_error_exception(
                "Generated extended device tree database file '{}' not found/ no access.".
                format(edts_file), 2)
        self._edts = EDTSDatabase()
        self._edts.load(str(edts_file))

        pprint.pprint(dict(self._edts))

        return 0


if __name__ == '__main__':
    EDTSDatabase().main()
