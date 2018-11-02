# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import sys
import os
import importlib
from pathlib import Path

class ImportMixin(object):
    __slots__ = []

    ##
    # @brief Import a CoGen module.
    #
    # Import a module from the cogen/modules package.
    #
    # @param name Module to import. Specified without any path.
    #
    def import_module(self, name):
        module_name = "{}.py".format(name)
        module_file = self.find_file_path(module_name, self.modules_paths())
        if module_file is None:
            raise self._get_error_exception(
                "Module file '{}' of module '{}' does not exist or is no file.\n".
                format(module_name, name) + \
                "Searched in {}.".format(self.modules_paths()), 1)

        sys.path.append(os.path.dirname(str(module_file)))
        module = importlib.import_module(name)
        sys.path.pop()
        self._context._globals[name] = module

