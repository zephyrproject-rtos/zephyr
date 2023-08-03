/*
 * Copyright (c) 2021, Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_cmsdk_uart

/**
 * @brief Driver for UART on ARM CMSDK APB UART.
 *
 * UART has two wires for RX and TX, and does not provide CTS or RTS.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control/arm_clock_control.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/linker/sections.h>
#include <zephyr/irq.h>

/* UART registers struct */
struct uart_cmsdk_apb {
	/* offset: 0x000 (r/w) data register    */
	volatile uint32_t  data;
	/* offset: 0x004 (r/w) status register  */
	volatile uint32_t  state;
	/* offset: 0x008 (r/w) control register */
	volatile uint32_t  ctrl;
	union {
		/* offset: 0x00c (r/ ) interrupt status register */
		volatile uint32_t  intstatus;
		/* offset: 0x00c ( /w) interrupt clear register  */
		volatile uint32_t  intclear;
	};
	/* offset: 0x010 (r/w) baudrate divider register */
	volatile uint32_t  bauddiv;
};

/* UART Bits */
/* CTRL Register */
#define UART_TX_EN	(1 << 0)
#define UART_RX_EN	(1 << 1)
#define UART_TX_IN_EN	(1 << 2)
#define UART_RX_IN_EN	(1 << 3)
#define UART_TX_OV_EN	(1 << 4)
#define UART_RX_OV_EN	(1 << 5)
#define UART_HS_TM_TX	(1 << 6)

/* STATE Register */
#define UART_TX_BF	(1 << 0)
#define UART_RX_BF	(1 << 1)
#define UART_TX_B_OV	(1 << 2)
#define UART_RX_B_OV	(1 << 3)

/* INTSTATUS Register */
#define UART_TX_IN	(1 << 0)
#define UART_RX_IN	(1 << 1)
#define UART_TX_OV_IN	(1 << 2)
#define UART_RX_OV_IN	(1 << 3)

struct uart_cmsdk_apb_config {
	volatile struct uart_cmsdk_apb *uart;
	uint32_t sys_clk_freq;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
};

/* Device data structure */
struct uart_cmsdk_apb_dev_data {
	uint32_t baud_rate;	/* Baud rate */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
	/* UART Clock control in Active State */
	const struct arm_clock_control_t uart_cc_as;
	/* UART Clock control in Sleep State */
	const struct arm_clock_control_t uart_cc_ss;
	/* UART Clock control in Deep Sleep State */
	const struct arm_clock_control_t uart_cc_dss;
};

static const struct uart_driver_api uart_cmsdk_apb_driver_api;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_isr(const struct device *dev);
#endif

/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UART.
 *
 * @param dev UART device struct
 */
