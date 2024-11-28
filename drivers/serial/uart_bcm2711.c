/*
 * Copyright (c) 2023 honglin leng <a909204013@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2711_aux_uart

/**
 * @brief BCM2711 Miniuart Serial Driver
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <stdbool.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

#define BCM2711_MU_IO			0x00
#define BCM2711_MU_IER			0x04
#define BCM2711_MU_IIR			0x08
#define BCM2711_MU_LCR			0x0c
#define BCM2711_MU_MCR			0x10
#define BCM2711_MU_LSR			0x14
#define BCM2711_MU_MSR			0x18
#define BCM2711_MU_SCRATCH		0x1c
#define BCM2711_MU_CNTL			0x20
#define BCM2711_MU_STAT			0x24
#define BCM2711_MU_BAUD			0x28

#define BCM2711_MU_IER_TX_INTERRUPT	BIT(1)
#define BCM2711_MU_IER_RX_INTERRUPT	BIT(0)

#define BCM2711_MU_IIR_RX_INTERRUPT	BIT(2)
#define BCM2711_MU_IIR_TX_INTERRUPT	BIT(1)
#define BCM2711_MU_IIR_FLUSH		0xc6

#define BCM2711_MU_LCR_7BIT		0x02
#define BCM2711_MU_LCR_8BIT		0x03

#define BCM2711_MU_LSR_TX_IDLE		BIT(6)
#define BCM2711_MU_LSR_TX_EMPTY		BIT(5)
#define BCM2711_MU_LSR_RX_OVERRUN	BIT(1)
#define BCM2711_MU_LSR_RX_READY		BIT(0)

#define BCM2711_MU_CNTL_RX_ENABLE	BIT(0)
#define BCM2711_MU_CNTL_TX_ENABLE	BIT(1)

struct bcm2711_uart_config {
	DEVICE_MMIO_ROM; /* Must be first */
	uint32_t baud_rate;
	uint32_t clocks;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct bcm2711_uart_data {
	DEVICE_MMIO_RAM; /* Must be first */
	mem_addr_t uart_addr;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static bool bcm2711_mu_lowlevel_can_getc(mem_addr_t base)
{
	return sys_read32(base + BCM2711_MU_LSR) & BCM2711_MU_LSR_RX_READY;
}

static bool bcm2711_mu_lowlevel_can_putc(mem_addr_t base)
{
	return sys_read32(base + BCM2711_MU_LSR) & BCM2711_MU_LSR_TX_EMPTY;
}

static void bcm2711_mu_lowlevel_putc(mem_addr_t base, uint8_t ch)
{
	/* Wait until there is data in the FIFO */
	while (!bcm2711_mu_lowlevel_can_putc(base)) {
		;
	}

	/* Send the character */
	sys_write32(ch, base + BCM2711_MU_IO);
}

static void bcm2711_mu_lowlevel_init(mem_addr_t base, bool skip_baudrate_config,
			      uint32_t baudrate, uint32_t input_clock)
{
	uint32_t divider;

	/* Wait until there is data in the FIFO */
	while (!bcm2711_mu_lowlevel_can_putc(base)) {
		;
	}

	/* Disable port */
	sys_write32(0x0, base + BCM2711_MU_CNTL);

	/* Disable interrupts */
	sys_write32(0x0, base + BCM2711_MU_IER);

	/* Setup 8bit data width and baudrate */
	sys_write32(BCM2711_MU_LCR_8BIT, base + BCM2711_MU_LCR);
	if (!skip_baudrate_config) {
		divider = (input_clock / (baudrate * 8));
		sys_write32(divider - 1, base + BCM2711_MU_BAUD);
	}

	/* Enable RX & TX port */
	sys_write32(BCM2711_MU_CNTL_RX_ENABLE | BCM2711_MU_CNTL_TX_ENABLE, base + BCM2711_MU_CNTL);
}

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_bcm2711_init(const struct device *dev)
{
	const struct bcm2711_uart_config *uart_cfg = dev->config;
	struct bcm2711_uart_data *uart_data = dev->data;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	uart_data->uart_addr = DEVICE_MMIO_GET(dev);
	bcm2711_mu_lowlevel_init(uart_data->uart_addr, 1, uart_cfg->baud_rate, uart_cfg->clocks);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_cfg->irq_config_func(dev);
#endif
	return 0;
}

static void uart_bcm2711_poll_out(const struct device *dev, unsigned char c)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	bcm2711_mu_lowlevel_putc(uart_data->uart_addr, c);
}

static int uart_bcm2711_poll_in(const struct device *dev, unsigned char *c)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	while (!bcm2711_mu_lowlevel_can_getc(uart_data->uart_addr)) {
		;
	}

	return sys_read32(uart_data->uart_addr + BCM2711_MU_IO) & 0xFF;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_bcm2711_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	int num_tx = 0U;
	struct bcm2711_uart_data *uart_data = dev->data;

	while ((size - num_tx) > 0) {
		/* Send a character */
		bcm2711_mu_lowlevel_putc(uart_data->uart_addr, tx_data[num_tx]);
		num_tx++;
	}

	return num_tx;
}

