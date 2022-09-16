/*
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_HASH_CHECKSUM_SHA256_
#define H_HASH_CHECKSUM_SHA256_

#include <zephyr/zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the SHA256 hash mcumgr handler.
 */
void fs_hash_checksum_mgmt_register_sha256(void);

/**
 * @brief Un-registers the SHA256 hash mcumgr handler.
 */
void fs_hash_checksum_mgmt_unregister_sha256(void);

#ifdef __cplusplus
}
#endif

#endif
