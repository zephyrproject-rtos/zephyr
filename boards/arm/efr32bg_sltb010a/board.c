/*
 * Copyright (c) 2021 Sateesh Kotapati
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(efr32bg_sltb010a, CONFIG_BOARD_EFR32BG22_LOG_LEVEL);

static int efr32bg_sltb010a_init(const struct device *dev)
{
	const struct gpio_dt_spec vcen_dev =
		GPIO_DT_SPEC_GET(DT_ALIAS(vcomenable), gpios);

	ARG_UNUSED(dev);

	if (!device_is_ready(vcen_dev.port)) {
		LOG_ERR("Virtual COM Port Enable device was not found!\n");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&vcen_dev, GPIO_OUTPUT_HIGH);

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(efr32bg_sltb010a_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
