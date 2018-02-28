# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import shlex
from pathlib import Path

class ConfigMixin(object):
    __slots__ = []

    _autoconf = None
    _autoconf_filename = None

    def _autoconf_assure(self):
        if self._autoconf is None:
            autoconf_file = self.cmake_variable("PROJECT_BINARY_DIR", None)
            if autoconf_file is None:
                if default == "<unset>":
                    raise self._get_error_exception(
                        "CMake variable PROJECT_BINARY_DIR not defined to codegen.", 2)
                return default
            autoconf_file = Path(autoconf_file).joinpath('include/generated/autoconf.h')
            if not autoconf_file.is_file():
                if default == "<unset>":
                    default = \
                    "Generated configuration {} not found/ no access.".format(autoconf_file)
                return default
            autoconf = {}
            with open(str(autoconf_file)) as autoconf_fd:
                for line in autoconf_fd:
                    if not line.startswith('#'):
                        continue
                    if " " not in line:
                        continue
                    key, value = shlex.split(line)[1:]
                    autoconf[key] = value
            self._autoconf = autoconf
            self._autoconf_filename = str(autoconf_file)

    def config_property(self, property_name, default="<unset>"):
        self._autoconf_assure()
        property_value = self._autoconf.get(property_name, default)
        if property_value == "<unset>":
            raise self._get_error_exception(
                "Config property '{}' not defined.".format(property_name), 1)
        return property_value

    ##
    # @brief Get all config properties.
    #
    # The property names are the ones autoconf.conf.
    #
    # @return A dictionary of config properties.
    #
    def config_properties(self):
        self._autoconf_assure()
        return self._autoconf
