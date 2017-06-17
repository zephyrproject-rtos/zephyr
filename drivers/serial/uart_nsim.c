/*
 * Copyright (c) 2014-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>
#include <linker/sections.h>
#include <misc/__assert.h>
#include <zephyr/types.h>
#include <misc/util.h>
#include <string.h>
#include <board.h>
#include <init.h>
#include <uart.h>

#define NSIM_UART_DATA 0
#define NSIM_UART_STATUS 1

#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)

#define DATA_REG(dev) (DEV_CFG(dev)->regs + NSIM_UART_DATA)
#define STATUS_REG(dev) (DEV_CFG(dev)->regs + NSIM_UART_STATUS)

#define TXEMPTY 0x80 /* Transmit FIFO empty and next character can be sent */

static const struct uart_driver_api uart_nsim_driver_api;

/**
 * @brief Initialize fake serial port
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_nsim_init(struct device *dev)
{
	ARG_UNUSED(dev);

	dev->driver_api = &uart_nsim_driver_api;

	return 0;
}

/*
 * @brief Output a character to serial port
 *
 * @param dev UART device struct
 * @param c character to output
 */
unsigned char uart_nsim_poll_out(struct device *dev, unsigned char c)
{

	/* wait for transmitter to ready to accept a character */
	while ((_arc_v2_aux_reg_read(STATUS_REG(dev)) & TXEMPTY) == 0)
		;
	_arc_v2_aux_reg_write(DATA_REG(dev), c);
	return c;
}

static int uart_nsim_poll_in(struct device *dev, unsigned char *c)
{
	return -ENOTSUP;

}

static const struct uart_driver_api uart_nsim_driver_api = {
	.poll_out = uart_nsim_poll_out,
	.poll_in = uart_nsim_poll_in,
};

static struct uart_device_config uart_nsim_dev_cfg_0 = {
	.regs = CONFIG_UART_NSIM_PORT_0_BASE_ADDR,
};

DEVICE_INIT(uart_nsim0, CONFIG_UART_NSIM_PORT_0_NAME, &uart_nsim_init,
			NULL, &uart_nsim_dev_cfg_0,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
