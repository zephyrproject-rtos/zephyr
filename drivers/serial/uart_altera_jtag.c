/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/sys_io.h>

#define DT_DRV_COMPAT   altr_jtag_uart

#define UART_ALTERA_JTAG_DATA_OFFSET    0x00        /* DATA : Register offset */
#define UART_ALTERA_JTAG_CTRL_OFFSET    0x04        /* CTRL : Register offset */
#define UART_IE_TX                      (1 << 0)    /* CTRL : TX Interrupt Enable */
#define UART_IE_RX                      (1 << 1)    /* CTRL : RX Interrupt Enable */
#define UART_DATA_MASK                  0xFF        /* DATA : Data Mask */
#define UART_WFIFO_MASK                 0xFFFF0000  /* CTRL : Transmit FIFO Mask */

#ifdef CONFIG_UART_ALTERA_JTAG_HAL
#include "altera_avalon_jtag_uart.h"
#include "altera_avalon_jtag_uart_regs.h"

extern int altera_avalon_jtag_uart_read(altera_avalon_jtag_uart_state *sp,
		char *buffer, int space, int flags);
extern int altera_avalon_jtag_uart_write(altera_avalon_jtag_uart_state *sp,
		const char *ptr, int count, int flags);
#else

/* device data */
struct uart_altera_jtag_device_data {
	struct k_spinlock lock;
};

/* device config */
struct uart_altera_jtag_device_config {
	mm_reg_t base;
};
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */

static void uart_altera_jtag_poll_out(const struct device *dev,
					       unsigned char c)
{
#ifdef CONFIG_UART_ALTERA_JTAG_HAL
	altera_avalon_jtag_uart_state ustate;

	ustate.base = JTAG_UART_0_BASE;
	altera_avalon_jtag_uart_write(&ustate, &c, 1, 0);
#else
	const struct uart_altera_jtag_device_config *config = dev->config;
	struct uart_altera_jtag_device_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* While TX FIFO full */
	while (!(sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET) & UART_WFIFO_MASK)) {
	}

	uint32_t data_val = sys_read32(config->base + UART_ALTERA_JTAG_DATA_OFFSET);

	data_val &= ~UART_DATA_MASK;
	data_val |= c;
	sys_write32(data_val, config->base + UART_ALTERA_JTAG_DATA_OFFSET);

	k_spin_unlock(&data->lock, key);
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */
}

static int uart_altera_jtag_init(const struct device *dev)
{
	/*
	 * Work around to clear interrupt enable bits
	 * as it is not being done by HAL driver explicitly.
	 */
#ifdef CONFIG_UART_ALTERA_JTAG_HAL
	IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(JTAG_UART_0_BASE, 0);
#else
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	ctrl_val &= ~(UART_IE_TX | UART_IE_RX);
	sys_write32(ctrl_val, config->base + UART_ALTERA_JTAG_CTRL_OFFSET);
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */
	return 0;
}

static const struct uart_driver_api uart_altera_jtag_driver_api = {
	.poll_in = NULL,
	.poll_out = &uart_altera_jtag_poll_out,
	.err_check = NULL,
};

#ifdef CONFIG_UART_ALTERA_JTAG_HAL
DEVICE_DT_INST_DEFINE(0, uart_altera_jtag_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_SERIAL_INIT_PRIORITY,
		      &uart_altera_jtag_driver_api);
#else
static struct uart_altera_jtag_device_data uart_altera_jtag_dev_data_0 = {
};

static const struct uart_altera_jtag_device_config uart_altera_jtag_dev_cfg_0 = {
	.base = DT_INST_REG_ADDR(0),
};
DEVICE_DT_INST_DEFINE(0,
		      uart_altera_jtag_init,
		      NULL,
		      &uart_altera_jtag_dev_data_0,
		      &uart_altera_jtag_dev_cfg_0,
		      PRE_KERNEL_1,
		      CONFIG_SERIAL_INIT_PRIORITY,
		      &uart_altera_jtag_driver_api);
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */
