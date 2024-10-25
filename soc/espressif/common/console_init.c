/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include "stubs.h"
#include "console_init.h"
#include "soc/uart_periph.h"
#include "soc/uart_channel.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_periph.h"
#include "soc/gpio_sig_map.h"
#include "soc/rtc.h"
#include "hal/clk_gate_ll.h"
#include "hal/gpio_hal.h"
#if CONFIG_SOC_SERIES_ESP32S2
#include "esp32s2/rom/usb/cdc_acm.h"
#include "esp32s2/rom/usb/usb_common.h"
#include "esp32s2/rom/usb/usb_persist.h"
#endif
#include "esp_rom_gpio.h"
#include "esp_rom_uart.h"
#include "esp_rom_sys.h"
#include "esp_rom_caps.h"

void esp_console_deinit(void)
{
#ifdef ESP_CONSOLE_UART
	/* Ensure any buffered log output is displayed */
	esp_rom_uart_flush_tx(ESP_CONSOLE_UART_NUM);
#endif /* ESP_CONSOLE_UART */
}

#ifdef ESP_CONSOLE_UART
void esp_console_init(void)
{
	const int uart_num = ESP_CONSOLE_UART_NUM;

	esp_rom_install_uart_printf();

	esp_rom_uart_tx_wait_idle(0);

	/* Set configured UART console baud rate */
	uint32_t clock_hz = rtc_clk_apb_freq_get();
#if ESP_ROM_UART_CLK_IS_XTAL
	/* From esp32-s3 on, UART clk source is selected to XTAL in ROM */
	clock_hz = (uint32_t)rtc_clk_xtal_freq_get() * MHZ(1);
#endif
	esp_rom_uart_set_clock_baudrate(uart_num, clock_hz, ESP_CONSOLE_UART_BAUDRATE);
}
#endif /* ESP_CONSOLE_UART */

#ifdef ESP_CONSOLE_USB_SERIAL_JTAG
void esp_console_init(void)
{
	esp_rom_uart_switch_buffer(ESP_ROM_USB_SERIAL_DEVICE_NUM);
}
#endif /* ESP_CONSOLE_USB_SERIAL_JTAG */
