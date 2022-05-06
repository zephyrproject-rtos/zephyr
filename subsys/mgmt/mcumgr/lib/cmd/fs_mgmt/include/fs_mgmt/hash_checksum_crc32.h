/*
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_HASH_CHECKSUM_CRC32_
#define H_HASH_CHECKSUM_CRC32_

#include <zephyr/zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the IEEE CRC32 checksum mcumgr handler.
 */
void fs_hash_checksum_mgmt_register_crc32(void);

/**
 * @brief Un-registers the IEEE CRC32 checksum mcumgr handler.
 */
void fs_hash_checksum_mgmt_unregister_crc32(void);

#ifdef __cplusplus
}
#endif

#endif
