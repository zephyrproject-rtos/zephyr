#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# Transport management group public API is exposed by MCUmgr API
# interface, when transport management is enabled.
zephyr_library()
zephyr_library_sources(src/transport_mgmt.c)
