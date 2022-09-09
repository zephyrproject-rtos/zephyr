#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MCUMGR_GRP_BASIC_CMD_STORAGE_ERASE src/basic_mgmt.c)
zephyr_library_link_libraries(MCUMGR)
