/*
 * Copyright (c) 2026 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_cracen

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/misc/nordic_cracen/nordic_cracen.h>

#include <nrfx_cracen.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only a single CRACEN instance is supported");

#ifndef CONFIG_SOC_SERIES_BSIM_NRFXX
/* Register blocks are relocated to simulated memory on BabbleSim */
BUILD_ASSERT(DT_INST_REG_ADDR_BY_NAME(0, wrapper) == NRF_CRACEN_S_BASE,
	     "CRACEN wrapper address does not match the MDK");
BUILD_ASSERT(DT_INST_REG_ADDR_BY_NAME(0, core) == NRF_CRACENCORE_S_BASE,
	     "CRACENCORE address does not match the MDK");
#endif /* CONFIG_SOC_SERIES_BSIM_NRFXX */

struct nordic_cracen_data {
#if defined(CONFIG_NORDIC_CRACEN_SYNC_SEMAPHORE)
	struct k_sem lock;
#elif defined(CONFIG_NORDIC_CRACEN_SYNC_IRQ_LOCK)
	unsigned int irq_key;
#else
	uint8_t unused;
#endif
};

int nordic_cracen_acquire(const struct device *dev, k_timeout_t timeout)
{
	struct nordic_cracen_data *data = dev->data;

#if defined(CONFIG_NORDIC_CRACEN_SYNC_SEMAPHORE)
	int rc;

	if (k_is_in_isr() || k_is_pre_kernel()) {
		timeout = K_NO_WAIT;
	}
	rc = k_sem_take(&data->lock, timeout);
	if (rc != 0) {
		return rc;
	}
#elif defined(CONFIG_NORDIC_CRACEN_SYNC_IRQ_LOCK)
	ARG_UNUSED(timeout);
	data->irq_key = irq_lock();
#else
	ARG_UNUSED(data);
	ARG_UNUSED(timeout);
#endif

	return 0;
}

void nordic_cracen_release(const struct device *dev)
{
	struct nordic_cracen_data *data = dev->data;

#if defined(CONFIG_NORDIC_CRACEN_SYNC_SEMAPHORE)
	k_sem_give(&data->lock);
#elif defined(CONFIG_NORDIC_CRACEN_SYNC_IRQ_LOCK)
	irq_unlock(data->irq_key);
#else
	ARG_UNUSED(data);
#endif
}

static int nordic_cracen_init(const struct device *dev)
{
	struct nordic_cracen_data *data = dev->data;
	unsigned int key;
	int rc;

#ifdef CONFIG_NORDIC_CRACEN_SYNC_SEMAPHORE
	k_sem_init(&data->lock, 1, 1);
#else
	ARG_UNUSED(data);
#endif /* CONFIG_NORDIC_CRACEN_SYNC_SEMAPHORE */

	/* Library initialisation seeds the CTR_DRBG on the CryptoMaster */
	key = irq_lock();
	rc = nrfx_cracen_init();
	irq_unlock(key);

	return rc;
}

static struct nordic_cracen_data nordic_cracen_data_0;

DEVICE_DT_INST_DEFINE(0, nordic_cracen_init, NULL, &nordic_cracen_data_0, NULL, PRE_KERNEL_1,
		      CONFIG_NORDIC_CRACEN_INIT_PRIORITY, NULL);
