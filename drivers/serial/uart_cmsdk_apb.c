/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for UART on ARM CMSDK APB UART.
 *
 * UART has two wires for RX and TX, and does not provide CTS or RTS.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <clock_control/arm_clock_control.h>
#include <sys/__assert.h>
#include <init.h>
#include <drivers/uart.h>
#include <linker/sections.h>

/* UART registers struct */
struct uart_cmsdk_apb {
	/* offset: 0x000 (r/w) data register    */
	volatile u32_t  data;
	/* offset: 0x004 (r/w) status register  */
	volatile u32_t  state;
	/* offset: 0x008 (r/w) control register */
	volatile u32_t  ctrl;
	union {
		/* offset: 0x00c (r/ ) interrupt status register */
		volatile u32_t  intstatus;
		/* offset: 0x00c ( /w) interrupt clear register  */
		volatile u32_t  intclear;
	};
	/* offset: 0x010 (r/w) baudrate divider register */
	volatile u32_t  bauddiv;
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

/* Device data structure */
struct uart_cmsdk_apb_dev_data {
	u32_t baud_rate;	/* Baud rate */
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

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_cmsdk_apb_dev_data * const)(dev)->driver_data)
#define UART_STRUCT(dev) \
	((volatile struct uart_cmsdk_apb *)(DEV_CFG(dev))->base)

static const struct uart_driver_api uart_cmsdk_apb_driver_api;
static void uart_cmsdk_apb_isr(void *arg);

