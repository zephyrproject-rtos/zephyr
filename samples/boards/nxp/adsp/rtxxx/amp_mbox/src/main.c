/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

#include "dsp.h"

static const struct device *mbox = DEVICE_DT_GET(DT_ALIAS(mbox));
static mbox_channel_id_t mbox_ch;

static uint32_t mbox_data = 0x12345678;

int main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD_TARGET);

	if (!device_is_ready(mbox)) {
		return 0;
	}
	mbox_set_enabled(mbox, mbox_ch, true);

	printk("[ARM] Starting DSP...\n");
	dsp_start();
	k_msleep(1000);

	struct mbox_msg msg = {.data = &mbox_data, .size = sizeof(mbox_data)};

	printk("[ARM] Sending mbox message...\n");
	if (mbox_send(mbox, mbox_ch, &msg) < 0) {
		printk("[ARM] mbox send failed!\n");
	}

	k_msleep(1000);
	printk("[ARM] Sending mbox IPI...\n");
	if (mbox_send(mbox, mbox_ch, NULL) < 0) {
		printk("[ARM] mbox IPI failed!\n");
	}

	return 0;
}
