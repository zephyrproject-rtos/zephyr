# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import hashlib

from west import log


def getHashes(filePath):
    """
    Scan for and return hashes.

    Arguments:
        - filePath: path to file to scan.
    Returns: tuple of (SHA1, SHA256, MD5) hashes for filePath, or
             None if file is not found.
    """
    hSHA1 = hashlib.sha1()
    hSHA256 = hashlib.sha256()
    hMD5 = hashlib.md5()

    log.dbg(f"  - getting hashes for {filePath}")

    try:
        with open(filePath, 'rb') as f:
            buf = f.read()
            hSHA1.update(buf)
            hSHA256.update(buf)
            hMD5.update(buf)
    except OSError:
        return None

    return (hSHA1.hexdigest(), hSHA256.hexdigest(), hMD5.hexdigest())
