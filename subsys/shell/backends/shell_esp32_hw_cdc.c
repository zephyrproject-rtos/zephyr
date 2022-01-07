/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <init.h>
#include <logging/log.h>
#include <soc.h>

static struct shell_transport shell_transport_esp32_hw_cdc;

SHELL_DEFINE(shell_esp32_hw_cdc, CONFIG_SHELL_PROMPT_ESP32_HW_CDC, &shell_transport_esp32_hw_cdc,
	     1, 0, SHELL_FLAG_OLF_CRLF);

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{

	*cnt = length;
	uint8_t *ptr = (uint8_t *)data;

	while (length--) {
		esp_rom_usb_uart_tx_one_char(*ptr++);
	}

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	int result = esp_rom_usb_uart_rx_one_char((uint8_t *)data);

	if (result) {
		*cnt = 0;
	} else {
		*cnt = 1;
	}

	return 0;
}

const struct shell_transport_api shell_transport_esp32_hw_cdc_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};

static int enable_shell_esp32_hw_cdc(const struct device *arg)
{
	ARG_UNUSED(arg);

	shell_transport_esp32_hw_cdc.api = &shell_transport_esp32_hw_cdc_api;

	static const struct shell_backend_config_flags cfg_flags =
					SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;
	shell_init(&shell_esp32_hw_cdc, NULL, cfg_flags, true, LOG_LEVEL_INF);
	return 0;
}
SYS_INIT(enable_shell_esp32_hw_cdc, POST_KERNEL, 0);