/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UART.
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void baudrate_set(struct device *dev)
{
	volatile struct uart_cmsdk_apb *uart = UART_STRUCT(dev);
	const struct uart_device_config * const dev_cfg = DEV_CFG(dev);
	struct uart_cmsdk_apb_dev_data *const dev_data = DEV_DATA(dev);
	/*
	 * If baudrate and/or sys_clk_freq are 0 the configuration remains
	 * unchanged. It can be useful in case that Zephyr it is run via
	 * a bootloader that brings up the serial and sets the baudrate.
	 */
	if ((dev_data->baud_rate != 0U) && (dev_cfg->sys_clk_freq != 0U)) {
		/* calculate baud rate divisor */
		uart->bauddiv = (dev_cfg->sys_clk_freq / dev_data->baud_rate);
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
static int uart_cmsdk_apb_init(struct device *dev)
{
	volatile struct uart_cmsdk_apb *uart = UART_STRUCT(dev);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_device_config * const dev_cfg = DEV_CFG(dev);
#endif

#ifdef CONFIG_CLOCK_CONTROL
	/* Enable clock for subsystem */
	struct device *clk =
		device_get_binding(CONFIG_ARM_CLOCK_CONTROL_DEV_NAME);

	struct uart_cmsdk_apb_dev_data * const data = DEV_DATA(dev);

#ifdef CONFIG_SOC_SERIES_BEETLE
	clock_control_on(clk, (clock_control_subsys_t *) &data->uart_cc_as);
	clock_control_on(clk, (clock_control_subsys_t *) &data->uart_cc_ss);
	clock_control_on(clk, (clock_control_subsys_t *) &data->uart_cc_dss);
#endif /* CONFIG_SOC_SERIES_BEETLE */
#endif /* CONFIG_CLOCK_CONTROL */

	/* Set baud rate */
	baudrate_set(dev);

	/* Enable receiver and transmitter */
	uart->ctrl = UART_RX_EN | UART_TX_EN;

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

static int uart_cmsdk_apb_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uart_cmsdk_apb *uart = UART_STRUCT(dev);

	/* If the receiver is not ready returns -1 */
	if (!(uart->state & UART_RX_BF)) {
		return -1;
	}

	/* got a character */
	*c = (unsigned char)uart->data;

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
static void uart_cmsdk_apb_poll_out(struct device *dev,
					     unsigned char c)
{
	volatile struct uart_cmsdk_apb *uart = UART_STRUCT(dev);

	/* Wait for transmitter to be ready */
	while (uart->state & UART_TX_BF) {
		; /* Wait */
	}

	/* Send a character */
	uart->data = (u32_t)c;
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
static int uart_cmsdk_apb_fifo_fill(struct device *dev,
				    const u8_t *tx_data, int len)
{
	volatile struct uart_cmsdk_apb *uart = UART_STRUCT(dev);

	/* No hardware FIFO present */
	if (len && !(uart->state & UART_TX_BF)) {
		uart->data = *tx_data;
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
static int uart_cmsdk_apb_fifo_read(struct device *dev,
				    u8_t *rx_data, const int size)
{
	volatile struct uart_cmsdk_apb *uart = UART_STRUCT(dev);

	/* No hardware FIFO present */
	if (size && uart->state & UART_RX_BF) {
		*rx_data = (unsigned char)uart->data;
		return 1;
	}

	return 0;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_cmsdk_apb_irq_tx_enable(struct device *dev)
{
	unsigned int key;

	UART_STRUCT(dev)->ctrl |= UART_TX_IN_EN;
	/* The expectation is that TX is a level interrupt, active for as
	 * long as TX buffer is empty. But in CMSDK UART, it appears to be
	 * edge interrupt, firing on a state change of TX buffer. So, we
	 * need to "prime" it here by calling ISR directly, to get interrupt
	 * processing going.
	 */
	key = irq_lock();
	uart_cmsdk_apb_isr(dev);
	irq_unlock(key);
}

/**
 * @brief Disable TX interrupt
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_cmsdk_apb_irq_tx_disable(struct device *dev)
{
	UART_STRUCT(dev)->ctrl &= ~UART_TX_IN_EN;
}

/**
 * @brief Verify if Tx interrupt has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an interrupt is ready, 0 otherwise
 */
static int uart_cmsdk_apb_irq_tx_ready(struct device *dev)
{
	return !(UART_STRUCT(dev)->state & UART_TX_BF);
}

/**
 * @brief Enable RX interrupt
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_cmsdk_apb_irq_rx_enable(struct device *dev)
{
	UART_STRUCT(dev)->ctrl |= UART_RX_IN_EN;
}

/**
 * @brief Disable RX interrupt
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_cmsdk_apb_irq_rx_disable(struct device *dev)
{
	UART_STRUCT(dev)->ctrl &= ~UART_RX_IN_EN;
}

/**
 * @brief Verify if Tx complete interrupt has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an interrupt is ready, 0 otherwise
 */
static int uart_cmsdk_apb_irq_tx_complete(struct device *dev)
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
static int uart_cmsdk_apb_irq_rx_ready(struct device *dev)
{
	return UART_STRUCT(dev)->state & UART_RX_BF;
}

/**
 * @brief Enable error interrupt
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_cmsdk_apb_irq_err_enable(struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Disable error interrupt
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_cmsdk_apb_irq_err_disable(struct device *dev)
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
static int uart_cmsdk_apb_irq_is_pending(struct device *dev)
{
	/* Return true if rx buffer full or tx buffer empty */
	return (UART_STRUCT(dev)->state & (UART_RX_BF | UART_TX_BF))
					!= UART_TX_BF;
}

/**
 * @brief Update the interrupt status
 *
 * @param dev UART device struct
 *
 * @return always 1
 */
static int uart_cmsdk_apb_irq_update(struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for an Interrupt.
 *
 * @param dev UART device structure
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void uart_cmsdk_apb_irq_callback_set(struct device *dev,
					    uart_irq_callback_user_data_t cb,
					    void *cb_data)
{
	DEV_DATA(dev)->irq_cb = cb;
	DEV_DATA(dev)->irq_cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * Calls the callback function, if exists.
 *
 * @param arg argument to interrupt service routine.
 *
 * @return N/A
 */
void uart_cmsdk_apb_isr(void *arg)
{
	struct device *dev = arg;
	volatile struct uart_cmsdk_apb *uart = UART_STRUCT(dev);
	struct uart_cmsdk_apb_dev_data *data = DEV_DATA(dev);

	/* Clear pending interrupts */
	uart->intclear = UART_RX_IN | UART_TX_IN;

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(data->irq_cb_data);
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

#ifdef DT_INST_0_ARM_CMSDK_UART

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_0(struct device *dev);
#endif

static const struct uart_device_config uart_cmsdk_apb_dev_cfg_0 = {
	.base = (u8_t *)DT_INST_0_ARM_CMSDK_UART_BASE_ADDRESS,
	.sys_clk_freq = DT_INST_0_ARM_CMSDK_UART_CLOCKS_CLOCK_FREQUENCY,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_0,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_0 = {
	.baud_rate = DT_INST_0_ARM_CMSDK_UART_CURRENT_SPEED,
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_0_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_0_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_0_ARM_CMSDK_UART_BASE_ADDRESS,},
};

DEVICE_AND_API_INIT(uart_cmsdk_apb_0,
		    DT_INST_0_ARM_CMSDK_UART_LABEL,
		    &uart_cmsdk_apb_init,
		    &uart_cmsdk_apb_dev_data_0,
		    &uart_cmsdk_apb_dev_cfg_0, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#ifdef DT_INST_0_ARM_CMSDK_UART_IRQ_0
static void uart_cmsdk_apb_irq_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_INST_0_ARM_CMSDK_UART_IRQ_0,
		    DT_INST_0_ARM_CMSDK_UART_IRQ_0_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_0),
		    0);
	irq_enable(DT_INST_0_ARM_CMSDK_UART_IRQ_0);
}
#else
static void uart_cmsdk_apb_irq_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_INST_0_ARM_CMSDK_UART_IRQ_TX,
		    DT_INST_0_ARM_CMSDK_UART_IRQ_TX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_0),
		    0);
	irq_enable(DT_INST_0_ARM_CMSDK_UART_IRQ_TX);

	IRQ_CONNECT(DT_INST_0_ARM_CMSDK_UART_IRQ_RX,
		    DT_INST_0_ARM_CMSDK_UART_IRQ_RX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_0),
		    0);
	irq_enable(DT_INST_0_ARM_CMSDK_UART_IRQ_RX);
}
#endif
#endif

#endif /* DT_INST_0_ARM_CMSDK_UART */

#ifdef DT_INST_1_ARM_CMSDK_UART

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_1(struct device *dev);
#endif

static const struct uart_device_config uart_cmsdk_apb_dev_cfg_1 = {
	.base = (u8_t *)DT_INST_1_ARM_CMSDK_UART_BASE_ADDRESS,
	.sys_clk_freq = DT_INST_1_ARM_CMSDK_UART_CLOCKS_CLOCK_FREQUENCY,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_1,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_1 = {
	.baud_rate = DT_INST_1_ARM_CMSDK_UART_CURRENT_SPEED,
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_1_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_1_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_1_ARM_CMSDK_UART_BASE_ADDRESS,},
};

DEVICE_AND_API_INIT(uart_cmsdk_apb_1,
		    DT_INST_1_ARM_CMSDK_UART_LABEL,
		    &uart_cmsdk_apb_init,
		    &uart_cmsdk_apb_dev_data_1,
		    &uart_cmsdk_apb_dev_cfg_1, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#ifdef DT_INST_1_ARM_CMSDK_UART_IRQ_0
static void uart_cmsdk_apb_irq_config_func_1(struct device *dev)
{
	IRQ_CONNECT(DT_INST_1_ARM_CMSDK_UART_IRQ_0,
		    DT_INST_1_ARM_CMSDK_UART_IRQ_0_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_1),
		    0);
	irq_enable(DT_INST_1_ARM_CMSDK_UART_IRQ_0);
}
#else
static void uart_cmsdk_apb_irq_config_func_1(struct device *dev)
{
	IRQ_CONNECT(DT_INST_1_ARM_CMSDK_UART_IRQ_TX,
		    DT_INST_1_ARM_CMSDK_UART_IRQ_TX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_1),
		    0);
	irq_enable(DT_INST_1_ARM_CMSDK_UART_IRQ_TX);

	IRQ_CONNECT(DT_INST_1_ARM_CMSDK_UART_IRQ_RX,
		    DT_INST_1_ARM_CMSDK_UART_IRQ_RX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_1),
		    0);
	irq_enable(DT_INST_1_ARM_CMSDK_UART_IRQ_RX);
}
#endif
#endif

#endif /* DT_INST_1_ARM_CMSDK_UART */

#ifdef DT_INST_2_ARM_CMSDK_UART

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_2(struct device *dev);
#endif

static const struct uart_device_config uart_cmsdk_apb_dev_cfg_2 = {
	.base = (u8_t *)DT_INST_2_ARM_CMSDK_UART_BASE_ADDRESS,
	.sys_clk_freq = DT_INST_2_ARM_CMSDK_UART_CLOCKS_CLOCK_FREQUENCY,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_2,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_2 = {
	.baud_rate = DT_INST_2_ARM_CMSDK_UART_CURRENT_SPEED,
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_2_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_2_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_2_ARM_CMSDK_UART_BASE_ADDRESS,},
};

DEVICE_AND_API_INIT(uart_cmsdk_apb_2,
		    DT_INST_2_ARM_CMSDK_UART_LABEL,
		    &uart_cmsdk_apb_init,
		    &uart_cmsdk_apb_dev_data_2,
		    &uart_cmsdk_apb_dev_cfg_2, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#ifdef CMSDK_APB_UART_2_IRQ
static void uart_cmsdk_apb_irq_config_func_2(struct device *dev)
{
	IRQ_CONNECT(CMSDK_APB_UART_2_IRQ,
		    DT_INST_2_ARM_CMSDK_UART_IRQ_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_2),
		    0);
	irq_enable(CMSDK_APB_UART_2_IRQ);
}
#else
static void uart_cmsdk_apb_irq_config_func_2(struct device *dev)
{
	IRQ_CONNECT(DT_INST_2_ARM_CMSDK_UART_IRQ_TX,
		    DT_INST_2_ARM_CMSDK_UART_IRQ_TX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_2),
		    0);
	irq_enable(DT_INST_2_ARM_CMSDK_UART_IRQ_TX);

	IRQ_CONNECT(DT_INST_2_ARM_CMSDK_UART_IRQ_RX,
		    DT_INST_2_ARM_CMSDK_UART_IRQ_RX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_2),
		    0);
	irq_enable(DT_INST_2_ARM_CMSDK_UART_IRQ_RX);
}
#endif
#endif

