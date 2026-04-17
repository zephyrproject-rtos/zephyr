/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(am62l_evm_board, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_AM62L_EVM_IO_EXPANDER

#define MAIN_GPIO0_2 DT_NODELABEL(main_gpio0_2)

static int am62l_evm_configure_io_expander(void)
{
	const struct device *gpio0_2_dev = DEVICE_DT_GET(MAIN_GPIO0_2);
	int ret = 0;

	if (!device_is_ready(gpio0_2_dev)) {
		LOG_ERR("GPIO controller not ready");
		return -ENODEV;
	}

	/* Configure GPIO0.89 */
	ret = gpio_pin_configure(gpio0_2_dev, 25, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO0.89 as output: %d", ret);
		return ret;
	}

	ret = gpio_pin_set(gpio0_2_dev, 25, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set GPIO0.89 high: %d", ret);
		return ret;
	}

	LOG_INF("I/O expander configured successfully");

	return 0;
}

SYS_INIT(am62l_evm_configure_io_expander, POST_KERNEL, UTIL_INC(CONFIG_GPIO_INIT_PRIORITY));

#endif /* CONFIG_AM62L_EVM_IO_EXPANDER */
