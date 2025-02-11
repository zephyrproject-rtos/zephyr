#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# Image management client group public API is exported by MCUmgr interface API,
# when Image Management client is enabled.

zephyr_library(mcumgr_grp_img_client)
zephyr_library_sources(
  src/img_mgmt_client.c
)

zephyr_library_include_directories(include)
zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
