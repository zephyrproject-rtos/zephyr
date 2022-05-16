/*
 * Copyright (c) 2018-2022 mcumgr authors
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_HASH_CHECKSUM_MGMT_
#define H_HASH_CHECKSUM_MGMT_

#include <zephyr/zephyr.h>
#include <zephyr/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @typedef hash_checksum_mgmt_handler_fn
 * @brief Function that gets called to generate a hash or checksum.
 *
 * @param file		Opened file context
 * @param output	Output buffer for hash/checksum
 * @param out_len	Updated with size of input data
 * @param len		Maximum length of data to perform hash/checksum on
 *
 * @return 0 on success, negative error code on failure.
 */
typedef int (*hash_checksum_mgmt_handler_fn)(struct fs_file_t *file,
					     uint8_t *output, uint32_t *out_len,
					     size_t len);

/**
 * @brief A collection of handlers for an entire hash/checksum group.
 */
struct hash_checksum_mgmt_group {
	/** Entry list node. */
	sys_snode_t node;

	/** Array of handlers; one entry per name. */
	const char *group_name;

	/** Byte string or numerical output. */
	bool byte_string;

	/** Size (in bytes) of output. */
	uint8_t output_size;

	/** Hash/checksum function pointer. */
	hash_checksum_mgmt_handler_fn function;
};

/**
 * @brief Registers a full hash/checksum group.
 *
 * @param group The group to register.
 */
void hash_checksum_mgmt_register_group(struct hash_checksum_mgmt_group *group);

/**
 * @brief Unregisters a full hash/checksum group.
 *
 * @param group The group to register.
 */
void hash_checksum_mgmt_unregister_group(struct hash_checksum_mgmt_group *group);

/**
 * @brief Finds a registered hash/checksum handler.
 *
 * @param name	The name of the hash/checksum group to find.
 *
 * @return	The requested hash/checksum handler on success;
 *		NULL on failure.
 */
const struct hash_checksum_mgmt_group *hash_checksum_mgmt_find_handler(const char *name);

#ifdef __cplusplus
}
#endif

#endif
