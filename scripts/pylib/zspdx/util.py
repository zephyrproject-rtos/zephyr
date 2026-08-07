# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import hashlib
import logging

_logger = logging.getLogger(__name__)


def get_hashes(file_path):
    """
    Scan for and return hashes.

    Arguments:
        - file_path: path to file to scan.
    Returns: tuple of (SHA1, SHA256, MD5) hashes for file_path, or
             None if file is not found.
    """
    h_sha1 = hashlib.sha1(usedforsecurity=False)
    h_sha256 = hashlib.sha256()
    h_md5 = hashlib.md5(usedforsecurity=False)

    _logger.debug("  - getting hashes for %s", file_path)

    try:
        with open(file_path, 'rb') as f:
            buf = f.read()
            h_sha1.update(buf)
            h_sha256.update(buf)
            h_md5.update(buf)
    except OSError:
        return None

    return (h_sha1.hexdigest(), h_sha256.hexdigest(), h_md5.hexdigest())
