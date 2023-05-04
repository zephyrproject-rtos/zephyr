#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Laird Connectivity
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# File System management group public API is exposed by MCUmgr API
# interface, when File System management is enabled.
zephyr_library(mgmt_mcumgr_grp_fs)
zephyr_library_sources(src/fs_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH src/fs_mgmt_hash_checksum.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_GRP_FS_CHECKSUM_IEEE_CRC32 src/fs_mgmt_hash_checksum_crc32.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_GRP_FS_HASH_SHA256 src/fs_mgmt_hash_checksum_sha256.c)

if (CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH AND CONFIG_MCUMGR_GRP_FS_HASH_SHA256)
  if (NOT CONFIG_TINYCRYPT)
    zephyr_library_link_libraries(mbedTLS)
  endif()
endif()

zephyr_library_include_directories(include)
