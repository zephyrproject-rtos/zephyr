# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
from dts.edtsdatabase import EDTSDatabase

class EDTSMixin(object):
    __slots__ = []

    _edts = None

    def _edts_assure(self):
        if self._edts is None:
            edts_file = self.cmake_variable("GENERATED_DTS_BOARD_EDTS")
            edts_file = Path(edts_file)
            if not edts_file.is_file():
                raise self._get_error_exception(
                    "Generated extended device tree database file '{}' not found/ no access.".
                    format(edts_file), 2)
            self._edts = EDTSDatabase()
            self._edts.load(str(edts_file))

    ##
    # @brief Get the extended device tree database.
    #
    # @return Extended device tree database.
    #
    def edts(self):
        self._edts_assure()
        return self._edts
