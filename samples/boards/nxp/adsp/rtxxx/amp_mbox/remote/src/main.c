/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>

const static struct device *mbox = DEVICE_DT_GET(DT_ALIAS(mbox));
static mbox_channel_id_t chmbox;

void mbox_cb(const struct device *dev, uint32_t channel, void *user_data, struct mbox_msg *data)
{
	if (data == NULL) {
		printk("[DSP] IPI from MU(A).\n");
	} else {
		uint32_t val = *(uint32_t *)(data->data);

		printk("[DSP] Incoming mbox message: data(uint32_t)=0x%x\n", val);
	}
}

int main(void)
{
	int ret;

	printk("[DSP] Hello World! %s\n", CONFIG_BOARD);

	if (!device_is_ready(mbox)) {
		return 0;
	}

	if (mbox_register_callback(mbox, chmbox, &mbox_cb, NULL) < 0) {
		return 0;
	}
	if (mbox_set_enabled(mbox, chmbox, true) < 0) {
		return 0;
	}

	return 0;
}
