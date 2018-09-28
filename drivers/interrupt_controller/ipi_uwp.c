/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>

#include "uwp_hal.h"
#include "ipi_uwp.h"

#define DEV_DATA(dev) \
	((struct ipi_uwp_data * const)(dev)->driver_data)

static struct device DEVICE_NAME_GET(ipi_uwp);

struct __ipi_cb {
	uwp_ipi_callback_t cb;
	void *data;
};

struct ipi_uwp_data {
	struct __ipi_cb irq;
};

static struct ipi_uwp_data ipi_uwp_dev_data;

void uwp_ipi_set_callback(uwp_ipi_callback_t cb, void *arg)
{
	struct device *dev = DEVICE_GET(ipi_uwp);
	struct ipi_uwp_data *data = DEV_DATA(dev);

	if (data->irq.cb != NULL) {
		printk("ipi irq callback has been registered.\n");
		return;
	}

	data->irq.cb = cb;
	data->irq.data = arg;
}

void uwp_ipi_unset_callback(void)
{
	struct device *dev = DEVICE_GET(ipi_uwp);
	struct ipi_uwp_data *data = DEV_DATA(dev);

	data->irq.cb = NULL;
	data->irq.data = NULL;
}

static void ipi_uwp_irq(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct ipi_uwp_data *data = DEV_DATA(dev);

	irq_disable(NVIC_INT_GNSS2BTWF_IPI);

	uwp_ipi_clear_remote(IPI_CORE_BTWF, IPI_TYPE_IRQ0);

	if (data->irq.cb)
		data->irq.cb(data->irq.data);

	irq_enable(NVIC_INT_GNSS2BTWF_IPI);
}

static int ipi_uwp_init(struct device *dev)
{
	uwp_sys_enable(BIT(APB_EB_IPI));
	uwp_sys_reset(BIT(APB_EB_IPI));

	IRQ_CONNECT(NVIC_INT_GNSS2BTWF_IPI, 5,
				ipi_uwp_irq,
				DEVICE_GET(ipi_uwp), 0);
	irq_enable(NVIC_INT_GNSS2BTWF_IPI);

	return 0;
}

DEVICE_INIT(ipi_uwp, CONFIG_IPI_UWP_DEVICE_NAME,
		    ipi_uwp_init, &ipi_uwp_dev_data, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
