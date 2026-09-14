/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_mxcrypto_trng

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/mfd/infineon_mxcrypto.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(entropy_infineon_mxcrypto_trng, CONFIG_ENTROPY_LOG_LEVEL);

#include "cy_crypto_core_trng.h"

struct entropy_mxcrypto_trng_config {
	const struct device *mfd;
};

static int entropy_mxcrypto_trng_get_entropy(const struct device *dev,
					       uint8_t *buffer, uint16_t length)
{
	const struct entropy_mxcrypto_trng_config *cfg = dev->config;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	cy_en_crypto_status_t status;
	uint32_t rand_val;
	uint16_t remaining = length;
	int ret = 0;

	/*
	 * The TRNG shares the MXCRYPTO hardware block with the sibling
	 * crypto (AES / SHA-256) function, so hold the parent MFD lock for the
	 * duration of the read to serialise against that function.
	 */
	ret = mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	while (remaining > 0) {
		status = Cy_Crypto_Core_Trng_Ext(base, 32U, &rand_val);
		if (status != CY_CRYPTO_SUCCESS) {
			ret = -EIO;
			break;
		}

		uint16_t chunk = MIN(remaining, sizeof(rand_val));

		memcpy(buffer, &rand_val, chunk);
		buffer += chunk;
		remaining -= chunk;
	}

	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	return ret;
}

static int entropy_mxcrypto_trng_init(const struct device *dev)
{
	const struct entropy_mxcrypto_trng_config *cfg = dev->config;

	if (!device_is_ready(cfg->mfd)) {
		LOG_ERR("MXCRYPTO MFD device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_mxcrypto_trng_api) = {
	.get_entropy = entropy_mxcrypto_trng_get_entropy,
};

static const struct entropy_mxcrypto_trng_config entropy_mxcrypto_trng_cfg = {
	.mfd = DEVICE_DT_GET(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, entropy_mxcrypto_trng_init, NULL,
		      NULL, &entropy_mxcrypto_trng_cfg,
		      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		      &entropy_mxcrypto_trng_api);
