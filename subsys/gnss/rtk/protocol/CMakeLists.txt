# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_GNSS_RTK_PROTOCOL_RTCM3 rtcm3.c)
