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

#if defined(CONFIG_PSA_WANT_ALG_SHA_256) && defined(CONFIG_BASE64)

#include <psa/crypto.h>
#include <zephyr/sys/base64.h>

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
