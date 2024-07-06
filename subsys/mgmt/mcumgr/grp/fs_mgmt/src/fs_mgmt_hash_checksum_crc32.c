/*
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/fs/fs.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_hash_checksum.h>
#include <string.h>

#include <mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_config.h>
#include <mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_hash_checksum_crc32.h>

#define CRC32_SIZE 4

static int fs_mgmt_hash_checksum_crc32(struct fs_file_t *file, uint8_t *output,
				       size_t *out_len, size_t len)
{
	/* Calculate IEEE CRC32 checksum of target file */
	uint8_t buffer[CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_CHUNK_SIZE];
	k_ssize_t bytes_read = 0;
	size_t read_size = CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_CHUNK_SIZE;
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

static struct fs_mgmt_hash_checksum_group crc32 = {
	.group_name = "crc32",
	.byte_string = false,
	.output_size = CRC32_SIZE,
	.function = fs_mgmt_hash_checksum_crc32,
};

void fs_mgmt_hash_checksum_register_crc32(void)
{
	fs_mgmt_hash_checksum_register_group(&crc32);
}

void fs_mgmt_hash_checksum_unregister_crc32(void)
{
	fs_mgmt_hash_checksum_unregister_group(&crc32);
}
