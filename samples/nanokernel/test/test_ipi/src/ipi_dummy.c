/* ipi_dummy.c - Fake IPI driver for testing upper-level drivers */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <ipi.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>

#define NUM_SW_IRQS 2

#include <irq_test_common.h>
#include "ipi_dummy.h"

static void (*trigger_dummy_isr)(void) = (vvfn)sw_isr_trigger_0;

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

	trigger_dummy_isr();

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
		trigger_dummy_isr();
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
	struct isrInitInfo iinfo;
	struct ipi_dummy_config_info *config_info;
	int irq, i;

	config_info = (struct ipi_dummy_config_info *)d->config->config_info;
	d->driver_api = &ipi_dummy_api;

	irq = config_info->sw_irq;
	if (irq >= NUM_SW_IRQS) {
		printk("ipi_dummy_init: invalid sw irq %d\n", irq);
		return DEV_INVALID_CONF;
	}

	for (i = 0; i < NUM_SW_IRQS; ++i) {
		if (i == irq) {
			iinfo.isr[i] = ipi_dummy_isr;
			iinfo.arg[i] = d;
		} else {
			iinfo.isr[i] = NULL;
			iinfo.arg[i] = NULL;
		}
	}

	if (initIRQ(&iinfo)) {
		printk("ipi_dummy_init: couldn't install sw irq handler\n");
		return DEV_FAIL;
	}

	return DEV_OK;
}

