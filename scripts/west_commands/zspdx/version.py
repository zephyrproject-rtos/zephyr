# Copyright (c) 2025 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from packaging.version import Version

SPDX_VERSION_2_2 = Version("2.2")
SPDX_VERSION_2_3 = Version("2.3")

SUPPORTED_SPDX_VERSIONS = [
    SPDX_VERSION_2_2,
    SPDX_VERSION_2_3,
]


def parse(version_str):
    v = Version(version_str)
    if v not in SUPPORTED_SPDX_VERSIONS:
        raise ValueError(f"Unsupported SPDX version: {version_str}")
    return v