static int uart_bcm2711_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	int num_rx = 0U;
	struct bcm2711_uart_data *uart_data = dev->data;

	while ((size - num_rx) > 0 && bcm2711_mu_lowlevel_can_getc(uart_data->uart_addr)) {
		/* Receive a character */
		rx_data[num_rx++] = sys_read32(uart_data->uart_addr + BCM2711_MU_IO) & 0xFF;
	}
	return num_rx;
}

static void uart_bcm2711_irq_tx_enable(const struct device *dev)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	sys_write32(BCM2711_MU_IER_TX_INTERRUPT, uart_data->uart_addr + BCM2711_MU_IER);
}

static void uart_bcm2711_irq_tx_disable(const struct device *dev)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	sys_write32((uint32_t)(~BCM2711_MU_IER_TX_INTERRUPT),
		    uart_data->uart_addr + BCM2711_MU_IER);
}

static int uart_bcm2711_irq_tx_ready(const struct device *dev)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	return bcm2711_mu_lowlevel_can_putc(uart_data->uart_addr);
}

static void uart_bcm2711_irq_rx_enable(const struct device *dev)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	sys_write32(BCM2711_MU_IER_RX_INTERRUPT, uart_data->uart_addr + BCM2711_MU_IER);
}

static void uart_bcm2711_irq_rx_disable(const struct device *dev)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	sys_write32((uint32_t)(~BCM2711_MU_IER_RX_INTERRUPT),
		    uart_data->uart_addr + BCM2711_MU_IER);
}

static int uart_bcm2711_irq_rx_ready(const struct device *dev)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	return bcm2711_mu_lowlevel_can_getc(uart_data->uart_addr);
}

static int uart_bcm2711_irq_is_pending(const struct device *dev)
{
	struct bcm2711_uart_data *uart_data = dev->data;

	return bcm2711_mu_lowlevel_can_getc(uart_data->uart_addr) ||
		bcm2711_mu_lowlevel_can_putc(uart_data->uart_addr);
}

static int uart_bcm2711_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_bcm2711_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct bcm2711_uart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * Note: imx UART Tx interrupts when ready to send; Rx interrupts when char
 * received.
 *
 * @param arg Argument to ISR.
 */
void uart_isr(const struct device *dev)
{
	struct bcm2711_uart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_bcm2711_driver_api) = {
	.poll_in  = uart_bcm2711_poll_in,
	.poll_out = uart_bcm2711_poll_out,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill		  = uart_bcm2711_fifo_fill,
	.fifo_read		  = uart_bcm2711_fifo_read,
	.irq_tx_enable	  = uart_bcm2711_irq_tx_enable,
	.irq_tx_disable   = uart_bcm2711_irq_tx_disable,
	.irq_tx_ready	  = uart_bcm2711_irq_tx_ready,
	.irq_rx_enable	  = uart_bcm2711_irq_rx_enable,
	.irq_rx_disable   = uart_bcm2711_irq_rx_disable,
	.irq_rx_ready	  = uart_bcm2711_irq_rx_ready,
	.irq_is_pending   = uart_bcm2711_irq_is_pending,
	.irq_update		  = uart_bcm2711_irq_update,
	.irq_callback_set = uart_bcm2711_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */

};

#define UART_DECLARE_CFG(n, IRQ_FUNC_INIT)                                                         \
	static const struct bcm2711_uart_config bcm2711_uart_##n##_config = {                      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)), .baud_rate = DT_INST_PROP(n, current_speed), \
		.clocks = DT_INST_PROP(n, clock_frequency), IRQ_FUNC_INIT}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_CONFIG_FUNC(n)                                                                        \
	static void irq_config_func_##n(const struct device *dev)                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_isr,                   \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define UART_IRQ_CFG_FUNC_INIT(n) .irq_config_func = irq_config_func_##n
#define UART_INIT_CFG(n)          UART_DECLARE_CFG(n, UART_IRQ_CFG_FUNC_INIT(n))
#else
#define UART_CONFIG_FUNC(n)
#define UART_IRQ_CFG_FUNC_INIT
#define UART_INIT_CFG(n) UART_DECLARE_CFG(n, UART_IRQ_CFG_FUNC_INIT)
#endif

#define UART_INIT(n)                                                                               \
	static struct bcm2711_uart_data bcm2711_uart_##n##_data;                                   \
                                                                                                   \
	static const struct bcm2711_uart_config bcm2711_uart_##n##_config;                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &uart_bcm2711_init, NULL, &bcm2711_uart_##n##_data,               \
			      &bcm2711_uart_##n##_config, PRE_KERNEL_1,                            \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_bcm2711_driver_api);              \
                                                                                                   \
	UART_CONFIG_FUNC(n)                                                                        \
                                                                                                   \
	UART_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_INIT)
