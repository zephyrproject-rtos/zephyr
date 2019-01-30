/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <string.h>

void main(void)
{
	if (strlen(CONFIG_UART_CONSOLE_ON_DEV_NAME) !=
	    strlen(CONFIG_CDC_ACM_PORT_NAME_0) ||
	    strncmp(CONFIG_UART_CONSOLE_ON_DEV_NAME, CONFIG_CDC_ACM_PORT_NAME_0,
		    strlen(CONFIG_UART_CONSOLE_ON_DEV_NAME))) {
		printk("Error: Console device name is not USB ACM\n");

		return;
	}

	while (1) {
		printk("Hello World! %s\n", CONFIG_ARCH);
		k_sleep(K_SECONDS(1));
	}
}
