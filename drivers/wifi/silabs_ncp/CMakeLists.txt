# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(wifi_ncp.c wifi_ncp_supp.c)
zephyr_library_link_libraries(hostap)
