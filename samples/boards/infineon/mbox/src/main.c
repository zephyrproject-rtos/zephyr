/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

#if defined(CONFIG_MBOX_SAMPLE_CORE_A)
#define TX_ID (1)
#define RX_ID (0)
static const bool core_a = true;
#else
#define TX_ID (0)
#define RX_ID (1)
static const bool core_a;
#endif

static uint32_t data;

static void rx_callback(const struct device *dev, uint32_t channel, void *user_data,
			struct mbox_msg *msg)
{
	printk("Pong [%s] (on channel %d [data %d, %d])\n", core_a ? "core A" : "core B", channel,
	       *((uint32_t *)msg->data), (int)msg->size);
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(mbox0));

	printk("Hello from APP [%s]\n", core_a ? "core A" : "core B");

	if (mbox_register_callback(dev, RX_ID, rx_callback, NULL)) {
		printk("mbox_register_callback() error\n");
		return 0;
	}

	if (mbox_set_enabled(dev, RX_ID, 1)) {
		printk("mbox_set_enabled() error\n");
		return 0;
	}

	while (1) {
		struct mbox_msg msg = {.data = &data, .size = sizeof(data)};

		printk("Ping [%s] (on channel %d)\n", core_a ? "core A" : "core B", TX_ID);

		if (mbox_send(dev, TX_ID, &msg) < 0) {
			printk("mbox_send() error [%s]\n", core_a ? "core A" : "core B");
		} else {
			data++;
		}

		k_sleep(K_MSEC(1000));
	}

	return 0;
}
