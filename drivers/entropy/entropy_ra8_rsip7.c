/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra8_rsip7_trng

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <soc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/irq.h>

#include <hw_sce_trng_private.h>
#include <hw_sce_private.h>

static int entropy_ra_rsip_trng_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	if (buf == NULL) {
		return -EINVAL;
	}
	uint8_t *buf_bytes = (uint8_t *)buf;

	while (len > 0) {
		uint32_t n[4];
		const uint32_t to_copy = MIN(sizeof(n), len);

		HW_SCE_RNG_Read(n);
		memcpy(buf_bytes, n, to_copy);
		buf_bytes += to_copy;
		len -= to_copy;
	}

	return 0;
}

static const struct entropy_driver_api entropy_ra_rsip_trng_api = {
	.get_entropy = entropy_ra_rsip_trng_get_entropy,
};

static int entropy_ra_rsip_trng_init(const struct device *dev)
{
	HW_SCE_McuSpecificInit();
	return 0;
}

DEVICE_DT_INST_DEFINE(0, entropy_ra_rsip_trng_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_ra_rsip_trng_api);
