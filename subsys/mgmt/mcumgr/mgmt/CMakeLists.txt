#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# Provides MCUmgr services, group registration, event handling etc.
add_library(mgmt_mcumgr_mgmt src/mgmt.c)

target_include_directories(mgmt_mcumgr_mgmt PUBLIC include)

target_link_libraries(mgmt_mcumgr_mgmt PRIVATE zephyr_interface)
target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_mgmt)

if(CONFIG_MCUMGR_SMP_BOOT_SETUP)
  if(CONFIG_MCUMGR_CMD_FS_MGMT)
    target_link_libraries(mgmt_mcumgr_mgmt PRIVATE mgmt_mcumgr_grp_fs)
  endif()

  if(CONFIG_MCUMGR_CMD_OS_MGMT)
    target_link_libraries(mgmt_mcumgr_mgmt PRIVATE mgmt_mcumgr_grp_os)
  endif()

  if(CONFIG_MCUMGR_CMD_IMG_MGMT)
    target_link_libraries(mgmt_mcumgr_mgmt PRIVATE mgmt_mcumgr_grp_img)
  endif()

  if(CONFIG_MCUMGR_CMD_STAT_MGMT)
    target_link_libraries(mgmt_mcumgr_mgmt PRIVATE mgmt_mcumgr_grp_stat)
  endif()

  if(CONFIG_MCUMGR_GRP_ZEPHYR_BASIC)
    target_link_libraries(mgmt_mcumgr_mgmt PRIVATE mgmt_mcumgr_grp_zephyr)
  endif()
endif()
