#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# Statistics management group public API is exposed by MCUmgr API
# interface, when Statistics management is enabled.
zephyr_library(mgmt_mcumgr_grp_stat)
zephyr_library_sources(src/stat_mgmt.c)

zephyr_library_include_directories(include)
