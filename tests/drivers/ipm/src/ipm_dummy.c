/* ipm_dummy.c - Fake IPM driver for testing upper-level drivers */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/ipm.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/irq_offload.h>

#include "ipm_dummy.h"



/* Implemented as a software interrupt so that callbacks are executed
 * in the expected context
 */
static void ipm_dummy_isr(const void *data)
{
	const struct device *d = (const struct device *)data;
	struct ipm_dummy_driver_data *driver_data = d->data;

	/* In a real driver the interrupt simply wouldn't fire, we fake
	 * that here
	 */
	if (!driver_data->regs.enabled || !driver_data->regs.busy) {
		return;
	}

	if (driver_data->cb) {
		driver_data->cb(d,
				driver_data->cb_context, driver_data->regs.id,
				(volatile void *)&driver_data->regs.data);
	}
	driver_data->regs.busy = 0U;
}


/* IPM API functions for the dummy driver */

static int ipm_dummy_send(const struct device *d, int wait, uint32_t id,
			  const void *data, int size)
{
	struct ipm_dummy_driver_data *driver_data;
	volatile uint8_t *datareg;
	const uint8_t *data8;
	int i;

	driver_data = d->data;
	if (size > ipm_max_data_size_get(d)) {
		return -EMSGSIZE;
	}

	if (driver_data->regs.busy) {
		return -EBUSY;
	}

	data8 = (const uint8_t *)data;
	datareg = (volatile uint8_t *)driver_data->regs.data;

	for (i = 0; i < size; ++i) {
		datareg[i] = data8[i];
	}
	driver_data->regs.id = id;
	driver_data->regs.busy = 1U;

	irq_offload(ipm_dummy_isr, (const void *)d);

	if (wait) {
		while (driver_data->regs.busy) {
			/* busy-wait */
		}
	}
	return 0;
}

static void ipm_dummy_register_callback(const struct device *d,
					ipm_callback_t cb,
					void *cb_context)
{
	struct ipm_dummy_driver_data *driver_data;

	driver_data = d->data;
	driver_data->cb = cb;
	driver_data->cb_context = cb_context;
}

static int ipm_dummy_set_enabled(const struct device *d, int enable)
{
	struct ipm_dummy_driver_data *driver_data = d->data;

	driver_data->regs.enabled = enable;
	if (enable) {
		/* In case there are pending messages */
		irq_offload(ipm_dummy_isr, (const void *)d);
	}
	return 0;
}

static uint32_t ipm_dummy_max_id_val_get(const struct device *d)
{
	return 0xFFFFFFFF;
}

static int ipm_dummy_max_data_size_get(const struct device *d)
{
	return DUMMY_IPM_DATA_WORDS * sizeof(uint32_t);
}

struct ipm_driver_api ipm_dummy_api = {
	.send = ipm_dummy_send,
	.register_callback = ipm_dummy_register_callback,
	.max_data_size_get = ipm_dummy_max_data_size_get,
	.max_id_val_get = ipm_dummy_max_id_val_get,
	.set_enabled = ipm_dummy_set_enabled
};

/* Dummy IPM driver initialization, will be bound at runtime
 * to high-level drivers under test
 */

int ipm_dummy_init(const struct device *d)
{
	return 0;
}
