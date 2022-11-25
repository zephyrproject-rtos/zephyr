#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Laird Connectivity
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# File System management group public API is exposed by MCUmgr API
# interface, when File System management is enabled.
add_library(mgmt_mcumgr_grp_fs src/fs_mgmt.c)

target_sources_ifdef(CONFIG_FS_MGMT_CHECKSUM_HASH
	mgmt_mcumgr_grp_fs PRIVATE src/fs_mgmt_hash_checksum.c
)
target_sources_ifdef(CONFIG_FS_MGMT_CHECKSUM_IEEE_CRC32
	mgmt_mcumgr_grp_fs PRIVATE src/fs_mgmt_hash_checksum_crc32.c
)
target_sources_ifdef(CONFIG_FS_MGMT_HASH_SHA256
	mgmt_mcumgr_grp_fs PRIVATE src/fs_mgmt_hash_checksum_sha256.c)

if (CONFIG_FS_MGMT_CHECKSUM_HASH AND CONFIG_FS_MGMT_HASH_SHA256)
  if (NOT CONFIG_TINYCRYPT)
    target_link_libraries(mgmt_mcumgr_grp_fs PRIVATE mbedTLS)
  endif()
endif()

target_include_directories(mgmt_mcumgr_grp_fs PUBLIC include)

target_link_libraries(mgmt_mcumgr_grp_fs PRIVATE
  zephyr_interface mgmt_mcumgr_mgmt mgmt_mcumgr_util
)

target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_grp_fs)
