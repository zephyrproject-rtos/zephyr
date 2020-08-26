/* main.c - OpenThread NCP */

/*
 * Copyright (c) 2020 Tridonic GmbH & Co KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(ot_br, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>

#define APP_BANNER "***** OpenThread NCP on Zephyr %s *****"

void main(void)
{
#if defined(CONFIG_OPENTHREAD_NCP_SPINEL_ON_UART_ACM)
	struct device *dev;
	uint32_t baudrate = 0U;
	int ret;

	dev = device_get_binding(
		CONFIG_OPENTHREAD_NCP_SPINEL_ON_UART_DEV_NAME);
	if (!dev) {
		LOG_ERR("UART device not found");
		return;
	}

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_INF("Wait for host to settle");
	k_sleep(K_SECONDS(1));

	ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_WRN("Failed to get baudrate, ret code %d", ret);
	} else {
		LOG_INF("Baudrate detected: %d", baudrate);
	}
#endif /* CONFIG_OPENTHREAD_NCP_SPINEL_ON_UART_ACM */

	LOG_INF(APP_BANNER, CONFIG_NET_SAMPLE_APPLICATION_VERSION);
}
