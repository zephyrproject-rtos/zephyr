#
# Copyright (c) 2018-2021 mcumgr authors
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory_ifdef(CONFIG_MCUMGR_CMD_FS_MGMT   fs_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_CMD_IMG_MGMT  img_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_CMD_OS_MGMT   os_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_CMD_STAT_MGMT stat_mgmt)
add_subdirectory_ifdef(CONFIG_MCUMGR_CMD_SHELL_MGMT shell_mgmt)
