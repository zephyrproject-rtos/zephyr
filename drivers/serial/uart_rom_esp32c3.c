/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32c3_uart

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <device.h>
#include <soc.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
#include <errno.h>
#include <sys/util.h>
#include <esp_attr.h>

static int uart_rom_esp32c3_poll_in(const struct device *dev, unsigned char *p_char)
{
	ARG_UNUSED(dev);
	return (int)esp_rom_uart_rx_one_char(p_char);
}

static IRAM_ATTR void uart_rom_esp32c3_poll_out(const struct device *dev,
				unsigned char c)
{
	ARG_UNUSED(dev);
	esp_rom_uart_tx_one_char(c);
}

static int uart_rom_esp32c3_poll_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int uart_rom_esp32c3_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const DRAM_ATTR struct uart_driver_api uart_rom_esp32c3_api = {
	.poll_in = uart_rom_esp32c3_poll_in,
	.poll_out = uart_rom_esp32c3_poll_out,
	.err_check = uart_rom_esp32c3_poll_err_check,
};

#define ESP32C3_ROM_UART_INIT(idx)	\
DEVICE_DT_DEFINE(DT_NODELABEL(uart##idx),	\
		    &uart_rom_esp32c3_init,	\
		    NULL,	\
		    NULL,	\
		    NULL,	\
		    PRE_KERNEL_1,	\
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &uart_rom_esp32c3_api);	\

DT_INST_FOREACH_STATUS_OKAY(ESP32C3_ROM_UART_INIT)
