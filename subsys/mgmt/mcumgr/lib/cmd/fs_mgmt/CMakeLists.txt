#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

target_include_directories(MCUMGR INTERFACE include)

zephyr_library_sources(
	src/zephyr_fs_mgmt.c
	src/fs_mgmt.c
)

zephyr_library_sources_ifdef(CONFIG_FS_MGMT_CHECKSUM_HASH src/hash_checksum_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_FS_MGMT_CHECKSUM_IEEE_CRC32 src/hash_checksum_crc32.c)
zephyr_library_sources_ifdef(CONFIG_FS_MGMT_HASH_SHA256 src/hash_checksum_sha256.c)

if (CONFIG_FS_MGMT_CHECKSUM_HASH AND CONFIG_FS_MGMT_HASH_SHA256)
    if (NOT CONFIG_TINYCRYPT)
        zephyr_link_libraries(mbedTLS)
    endif()
endif()
