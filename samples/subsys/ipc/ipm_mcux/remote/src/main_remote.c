/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/ipm.h>

struct device *ipm;

void ping_ipm_callback(void *context, u32_t id, volatile void *data)
{
	ipm_send(ipm, 1, 0, (const void *)data, 4);
}



void main(void)
{
	ipm = device_get_binding(DT_INST_0_NXP_LPC_MAILBOX_LABEL);
	if (!ipm) {
		while (1) {
		}
	}
	ipm_register_callback(ipm, ping_ipm_callback, NULL);
	ipm_set_enabled(ipm, 1);
	while (1) {
	}
}
