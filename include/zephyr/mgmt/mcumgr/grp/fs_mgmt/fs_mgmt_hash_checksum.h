/*
 * Copyright (c) 2018-2022 mcumgr authors
 * Copyright (c) 2022 Laird Connectivity
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MGMT_MCUMGR_GRP_FS_MGMT_CHKSUM_
#define H_MGMT_MCUMGR_GRP_FS_MGMT_CHKSUM_

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @typedef fs_mgmt_hash_checksum_handler_fn
 * @brief Function that gets called to generate a hash or checksum.
 *
 * @param file		Opened file context
 * @param output	Output buffer for hash/checksum
 * @param out_len	Updated with size of input data
 * @param len		Maximum length of data to perform hash/checksum on
 *
 * @return 0 on success, negative error code on failure.
 */
typedef int (*fs_mgmt_hash_checksum_handler_fn)(struct fs_file_t *file, uint8_t *output,
						size_t *out_len, size_t len);

/**
 * @brief A collection of handlers for an entire hash/checksum group.
 */
struct fs_mgmt_hash_checksum_group {
	/** Entry list node. */
	sys_snode_t node;

	/** Array of handlers; one entry per name. */
	const char *group_name;

	/** Byte string or numerical output. */
	bool byte_string;

	/** Size (in bytes) of output. */
	uint8_t output_size;

	/** Hash/checksum function pointer. */
	fs_mgmt_hash_checksum_handler_fn function;
};

/** @typedef fs_mgmt_hash_checksum_list_cb
 * @brief Function that gets called with hash/checksum details
 *
 * @param group         Details about a supported hash/checksum
 * @param user_data     User-supplied value to calling function
 */
typedef void (*fs_mgmt_hash_checksum_list_cb)(const struct fs_mgmt_hash_checksum_group *group,
					      void *user_data);

/**
 * @brief Registers a full hash/checksum group.
 *
 * @param group The group to register.
 */
void fs_mgmt_hash_checksum_register_group(struct fs_mgmt_hash_checksum_group *group);

/**
 * @brief Unregisters a full hash/checksum group.
 *
 * @param group The group to register.
 */
void fs_mgmt_hash_checksum_unregister_group(struct fs_mgmt_hash_checksum_group *group);

/**
 * @brief Finds a registered hash/checksum handler.
 *
 * @param name	The name of the hash/checksum group to find.
 *
 * @return	The requested hash/checksum handler on success;
 *		NULL on failure.
 */
const struct fs_mgmt_hash_checksum_group *fs_mgmt_hash_checksum_find_handler(const char *name);

/**
 * @brief Runs a callback with all supported hash/checksum types.
 *
 * @param cb		The callback function to call with each hash/checksum type.
 * @param user_data	Data to pass back with the callback function.
 */
void fs_mgmt_hash_checksum_find_handlers(fs_mgmt_hash_checksum_list_cb cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* ifndef H_MGMT_MCUMGR_GRP_FS_MGMT_CHKSUM_ */