static void baudrate_set(const struct device *dev)
{
	const struct uart_cmsdk_apb_config * const dev_cfg = dev->config;
	struct uart_cmsdk_apb_dev_data *const dev_data = dev->data;
	/*
	 * If baudrate and/or sys_clk_freq are 0 the configuration remains
	 * unchanged. It can be useful in case that Zephyr it is run via
	 * a bootloader that brings up the serial and sets the baudrate.
	 */
	if ((dev_data->baud_rate != 0U) && (dev_cfg->sys_clk_freq != 0U)) {
		/* calculate baud rate divisor */
		dev_cfg->uart->bauddiv = (dev_cfg->sys_clk_freq / dev_data->baud_rate);
	}
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
static int uart_cmsdk_apb_init(const struct device *dev)
{
	const struct uart_cmsdk_apb_config * const dev_cfg = dev->config;

#ifdef CONFIG_CLOCK_CONTROL
	/* Enable clock for subsystem */
	const struct device *const clk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(0, 1));
	struct uart_cmsdk_apb_dev_data * const data = dev->data;

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

#ifdef CONFIG_SOC_SERIES_BEETLE
	clock_control_on(clk, (clock_control_subsys_t) &data->uart_cc_as);
	clock_control_on(clk, (clock_control_subsys_t) &data->uart_cc_ss);
	clock_control_on(clk, (clock_control_subsys_t) &data->uart_cc_dss);
#endif /* CONFIG_SOC_SERIES_BEETLE */
#endif /* CONFIG_CLOCK_CONTROL */

	/* Set baud rate */
	baudrate_set(dev);

	/* Enable receiver and transmitter */
	dev_cfg->uart->ctrl = UART_RX_EN | UART_TX_EN;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_cfg->irq_config_func(dev);
#endif

	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

static int uart_cmsdk_apb_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	/* If the receiver is not ready returns -1 */
	if (!(dev_cfg->uart->state & UART_RX_BF)) {
		return -1;
	}

	/* got a character */
	*c = (unsigned char)dev_cfg->uart->data;

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_cmsdk_apb_poll_out(const struct device *dev,
					     unsigned char c)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	/* Wait for transmitter to be ready */
	while (dev_cfg->uart->state & UART_TX_BF) {
		; /* Wait */
	}

	/* Send a character */
	dev_cfg->uart->data = (uint32_t)c;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param len Number of bytes to send
 *
 * @return the number of characters that have been read
 */
static int uart_cmsdk_apb_fifo_fill(const struct device *dev,
				    const uint8_t *tx_data, int len)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	/*
	 * No hardware FIFO present. Only 1 byte
	 * to write if TX buffer is empty.
	 */
	if (len && !(dev_cfg->uart->state & UART_TX_BF)) {
		/*
		 * Clear TX int. pending flag before pushing byte to "FIFO".
		 * If TX interrupt is enabled the UART_TX_IN bit will be set
		 * again automatically by the UART hardware machinery once
		 * the "FIFO" becomes empty again.
		 */
		dev_cfg->uart->intclear = UART_TX_IN;
		dev_cfg->uart->data = *tx_data;
		return 1;
	}

	return 0;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct
 * @param rx_data Pointer to data container
 * @param size Container size in bytes
 *
 * @return the number of characters that have been read
 */
static int uart_cmsdk_apb_fifo_read(const struct device *dev,
				    uint8_t *rx_data, const int size)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	/*
	 * No hardware FIFO present. Only 1 byte
	 * to read if RX buffer is full.
	 */
	if (size && dev_cfg->uart->state & UART_RX_BF) {
		/*
		 * Clear RX int. pending flag before popping byte from "FIFO".
		 * If RX interrupt is enabled the UART_RX_IN bit will be set
		 * again automatically by the UART hardware machinery once
		 * the "FIFO" becomes full again.
		 */
		dev_cfg->uart->intclear = UART_RX_IN;
		*rx_data = (unsigned char)dev_cfg->uart->data;
		return 1;
	}

	return 0;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev UART device struct
 */
static void uart_cmsdk_apb_irq_tx_enable(const struct device *dev)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;
	unsigned int key;

	dev_cfg->uart->ctrl |= UART_TX_IN_EN;
	/* The expectation is that TX is a level interrupt, active for as
	 * long as TX buffer is empty. But in CMSDK UART it's an edge
	 * interrupt, firing on a state change of TX buffer from full to
	 * empty. So, we need to "prime" it here by calling ISR directly,
	 * to get interrupt processing going, as there is no previous
	 * full state to allow a transition from full to empty buffer
	 * that will trigger a TX interrupt.
	 */
	key = irq_lock();
	uart_cmsdk_apb_isr(dev);
	irq_unlock(key);
}

/**
 * @brief Disable TX interrupt
 *
 * @param dev UART device struct
 */
static void uart_cmsdk_apb_irq_tx_disable(const struct device *dev)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	dev_cfg->uart->ctrl &= ~UART_TX_IN_EN;
	/* Clear any pending TX interrupt after disabling it */
	dev_cfg->uart->intclear = UART_TX_IN;
}

