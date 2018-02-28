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
    # @brief Import a CodeGen module.
    #
    # Import a module from the codegen/modules package.
    #
    # @param name Module to import. Specified without any path.
    #
    def import_module(self, name):
        try:
            module_file = self.zephyr_path().joinpath(
                "scripts/codegen/modules/{}.py".format(name)).resolve()
        except FileNotFoundError:
            # Python 3.4/3.5 will throw this exception
            # Python >= 3.6 will not throw this exception
            module_file = self.zephyr_path().joinpath(
                "scripts/codegen/modules/{}.py".format(name))
        if not module_file.is_file():
            raise self._get_error_exception(
                "Module file '{}' of module '{}' does not exist or is no file.".
                format(module_file, name), 1)
        sys.path.append(os.path.dirname(str(module_file)))
        module = importlib.import_module(name)
        sys.path.pop()
        self.generator_globals[name] = module

