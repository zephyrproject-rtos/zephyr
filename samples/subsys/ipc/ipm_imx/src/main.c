/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/ipm.h>

static struct device *ipm;

static void ipm_callback(void *context, u32_t id, volatile void *data)
{
	int i;
	int status;
	u32_t *data32 = (u32_t *)data;

	printk("%s: id = %u, data = 0x", __func__, id);
	for (i = 0; i < (CONFIG_IPM_IMX_MAX_DATA_SIZE / 4); i++) {
		printk("%08x", data32[i]);
	}
	printk("\n");

	status = ipm_send(ipm, 1, id, (const void *)data,
			  CONFIG_IPM_IMX_MAX_DATA_SIZE);
	if (status) {
		printk("ipm_send() failed: %d\n", status);
	}
}

void main(void)
{
	ipm = device_get_binding("MU_B");
	if (!ipm) {
		while (1) {
		}
	}
	ipm_register_callback(ipm, ipm_callback, NULL);
	ipm_set_enabled(ipm, 1);
	printk("IPM initialized, data size = %d\n",
		CONFIG_IPM_IMX_MAX_DATA_SIZE);

	while (1) {
	}
}