/**
 * @brief Verify if Tx interrupt has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an interrupt is ready, 0 otherwise
 */
static int uart_cmsdk_apb_irq_tx_ready(const struct device *dev)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	return !(dev_cfg->uart->state & UART_TX_BF);
}

/**
 * @brief Enable RX interrupt
 *
 * @param dev UART device struct
 */
static void uart_cmsdk_apb_irq_rx_enable(const struct device *dev)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	dev_cfg->uart->ctrl |= UART_RX_IN_EN;
}

/**
 * @brief Disable RX interrupt
 *
 * @param dev UART device struct
 */
static void uart_cmsdk_apb_irq_rx_disable(const struct device *dev)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	dev_cfg->uart->ctrl &= ~UART_RX_IN_EN;
	/* Clear any pending RX interrupt after disabling it */
	dev_cfg->uart->intclear = UART_RX_IN;
}

/**
 * @brief Verify if Tx complete interrupt has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an interrupt is ready, 0 otherwise
 */
static int uart_cmsdk_apb_irq_tx_complete(const struct device *dev)
{
	return uart_cmsdk_apb_irq_tx_ready(dev);
}

/**
 * @brief Verify if Rx interrupt has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an interrupt is ready, 0 otherwise
 */
static int uart_cmsdk_apb_irq_rx_ready(const struct device *dev)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	return (dev_cfg->uart->state & UART_RX_BF) == UART_RX_BF;
}

/**
 * @brief Enable error interrupt
 *
 * @param dev UART device struct
 */
static void uart_cmsdk_apb_irq_err_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Disable error interrupt
 *
 * @param dev UART device struct
 */
static void uart_cmsdk_apb_irq_err_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Verify if Tx or Rx interrupt is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if Tx or Rx interrupt is pending, 0 otherwise
 */
static int uart_cmsdk_apb_irq_is_pending(const struct device *dev)
{
	const struct uart_cmsdk_apb_config *dev_cfg = dev->config;

	return (dev_cfg->uart->intstatus & (UART_RX_IN | UART_TX_IN));
}

/**
 * @brief Update the interrupt status
 *
 * @param dev UART device struct
 *
 * @return always 1
 */
static int uart_cmsdk_apb_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for an Interrupt.
 *
 * @param dev UART device structure
 * @param cb Callback function pointer.
 */
static void uart_cmsdk_apb_irq_callback_set(const struct device *dev,
					    uart_irq_callback_user_data_t cb,
					    void *cb_data)
{
	struct uart_cmsdk_apb_dev_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * Calls the callback function, if exists.
 *
 * @param arg argument to interrupt service routine.
 */
void uart_cmsdk_apb_isr(const struct device *dev)
{
	struct uart_cmsdk_apb_dev_data *data = dev->data;

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static const struct uart_driver_api uart_cmsdk_apb_driver_api = {
	.poll_in = uart_cmsdk_apb_poll_in,
	.poll_out = uart_cmsdk_apb_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_cmsdk_apb_fifo_fill,
	.fifo_read = uart_cmsdk_apb_fifo_read,
	.irq_tx_enable = uart_cmsdk_apb_irq_tx_enable,
	.irq_tx_disable = uart_cmsdk_apb_irq_tx_disable,
	.irq_tx_ready = uart_cmsdk_apb_irq_tx_ready,
	.irq_rx_enable = uart_cmsdk_apb_irq_rx_enable,
	.irq_rx_disable = uart_cmsdk_apb_irq_rx_disable,
	.irq_tx_complete = uart_cmsdk_apb_irq_tx_complete,
	.irq_rx_ready = uart_cmsdk_apb_irq_rx_ready,
	.irq_err_enable = uart_cmsdk_apb_irq_err_enable,
	.irq_err_disable = uart_cmsdk_apb_irq_err_disable,
	.irq_is_pending = uart_cmsdk_apb_irq_is_pending,
	.irq_update = uart_cmsdk_apb_irq_update,
	.irq_callback_set = uart_cmsdk_apb_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_0(const struct device *dev);
#endif

static const struct uart_cmsdk_apb_config uart_cmsdk_apb_dev_cfg_0 = {
	.uart = (volatile struct uart_cmsdk_apb *)DT_INST_REG_ADDR(0),
	.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(0, clocks, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_0,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_0 = {
	.baud_rate = DT_INST_PROP(0, current_speed),
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_REG_ADDR(0),},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_REG_ADDR(0),},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_REG_ADDR(0),},
};

DEVICE_DT_INST_DEFINE(0,
		    &uart_cmsdk_apb_init,
		    NULL,
		    &uart_cmsdk_apb_dev_data_0,
		    &uart_cmsdk_apb_dev_cfg_0, PRE_KERNEL_1,
		    CONFIG_SERIAL_INIT_PRIORITY,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#if DT_NUM_IRQS(DT_DRV_INST(0)) == 1
static void uart_cmsdk_apb_irq_config_func_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(0));
}
#else
static void uart_cmsdk_apb_irq_config_func_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, tx, irq),
		    DT_INST_IRQ_BY_NAME(0, tx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, tx, irq));

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, rx, irq),
		    DT_INST_IRQ_BY_NAME(0, rx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, rx, irq));
}
#endif
#endif

