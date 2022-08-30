#
# Copyright (c) 2018-2021 mcumgr authors
#
# SPDX-License-Identifier: Apache-2.0
#

target_include_directories(MCUMGR INTERFACE
    include
)

zephyr_library_sources(
    src/zephyr_img_mgmt.c
    src/zephyr_img_mgmt_log.c
    src/img_mgmt.c
    src/img_mgmt_state.c
    src/img_mgmt_util.c
)
