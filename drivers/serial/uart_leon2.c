/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/uart.h>
#include <soc.h>

#define CTRL_RE BIT(0)
#define CTRL_TE BIT(1)

#define STAT_DR BIT(0)
#define STAT_BR BIT(3)
#define STAT_OV BIT(4)
#define STAT_PE BIT(5)
#define STAT_FE BIT(6)
#define STAT_RESET (0)

#define SCALER_RELOAD (0)

#define UART1_IRQ (3)
#define UART2_IRQ (2)


static void uart_leon2_poll_out(struct device *dev, uint8_t c)
{
	sys_write32(c, LEON2_UART1_DATA);
}

static int uart_leon2_poll_in(struct device *dev, uint8_t *c)
{
	u32_t stat;

	stat = sys_read32(LEON2_UART1_STAT);
	if ((stat & STAT_DR) != 0) {
		*c = (uint8_t)sys_read32(LEON2_UART1_DATA);
		return 0;
	} else {
		return -1;
	}
}

static int uart_leon2_init(struct device *dev)
{
	/* disable receiver and transmitter */
	sys_write32(~CTRL_RE | ~CTRL_TE, LEON2_UART1_CTL);

	sys_write32(SCALER_RELOAD, LEON2_UART1_SCL);
	sys_write32(STAT_RESET, LEON2_UART1_STAT);

	/* activate receiver and transmitter */
	sys_write32(CTRL_RE | CTRL_TE, LEON2_UART1_CTL);

	return 0;
}

static const struct uart_driver_api uart_leon2_driver_api = {
	.poll_in = uart_leon2_poll_in,
	.poll_out = uart_leon2_poll_out,

	.err_check = NULL,
};

DEVICE_AND_API_INIT(uart_leon2_0, CONFIG_UART_LEON2_DEV_NAME,
		    uart_leon2_init, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_leon2_driver_api);
