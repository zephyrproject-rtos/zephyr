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
            autoconf_file = self._context._options.config_file
            if autoconf_file is None:
                raise self._get_error_exception(
                    "No path defined for the config file.", 2)
            autoconf_file = Path(autoconf_file)
            if not autoconf_file.is_file():
                raise self._get_error_exception(
                    "Config file {} not found/ no access.".
                    format(autoconf_file), 2)

            self.log('s{}: access config {}'
                     .format(len(self.cogeno_module_states),
                             str(autoconf_file)))

            autoconf = {}
            with autoconf_file.open(mode = 'r', encoding = 'utf-8') as autoconf_fd:
                for line in autoconf_fd:
                    if line.startswith('#') or not line.strip():
                        continue
                    config = line.split('=')
                    key = config[0].strip()
                    value = config[1].strip()
                    if value in ('y'):
                        value = "true"
                    elif value in ('n'):
                        value = "false"
                    else:
                        value = value.replace('"','')
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
