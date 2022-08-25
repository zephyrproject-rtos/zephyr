/*
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/crc.h>
#include <zephyr/fs/fs.h>
#include <mgmt/mgmt.h>
#include <fs_mgmt/fs_mgmt_config.h>
#include <fs_mgmt/fs_mgmt_impl.h>
#include "fs_mgmt/hash_checksum_mgmt.h"
#include "fs_mgmt/hash_checksum_crc32.h"

#define CRC32_SIZE 4

static int fs_hash_checksum_mgmt_crc32(struct fs_file_t *file, uint8_t *output,
				       size_t *out_len, size_t len)
{
	/* Calculate IEEE CRC32 checksum of target file */
	uint8_t buffer[CONFIG_FS_MGMT_CHECKSUM_HASH_CHUNK_SIZE];
	ssize_t bytes_read = 0;
	size_t read_size = CONFIG_FS_MGMT_CHECKSUM_HASH_CHUNK_SIZE;
	uint32_t crc32 = 0;

	/* Clear length prior to calculation */
	*out_len = 0;

	do {
		if ((read_size + *out_len) >= len) {
			/* Limit read size to size of requested data */
			read_size = len - *out_len;
		}

		bytes_read = fs_read(file, buffer, read_size);

		if (bytes_read < 0) {
			/* Failed to read file data, pass generic unknown error back */
			return MGMT_ERR_EUNKNOWN;
		} else if (bytes_read > 0) {
			crc32 = crc32_ieee_update(crc32, buffer, bytes_read);
			*out_len += bytes_read;
		}
	} while (bytes_read > 0 && *out_len < len);

	memcpy(output, &crc32, sizeof(crc32));

	return 0;
}

struct hash_checksum_mgmt_group crc32 = {
	.group_name = "crc32",
	.byte_string = false,
	.output_size = CRC32_SIZE,
	.function = fs_hash_checksum_mgmt_crc32,
};

void fs_hash_checksum_mgmt_register_crc32(void)
{
	hash_checksum_mgmt_register_group(&crc32);
}

void fs_hash_checksum_mgmt_unregister_crc32(void)
{
	hash_checksum_mgmt_unregister_group(&crc32);
}
