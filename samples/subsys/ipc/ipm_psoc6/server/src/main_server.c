/*
 * Copyright (c) 2018, Cypress Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>

#include "cy_sysint.h"
#include "cy_wdt.h"
#include <ipm.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>
#include <string.h>

#define SLEEP_TIME 10000

void message_ipm_callback(void *context, u32_t id, volatile void *data)
{
	struct device *ipm = (struct device *)context;
	u32_t *data32 = (u32_t *)data;

	printk("Received %u via IPC, sending it back\n", *data32);

	ipm_send(ipm, 1, 0, data32, 4);
}

void main(void)
{
	struct device *ipm;

	/* Disable watchdog */
	Cy_WDT_Unlock();
	Cy_WDT_Disable();

	ipm = device_get_binding(DT_CYPRESS_PSOC6_MAILBOX_PSOC6_IPM0_LABEL);
	ipm_register_callback(ipm, message_ipm_callback, ipm);
	ipm_set_enabled(ipm, 1);

	printk("PSoC6 IPM Server example has started\n");

	/* Delay to avoid terminal output corruption */
	k_sleep(200u);

	/* Enable CM4 */
	Cy_SysEnableCM4(CONFIG_SLAVE_BOOT_ADDRESS_PSOC6);

	while (1) {
		k_sleep(SLEEP_TIME);
	}
}
