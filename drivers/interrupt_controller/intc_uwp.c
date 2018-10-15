/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>

#include "intc_uwp.h"
#include "uwp_hal.h"

/* convenience defines */
#define DEV_DATA(dev) \
	((struct intc_uwp_data * const)(dev)->driver_data)

static struct device DEVICE_NAME_GET(intc_uwp);
static struct device DEVICE_NAME_GET(aon_intc_uwp);

struct __intc_cb {
	uwp_intc_callback_t cb;
	void *data;
};

struct intc_uwp_data {
	struct __intc_cb irq[INTC_MAX_CHANNEL];
	struct __intc_cb fiq[INTC_MAX_CHANNEL];
};

static struct intc_uwp_data intc_uwp_dev_data;
static struct intc_uwp_data aon_intc_uwp_dev_data;

void uwp_intc_set_irq_callback(int channel,
		uwp_intc_callback_t cb, void *arg)
{
	struct device *dev = DEVICE_GET(intc_uwp);
	struct intc_uwp_data *data = DEV_DATA(dev);

	if (data->irq[channel].cb != NULL) {
		printk("IRQ %d callback has been registered.\n", channel);
		return;
	}

	data->irq[channel].cb = cb;
	data->irq[channel].data = arg;
}

void uwp_intc_unset_irq_callback(int channel)
{
	struct device *dev = DEVICE_GET(intc_uwp);
	struct intc_uwp_data *data = DEV_DATA(dev);

	data->irq[channel].cb = NULL;
	data->irq[channel].data = NULL;
}

void uwp_intc_set_fiq_callback(int channel,
		uwp_intc_callback_t cb, void *arg)
{
	struct device *dev = DEVICE_GET(intc_uwp);
	struct intc_uwp_data *data = DEV_DATA(dev);

	if (data->fiq[channel].cb != NULL) {
		printk("FIQ %d callback has been registered.\n", channel);
		return;
	}

	data->fiq[channel].cb = cb;
	data->fiq[channel].data = arg;
}

void uwp_intc_unset_fiq_callback(int channel)
{
	struct device *dev = DEVICE_GET(intc_uwp);
	struct intc_uwp_data *data = DEV_DATA(dev);

	data->fiq[channel].cb = NULL;
	data->fiq[channel].data = NULL;
}

void uwp_aon_intc_set_irq_callback(int channel,
		uwp_intc_callback_t cb, void *arg)
{
	struct device *dev = DEVICE_GET(aon_intc_uwp);
	struct intc_uwp_data *data = DEV_DATA(dev);

	if (data->irq[channel].cb != NULL) {
		printk("IRQ %d callback has been registered.\n", channel);
		return;
	}

	data->irq[channel].cb = cb;
	data->irq[channel].data = arg;
}

void uwp_aon_intc_unset_irq_callback(int channel)
{
	struct device *dev = DEVICE_GET(aon_intc_uwp);
	struct intc_uwp_data *data = DEV_DATA(dev);

	data->irq[channel].cb = NULL;
	data->irq[channel].data = NULL;
}

static void aon_intc_uwp_irq(void *arg)
{
	int ch;
	struct device *dev = (struct device *)arg;
	struct intc_uwp_data *data = DEV_DATA(dev);

	for (ch = 0; ch < INTC_MAX_CHANNEL; ch++) {
		if (uwp_aon_irq_status(ch) && data->irq[ch].cb) {
			uwp_aon_irq_disable(ch);
			data->irq[ch].cb(ch, data->irq[ch].data);
			uwp_aon_irq_enable(ch);
		}
	}
}

static void intc_uwp_irq(void *arg)
{
	int ch;
	struct device *dev = (struct device *)arg;
	struct intc_uwp_data *data = DEV_DATA(dev);

	for (ch = 0; ch < INTC_MAX_CHANNEL; ch++) {
		if (uwp_irq_status(ch) && data->irq[ch].cb) {
			uwp_irq_disable(ch);
			data->irq[ch].cb(ch, data->irq[ch].data);
			uwp_irq_enable(ch);
		}
	}
}

static void intc_uwp_fiq(void *arg)
{
	int ch;
	struct device *dev = (struct device *)arg;
	struct intc_uwp_data *data = DEV_DATA(dev);

	for (ch = 0; ch < INTC_MAX_CHANNEL; ch++) {
		if (uwp_fiq_status(ch) && data->fiq[ch].cb) {
			uwp_fiq_disable(ch);
			data->fiq[ch].cb(ch, data->fiq[ch].data);
			uwp_fiq_enable(ch);
		}
	}
}

static int intc_uwp_init(struct device *dev)
{
	uwp_sys_enable(BIT(APB_EB_INTC));
	uwp_sys_reset(BIT(APB_EB_INTC));

	IRQ_CONNECT(NVIC_INT_IRQ, 5,
				intc_uwp_irq,
				DEVICE_GET(intc_uwp), 0);
	irq_enable(NVIC_INT_IRQ);

	IRQ_CONNECT(NVIC_INT_FIQ, 5,
				intc_uwp_fiq,
				DEVICE_GET(intc_uwp), 0);
	irq_enable(NVIC_INT_FIQ);

	return 0;
}

static int aon_intc_uwp_init(struct device *dev)
{
	uwp_aon_enable(BIT(AON_EB_INTC));
	uwp_aon_reset(BIT(AON_RST_INTC));

	IRQ_CONNECT(NVIC_INT_AON_INTC, 5,
				aon_intc_uwp_irq,
				DEVICE_GET(aon_intc_uwp), 0);
	irq_enable(NVIC_INT_AON_INTC);

	return 0;
}

DEVICE_INIT(intc_uwp, CONFIG_INTC_UWP_DEVICE_NAME,
		    intc_uwp_init, &intc_uwp_dev_data, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

DEVICE_INIT(aon_intc_uwp, CONFIG_AON_INTC_UWP_DEVICE_NAME,
		    aon_intc_uwp_init, &aon_intc_uwp_dev_data, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
