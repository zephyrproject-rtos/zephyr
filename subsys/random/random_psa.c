/*
 * Copyright (c) 2019, NXP
 * Copyright (c) 2025, HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>

#include <psa/crypto.h>

static bool _initialised;
static K_MUTEX_DEFINE(_lock);

int z_impl_sys_csrand_get(void *dst, size_t outlen)
{
	psa_status_t ret = PSA_SUCCESS;

	if (unlikely(!_initialised)) {
		k_mutex_lock(&_lock, K_FOREVER);
		ret = psa_crypto_init();
		k_mutex_unlock(&_lock);

		if (ret != PSA_SUCCESS) {
			goto end;
		}

		_initialised = true;
	}

	ret = psa_generate_random((uint8_t *)dst, outlen);

end:
	return ret == PSA_SUCCESS ? 0 : -EIO;
}
