/* ipi_dummy.c - Fake IPI driver for testing upper-level drivers */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <ipi.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>
#include <irq_offload.h>

#include "ipi_dummy.h"


/* Implemented as a software interrupt so that callbacks are executed
 * in the expected context */
static void ipi_dummy_isr(void *data)
{
	struct device *d = (struct device *)data;
	struct ipi_dummy_driver_data *driver_data = d->driver_data;

	/* In a real driver the interrupt simply wouldn't fire, we fake
	 * that here */
	if (!driver_data->regs.enabled || !driver_data->regs.busy)
		return;

	if (driver_data->cb) {
		driver_data->cb(driver_data->cb_context, driver_data->regs.id,
				(volatile void *)&driver_data->regs.data);
	}
	driver_data->regs.busy = 0;
}


/* IPI API functions for the dummy driver */

static int ipi_dummy_send(struct device *d, int wait, uint32_t id,
			  const void *data, int size)
{
	struct ipi_dummy_driver_data *driver_data;
	volatile uint8_t *datareg;
	const uint8_t *data8;
	int i;

	driver_data = d->driver_data;
	if (size > ipi_max_data_size_get(d)) {
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
	driver_data->regs.busy = 1;

	irq_offload(ipi_dummy_isr, d);

	if (wait) {
		while(driver_data->regs.busy) {
			/* busy-wait */
		}
	}
	return 0;
}

static void ipi_dummy_register_callback(struct device *d, ipi_callback_t cb,
					void *cb_context)
{
	struct ipi_dummy_driver_data *driver_data;
	driver_data = d->driver_data;
	driver_data->cb = cb;
	driver_data->cb_context = cb_context;
}

static int ipi_dummy_set_enabled(struct device *d, int enable)
{
	struct ipi_dummy_driver_data *driver_data = d->driver_data;

	driver_data->regs.enabled = enable;
	if (enable) {
		/* In case there are pending messages */
		irq_offload(ipi_dummy_isr, d);
	}
	return 0;
}

static uint32_t ipi_dummy_max_id_val_get(struct device *d)
{
	return 0xFFFFFFFF;
}

static int ipi_dummy_max_data_size_get(struct device *d)
{
	return DUMMY_IPI_DATA_WORDS * sizeof(uint32_t);
}

struct ipi_driver_api ipi_dummy_api = {
	.send = ipi_dummy_send,
	.register_callback = ipi_dummy_register_callback,
	.max_data_size_get = ipi_dummy_max_data_size_get,
	.max_id_val_get = ipi_dummy_max_id_val_get,
	.set_enabled = ipi_dummy_set_enabled
};

/* Dummy IPI driver initialization, will be bound at runtime
 * to high-level drivers under test */

int ipi_dummy_init(struct device *d)
{
	struct ipi_dummy_driver_data *driver_data;

	driver_data = d->driver_data;
	d->driver_api = &ipi_dummy_api;

	return DEV_OK;
}

