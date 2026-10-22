# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(rtk.c)

add_subdirectory(protocol)
add_subdirectory_ifdef(CONFIG_GNSS_RTK_SERIAL serial)
