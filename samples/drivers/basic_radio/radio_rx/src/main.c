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

/* Runtime parameters on top of the boot PHY */
/* 10 Hz units */
#define RADIO_FREQUENCY    245000000U
#define RADIO_SYNC_WORD    0x930B51DEU
#define RADIO_TX_POWER_DBM 5

static const struct device *const rf_dev = DEVICE_DT_GET(BASIC_RADIO_NODE);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define RX_BUF_LEN CONFIG_BASIC_RADIO_MAX_PACKET_LEN

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

	struct basic_radio_caps caps;

	ret = basic_radio_get_caps(rf_dev, &caps);
	if (ret == 0) {
		LOG_INF("caps=0x%08x tx_power=[%d, %d] dBm", caps.flags, caps.tx_power_min_dbm,
			caps.tx_power_max_dbm);
	}

	static const struct basic_radio_config cfg = {
		.phy = NULL,
		.frequency = RADIO_FREQUENCY,
		.sync_word = RADIO_SYNC_WORD,
		.max_packet_len = CONFIG_BASIC_RADIO_MAX_PACKET_LEN,
		.tx_power = RADIO_TX_POWER_DBM,
	};
	ret = basic_radio_config(rf_dev, &cfg);

	if (ret != 0)
	{
		LOG_ERR("basic_radio_config failed: %d", ret);
		return ret;
	}

	while (true)
	{
		struct basic_radio_rx_info info = {0};
		LOG_INF("Listening for packets at %u0 Hz", cfg.frequency);
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
