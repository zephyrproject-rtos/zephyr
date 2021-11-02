/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <storage/flash_map.h>
#include "flash_map_priv.h"
#include <drivers/flash.h>
#include <soc.h>
#include <init.h>

#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#include <string.h>

int flash_area_check_int_sha256(const struct flash_area *fa,
				const struct flash_area_check *fac)
{
	unsigned char hash[TC_SHA256_DIGEST_SIZE];
	struct tc_sha256_state_struct sha;
	const struct device *dev;
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

	if (tc_sha256_init(&sha) != TC_CRYPTO_SUCCESS) {
		return -ESRCH;
	}

	dev = device_get_binding(fa->fa_dev_name);
	to_read = fac->rblen;

	for (pos = 0; pos < fac->clen; pos += to_read) {
		if (pos + to_read > fac->clen) {
			to_read = fac->clen - pos;
		}

		rc = flash_read(dev, (fa->fa_off + fac->off + pos),
				fac->rbuf, to_read);
		if (rc != 0) {
			return rc;
		}

		if (tc_sha256_update(&sha,
				     fac->rbuf,
				     to_read) != TC_CRYPTO_SUCCESS) {
			return -ESRCH;
		}
	}

	if (tc_sha256_final(hash, &sha) != TC_CRYPTO_SUCCESS) {
		return -ESRCH;
	}

	if (memcmp(hash, fac->match, TC_SHA256_DIGEST_SIZE)) {
		return -EILSEQ;
	}

	return 0;
}
