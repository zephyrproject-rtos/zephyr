/* Copyright (c) 2018, Synopsys, Inc.
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

/*
 * for nsimdrv, "nsim_mem-dev=uart0,base=0xf0000000,irq=24" is
 * used to simulate a uart.
 *
 * UART Register set (this is not a Standards Compliant IP)
 * Also each reg is Word aligned, but only 8 bits wide
 */
#define R_ID0	0
#define R_ID1	4
#define R_ID2	8
#define R_ID3	12
#define R_DATA	16
#define R_STS	20
#define R_BAUDL	24
#define R_BAUDH	28

/* Bits for UART Status Reg (R/W) */
#define RXIENB  0x04	/* Receive Interrupt Enable */
#define TXIENB  0x40	/* Transmit Interrupt Enable */

#define RXEMPTY 0x20	/* Receive FIFO Empty: No char receivede */
#define TXEMPTY 0x80	/* Transmit FIFO Empty, thus char can be written into */

#define RXFULL  0x08	/* Receive FIFO full */
#define RXFULL1 0x10	/* Receive FIFO has space for 1 char (tot space=4) */

#define RXFERR  0x01	/* Frame Error: Stop Bit not detected */
#define RXOERR  0x02	/* OverFlow Err: Char recv but RXFULL still set */


#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)


#define UART_REG_SET(u, r, v) ((*(u8_t *)(u + r)) = v)
#define UART_REG_GET(u, r)    (*(u8_t *)(u + r))

#define UART_REG_OR(u, r, v)  UART_REG_SET(u, r, UART_REG_GET(u, r) | (v))
#define UART_REG_CLR(u, r, v) UART_REG_SET(u, r, UART_REG_GET(u, r) & ~(v))

#define UART_SET_DATA(uart, val)   UART_REG_SET(uart, R_DATA, val)
#define UART_GET_DATA(uart)        UART_REG_GET(uart, R_DATA)

#define UART_CLR_STATUS(uart, val) UART_REG_CLR(uart, R_STS, val)
#define UART_GET_STATUS(uart)      UART_REG_GET(uart, R_STS)


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
	u32_t regs = DEV_CFG(dev)->regs;
	/* wait for transmitter to ready to accept a character */

	while (!(UART_GET_STATUS(regs) & TXEMPTY))
		;

	UART_SET_DATA(regs, c);

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
