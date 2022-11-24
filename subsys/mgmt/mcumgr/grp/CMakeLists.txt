#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_FS        fs_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_IMG       img_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_OS        os_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_STAT      stat_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_SHELL     shell_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_ZBASIC	   zephyr_basic)
