/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx_cracen.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/entropy.h>

#define DT_DRV_COMPAT nordic_nrf_cracen_ctrdrbg

static int nrf_cracen_get_entropy_isr(const struct device *dev, uint8_t *buf, uint16_t len,
				      uint32_t flags)
{
	(void)dev;
	(void)flags;

	unsigned int key = irq_lock();

	/* This will take approximately 2 + (ceil(len/16) + 3)*3 us. i.e. 14us for 16 bytes */
	int ret = nrfx_cracen_ctr_drbg_random_get(buf, len);

	irq_unlock(key);

	if (likely(ret == NRFX_SUCCESS)) {
		return len;
	} else if (ret == NRFX_ERROR_INVALID_PARAM) {
		return -EINVAL;
	} else {
		return -EAGAIN;
	}
}

static int nrf_cracen_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	int ret = nrf_cracen_get_entropy_isr(dev, buf, len, 0);

	if (ret < 0) {
		return ret;
	} else {
		return 0;
	}
}

static int nrf_cracen_cracen_init(const struct device *dev)
{
	(void)dev;

	int ret = nrfx_cracen_ctr_drbg_init();

	if (ret == NRFX_SUCCESS) {
		return 0;
	} else {
		return -EIO;
	}
}

static DEVICE_API(entropy, nrf_cracen_api_funcs) = {
	.get_entropy = nrf_cracen_get_entropy,
	.get_entropy_isr = nrf_cracen_get_entropy_isr
};

DEVICE_DT_INST_DEFINE(0, nrf_cracen_cracen_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &nrf_cracen_api_funcs);
