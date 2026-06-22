# Copyright (c) 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import logging

_logger = logging.getLogger(__name__)


# Parse a CMakeCache file and return a dict of key:value (discarding
# type hints).
def parseCMakeCacheFile(filePath):
    _logger.debug("parsing CMake cache file at %s", filePath)
    kv = {}
    try:
        with open(filePath) as f:
            # should be a short file, so we'll use readlines
            lines = f.readlines()

            # walk through and look for non-comment, non-empty lines
            for line in lines:
                sline = line.strip()
                if sline == "":
                    continue
                if sline.startswith("#") or sline.startswith("//"):
                    continue

                # parse out : and = characters
                pline1 = sline.split(":", maxsplit=1)
                if len(pline1) != 2:
                    continue
                pline2 = pline1[1].split("=", maxsplit=1)
                if len(pline2) != 2:
                    continue
                kv[pline1[0]] = pline2[1]

            return kv

    except OSError:
        _logger.exception("Error loading %s", filePath)
        return {}
