/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <uart.h>
#include <sys_io.h>

#define DEV_CFG(dev)					\
	((const struct uart_device_config * const)	\
	 (dev)->config->config_info)

static unsigned char uart_riscv_qemu_poll_out(struct device *dev,
					      unsigned char c)
{
	sys_write8(c, DEV_CFG(dev)->regs);
	return c;
}

static int uart_riscv_qemu_poll_in(struct device *dev, unsigned char *c)
{
	*c = sys_read8(DEV_CFG(dev)->regs);
	return 0;
}

static int uart_riscv_qemu_init(struct device *dev)
{
	/* Nothing to do */

	return 0;
}


static const struct uart_driver_api uart_riscv_qemu_driver_api = {
	.poll_in = uart_riscv_qemu_poll_in,
	.poll_out = uart_riscv_qemu_poll_out,
	.err_check = NULL,
};

static const struct uart_device_config uart_riscv_qemu_dev_cfg_0 = {
	.regs = RISCV_QEMU_UART_BASE,
};

DEVICE_AND_API_INIT(uart_riscv_qemu_0, "uart0",
		    uart_riscv_qemu_init, NULL,
		    &uart_riscv_qemu_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_riscv_qemu_driver_api);
