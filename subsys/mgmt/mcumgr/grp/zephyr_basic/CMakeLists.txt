#
# Copyright (c) 2021-2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

add_library(mgmt_mcumgr_grp_zephyr)

target_sources_ifdef(CONFIG_MCUMGR_GRP_BASIC_CMD_STORAGE_ERASE
  mgmt_mcumgr_grp_zephyr PRIVATE
  src/basic_mgmt.c
)

target_link_libraries(mgmt_mcumgr_grp_zephyr PRIVATE
  zephyr_interface mgmt_mcumgr_mgmt mgmt_mcumgr_util
)

target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_grp_zephyr)
