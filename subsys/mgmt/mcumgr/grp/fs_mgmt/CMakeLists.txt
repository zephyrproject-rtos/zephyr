#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Laird Connectivity
# Copyright (c) 2022-2023 Nordic Semiconductor ASA
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

if(CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH AND CONFIG_MCUMGR_GRP_FS_HASH_SHA256)
  if(CONFIG_MBEDTLS_SHA256)
    zephyr_library_link_libraries(mbedTLS)
  endif()
endif()

zephyr_library_include_directories(include)

if(CONFIG_MCUMGR_GRP_FS AND NOT CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
  message(WARNING "Note: MCUmgr file system management is enabled but file access hooks are "
                  "disabled, this is an insecure configuration and not recommended for production "
                  "use, as all files on the filesystem can be manipulated by a remote device. See "
                  "https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr_callbacks.html "
                  "for details on enabling and using MCUmgr hooks.")
endif()
