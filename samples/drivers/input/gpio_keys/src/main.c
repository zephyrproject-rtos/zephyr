/*
 * Copyright (c) 2023 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/poweroff.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_keys, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	printf("System Wake-Up: Going to Power-Off in 10s\n");

	/* WARNING
	 *
	 * Some systems may be complete unavailable when in Power-Off. This
	 * means that in few cases even J-TAG/SWD is unavailable. To keep the
	 * access to programming and debugging interface the 10s sleep counter
	 * is provided.
	 */
	for (int count = 0; count < 10; ++count) {
		k_msleep(1000);
		printf(".");
	}

	printf("\nPower-Off: Press the user button defined at gpio-key to Wake-Up the system\n");
	k_msleep(1);
	sys_poweroff();

	return 0;
}
