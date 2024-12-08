#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022-2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_FS             fs_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_IMG            img_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_IMG_CLIENT     img_mgmt_client)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_OS             os_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_OS_CLIENT      os_mgmt_client)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_STAT           stat_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_SHELL          shell_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_ZBASIC         zephyr_basic)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_SETTINGS       settings_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_ENUM           enum_mgmt)
