/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_rsip_e51a_trng

#include <soc.h>
#include <zephyr/drivers/entropy.h>

#include "hw_sce_trng_private.h"
#include "hw_sce_private.h"

static int entropy_ra_rsip_trng_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	ARG_UNUSED(dev);

	if (buf == NULL) {
		return -EINVAL;
	}

	while (len > 0) {
		uint32_t n[4];
		const uint32_t to_copy = MIN(sizeof(n), len);

		if (FSP_SUCCESS != HW_SCE_RNG_Read(n)) {
			return -ENODATA;
		}
		memcpy(buf, n, to_copy);
		buf += to_copy;
		len -= to_copy;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_ra_rsip_trng_api) = {
	.get_entropy = entropy_ra_rsip_trng_get_entropy,
};

static int entropy_ra_rsip_trng_init(const struct device *dev)
{
	HW_SCE_McuSpecificInit();
	return 0;
}

DEVICE_DT_INST_DEFINE(0, entropy_ra_rsip_trng_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_ra_rsip_trng_api);
