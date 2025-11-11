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

static const char request[] = {"PRO_CPU: request to APP_CPU"};

static const struct device *ipm_dev;
static char received_string[64];
static struct k_sem sync;

static void ipm_receive_callback(const struct device *ipmdev, void *user_data, uint32_t id,
				 volatile void *data)
{
	ARG_UNUSED(ipmdev);
	ARG_UNUSED(user_data);

	strncpy(received_string, (const char *)data, sizeof(received_string));
	k_sem_give(&sync);
}

int main(void)
{
	int ret;

	k_sem_init(&sync, 0, 1);

	ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm0));
	if (!ipm_dev) {
		printk("Failed to get IPM device.\n\r");
		return 0;
	}

	ipm_register_callback(ipm_dev, ipm_receive_callback, NULL);

	/* Workaround to catch up with APPCPU */
	k_sleep(K_MSEC(50));

	while (1) {
		printk("PRO_CPU is sending a request, waiting remote response...\n\r");

		ipm_send(ipm_dev, -1, sizeof(request), &request, sizeof(request));

		ret = k_sem_take(&sync, K_MSEC(5000));

		if (ret) {
			printk("No response from APP_CPU - trying again.\r\n");
		} else {
			printk("PRO_CPU received a message from APP_CPU : %s\n\r", received_string);
		}

		k_sleep(K_MSEC(1000));
	}
	return 0;
}
