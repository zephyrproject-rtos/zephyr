# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import sys
from pathlib import Path

class PathsMixin(object):
    __slots__ = []

    _modules_paths = None
    _templates_paths = None

    @staticmethod
    def cogeno_path():
        return Path(__file__).resolve().parent

    @staticmethod
    def find_file_path(file_name, paths):
        # Assure we have a string here
        file_name = str(file_name)
        try:
            file_path = Path(file_name).resolve()
        except FileNotFoundError:
            # Python 3.4/3.5 will throw this exception
            # Python >= 3.6 will not throw this exception
            file_path = Path(file_name)
        if file_path.is_file():
            return file_path

        # don't resolve upfront
        file_dir_parts = list(Path(file_name).parent.parts)
        for path in paths:
            file_path = None
            if len(file_dir_parts) > 0:
                path_parts = list(path.parts)
                common_start_count = path_parts.count(file_dir_parts[0])
                common_start_index = 0
                while common_start_count > 0:
                    common_start_count -= 1
                    common_start_index = path_parts.index(file_dir_parts[0], common_start_index)
                    common_length = len(path_parts) - common_start_index
                    if common_length > len(file_dir_parts):
                        # does not fit
                        continue
                    if path_parts[common_start_index:] == file_dir_parts[0:common_length]:
                        # We have a common part
                        file_path = path.parents[common_length - 1].joinpath(file_name)
                        if file_path.is_file():
                            # we got it
                            return file_path
                        else:
                            # may be there is another combination
                            file_path = None
            if file_path is None:
                # Just append file path to path
                file_path = path.joinpath(file_name)
            if file_path.is_file():
                return file_path
        return None

    def modules_paths_append(self, path):
        self._context._modules_paths.append(Path(path))
        self.cogeno_module.path.extend(str(path))
        sys.path.extend(str(path))

    def modules_paths(self):
        return self._context._modules_paths

    def templates_paths_append(self, path):
        self._context._templates_paths.append(Path(path))

    def templates_paths(self):
        return self._context._templates_paths

    def bindings_paths(self):
        return self._context._options.bindings_paths




