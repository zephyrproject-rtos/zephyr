/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_uart

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <soc.h>

/* Defines for UART0 */
#define UART_0_IBRD_33_kHZ_9600_BAUD (1)
#define UART_0_FBRD_33_kHZ_9600_BAUD (9)

/* Driverlib includes */
#include <ti/driverlib/dl_uart_main.h>

struct uart_mspm0_config {
	UART_Regs *regs;
	uint32_t clock_frequency;
	uint32_t current_speed;
	const struct pinctrl_dev_config *pinctrl;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
};

struct uart_mspm0_data {
	/* UART clock structure */
	DL_UART_Main_ClockConfig UART_ClockConfig;
	/* UART config structure */
	DL_UART_Main_Config UART_Config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /* Callback function pointer */
	void *cb_data;                    /* Callback function arg */
#endif                                    /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mspm0_isr(const struct device *dev);
#define MSPM0_INTERRUPT_CALLBACK_FN(index) .cb = NULL,
#else
#define MSPM0_INTERRUPT_CALLBACK_FN(index)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_mspm0_init(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	int ret;

	/* Reset power */
	DL_UART_Main_reset(config->regs);
	DL_UART_Main_enablePower(config->regs);
	delay_cycles(POWER_STARTUP_DELAY);

	/* Init UART pins */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Set UART configs */
	DL_UART_Main_setClockConfig(config->regs,
				    (DL_UART_Main_ClockConfig *)&data->UART_ClockConfig);
	DL_UART_Main_init(config->regs, (DL_UART_Main_Config *)&data->UART_Config);

	/*
	 * Configure baud rate by setting oversampling and baud rate divisor
	 * from the device tree data current-speed
	 */
	DL_UART_Main_configBaudRate(config->regs, 32000000, config->current_speed);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	/* Enable UART */
	DL_UART_Main_enable(config->regs);

	return 0;
}

static int uart_mspm0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_mspm0_config *config = dev->config;

	return (DL_UART_Main_receiveDataCheck(config->regs, c)) ? 0 : -1;
}

static void uart_mspm0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_transmitDataBlocking(config->regs, c);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define UART_MSPM0_TX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_TX | DL_UART_MAIN_INTERRUPT_EOT_DONE)
#define UART_MSPM0_RX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_RX)

static int uart_mspm0_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_mspm0_config *config = dev->config;

	return (int)DL_UART_Main_fillTXFIFO(config->regs, (uint8_t *)tx_data, size);
}

static int uart_mspm0_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_mspm0_config *config = dev->config;

	return (int)DL_UART_Main_drainRXFIFO(config->regs, rx_data, size);
}

static void uart_mspm0_irq_tx_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_enableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);
}

static void uart_mspm0_irq_tx_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);
}

static int uart_mspm0_irq_tx_ready(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	return (DL_UART_Main_getEnabledInterruptStatus(
		config->regs, DL_UART_MAIN_INTERRUPT_TX)) ? 0 : 1;
}

static void uart_mspm0_irq_rx_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_enableInterrupt(config->regs, UART_MSPM0_RX_INTERRUPTS);
}

static void uart_mspm0_irq_rx_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_RX_INTERRUPTS);
}

static int uart_mspm0_irq_tx_complete(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	return (DL_UART_Main_isTXFIFOEmpty(config->regs)) ? 1 : 0;
}

static int uart_mspm0_irq_rx_ready(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	return (DL_UART_Main_getEnabledInterruptStatus(
		config->regs, UART_MSPM0_RX_INTERRUPTS)) ? 1 : 0;
}

static int uart_mspm0_irq_is_pending(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	return (DL_UART_Main_getEnabledInterruptStatus(config->regs,
		UART_MSPM0_RX_INTERRUPTS | UART_MSPM0_TX_INTERRUPTS)) ? 1 : 0;
}

static int uart_mspm0_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void uart_mspm0_irq_callback_set(const struct device *dev,
					     uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	/* Set callback function and data */
	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 */
static void uart_mspm0_isr(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *const dev_data = dev->data;

	int int_status = DL_UART_Main_getEnabledInterruptStatus(config->regs,
		UART_MSPM0_RX_INTERRUPTS | UART_MSPM0_TX_INTERRUPTS);

	/* Perform callback if defined */
	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}

	/*
	 * Clear interrupts only after cb called, as Zephyr UART clients expect
	 * to check interrupt status during the callback.
	 */

	DL_UART_Main_clearInterruptStatus(config->regs, int_status);

}

#define MSP_UART_IRQ_REGISTER(index)	\
static void uart_mspm0_##index##_irq_register(const struct device *dev)	\
{\
	IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),	\
		uart_mspm0_isr, DEVICE_DT_INST_GET(index), 0);	\
	irq_enable(DT_INST_IRQN(index));	\
}

#else

#define MSP_UART_IRQ_REGISTER(index)

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_mspm0_driver_api = {
	.poll_in = uart_mspm0_poll_in,
	.poll_out = uart_mspm0_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_mspm0_fifo_fill,
	.fifo_read = uart_mspm0_fifo_read,
	.irq_tx_enable = uart_mspm0_irq_tx_enable,
	.irq_tx_disable = uart_mspm0_irq_tx_disable,
	.irq_tx_ready = uart_mspm0_irq_tx_ready,
	.irq_rx_enable = uart_mspm0_irq_rx_enable,
	.irq_rx_disable = uart_mspm0_irq_rx_disable,
	.irq_tx_complete = uart_mspm0_irq_tx_complete,
	.irq_rx_ready = uart_mspm0_irq_rx_ready,
	.irq_is_pending = uart_mspm0_irq_is_pending,
	.irq_update = uart_mspm0_irq_update,
	.irq_callback_set = uart_mspm0_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define MSPM0_UART_INIT_FN(index)		\
									\
	PINCTRL_DT_INST_DEFINE(index);	\
	\
	MSP_UART_IRQ_REGISTER(index)	\
	\
	static const struct uart_mspm0_config uart_mspm0_cfg_##index = {	\
		.regs = (UART_Regs *)DT_INST_REG_ADDR(index),		\
		.current_speed = DT_INST_PROP(index, current_speed),\
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),	\
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,	\
		(.irq_config_func = uart_mspm0_##index##_irq_register,)) \
	};								\
									\
	static struct uart_mspm0_data uart_mspm0_data_##index = {	\
		.UART_ClockConfig = {		\
			.clockSel = DL_UART_MAIN_CLOCK_BUSCLK,				\
			.divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1	\
		},							\
		.UART_Config =				\
			{						\
				.mode = DL_UART_MAIN_MODE_NORMAL,				\
				.direction = DL_UART_MAIN_DIRECTION_TX_RX,		\
				.flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,	\
				.parity = DL_UART_MAIN_PARITY_NONE,				\
				.wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,	\
				.stopBits = DL_UART_MAIN_STOP_BITS_ONE,			\
			},								\
		MSPM0_INTERRUPT_CALLBACK_FN(index)	\
		};									\
											\
	DEVICE_DT_INST_DEFINE(index, &uart_mspm0_init, NULL,			\
				&uart_mspm0_data_##index, &uart_mspm0_cfg_##index,	\
				PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,			\
				&uart_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_UART_INIT_FN)
