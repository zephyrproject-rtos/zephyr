/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/device.h>
#include <string.h>

static const char fake_request[] = {"PRO_CPU: Fake request to APP_CPU"};

static const struct device *ipm_dev;
static char received_string[64];
static struct k_sem sync;

static void ipm_receive_callback(const struct device *ipmdev, void *user_data,
			       uint32_t id, volatile void *data)
{
	ARG_UNUSED(ipmdev);
	ARG_UNUSED(user_data);

	strcpy(received_string, (const char *)data);
	k_sem_give(&sync);
}

void main(void)
{
	k_sem_init(&sync, 0, 1);

	ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm0));
	if (!ipm_dev) {
		printk("Failed to get IPM device.\n\r");
		return;
	}

	ipm_register_callback(ipm_dev, ipm_receive_callback, NULL);

	while (1) {
		printk("PRO_CPU is sending a fake request, waiting remote response...\n\r");

		ipm_send(ipm_dev, -1, sizeof(fake_request), &fake_request, sizeof(fake_request));
		k_sem_take(&sync, K_FOREVER);

		printk("PRO_CPU received a message from APP_CPU : %s\n\r", received_string);

		k_sleep(K_MSEC(200));
	}
}
