#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# Statistics management group public API is exposed by MCUmgr API
# interface, when Statistics management is enabled.
add_library(mgmt_mcumgr_grp_stat src/stat_mgmt.c)

target_include_directories(mgmt_mcumgr_grp_stat PUBLIC include)

target_link_libraries(mgmt_mcumgr_grp_stat PRIVATE
  zephyr_interface mgmt_mcumgr_mgmt mgmt_mcumgr_util
)

target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_grp_stat)
