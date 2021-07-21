#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
if (CONFIG_MCUMGR_GRP_BASIC_CMD_STORAGE_ERASE OR CONFIG_MCUMGR_GRP_BASIC_CMD_IMAGE_LIST)
	zephyr_library_sources(basic_mgmt.c)
endif()
zephyr_library_link_libraries(MCUMGR)

if(CONFIG_MCUMGR_GRP_BASIC_CMD_IMAGE_LIST)
	zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()
