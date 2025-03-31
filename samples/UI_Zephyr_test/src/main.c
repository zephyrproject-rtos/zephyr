/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device.h>
#include "../generated/gui_guider.h"
#include "../generated/events_init.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

lv_ui guider_ui;

int main(void)
{
	const struct device *display_dev;

	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

    int ret_old = 0;

	// display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	display_dev = DEVICE_DT_GET(DT_NODELABEL(gc9x01_display));
	printk("display dev name:%s \n",display_dev->name);
	printk("display dev config: 0x%x \n",display_dev->config);
	printk("display dev api: 0x%x \n",display_dev->api);

	// struct spi_dt_spec spi;
	// spi = SPI_DT_SPEC_INST_GET(1, 0 | SPI_WORD_SET(8), 0);
	// printk("%d",spi.config.frequency);
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	ret_old = gpio_pin_configure(dev, 1, 1);
    ret_old = gpio_port_toggle_bits(dev, 1);
    ret_old = gpio_port_toggle_bits(dev, 1);

	setup_ui(&guider_ui);
   	events_init(&guider_ui);

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
		printk("Inside loop");
	}
}
