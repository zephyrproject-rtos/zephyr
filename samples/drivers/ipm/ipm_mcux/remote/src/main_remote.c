/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>

void ping_ipm_callback(const struct device *dev, void *context,
		       uint32_t id, volatile void *data)
{
	ipm_send(dev, 1, 0, (const void *)data, 4);
}



int main(void)
{
	const struct device *ipm;

	ipm = DEVICE_DT_GET_ANY(nxp_lpc_mailbox);
	if (!(ipm && device_is_ready(ipm))) {
		while (1) {
		}
	}
	ipm_register_callback(ipm, ping_ipm_callback, NULL);
	ipm_set_enabled(ipm, 1);
	while (1) {
	}
	return 0;
}