#endif /* DT_INST_2_ARM_CMSDK_UART */

#ifdef DT_INST_3_ARM_CMSDK_UART

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_3(struct device *dev);
#endif

static const struct uart_device_config uart_cmsdk_apb_dev_cfg_3 = {
	.base = (u8_t *)DT_INST_3_ARM_CMSDK_UART_BASE_ADDRESS,
	.sys_clk_freq = DT_INST_3_ARM_CMSDK_UART_CLOCKS_CLOCK_FREQUENCY,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_3,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_3 = {
	.baud_rate = DT_INST_3_ARM_CMSDK_UART_CURRENT_SPEED,
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_3_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_3_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_3_ARM_CMSDK_UART_BASE_ADDRESS,},
};

DEVICE_AND_API_INIT(uart_cmsdk_apb_3,
		    DT_INST_3_ARM_CMSDK_UART_LABEL,
		    &uart_cmsdk_apb_init,
		    &uart_cmsdk_apb_dev_data_3,
		    &uart_cmsdk_apb_dev_cfg_3, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#ifdef CMSDK_APB_UART_3_IRQ
static void uart_cmsdk_apb_irq_config_func_3(struct device *dev)
{
	IRQ_CONNECT(CMSDK_APB_UART_3_IRQ,
		    DT_INST_3_ARM_CMSDK_UART_IRQ_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_3),
		    0);
	irq_enable(CMSDK_APB_UART_3_IRQ);
}
#else
static void uart_cmsdk_apb_irq_config_func_3(struct device *dev)
{
	IRQ_CONNECT(DT_INST_3_ARM_CMSDK_UART_IRQ_TX,
		    DT_INST_3_ARM_CMSDK_UART_IRQ_TX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_3),
		    0);
	irq_enable(DT_INST_3_ARM_CMSDK_UART_IRQ_TX);

	IRQ_CONNECT(DT_INST_3_ARM_CMSDK_UART_IRQ_RX,
		    DT_INST_3_ARM_CMSDK_UART_IRQ_RX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_3),
		    0);
	irq_enable(DT_INST_3_ARM_CMSDK_UART_IRQ_RX);
}
#endif
#endif

