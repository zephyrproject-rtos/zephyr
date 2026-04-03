# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_EC_HOST_CMD          ec_host_cmd)
add_subdirectory_ifdef(CONFIG_MCUMGR               mcumgr)
add_subdirectory_ifdef(CONFIG_HAWKBIT              hawkbit)
add_subdirectory_ifdef(CONFIG_UPDATEHUB            updatehub)
add_subdirectory_ifdef(CONFIG_OSDP                 osdp)
