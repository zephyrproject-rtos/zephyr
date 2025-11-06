/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lcd_par_s035);

#define INT_GPIO_DT_SPEC GPIO_DT_SPEC_GET(DT_NODELABEL(gt911_lcd_par_s035), irq_gpios)

static int lcd_par_s035_init(void)
{
	int ret;

#ifdef CONFIG_INPUT_GT911
	static const struct gpio_dt_spec int_gpio = INT_GPIO_DT_SPEC;

	if (!gpio_is_ready_dt(&int_gpio)) {
		LOG_ERR("GT911 INT_GPIO controller device not ready");
		return -ENODEV;
	}

	/*
	 * We need to configure the int-pin to 0, in order to enter the
	 * AddressMode0. Keeping the INT pin low during the reset sequence
	 * should result in the device selecting an I2C address of 0x5D.
	 */
	ret = gpio_pin_configure_dt(&int_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure GT911 INT_GPIO: %d", ret);
		return ret;
	}
#endif /* CONFIG_INPUT_GT911 */

#ifdef CONFIG_MIPI_DBI
	ret = device_init(DEVICE_DT_GET(DT_ALIAS(mipi_dbi)));
	if (ret != 0) {
		LOG_ERR("Failed to init mipi_dbi: %d", ret);
		return ret;
	}
#endif /* CONFIG_MIPI_DBI */

#ifdef CONFIG_DISPLAY
	ret = device_init(DEVICE_DT_GET(DT_NODELABEL(st7796s)));
	if (ret != 0) {
		LOG_ERR("Failed to init st7796s display driver: %d", ret);
		return ret;
	}
#endif /* CONFIG_DISPLAY */

#ifdef CONFIG_INPUT_GT911
	ret = device_init(DEVICE_DT_GET(DT_NODELABEL(gt911_lcd_par_s035)));
	if (ret != 0) {
		LOG_ERR("Failed to init gt911_lcd_par_s035 input driver: %d", ret);
		return ret;
	}
#endif /* CONFIG_DISPLAY */

	return 0;
}

SYS_INIT(lcd_par_s035_init, POST_KERNEL, CONFIG_SHIELD_LCD_PAR_S035_INIT_PRIORITY);
