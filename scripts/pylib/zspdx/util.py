# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import hashlib
import logging
import os
import re
from datetime import UTC, datetime

_logger = logging.getLogger(__name__)


def get_hashes(file_path):
    """
    Scan for and return hashes.

    Arguments:
        - file_path: path to file to scan.
    Returns: tuple of (SHA1, SHA256, SHA512) hashes for file_path, or
             None if file is not found.
    """
    h_sha1 = hashlib.sha1(usedforsecurity=False)
    h_sha256 = hashlib.sha256()
    h_sha512 = hashlib.sha512()

    _logger.debug("  - getting hashes for %s", file_path)

    try:
        with open(file_path, 'rb') as f:
            buf = f.read()
            h_sha1.update(buf)
            h_sha256.update(buf)
            h_sha512.update(buf)
    except OSError:
        return None

    return (h_sha1.hexdigest(), h_sha256.hexdigest(), h_sha512.hexdigest())


def source_date_epoch():
    """
    Return the time set by the SOURCE_DATE_EPOCH environment variable.

    See https://reproducible-builds.org/specs/source-date-epoch/.

    Returns: timezone-aware UTC datetime, or None if the variable is unset
             or empty.
    Raises: ValueError if the value is not a number of seconds since the
            Unix epoch.
    """
    value = os.environ.get("SOURCE_DATE_EPOCH", "")
    if not value:
        return None

    if not re.fullmatch(r"[0-9]+", value):
        raise ValueError(f"SOURCE_DATE_EPOCH is not a number of seconds: '{value}'")
    try:
        return datetime.fromtimestamp(int(value), UTC)
    except (OverflowError, OSError, ValueError) as e:
        raise ValueError(f"SOURCE_DATE_EPOCH is out of range: '{value}'") from e
