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

LOG_MODULE_REGISTER(basic_radio_rx, LOG_LEVEL_INF);

#define BASIC_RADIO_NODE 	DT_NODELABEL(basic_radio0)
#define PHY0_NODE    	 	DT_NODELABEL(phy0)

static const struct device *const rf_dev = DEVICE_DT_GET(BASIC_RADIO_NODE);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define RX_BUF_LEN DT_PROP_OR(PHY0_NODE, max_packet_length, 128)

static uint8_t rx_buf[RX_BUF_LEN];

int main(void)
{
	int ret;
	uint32_t cnt = 0;

	if (!device_is_ready(rf_dev))
	{
		LOG_ERR("basic_radio device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(led.port)) {
		LOG_ERR("LED device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
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
		struct basic_radio_rx_info info = {0};
		LOG_INF("Listening for packets at %u Hz", cfg.frequency);
		ret = basic_radio_recv(rf_dev, rx_buf, sizeof(rx_buf), NULL, K_FOREVER, &info);

		cnt++;
		LOG_INF("RX #%u: %d bytes RSSI=%d dBm crc_ok=%d", cnt, ret, info.rssi, (int)info.crc_ok);
		LOG_HEXDUMP_INF(rx_buf, ret, "payload");

		gpio_pin_set_dt(&led, 1);
		k_sleep(K_MSEC(100));
		gpio_pin_set_dt(&led, 0);
	}

	return 0;
}