/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/ipm.h>

void ping_ipm_callback(const struct device *dev, void *context,
		       uint32_t id, volatile void *data)
{
	struct ipm_msg msg;

	msg.data = data;
	msg.size = 4;
	msg.id = 0;

	ipm_send(dev, 1, 0, &msg);
}



void main(void)
{
	const struct device *ipm;

	ipm = DEVICE_DT_GET_ANY(nxp_lpc_mailbox);
	if (!(ipm && device_is_ready(ipm))) {
		while (1) {
		}
	}
	ipm_register_callback(ipm, ping_ipm_callback, 0, NULL);
	ipm_set_enabled(ipm, 1);
	while (1) {
	}
}
