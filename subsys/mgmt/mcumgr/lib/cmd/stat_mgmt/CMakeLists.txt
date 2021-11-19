#
# Copyright (c) 2018-2021 mcumgr authors
#
# SPDX-License-Identifier: Apache-2.0
#

target_include_directories(MCUMGR INTERFACE
    include
)

zephyr_library_sources(
    src/zephyr_stat_mgmt.c
    src/stat_mgmt.c
)
