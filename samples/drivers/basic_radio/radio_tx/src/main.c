/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/basic_radio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(basic_radio_tx, LOG_LEVEL_INF);

#define BASIC_RADIO_NODE 	DT_NODELABEL(basic_radio0)		/* select parent device node */
#define PHY0_NODE    		DT_NODELABEL(phy0)				/* select child PHY node */

static const struct device *const rf_dev = DEVICE_DT_GET(BASIC_RADIO_NODE);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define PACKET_INTERVAL_MS 2000U

static const uint8_t payload[] =
{
	75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60,
	59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44,
	43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28,
	27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
	11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,
};

int main(void)
{
	int ret;
	uint32_t cnt = 0;

	if (!device_is_ready(rf_dev))
	{
		LOG_ERR("basic_radio device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(led.port))
	{
		LOG_ERR("LED device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret != 0)
	{
		LOG_ERR("LED configure failed: %d", ret);
		return ret;
	}

	uint32_t caps = basic_radio_get_caps(rf_dev);
	LOG_INF("caps=0x%08x:", caps);

	static const struct basic_radio_config cfg = BASIC_RADIO_CONFIG_DT(PHY0_NODE);
	ret = basic_radio_config(rf_dev, &cfg);

	if (ret != 0)
	{
		LOG_ERR("basic_radio_config failed: %d", ret);
		return ret;
	}

	while (true)
	{
		LOG_INF("Transmitting at %u Hz every %u ms", cfg.frequency, PACKET_INTERVAL_MS);
		ret = basic_radio_send(rf_dev, payload, sizeof(payload), NULL, K_MSEC(500));
		
		if (ret != 0)
		{
			LOG_ERR("send failed: %d", ret);
		} 
		else
		{
			cnt++;
			LOG_INF("TX #%u: %u bytes sent", cnt, sizeof(payload));
			LOG_HEXDUMP_INF(payload, sizeof(payload), "payload");

			gpio_pin_set_dt(&led, 1);
			k_sleep(K_MSEC(100));
			gpio_pin_set_dt(&led, 0);
		}

		k_sleep(K_MSEC(PACKET_INTERVAL_MS));
	}

	return 0;
}