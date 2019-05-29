/*
 * Copyright (c) 2019 Western Digital Corporation or its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <uart.h>

struct uart_mmio8_device_config {
	u32_t base;
};

static const struct uart_mmio8_device_config uart_mmio8_dev_cfg_0 = {
	.base		= CONFIG_UART_MMIO8_BASE
};

static int uart_mmio8_init(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int uart_mmio8_poll_in(struct device *dev, unsigned char *c)
{
	return -ENOTSUP;
}

static void uart_mmio8_poll_out(struct device *dev, unsigned char c)
{
	const struct uart_mmio8_device_config *config = dev->config->config_info;
	sys_write8(c, config->base);
}

static const struct uart_driver_api uart_mmio8_driver_api = {
	.poll_in          = uart_mmio8_poll_in,
	.poll_out         = uart_mmio8_poll_out,
	.err_check        = NULL,
};

DEVICE_AND_API_INIT(uart_mmio8_0, "uart0", &uart_mmio8_init,
		    NULL, &uart_mmio8_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_mmio8_driver_api);
