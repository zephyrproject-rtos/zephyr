#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# Shell Management group public API is exposed by MCUmgr API
# interface, when Shell Management is enabled.
zephyr_library(mgmt_mcumgr_grp_shell)
zephyr_library_sources(src/shell_mgmt.c)

zephyr_library_include_directories(include)
