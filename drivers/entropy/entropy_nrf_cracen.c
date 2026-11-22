/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx_cracen.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/misc/nordic_cracen/nordic_cracen.h>

#define DT_DRV_COMPAT nordic_nrf_cracen_ctrdrbg

#define CRACEN_DEV DEVICE_DT_GET(DT_INST_PARENT(0))

/* The parent device initialises the CRACEN library and must be ready first */
BUILD_ASSERT(CONFIG_NORDIC_CRACEN_INIT_PRIORITY < CONFIG_ENTROPY_INIT_PRIORITY,
	     "NORDIC_CRACEN_INIT_PRIORITY must be lower than ENTROPY_INIT_PRIORITY");

static int nrf_cracen_random_get(uint8_t *buf, uint16_t len, k_timeout_t timeout)
{
	int ret;

	ret = nordic_cracen_acquire(CRACEN_DEV, timeout);
	if (ret < 0) {
		return ret;
	}

	/* This will take approximately 2 + (ceil(len/16) + 3)*3 us. i.e. 14us for 16 bytes */
	ret = nrfx_cracen_ctr_drbg_random_get(buf, len);

	nordic_cracen_release(CRACEN_DEV);
	return ret;
}

static int nrf_cracen_get_entropy_isr(const struct device *dev, uint8_t *buf, uint16_t len,
				      uint32_t flags)
{
	ARG_UNUSED(dev);

	/*
	 * With NORDIC_CRACEN_SYNC_IRQ_LOCK interrupts are locked for the whole operation and
	 * this always succeeds, as it does with NORDIC_CRACEN_SYNC_NONE. With
	 * NORDIC_CRACEN_SYNC_SEMAPHORE the call only waits for the CRACEN when ENTROPY_BUSYWAIT is
	 * requested, otherwise failing with -EBUSY if it is in use. Waiting is always downgraded to
	 * K_NO_WAIT in interrupt context.
	 */
	k_timeout_t timeout = (flags & ENTROPY_BUSYWAIT) ? K_FOREVER : K_NO_WAIT;
	int ret = nrf_cracen_random_get(buf, len, timeout);

	if (likely(ret == 0)) {
		return len;
	} else {
		return ret;
	}
}

static int nrf_cracen_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	ARG_UNUSED(dev);

	return nrf_cracen_random_get(buf, len, K_FOREVER);
}

static DEVICE_API(entropy, nrf_cracen_api_funcs) = {
	.get_entropy = nrf_cracen_get_entropy,
	.get_entropy_isr = nrf_cracen_get_entropy_isr,
};

static int nrf_cracen_cracen_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * The CRACEN library is initialised (and the CTR_DRBG seeded) by the parent device.
	 * Init ordering does not imply success, so propagate a parent failure to ensure the
	 * entropy device is not reported as ready with an unseeded DRBG.
	 */
	return device_is_ready(CRACEN_DEV) ? 0 : -ENODEV;
}

DEVICE_DT_INST_DEFINE(0, nrf_cracen_cracen_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &nrf_cracen_api_funcs);
