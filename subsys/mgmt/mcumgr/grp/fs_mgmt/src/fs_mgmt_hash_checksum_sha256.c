/*
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_hash_checksum.h>
#include <string.h>

#include <mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_config.h>
#include <mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_hash_checksum_sha256.h>

#ifdef CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT
#include <psa/crypto.h>
typedef psa_hash_operation_t hash_ctx_t;
#define SUCCESS_VALUE PSA_SUCCESS

#else
#include <mbedtls/sha256.h>
typedef mbedtls_sha256_context hash_ctx_t;
#define SUCCESS_VALUE 0

#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT */

#define SHA256_DIGEST_SIZE 32

/* The API that the different hash implementations provide further down. */
static int hash_setup(hash_ctx_t *);
static int hash_update(hash_ctx_t *, const uint8_t *input, size_t ilen);
static int hash_finish(hash_ctx_t *, uint8_t *output);
static void hash_teardown(hash_ctx_t *);

static int fs_mgmt_hash_checksum_sha256(struct fs_file_t *file, uint8_t *output,
					size_t *out_len, size_t len)
{
	int rc = MGMT_ERR_EUNKNOWN;
	k_ssize_t bytes_read = 0;
	size_t read_size = CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_CHUNK_SIZE;
	uint8_t buffer[CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_CHUNK_SIZE];
	hash_ctx_t hash_ctx;

	/* Clear variables prior to calculation */
	*out_len = 0;
	memset(output, 0, SHA256_DIGEST_SIZE);

	if (hash_setup(&hash_ctx) != SUCCESS_VALUE) {
		goto teardown;
	}

	/* Read all data from file and add to SHA256 hash calculation */
	do {
		if ((read_size + *out_len) >= len) {
			/* Limit read size to size of requested data */
			read_size = len - *out_len;
		}

		bytes_read = fs_read(file, buffer, read_size);

		if (bytes_read < 0) {
			/* Failed to read file data */
			goto teardown;
		} else if (bytes_read > 0) {
			if (hash_update(&hash_ctx, buffer, bytes_read) != SUCCESS_VALUE) {
				goto teardown;
			}

			*out_len += bytes_read;
		}
	} while (bytes_read > 0 && *out_len < len);

	/* Finalise SHA256 hash calculation and store output in provided output buffer */
	if (hash_finish(&hash_ctx, output) == SUCCESS_VALUE) {
		rc = 0;
	}

teardown:
	hash_teardown(&hash_ctx);

	return rc;
}

static struct fs_mgmt_hash_checksum_group sha256 = {
	.group_name = "sha256",
	.byte_string = true,
	.output_size = SHA256_DIGEST_SIZE,
	.function = fs_mgmt_hash_checksum_sha256,
};

void fs_mgmt_hash_checksum_register_sha256(void)
{
	fs_mgmt_hash_checksum_register_group(&sha256);
}

void fs_mgmt_hash_checksum_unregister_sha256(void)
{
	fs_mgmt_hash_checksum_unregister_group(&sha256);
}

#ifdef CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT

static int hash_setup(psa_hash_operation_t *ctx)
{
	*ctx = psa_hash_operation_init();
	return psa_hash_setup(ctx, PSA_ALG_SHA_256);
}
static int hash_update(psa_hash_operation_t *ctx, const uint8_t *input, size_t ilen)
{
	return psa_hash_update(ctx, input, ilen);
}
static int hash_finish(psa_hash_operation_t *ctx, uint8_t *output)
{
	size_t output_length;

	return psa_hash_finish(ctx, output, SHA256_DIGEST_SIZE, &output_length);
}
static void hash_teardown(psa_hash_operation_t *ctx)
{
	psa_hash_abort(ctx);
}

#else

static int hash_setup(mbedtls_sha256_context *ctx)
{
	mbedtls_sha256_init(ctx);
	return mbedtls_sha256_starts(ctx, false);
}
static int hash_update(mbedtls_sha256_context *ctx, const uint8_t *input, size_t ilen)
{
	return mbedtls_sha256_update(ctx, input, ilen);
}
static int hash_finish(mbedtls_sha256_context *ctx, uint8_t *output)
{
	return mbedtls_sha256_finish(ctx, output);
}
static void hash_teardown(mbedtls_sha256_context *ctx)
{
	mbedtls_sha256_free(ctx);
}

#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT */
