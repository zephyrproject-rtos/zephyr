/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <string.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */
#include <mbedtls/ctr_drbg.h>

/*
 * entropy_dev is initialized at runtime to allow first time initialization
 * of the ctr_drbg engine.
 */
static const struct device *entropy_dev;
static const unsigned char drbg_seed[] = CONFIG_CS_CTR_DRBG_PERSONALIZATION;
static bool ctr_initialised;
static struct k_mutex ctr_lock;

static mbedtls_ctr_drbg_context ctr_ctx;

static int ctr_drbg_entropy_func(void *ctx, unsigned char *buf, size_t len)
{
	return entropy_get_entropy(entropy_dev, (void *)buf, len);
}

static int ctr_drbg_initialize(void)
{
	int ret;

	entropy_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

	if (!device_is_ready(entropy_dev)) {
		__ASSERT(0, "Entropy device %s not ready", entropy_dev->name);
		return -ENODEV;
	}

	mbedtls_ctr_drbg_init(&ctr_ctx);

	ret = mbedtls_ctr_drbg_seed(&ctr_ctx,
				    ctr_drbg_entropy_func,
				    NULL,
				    drbg_seed,
				    sizeof(drbg_seed));

	if (ret != 0) {
		mbedtls_ctr_drbg_free(&ctr_ctx);
		return -EIO;
	}

	ctr_initialised = true;
	return 0;
}


int z_impl_sys_csrand_get(void *dst, uint32_t outlen)
{
	int ret;

	k_mutex_lock(&ctr_lock, K_FOREVER);

	if (unlikely(!ctr_initialised)) {
		ret = ctr_drbg_initialize();
		if (ret != 0) {
			ret = -EIO;
			goto end;
		}
	}

	ret = mbedtls_ctr_drbg_random(&ctr_ctx, (unsigned char *)dst, outlen);

end:
	k_mutex_unlock(&ctr_lock);

	return ret;
}
