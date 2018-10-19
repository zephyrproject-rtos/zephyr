#!/usr/bin/env python3
#
# Copyright (c) 2018 Bobby Noelte
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

import json
from pathlib import Path

##
# @brief ETDS Database provider
#
# Methods for ETDS database creation.
#
class EDTSHeaderMakerMixin(object):
    __slots__ = []

    @staticmethod
    def _convert_string_to_label(s):
        # Transmute ,-@/ to _
        s = s.replace("-", "_")
        s = s.replace(",", "_")
        s = s.replace("@", "_")
        s = s.replace("/", "_")
        # Uppercase the string
        s = s.upper()
        return s

    ##
    # @brief Write header file
    #
    # @param file_path Path of the file to write
    #
    def export_header(self, file_path):
        header = "/* device tree header file - do not change - generated automatically */\n"
        for device_id in self._edts['devices']:
            path_prefix = device_id + ':'
            device = cogeno.edts().get_device_by_device_id(device_id)
            properties = device.get_properties_flattened(path_prefix)
            label = device.get_property('label').strip('"')
            for prop_path, prop_value in properties.items():
                header_label = self._convert_string_to_label(
                                        "DT_{}_{}".format(label, prop_path))
                header += "#define {}\t{}\n".format(header_label, prop_value)
        with Path(file_path).open(mode="w", encoding="utf-8") as save_file:
            save_file.write(header)
