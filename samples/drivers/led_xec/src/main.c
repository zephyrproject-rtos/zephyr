/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
LOG_MODULE_DECLARE(led, CONFIG_LED_LOG_LEVEL);

#define K_WAIT_DELAY          100u

struct dev_info {
	const struct device *dev;
	uint32_t base_addr;
};

#define XEC_BBLED_DEV(node_id)		\
	{ .dev = DEVICE_DT_GET(node_id), .base_addr = (uint32_t)DT_REG_ADDR(node_id) },

static const struct dev_info led_dev_table[] = {
	DT_FOREACH_STATUS_OKAY(microchip_xec_bbled, XEC_BBLED_DEV)
};

static void print_bbled_regs(mem_addr_t bbled_base)
{
	uint32_t r = 0;

	if (!bbled_base) {
		return;
	}

	LOG_INF("BBLED @ 0x%lx", bbled_base);

	r = sys_read32(bbled_base);
	LOG_INF("config = 0x%x", r);
	r = sys_read32(bbled_base + 4U);
	LOG_INF("limits = 0x%x", r);
	r = sys_read32(bbled_base + 8U);
	LOG_INF("delay = 0x%x", r);
	r = sys_read32(bbled_base + 0xcU);
	LOG_INF("update_step_size = 0x%x", r);
	r = sys_read32(bbled_base + 0x10U);
	LOG_INF("update_interval = 0x%x", r);
	r = sys_read32(bbled_base + 0x14U);
	LOG_INF("output_delay = 0x%x", r);
}

int led_test(void)
{
	int ret = 0;
	size_t n = 0;
	uint32_t delay_on = 0, delay_off = 0;

	/* Account for the time serial port is detected so log messages can
	 * be seen
	 */
	k_sleep(K_SECONDS(1));

	LOG_INF("Microchip XEC EVB BBLED Sample");

	for (n = 0; n < ARRAY_SIZE(led_dev_table); n++) {
		const struct device *dev = led_dev_table[n].dev;
		uint32_t base = led_dev_table[n].base_addr;

		LOG_INF("BBLED instance %d @ %x", n, base);
		print_bbled_regs(base);

		if (!device_is_ready(dev)) {
			LOG_ERR("%s: device not ready", dev->name);
			continue;
		}

		LOG_INF("blink: T = 0.5 second, duty cycle = 0.5");
		delay_on = 250;
		delay_off = 250;
		ret = led_blink(dev, 0, delay_on, delay_off);
		if (ret) {
			LOG_ERR("LED blink API returned error %d", ret);
		}
		print_bbled_regs(base);

		LOG_INF("Delay 5 seconds");
		k_sleep(K_SECONDS(5));

		LOG_INF("blink: T = 3 seconds, duty cycle = 0.4");
		delay_on = 1200;
		delay_off = 1800;
		ret = led_blink(dev, 0, delay_on, delay_off);
		if (ret) {
			LOG_ERR("LED blink API returned error %d", ret);
		}
		print_bbled_regs(base);

		LOG_INF("Delay 15 seconds");
		k_sleep(K_SECONDS(15));

		LOG_INF("Set ON");
		ret = led_on(dev, 0);
		if (ret) {
			LOG_ERR("LED ON API returned error %d", ret);
		}
		print_bbled_regs(base);

		LOG_INF("Delay 2 seconds");
		k_sleep(K_SECONDS(2));

		LOG_INF("Set OFF");
		ret = led_off(dev, 0);
		if (ret) {
			LOG_ERR("LED OFF API returned error %d", ret);
		}
		print_bbled_regs(base);

		LOG_INF("Delay 2 seconds");
		k_sleep(K_SECONDS(2));
	}

	LOG_INF("LED test done");

	return 0;
}

void main(void)
{
	led_test();
}