#endif /* DT_INST_3_ARM_CMSDK_UART */

#ifdef DT_INST_4_ARM_CMSDK_UART

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cmsdk_apb_irq_config_func_4(struct device *dev);
#endif

static const struct uart_device_config uart_cmsdk_apb_dev_cfg_4 = {
	.base = (u8_t *)DT_INST_4_ARM_CMSDK_UART_BASE_ADDRESS,
	.sys_clk_freq = DT_INST_4_ARM_CMSDK_UART_CLOCKS_CLOCK_FREQUENCY,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_cmsdk_apb_irq_config_func_4,
#endif
};

static struct uart_cmsdk_apb_dev_data uart_cmsdk_apb_dev_data_4 = {
	.baud_rate = DT_INST_4_ARM_CMSDK_UART_CURRENT_SPEED,
	.uart_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
		       .device = DT_INST_4_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
		       .device = DT_INST_4_ARM_CMSDK_UART_BASE_ADDRESS,},
	.uart_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			.device = DT_INST_4_ARM_CMSDK_UART_BASE_ADDRESS,},
};

DEVICE_AND_API_INIT(uart_cmsdk_apb_4,
		    DT_INST_4_ARM_CMSDK_UART_LABEL,
		    &uart_cmsdk_apb_init,
		    &uart_cmsdk_apb_dev_data_4,
		    &uart_cmsdk_apb_dev_cfg_4, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_cmsdk_apb_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#ifdef CMSDK_APB_UART_4_IRQ
static void uart_cmsdk_apb_irq_config_func_4(struct device *dev)
{
	IRQ_CONNECT(CMSDK_APB_UART_4_IRQ,
		    DT_INST_4_ARM_CMSDK_UART_IRQ_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_4),
		    0);
	irq_enable(CMSDK_APB_UART_4_IRQ);
}
#else
static void uart_cmsdk_apb_irq_config_func_4(struct device *dev)
{
	IRQ_CONNECT(DT_INST_4_ARM_CMSDK_UART_IRQ_TX,
		    DT_INST_4_ARM_CMSDK_UART_IRQ_TX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_4),
		    0);
	irq_enable(DT_INST_4_ARM_CMSDK_UART_IRQ_TX);

	IRQ_CONNECT(DT_INST_4_ARM_CMSDK_UART_IRQ_RX,
		    DT_INST_4_ARM_CMSDK_UART_IRQ_RX_PRIORITY,
		    uart_cmsdk_apb_isr,
		    DEVICE_GET(uart_cmsdk_apb_4),
		    0);
	irq_enable(DT_INST_4_ARM_CMSDK_UART_IRQ_RX);
}
#endif
#endif

#endif /* DT_INST_4_ARM_CMSDK_UART */
