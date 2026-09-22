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
static mbox_channel_id_t mbox_ch;

void mbox_cb(const struct device *dev, uint32_t channel, void *user_data, struct mbox_msg *data)
{
	if (data == NULL) {
		printk("[DSP] mbox IPI.\n");
	} else {
		uint32_t val = *(const uint32_t *)(data->data);

		printk("[DSP] Incoming mbox message: data(uint32_t)=0x%x\n", val);
	}
}

int main(void)
{
	int ret;

	printk("[DSP] Hello World! %s\n", CONFIG_BOARD_TARGET);

	if (!device_is_ready(mbox)) {
		return -ENODEV;
	}

	ret = mbox_register_callback(mbox, mbox_ch, &mbox_cb, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = mbox_set_enabled(mbox, mbox_ch, true);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
