/* ipm_dummy.c - Fake IPM driver for testing upper-level drivers */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ipm.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>
#include <irq_offload.h>

#include "ipm_dummy.h"



/* Implemented as a software interrupt so that callbacks are executed
 * in the expected context
 */
static void ipm_dummy_isr(void *data)
{
	struct device *d = (struct device *)data;
	struct ipm_dummy_driver_data *driver_data = d->driver_data;

	/* In a real driver the interrupt simply wouldn't fire, we fake
	 * that here
	 */
	if (!driver_data->regs.enabled || !driver_data->regs.busy) {
		return;
	}

	if (driver_data->cb) {
		driver_data->cb(driver_data->cb_context, driver_data->regs.id,
				(volatile void *)&driver_data->regs.data);
	}
	driver_data->regs.busy = 0;
}


/* IPM API functions for the dummy driver */

static int ipm_dummy_send(struct device *d, int wait, u32_t id,
			  const void *data, int size)
{
	struct ipm_dummy_driver_data *driver_data;
	volatile u8_t *datareg;
	const u8_t *data8;
	int i;

	driver_data = d->driver_data;
	if (size > ipm_max_data_size_get(d)) {
		return -EMSGSIZE;
	}

	if (driver_data->regs.busy) {
		return -EBUSY;
	}

	data8 = (const u8_t *)data;
	datareg = (volatile u8_t *)driver_data->regs.data;

	for (i = 0; i < size; ++i) {
		datareg[i] = data8[i];
	}
	driver_data->regs.id = id;
	driver_data->regs.busy = 1;

	irq_offload(ipm_dummy_isr, d);

	if (wait) {
		while (driver_data->regs.busy) {
			/* busy-wait */
		}
	}
	return 0;
}

static void ipm_dummy_register_callback(struct device *d, ipm_callback_t cb,
					void *cb_context)
{
	struct ipm_dummy_driver_data *driver_data;

	driver_data = d->driver_data;
	driver_data->cb = cb;
	driver_data->cb_context = cb_context;
}

static int ipm_dummy_set_enabled(struct device *d, int enable)
{
	struct ipm_dummy_driver_data *driver_data = d->driver_data;

	driver_data->regs.enabled = enable;
	if (enable) {
		/* In case there are pending messages */
		irq_offload(ipm_dummy_isr, d);
	}
	return 0;
}

static u32_t ipm_dummy_max_id_val_get(struct device *d)
{
	return 0xFFFFFFFF;
}

static int ipm_dummy_max_data_size_get(struct device *d)
{
	return DUMMY_IPM_DATA_WORDS * sizeof(u32_t);
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

int ipm_dummy_init(struct device *d)
{
	struct ipm_dummy_driver_data *driver_data;

	driver_data = d->driver_data;
	d->driver_api = &ipm_dummy_api;

	return 0;
}

