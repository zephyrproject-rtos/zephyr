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
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>
#include "flash_map_priv.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY)
#define SHA256_DIGEST_SIZE 32
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#else
#include <mbedtls/md.h>
#endif
#include <string.h>
#endif /* CONFIG_FLASH_AREA_CHECK_INTEGRITY */

int flash_area_check_int_sha256(const struct flash_area *fa,
				const struct flash_area_check *fac)
{
	unsigned char hash[SHA256_DIGEST_SIZE];
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
	struct tc_sha256_state_struct sha;
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
	mbedtls_md_context_t mbed_hash_ctx;
	const mbedtls_md_info_t *mbed_hash_info;
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

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
	if (tc_sha256_init(&sha) != TC_CRYPTO_SUCCESS) {
		return -ESRCH;
	}
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
	mbed_hash_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

	mbedtls_md_init(&mbed_hash_ctx);

	if (mbedtls_md_setup(&mbed_hash_ctx, mbed_hash_info, 0) != 0) {
		return -ESRCH;
	}

	if (mbedtls_md_starts(&mbed_hash_ctx)) {
		rc = -ESRCH;
		goto error;
	}
#endif

	to_read = fac->rblen;

	for (pos = 0; pos < fac->clen; pos += to_read) {
		if (pos + to_read > fac->clen) {
			to_read = fac->clen - pos;
		}

		rc = flash_read(fa->fa_dev, (fa->fa_off + fac->off + pos),
				fac->rbuf, to_read);
		if (rc != 0) {
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
			return rc;
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
			goto error;
#endif
		}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
		if (tc_sha256_update(&sha,
				     fac->rbuf,
				     to_read) != TC_CRYPTO_SUCCESS) {
			return -ESRCH;
		}
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
		if (mbedtls_md_update(&mbed_hash_ctx, fac->rbuf, to_read) != 0) {
			rc = -ESRCH;
			goto error;
		}
#endif
	}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
	if (tc_sha256_final(hash, &sha) != TC_CRYPTO_SUCCESS) {
		return -ESRCH;
	}
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
	if (mbedtls_md_finish(&mbed_hash_ctx, hash) != 0) {
		rc = -ESRCH;
		goto error;
	}
#endif

	if (memcmp(hash, fac->match, SHA256_DIGEST_SIZE)) {
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
		return -EILSEQ;
#else /* CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS */
		rc = -EILSEQ;
		goto error;
#endif
	}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS)
error:
	mbedtls_md_free(&mbed_hash_ctx);
#endif
	return rc;
}
