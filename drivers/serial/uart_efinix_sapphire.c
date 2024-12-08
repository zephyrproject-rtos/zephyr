/*
 * Copyright (c) 2023 Efinix Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT efinix_sapphire_uart0

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#define UART_IRQ        DT_INST_IRQN(0)
#define UART0_BASE_ADDR DT_INST_REG_ADDR(0)

#define BSP_UART_DATA          0x00
#define BSP_UART_STATUS        0x04
#define BSP_UART_CLOCK_DIVIDER 0x08
#define BSP_UART_FRAME_CONFIG  0x0C

#define BSP_UART_WRITE_AVAILABILITY_MASK GENMASK(23, 16)
#define BSP_UART_READ_OCCUPANCY_MASK     GENMASK(31, 24)

#define UART0_DATA_REG_ADDR   UART0_BASE_ADDR + BSP_UART_DATA
#define UART0_STATUS_REG_ADDR UART0_BASE_ADDR + BSP_UART_STATUS
#define UART0_CLOCK_REG_ADDR  UART0_BASE_ADDR + BSP_UART_CLOCK_DIVIDER
#define UART0_FRAME_REG_ADDR  UART0_BASE_ADDR + BSP_UART_FRAME_CONFIG

#define UART0_SAMPLE_PER_BAUD 8
#define UART0_PARITY          0 /* Off */
#define UART0_STOP            0 /* 1 stop bit */


struct uart_efinix_sapphire_config {
	uint32_t baudrate;
};

static void uart_efinix_sapphire_poll_out(const struct device *dev, unsigned char c)
{
	/* uart_writeAvailability */
	while ((sys_read32(UART0_STATUS_REG_ADDR) & BSP_UART_WRITE_AVAILABILITY_MASK) == 0) {
	}

	sys_write8(c, UART0_DATA_REG_ADDR);
}

static int uart_efinix_sapphire_poll_in(const struct device *dev, unsigned char *c)
{

	if ((sys_read32(UART0_STATUS_REG_ADDR) & BSP_UART_READ_OCCUPANCY_MASK) != 0) {
		*c = (unsigned char)sys_read8(UART0_DATA_REG_ADDR);
		return 0;
	}

	return -1;
}

static DEVICE_API(uart, uart_efinix_sapphire_api) = {
	.poll_in = uart_efinix_sapphire_poll_in,
	.poll_out = uart_efinix_sapphire_poll_out,
	.err_check = NULL,
};

static const struct uart_efinix_sapphire_config uart_efinix_sapphire_cfg_0 = {
	.baudrate = DT_INST_PROP(0, current_speed),
};

static int uart_efinix_sapphire_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t prescaler = ((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC /
			       (uart_efinix_sapphire_cfg_0.baudrate * UART0_SAMPLE_PER_BAUD)) -
			      1) &
			     0xFFFFF;
	sys_write32(prescaler, UART0_CLOCK_REG_ADDR);

	/* 8 data bits, no parity, 1 stop bit */
	uint32_t frame_config = (UART0_SAMPLE_PER_BAUD - 1) | UART0_PARITY << 8 | UART0_STOP << 16;

	sys_write32(frame_config, UART0_FRAME_REG_ADDR);

	return 0;
}

/* Device tree instance 0 init */
DEVICE_DT_INST_DEFINE(0, uart_efinix_sapphire_init, NULL, NULL, &uart_efinix_sapphire_cfg_0,
		      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, (void *)&uart_efinix_sapphire_api);
