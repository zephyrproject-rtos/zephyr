/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>
#include "flash_map_priv.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>

#define SHA256_DIGEST_SIZE 32
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_PSA)
#include <psa/crypto.h>
#define SUCCESS_VALUE PSA_SUCCESS
#else
#include <mbedtls/sha256.h>
#define SUCCESS_VALUE 0
#endif

int flash_area_check_int_sha256(const struct flash_area *fa,
				const struct flash_area_check *fac)
{
	unsigned char hash[SHA256_DIGEST_SIZE];
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_PSA)
	psa_hash_operation_t hash_ctx;
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
	mbedtls_sha256_context hash_ctx;
#endif
	int to_read;
	int pos;
	int rc;

	if (fa == NULL || fac == NULL || fac->match == NULL ||
	    fac->rbuf == NULL || fac->clen == 0 || fac->rblen == 0) {
		return -EINVAL;
	}

	if (!is_in_flash_area_bounds(fa, fac->off, fac->clen)) {
		return -EINVAL;
	}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_PSA)
	hash_ctx = psa_hash_operation_init();
	rc = psa_hash_setup(&hash_ctx, PSA_ALG_SHA_256);
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
	mbedtls_sha256_init(&hash_ctx);
	rc = mbedtls_sha256_starts(&hash_ctx, false);
#endif
	if (rc != SUCCESS_VALUE) {
		return -ESRCH;
	}

	to_read = fac->rblen;

	for (pos = 0; pos < fac->clen; pos += to_read) {
		if (pos + to_read > fac->clen) {
			to_read = fac->clen - pos;
		}

		rc = flash_read(fa->fa_dev, (fa->fa_off + fac->off + pos),
				fac->rbuf, to_read);
		if (rc != 0) {
			goto error;
		}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_PSA)
		rc = psa_hash_update(&hash_ctx, fac->rbuf, to_read);
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
		rc = mbedtls_sha256_update(&hash_ctx, fac->rbuf, to_read);
#endif
		if (rc != SUCCESS_VALUE) {
			rc = -ESRCH;
			goto error;
		}
	}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_PSA)
	size_t hash_len;

	rc = psa_hash_finish(&hash_ctx, hash, sizeof(hash), &hash_len);
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
	rc = mbedtls_sha256_finish(&hash_ctx, hash);
#endif
	if (rc != SUCCESS_VALUE) {
		rc = -ESRCH;
		goto error;
	}

	if (memcmp(hash, fac->match, SHA256_DIGEST_SIZE)) {
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_PSA)
		/* The operation has already been terminated. */
		return -EILSEQ;
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
		rc = -EILSEQ;
		goto error;
#endif
	}

error:
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_PSA)
	psa_hash_abort(&hash_ctx);
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
	mbedtls_sha256_free(&hash_ctx);
#endif
	return rc;
}
