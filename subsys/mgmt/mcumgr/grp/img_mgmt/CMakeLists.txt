#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# Image management group public API is exported by MCUmgr interface API,
# when Image Management is enabled.
add_library(mgmt_mcumgr_grp_img
  src/zephyr_img_mgmt.c
  src/zephyr_img_mgmt_log.c
  src/img_mgmt_state.c
  src/img_mgmt_util.c
  src/img_mgmt.c
)

target_include_directories(mgmt_mcumgr_grp_img PUBLIC include)

# Allow fs group interface to be included out of MCUmgr subsystem scope
target_link_libraries(mgmt_mcumgr_grp_img PRIVATE
  zephyr_interface mgmt_mcumgr_mgmt mgmt_mcumgr_util
)

if(CONFIG_MCUBOOT_IMG_MANAGER)
  target_link_libraries(mgmt_mcumgr_grp_img PRIVATE MCUBOOT_BOOTUTIL)
endif()

target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_grp_img)
