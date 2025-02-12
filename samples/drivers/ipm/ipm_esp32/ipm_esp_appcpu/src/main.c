/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/device.h>

static const struct device *ipm_dev;
static const char fake_resp[] = {"APP_CPU: This is a response"};
struct k_sem sync;

static void ipm_receive_callback(const struct device *ipmdev, void *user_data,
			       uint32_t id, volatile void *data)
{
	k_sem_give(&sync);
}

int main(void)
{
	k_sem_init(&sync, 0, 1);

	ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm0));
	if (!ipm_dev) {
		printk("Failed to get IPM device.\n\r");
		return 0;
	}

	ipm_register_callback(ipm_dev, ipm_receive_callback, NULL);

	while (1) {
		k_sem_take(&sync, K_FOREVER);
		ipm_send(ipm_dev, -1, sizeof(fake_resp), &fake_resp, sizeof(fake_resp));
	}
	return 0;
}
