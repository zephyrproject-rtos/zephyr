/* ipm_quark_se.c - Quark SE mailbox driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <zephyr/types.h>
#include <string.h>
#include <device.h>
#include <init.h>
#include <ipm.h>
#include <arch/cpu.h>
#include <misc/printk.h>
#include <misc/__assert.h>
#include <errno.h>
#include "ipm_quark_se.h"


/* We have a single ISR for all channels, so in order to properly handle
 * messages we need to figure out which device object corresponds to
 * in incoming channel
 */
static struct device *device_by_channel[QUARK_SE_IPM_CHANNELS];
static u32_t inbound_channels;

static u32_t quark_se_ipm_sts_get(void)
{
	return sys_read32(QUARK_SE_IPM_CHALL_STS) & inbound_channels;
}

static void set_channel_irq_state(int channel, int enable)
{
	mem_addr_t addr = QUARK_SE_IPM_MASK;
	int bit = channel + QUARK_SE_IPM_MASK_START_BIT;

	if (enable) {
		sys_clear_bit(addr, bit);
	} else {
		sys_set_bit(addr, bit);
	}
}


/* Interrupt handler, gets messages on all incoming enabled mailboxes */
void quark_se_ipm_isr(void *param)
{
	int channel;
	int sts, bit;
	struct device *d;
	const struct quark_se_ipm_config_info *config;
	struct quark_se_ipm_driver_data *driver_data;
	volatile struct quark_se_ipm *ipm;
	unsigned int key;

	ARG_UNUSED(param);

	while ((sts = quark_se_ipm_sts_get())) {
		__ASSERT(sts, "spurious IPM interrupt");
		bit = find_msb_set(sts) - 1;
		channel = bit / 2;
		d = device_by_channel[channel];

		__ASSERT(d, "got IRQ on channel with no IPM device");
		config = d->config->config_info;
		driver_data = d->driver_data;
		ipm = config->ipm;

		__ASSERT(driver_data->callback,
			 "enabled IPM channel with no callback");
		driver_data->callback(driver_data->callback_ctx,
				      ipm->ctrl & QUARK_SE_IPM_CTRL_CTRL_MASK,
				      &ipm->data);

		key = irq_lock();

		/* Clear the interrupt bit */
		ipm->sts = QUARK_SE_IPM_STS_IRQ_BIT;
		/* Clear channel status bit */
		ipm->sts = QUARK_SE_IPM_STS_STS_BIT;

		/* Wait for the above register writes to clear the channel
		 * to propagate to the global channel status register
		 */
		while (quark_se_ipm_sts_get() & (0x3 << (channel * 2))) {
			/* Busy-wait */
		}
		irq_unlock(key);
	}
}

static int quark_se_ipm_send(struct device *d, int wait, u32_t id,
			const void *data, int size)
{
	const struct quark_se_ipm_config_info *config = d->config->config_info;
	volatile struct quark_se_ipm *ipm = config->ipm;
	u32_t data32[4]; /* Until we change API to u32_t array */
	unsigned int flags;
	int i;

	if (id > QUARK_SE_IPM_MAX_ID_VAL) {
		return -EINVAL;
	}

	if (config->direction != QUARK_SE_IPM_OUTBOUND) {
		return -EINVAL;
	}

	if (size > QUARK_SE_IPM_DATA_REGS * sizeof(u32_t)) {
		return -EMSGSIZE;
	}

	flags = irq_lock();

	if (ipm->sts & QUARK_SE_IPM_STS_STS_BIT) {
		irq_unlock(flags);
		return -EBUSY;
	}

	/* Actual message is passing using 32 bits registers */
	memcpy(data32, data, size);

	for (i = 0; i < ARRAY_SIZE(data32); ++i) {
		ipm->data[i] = data32[i];
	}

	ipm->ctrl = id | QUARK_SE_IPM_CTRL_IRQ_BIT;

	/* Wait for HW to set the sts bit */
	while (!(ipm->sts & QUARK_SE_IPM_STS_STS_BIT)) {
	}

	irq_unlock(flags);

	if (wait) {
		/* Loop until remote clears the status bit */
		while (ipm->sts & QUARK_SE_IPM_STS_STS_BIT) {
		}
	}

	return 0;
}


static int quark_se_ipm_max_data_size_get(struct device *d)
{
	ARG_UNUSED(d);

	return QUARK_SE_IPM_DATA_REGS * sizeof(u32_t);
}


static u32_t quark_se_ipm_max_id_val_get(struct device *d)
{
	ARG_UNUSED(d);

	return QUARK_SE_IPM_MAX_ID_VAL;
}

static void quark_se_ipm_register_callback(struct device *d, ipm_callback_t cb,
				       void *context)
{
	struct quark_se_ipm_driver_data *driver_data = d->driver_data;

	driver_data->callback = cb;
	driver_data->callback_ctx = context;
}


static int quark_se_ipm_set_enabled(struct device *d, int enable)
{
	const struct quark_se_ipm_config_info *config_info =
		d->config->config_info;

	if (config_info->direction != QUARK_SE_IPM_INBOUND) {
		return -EINVAL;
	}
	set_channel_irq_state(config_info->channel, enable);
	return 0;
}

const struct ipm_driver_api ipm_quark_se_api_funcs = {
	.send = quark_se_ipm_send,
	.register_callback = quark_se_ipm_register_callback,
	.max_data_size_get = quark_se_ipm_max_data_size_get,
	.max_id_val_get = quark_se_ipm_max_id_val_get,
	.set_enabled = quark_se_ipm_set_enabled
};

int quark_se_ipm_controller_initialize(struct device *d)
{
	const struct quark_se_ipm_controller_config_info *config =
		d->config->config_info;
#if CONFIG_IPM_QUARK_SE_MASTER
	int i;

	/* Mask all mailbox interrupts, we'll enable them
	 * individually later. Clear out any pending messages
	 */
	sys_write32(0xFFFFFFFF, QUARK_SE_IPM_MASK);
	for (i = 0; i < QUARK_SE_IPM_CHANNELS; ++i) {
		volatile struct quark_se_ipm *ipm = QUARK_SE_IPM(i);

		ipm->sts = 0;
	}
#endif

	if (config->controller_init) {
		return config->controller_init();
	}
	return 0;
}


int quark_se_ipm_initialize(struct device *d)
{
	const struct quark_se_ipm_config_info *config = d->config->config_info;

	device_by_channel[config->channel] = d;
	if (config->direction == QUARK_SE_IPM_INBOUND) {
		inbound_channels |= (0x3 << (config->channel * 2));
	}

	return 0;
}
