/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_mxcryptolite_trng

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/mfd/infineon_mxcryptolite.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(entropy_infineon_mxcryptolite_trng, CONFIG_ENTROPY_LOG_LEVEL);

#include "cy_cryptolite_trng.h"

struct entropy_mxcryptolite_trng_config {
	const struct device *mfd;
};

static int entropy_mxcryptolite_trng_get_entropy(const struct device *dev,
					       uint8_t *buffer, uint16_t length)
{
	const struct entropy_mxcryptolite_trng_config *cfg = dev->config;
	CRYPTOLITE_Type *base = mfd_infineon_mxcryptolite_get_base(cfg->mfd);
	cy_en_cryptolite_status_t status;
	uint32_t rand_val;
	uint16_t remaining = length;
	int ret = 0;

	/*
	 * The TRNG shares the MXCRYPTOLITE hardware block with the sibling
	 * crypto (AES / SHA-256) function, so hold the parent MFD lock for the
	 * duration of the read to serialise against that function.
	 */
	ret = mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	while (remaining > 0) {
		status = Cy_Cryptolite_Trng_ReadData(base, &rand_val);
		if (status != CY_CRYPTOLITE_SUCCESS) {
			ret = -EIO;
			break;
		}

		uint16_t chunk = MIN(remaining, sizeof(rand_val));

		memcpy(buffer, &rand_val, chunk);
		buffer += chunk;
		remaining -= chunk;
	}

	mfd_infineon_mxcryptolite_unlock(cfg->mfd);

	return ret;
}

static int entropy_mxcryptolite_trng_init(const struct device *dev)
{
	const struct entropy_mxcryptolite_trng_config *cfg = dev->config;
	CRYPTOLITE_Type *base;
	cy_en_cryptolite_status_t status;

	if (!device_is_ready(cfg->mfd)) {
		LOG_ERR("MXCRYPTOLITE MFD device not ready");
		return -ENODEV;
	}

	base = mfd_infineon_mxcryptolite_get_base(cfg->mfd);

	status = Cy_Cryptolite_Trng_Init(base,
					 (cy_stc_cryptolite_trng_config_t *)
					 &cy_cryptolite_trngDefaultConfig);
	if (status != CY_CRYPTOLITE_SUCCESS) {
		LOG_ERR("TRNG init failed: %d", (int)status);
		return -EIO;
	}

	status = Cy_Cryptolite_Trng_Enable(base);
	if (status != CY_CRYPTOLITE_SUCCESS) {
		LOG_ERR("TRNG enable failed: %d", (int)status);
		return -EIO;
	}

	Cy_Cryptolite_Trng_WaitForInit(base);

	return 0;
}

static DEVICE_API(entropy, entropy_mxcryptolite_trng_api) = {
	.get_entropy = entropy_mxcryptolite_trng_get_entropy,
};

static const struct entropy_mxcryptolite_trng_config entropy_mxcryptolite_trng_cfg = {
	.mfd = DEVICE_DT_GET(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, entropy_mxcryptolite_trng_init, NULL,
		      NULL, &entropy_mxcryptolite_trng_cfg,
		      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		      &entropy_mxcryptolite_trng_api);
