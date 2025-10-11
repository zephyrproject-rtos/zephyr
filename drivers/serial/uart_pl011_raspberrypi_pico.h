/*
 * Copyright (c) 2024 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_PL011_RASPBERRYPI_PICO_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_PL011_RASPBERRYPI_PICO_H_

#define RASPBERRYPI_PICO_UART_CLOCK_CTLR_SUBSYS_CELL clk_id

static inline int pwr_on_raspberrypi_pico_uart(const struct device *dev)
{
	return 0;
}

static inline int clk_enable_raspberrypi_pico_uart(const struct device *dev, uint32_t clk)
{
	return 0;
}

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_PL011_RASPBERRYPI_PICO_H_ */
