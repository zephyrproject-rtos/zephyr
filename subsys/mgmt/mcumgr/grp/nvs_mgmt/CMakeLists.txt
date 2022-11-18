#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# Non-Volatile Storage management group public API is exposed by MCUmgr API
# interface, when NVS management is enabled.
add_library(mgmt_mcumgr_grp_nvs src/nvs_mgmt.c)

#if (CONFIG_FS_MGMT_CHECKSUM_HASH AND CONFIG_FS_MGMT_HASH_SHA256)
#  if (NOT CONFIG_TINYCRYPT)
#    target_link_libraries(mgmt_mcumgr_grp_fs PRIVATE mbedTLS)
#  endif()
#endif()

target_include_directories(mgmt_mcumgr_grp_nvs PUBLIC include)

target_link_libraries(mgmt_mcumgr_grp_nvs PRIVATE
  zephyr_interface mgmt_mcumgr_mgmt mgmt_mcumgr_util
)

target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_grp_nvs)
