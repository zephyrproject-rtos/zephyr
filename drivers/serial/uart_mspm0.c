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
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/irq.h>
#include <soc.h>

/* Driverlib includes */
#include <ti/driverlib/dl_uart_main.h>

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_MSPM0_TX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_TX | DL_UART_MAIN_INTERRUPT_EOT_DONE)
#define UART_MSPM0_RX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_RX)
#endif

struct uart_mspm0_config {
	UART_Regs *regs;
	uint32_t clock_frequency;
	uint32_t current_speed;
	const struct mspm0_clockSys *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
};

struct uart_mspm0_data {
	/* UART clock structure */
	DL_UART_Main_ClockConfig clock_config;
	/* UART config structure */
	DL_UART_Main_Config config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uint32_t interruptState;          /* Masked Interrupt Status when called by irq_update */
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
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(clkmux));
	uint32_t clock_rate;
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
				    (DL_UART_Main_ClockConfig *)&data->clock_config);
	DL_UART_Main_init(config->regs, (DL_UART_Main_Config *)&data->config);

	/*
	 * Configure baud rate by setting oversampling and baud rate divisor
	 * from the device tree data current-speed
	 */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clock_subsys,
				     &clock_rate);

	if (ret < 0) {
		return ret;
	}

	DL_UART_Main_configBaudRate(config->regs, clock_rate, config->current_speed);

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

static int uart_mspm0_irq_rx_ready(const struct device *dev)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	return ((dev_data->interruptState & DL_UART_MAIN_INTERRUPT_RX) == DL_UART_MAIN_INTERRUPT_RX)
		       ? 1
		       : 0;
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
	struct uart_mspm0_data *const dev_data = dev->data;

	return (((dev_data->interruptState & DL_UART_MAIN_INTERRUPT_TX) ==
		 DL_UART_MAIN_INTERRUPT_TX) ||
		DL_UART_Main_isTXFIFOEmpty(config->regs))
		       ? 1
		       : 0;
}

static int uart_mspm0_irq_tx_complete(const struct device *dev)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	return ((dev_data->interruptState & DL_UART_MAIN_INTERRUPT_EOT_DONE) ==
		DL_UART_MAIN_INTERRUPT_EOT_DONE)
		       ? 1
		       : 0;
}

static int uart_mspm0_irq_is_pending(const struct device *dev)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	return ((dev_data->interruptState != 0) ? 1 : 0);
}

static int uart_mspm0_irq_update(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *const dev_data = dev->data;

	dev_data->interruptState = DL_UART_Main_getEnabledInterruptStatus(
		config->regs, UART_MSPM0_RX_INTERRUPTS | UART_MSPM0_TX_INTERRUPTS);

	/*
	 * Clear interrupts explicitly after storing all in the update. Interrupts
	 * can be re-set by the MIS during the ISR should they be available.
	 */

	DL_UART_Main_clearInterruptStatus(config->regs, dev_data->interruptState);

	return 1;
}

static void uart_mspm0_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
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
	uint32_t int_status;

	/* Perform callback if defined */
	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	} else {
		/* error, callback necessary in order to make progress. Clear interrupts
		 * temporarily.
		 */
		int_status = DL_UART_Main_getEnabledInterruptStatus(
			config->regs, UART_MSPM0_TX_INTERRUPTS | UART_MSPM0_RX_INTERRUPTS);
		DL_UART_Main_clearInterruptStatus(config->regs, int_status);
	}
}

#define MSP_UART_IRQ_REGISTER(index)                                                               \
	static void uart_mspm0_##index##_irq_register(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), uart_mspm0_isr,     \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
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

#define MSPM0_UART_INIT_FN(index)                                                                  \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct mspm0_clockSys mspm0_uart_clockSys##index =                            \
		MSPM0_CLOCK_SUBSYS_FN(index);                                                      \
                                                                                                   \
	MSP_UART_IRQ_REGISTER(index)                                                               \
                                                                                                   \
	static const struct uart_mspm0_config uart_mspm0_cfg_##index = {                           \
		.regs = (UART_Regs *)DT_INST_REG_ADDR(index),                                      \
		.current_speed = DT_INST_PROP(index, current_speed),                               \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                  \
		.clock_subsys = &mspm0_uart_clockSys##index,                                       \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                               \
		(.irq_config_func = uart_mspm0_##index##_irq_register,)) };               \
                                                                                                   \
	static struct uart_mspm0_data uart_mspm0_data_##index = {                                  \
		.clock_config = {.clockSel = (DT_INST_CLOCKS_CELL(index, bus) &                \
						  MSPM0_CLOCK_SEL_MASK),                           \
				     .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1},            \
		.config =                                                                     \
			{                                                                          \
				.mode = DL_UART_MAIN_MODE_NORMAL,                                  \
				.direction = DL_UART_MAIN_DIRECTION_TX_RX,                         \
				.flowControl = (DT_INST_PROP(index, hw_flow_control)               \
							? DL_UART_MAIN_FLOW_CONTROL_RTS_CTS        \
							: DL_UART_MAIN_FLOW_CONTROL_NONE),         \
				.parity = DL_UART_MAIN_PARITY_NONE,                                \
				.wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,                     \
				.stopBits = DL_UART_MAIN_STOP_BITS_ONE,                            \
			},                                                                         \
		MSPM0_INTERRUPT_CALLBACK_FN(index)};                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &uart_mspm0_init, NULL, &uart_mspm0_data_##index,             \
			      &uart_mspm0_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &uart_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_UART_INIT_FN)
