# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import time
from pathlib import Path

##
# Make relative import work also with __main__
if __package__ is None or __package__ == '':
    # use current directory visibility
    from modules.edtsdatabase import EDTSDatabase
else:
    # use current package visibility
    from .modules.edtsdatabase import EDTSDatabase

class EDTSMixin(object):
    __slots__ = []

    _edts = None

    def _edts_assure(self):
        if self._edts is not None:
            return

        # EDTS database file
        edts_file = self._context._options.edts_file
        if edts_file is None:
                raise self._get_error_exception(
                    "No path defined for the extended device tree database file.", 2)
        edts_file = Path(edts_file)
        # DTS compiled file
        dts_file = self._context._options.dts_file
        if dts_file is None:
                raise self._get_error_exception(
                    "No path defined for the device tree specification file.", 2)
        dts_file = Path(dts_file)
        if not dts_file.is_file() and not edts_file.is_file():
            raise self._get_error_exception(
                "Device tree specification file '{}' not found/ no access.".
                format(dts_file), 2)
        # Bindings
        bindings_paths = self._context._options.bindings_paths
        if len(bindings_paths) == 0 and not edts_file.is_file():
            raise self._get_error_exception(
                    "No path defined for the device tree bindings.", 2)

        # Check whether extraction is necessary
        if edts_file.is_file():
            # EDTS file must be newer than the DTS compiler file
            dts_date = dts_file.stat().st_mtime
            edts_date = edts_file.stat().st_mtime
            if dts_date > edts_date:
                extract = True
            else:
                extract = False
        else:
            extract = True

        self.log('s{}: access EDTS {} with lock {}'
                 .format(len(self.cogeno_module_states),
                         str(edts_file), self._context._lock_file))

        # Try to get a lock for the database file
        # If we do not get the log for 10 seconds an
        # exception is thrown.
        try:
            with self.lock().acquire(timeout = 10):
                if extract:
                    self.log('s{}: extract EDTS {} from {} with bindings {}'
                             .format(len(self.cogeno_module_states),
                                     str(edts_file),
                                     str(dts_file),
                                     bindings_paths))
                    if edts_file.is_file():
                        # Remove old file
                        edts_file.unlink()
                        unlink_wait_count = 0
                        while edts_file.is_file():
                            # do dummy access to wait for unlink
                            time.sleep(1)
                            unlink_wait_count += 1
                            if unlink_wait_count > 5:
                                self.error(
                                    "Generated extended device tree database file '{}' no unlink."
                                    .format(edts_file), frame_index = 2)
                    # Create EDTS database by extraction
                    self._edts = EDTSDatabase()
                    self._edts.extract(dts_file, bindings_paths)
                    # Store file to be reused
                    self._edts.save(edts_file)
                else:
                    if not edts_file.is_file():
                        self.error(
                            "Generated extended device tree database file '{}' not found/ no access."
                            .format(edts_file), frame_index = 2)
                    self._edts = EDTSDatabase()
                    self._edts.load(edts_file)

        except self.LockTimeout:
            # Something went really wrong - we did not get the lock
            self.error(
                "Generated extended device tree database file '{}' no access."
                .format(edts_file), frame_index = 2)
        except:
            raise

    ##
    # @brief Get the extended device tree database.
    #
    # @return Extended device tree database.
    #
    def edts(self):
        self._edts_assure()
        return self._edts