#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay) */

#if DT_NODE_HAS_STATUS(DT_DRV_INST(1), okay)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_1(const struct device *dev);
#endif

static const struct uart_cmsdk_apb_config uart_cmsdk_apb_dev_cfg_1 = {
	.uart = (volatile struct uart_cmsdk_apb *)DT_INST_REG_ADDR(1),
	.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(1, clocks, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_1,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_1 = {
	.baud_rate = DT_INST_PROP(1, current_speed),
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_REG_ADDR(1),},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_REG_ADDR(1),},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_REG_ADDR(1),},
};

DEVICE_DT_INST_DEFINE(1,
		    &uart_cmsdk_apb_init,
		    NULL,
		    &uart_cmsdk_apb_dev_data_1,
		    &uart_cmsdk_apb_dev_cfg_1, PRE_KERNEL_1,
		    CONFIG_SERIAL_INIT_PRIORITY,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#if DT_NUM_IRQS(DT_DRV_INST(1)) == 1
static void uart_cmsdk_apb_irq_config_func_1(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(1),
		    DT_INST_IRQ(1, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(1),
		    0);
	irq_enable(DT_INST_IRQN(1));
}
#else
static void uart_cmsdk_apb_irq_config_func_1(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(1, tx, irq),
		    DT_INST_IRQ_BY_NAME(1, tx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(1),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(1, tx, irq));

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(1, rx, irq),
		    DT_INST_IRQ_BY_NAME(1, rx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(1),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(1, rx, irq));
}
#endif
#endif

#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(1), okay) */

#if DT_NODE_HAS_STATUS(DT_DRV_INST(2), okay)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_2(const struct device *dev);
#endif

static const struct uart_cmsdk_apb_config uart_cmsdk_apb_dev_cfg_2 = {
	.uart = (volatile struct uart_cmsdk_apb *)DT_INST_REG_ADDR(2),
	.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(2, clocks, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_2,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_2 = {
	.baud_rate = DT_INST_PROP(2, current_speed),
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_REG_ADDR(2),},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_REG_ADDR(2),},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_REG_ADDR(2),},
};

