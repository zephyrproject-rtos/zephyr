/*
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <mgmt/mgmt.h>
#include <fs_mgmt/fs_mgmt_config.h>
#include <fs_mgmt/fs_mgmt_impl.h>
#include "fs_mgmt/hash_checksum_mgmt.h"
#include "fs_mgmt/hash_checksum_sha256.h"

#if defined(CONFIG_TINYCRYPT_SHA256)
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#else
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#endif

#define SHA256_DIGEST_SIZE 32

#if defined(CONFIG_TINYCRYPT_SHA256)
/* Tinycrypt SHA256 implementation */
static int fs_hash_checksum_mgmt_sha256(struct fs_file_t *file, uint8_t *output,
					size_t *out_len, size_t len)
{
	int rc = 0;
	ssize_t bytes_read = 0;
	size_t read_size = CONFIG_FS_MGMT_CHECKSUM_HASH_CHUNK_SIZE;
	uint8_t buffer[CONFIG_FS_MGMT_CHECKSUM_HASH_CHUNK_SIZE];
	struct tc_sha256_state_struct sha;

	/* Clear variables prior to calculation */
	*out_len = 0;
	memset(output, 0, SHA256_DIGEST_SIZE);

	if (tc_sha256_init(&sha) != TC_CRYPTO_SUCCESS) {
		return MGMT_ERR_EUNKNOWN;
	}

	/* Read all data from file and add to SHA256 hash calculation */
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
			if (tc_sha256_update(&sha, buffer, bytes_read) != TC_CRYPTO_SUCCESS) {
				return MGMT_ERR_EUNKNOWN;
			}

			*out_len += bytes_read;
		}
	} while (bytes_read > 0 && *out_len < len);

	/* Finalise SHA256 hash calculation and store output in provided output buffer */
	if (tc_sha256_final(output, &sha) != TC_CRYPTO_SUCCESS) {
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#else
/* mbedtls SHA256 implementation */
static int fs_hash_checksum_mgmt_sha256(struct fs_file_t *file, uint8_t *output,
					size_t *out_len, size_t len)
{
	int rc = 0;
	ssize_t bytes_read = 0;
	size_t read_size = CONFIG_FS_MGMT_CHECKSUM_HASH_CHUNK_SIZE;
	uint8_t buffer[CONFIG_FS_MGMT_CHECKSUM_HASH_CHUNK_SIZE];
	mbedtls_md_context_t mbed_hash_ctx;
	const mbedtls_md_info_t *mbed_hash_info;

	/* Clear variables prior to calculation */
	*out_len = 0;
	memset(output, 0, SHA256_DIGEST_SIZE);

	mbed_hash_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	mbedtls_md_init(&mbed_hash_ctx);

	if (mbedtls_md_setup(&mbed_hash_ctx, mbed_hash_info, 0) != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	if (mbedtls_md_starts(&mbed_hash_ctx)) {
		rc = MGMT_ERR_EUNKNOWN;
		goto error;
	}

	/* Read all data from file and add to SHA256 hash calculation */
	do {
		if ((read_size + *out_len) >= len) {
			/* Limit read size to size of requested data */
			read_size = len - *out_len;
		}

		bytes_read = fs_read(file, buffer, read_size);

		if (bytes_read < 0) {
			/* Failed to read file data, pass generic unknown error back */
			rc = MGMT_ERR_EUNKNOWN;
			goto error;
		} else if (bytes_read > 0) {
			if (mbedtls_md_update(&mbed_hash_ctx, buffer, bytes_read) != 0) {
				rc = MGMT_ERR_EUNKNOWN;
				goto error;
			}

			*out_len += bytes_read;
		}
	} while (bytes_read > 0 && *out_len < len);

	/* Finalise SHA256 hash calculation and store output in provided output buffer */
	if (mbedtls_md_finish(&mbed_hash_ctx, output) != 0) {
		rc = MGMT_ERR_EUNKNOWN;
	}

error:
	mbedtls_md_free(&mbed_hash_ctx);

	return rc;
}
#endif

struct hash_checksum_mgmt_group sha256 = {
	.group_name = "sha256",
	.byte_string = true,
	.output_size = SHA256_DIGEST_SIZE,
	.function = fs_hash_checksum_mgmt_sha256,
};

void fs_hash_checksum_mgmt_register_sha256(void)
{
	hash_checksum_mgmt_register_group(&sha256);
}

void fs_hash_checksum_mgmt_unregister_sha256(void)
{
	hash_checksum_mgmt_unregister_group(&sha256);
}
