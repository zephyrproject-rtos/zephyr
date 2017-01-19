/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel.h>
#include <arch/cpu.h>
#include <uart.h>
#include <sys_io.h>

#define UART_ALTERA_JTAG_DATA_REG                  0
#define UART_ALTERA_JTAG_DATA_DATA_MSK             (0x000000FF)
#define UART_ALTERA_JTAG_DATA_DATA_OFST            (0)
#define UART_ALTERA_JTAG_DATA_RVALID_MSK           (0x00008000)
#define UART_ALTERA_JTAG_DATA_RVALID_OFST          (15)
#define UART_ALTERA_JTAG_DATA_RAVAIL_MSK           (0xFFFF0000)
#define UART_ALTERA_JTAG_DATA_RAVAIL_OFST          (16)


#define UART_ALTERA_JTAG_CONTROL_REG               1
#define UART_ALTERA_JTAG_CONTROL_RE_MSK            (0x00000001)
#define UART_ALTERA_JTAG_CONTROL_RE_OFST           (0)
#define UART_ALTERA_JTAG_CONTROL_WE_MSK            (0x00000002)
#define UART_ALTERA_JTAG_CONTROL_WE_OFST           (1)
#define UART_ALTERA_JTAG_CONTROL_RI_MSK            (0x00000100)
#define UART_ALTERA_JTAG_CONTROL_RI_OFST           (8)
#define UART_ALTERA_JTAG_CONTROL_WI_MSK            (0x00000200)
#define UART_ALTERA_JTAG_CONTROL_WI_OFST           (9)
#define UART_ALTERA_JTAG_CONTROL_AC_MSK            (0x00000400)
#define UART_ALTERA_JTAG_CONTROL_AC_OFST           (10)
#define UART_ALTERA_JTAG_CONTROL_WSPACE_MSK        (0xFFFF0000)
#define UART_ALTERA_JTAG_CONTROL_WSPACE_OFST       (16)

#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)

static uint32_t control_reg_read(void *base)
{
	return _nios2_reg_read(base, UART_ALTERA_JTAG_CONTROL_REG);
}


static void control_reg_write(void *base, uint32_t data)
{
	return _nios2_reg_write(base, UART_ALTERA_JTAG_CONTROL_REG, data);
}


static void data_reg_write(void *base, uint32_t data)
{
	return _nios2_reg_write(base, UART_ALTERA_JTAG_DATA_REG, data);
}


static unsigned char uart_altera_jtag_poll_out(struct device *dev,
					       unsigned char c)
{
	const struct uart_device_config *config;

	config = DEV_CFG(dev);

	while (!(control_reg_read(config->base) &
		 UART_ALTERA_JTAG_CONTROL_WSPACE_MSK)) {
		/* Busy loop until FIFO space frees up */
	}
	data_reg_write(config->base, c);
	return 0;
}


static int uart_altera_jtag_init(struct device *dev)
{
	const struct uart_device_config *config;

	config = DEV_CFG(dev);
	/* Clear interrupt enable bits */
	control_reg_write(config->base, 0);
	return 0;
}


static const struct uart_driver_api uart_altera_jtag_driver_api = {
	.poll_in = NULL,
	.poll_out = &uart_altera_jtag_poll_out,
	.err_check = NULL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#error "Interrupt-driven Altera JTAG UART not implemented yet"

#endif
};

static const struct uart_device_config uart_altera_jtag_dev_cfg_0 = {
	.base = (void *)JTAG_UART_0_BASE,
	.sys_clk_freq = 0, /* Unused */
};

DEVICE_AND_API_INIT(uart_altera_jtag_0, "jtag_uart0",
		    uart_altera_jtag_init, NULL,
		    &uart_altera_jtag_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_altera_jtag_driver_api);