DEVICE_DT_INST_DEFINE(2,
		    &uart_cmsdk_apb_init,
		    NULL,
		    &uart_cmsdk_apb_dev_data_2,
		    &uart_cmsdk_apb_dev_cfg_2, PRE_KERNEL_1,
		    CONFIG_SERIAL_INIT_PRIORITY,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#if DT_NUM_IRQS(DT_DRV_INST(2)) == 1
static void uart_cmsdk_apb_irq_config_func_2(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(2),
		    DT_INST_IRQ_BY_NAME(2, priority, irq),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(2),
		    0);
	irq_enable(DT_INST_IRQN(2));
}
#else
static void uart_cmsdk_apb_irq_config_func_2(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(2, tx, irq),
		    DT_INST_IRQ_BY_NAME(2, tx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(2),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(2, tx, irq));

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(2, rx, irq),
		    DT_INST_IRQ_BY_NAME(2, rx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(2),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(2, rx, irq));
}
#endif
#endif

#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(2), okay) */

#if DT_NODE_HAS_STATUS(DT_DRV_INST(3), okay)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_3(const struct device *dev);
#endif

static const struct uart_cmsdk_apb_config uart_cmsdk_apb_dev_cfg_3 = {
	.uart = (volatile struct uart_cmsdk_apb *)DT_INST_REG_ADDR(3),
	.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(3, clocks, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_3,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_3 = {
	.baud_rate = DT_INST_PROP(3, current_speed),
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_REG_ADDR(3),},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_REG_ADDR(3),},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_REG_ADDR(3),},
};

DEVICE_DT_INST_DEFINE(3,
		    &uart_cmsdk_apb_init,
		    NULL,
		    &uart_cmsdk_apb_dev_data_3,
		    &uart_cmsdk_apb_dev_cfg_3, PRE_KERNEL_1,
		    CONFIG_SERIAL_INIT_PRIORITY,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#if DT_NUM_IRQS(DT_DRV_INST(3)) == 1
static void uart_cmsdk_apb_irq_config_func_3(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(3),
		    DT_INST_IRQ(3, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(3),
		    0);
	irq_enable(DT_INST_IRQN(3));
}
#else
static void uart_cmsdk_apb_irq_config_func_3(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(3, tx, irq),
		    DT_INST_IRQ_BY_NAME(3, tx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(3),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(3, tx, irq));

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(3, rx, irq),
		    DT_INST_IRQ_BY_NAME(3, rx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(3),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(3, rx, irq));
}
#endif
#endif

#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(3), okay) */

#if DT_NODE_HAS_STATUS(DT_DRV_INST(4), okay)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_4(const struct device *dev);
#endif

static const struct uart_cmsdk_apb_config uart_cmsdk_apb_dev_cfg_4 = {
	.uart = (volatile struct uart_cmsdk_apb *)DT_INST_REG_ADDR(4),
	.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(4, clocks, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_4,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_4 = {
	.baud_rate = DT_INST_PROP(4, current_speed),
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_REG_ADDR(4),},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_REG_ADDR(4),},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_REG_ADDR(4),},
};

DEVICE_DT_INST_DEFINE(4,
		    &uart_cmsdk_apb_init,
		    NULL,
		    &uart_cmsdk_apb_dev_data_4,
		    &uart_cmsdk_apb_dev_cfg_4, PRE_KERNEL_1,
		    CONFIG_SERIAL_INIT_PRIORITY,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#if DT_NUM_IRQS(DT_DRV_INST(4)) == 1
static void uart_cmsdk_apb_irq_config_func_4(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(4),
		    DT_INST_IRQ_BY_NAME(4, priority, irq),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(4),
		    0);
	irq_enable(DT_INST_IRQN(4));
}
#else
static void uart_cmsdk_apb_irq_config_func_4(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(4, tx, irq),
		    DT_INST_IRQ_BY_NAME(4, tx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(4),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(4, tx, irq));

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(4, rx, irq),
		    DT_INST_IRQ_BY_NAME(4, rx, priority),
		    uart_cmsdk_apb_isr,
		    DEVICE_DT_INST_GET(4),
		    0);
	irq_enable(DT_INST_IRQ_BY_NAME(4, rx, irq));
}
#endif
#endif

#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(4), okay) */
