#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

import hashlib
import sys

with open("merged_nrf9160dk_0_14_0_nrf9160.hex", "rb") as f:
    md5type = hashlib.md5(usedforsecurity=False)
    md5type.update(f.read())
    nrf9160 = md5type.hexdigest()
with open("merged_nrf9160dk_0_14_0_nrf52840.hex", "rb") as f:
    md5type = hashlib.md5(usedforsecurity=False)
    md5type.update(f.read())
    nrf52840 = md5type.hexdigest()
with open("test1.hex", "rb") as f:
    md5type = hashlib.md5(usedforsecurity=False)
    md5type.update(f.read())
    test1 = md5type.hexdigest()
with open("test2.hex", "rb") as f:
    md5type = hashlib.md5(usedforsecurity=False)
    md5type.update(f.read())
    test2 = md5type.hexdigest()

if nrf9160 != test1:
    sys.exit("nrf9160 <-> test1 file contents mismatch")

if nrf52840 != test2:
    sys.exit("nrf52840 <-> test2 file contents mismatch")
