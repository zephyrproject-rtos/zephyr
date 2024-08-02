#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# Image management group public API is exported by MCUmgr interface API,
# when Image Management is enabled.
zephyr_library(mgmt_mcumgr_grp_img)
zephyr_library_sources(
  src/zephyr_img_mgmt.c
  src/img_mgmt_state.c
  src/img_mgmt_util.c
  src/img_mgmt.c
)

zephyr_library_include_directories(include)

if(CONFIG_MCUBOOT_IMG_MANAGER)
  zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()
