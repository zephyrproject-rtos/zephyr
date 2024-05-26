/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/* This file provides an (internal-use-only) credential digest function that backends storing
 * raw credentials can use.
 */

#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include "tls_internal.h"
#include "tls_credentials_digest_raw.h"

/* Grab mbedTLS headers if they are available so that we can check whether SHA256 is supported */

#if defined(CONFIG_MBEDTLS)
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */
#endif /* CONFIG_MBEDTLS */

#if defined(CONFIG_TINYCRYPT_SHA256) && defined(CONFIG_BASE64)

#include <tinycrypt/sha256.h>
#include <zephyr/sys/base64.h>

int credential_digest_raw(struct tls_credential *credential, void *dest, size_t *len)
{
	int err = 0;
	size_t written = 0;
	struct tc_sha256_state_struct sha_state;
	uint8_t digest_buf[TC_SHA256_DIGEST_SIZE];

	/* Compute digest. */
	(void)tc_sha256_init(&sha_state);
	(void)tc_sha256_update(&sha_state, credential->buf, credential->len);
	(void)tc_sha256_final(digest_buf, &sha_state);

	/* Attempt to encode digest to destination.
	 * Will return -ENOMEM if there is not enough space in the destination buffer.
	 */
	err = base64_encode(dest, *len, &written, digest_buf, sizeof(digest_buf));
	*len = err ? 0 : written;

	/* Clean up. */
	memset(&sha_state, 0, sizeof(sha_state));
	memset(digest_buf, 0, sizeof(digest_buf));
	return err;
}

#elif defined(CONFIG_PSA_WANT_ALG_SHA_256) && defined(CONFIG_BASE64)

#include <psa/crypto.h>

int credential_digest_raw(struct tls_credential *credential, void *dest, size_t *len)
{
	int err = 0;
	size_t written = 0;
	uint8_t digest_buf[32];
	size_t digest_len;
	psa_status_t status;

	/* Compute digest. */
	status = psa_hash_compute(PSA_ALG_SHA_256, credential->buf, credential->len,
				  digest_buf, sizeof(digest_buf), &digest_len);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	/* Attempt to encode digest to destination.
	 * Will return -ENOMEM if there is not enough space in the destination buffer.
	 */
	err = base64_encode(dest, *len, &written, digest_buf, sizeof(digest_buf));
	*len = err ? 0 : written;

	/* Clean up. */
	memset(digest_buf, 0, sizeof(digest_buf));

	return err;
}

#else

int credential_digest_raw(struct tls_credential *credential, void *dest, size_t *len)
{
	*len = 0;
	return -ENOTSUP;
}

#endif
