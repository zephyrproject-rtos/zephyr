#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# OS Management client group public API is exposed through zephyr_interface,
# when OS Management is enabled.
zephyr_library(mgmt_mcumgr_grp_os_client)
zephyr_library_sources(src/os_mgmt_client.c)
zephyr_library_include_directories(include)
