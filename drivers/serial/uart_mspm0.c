/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz
 * Copyright (c) 2025 Bang & Olufsen A/S, Denmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_uart

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

/* Driverlib includes */
#include <ti/driverlib/dl_uart_main.h>

struct uart_mspm0_config {
	UART_Regs *regs;
	uint32_t current_speed;
	const struct mspm0_sys_clock *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct uart_mspm0_data {
	/* UART clock structure */
	DL_UART_Main_ClockConfig uart_clockconfig;
	/* UART config structure */
	DL_UART_Main_Config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Callback function pointer */
	uart_irq_callback_user_data_t cb;
	/* Callback function arg */
	void *cb_data;
	/* Pending interrupt backup */
	DL_UART_IIDX pending_interrupt;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_mspm0_init(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	uint32_t clock_rate;
	int ret;

	/* Reset power */
	DL_UART_Main_reset(config->regs);
	DL_UART_Main_enablePower(config->regs);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	/* Init UART pins */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Set UART configs */
	DL_UART_Main_setClockConfig(config->regs,
				    &data->uart_clockconfig);
	DL_UART_Main_init(config->regs, &data->uart_config);

	/*
	 * Configure baud rate by setting oversampling and baud rate divisor
	 * from the device tree data current-speed
	 */
	ret = clock_control_get_rate(clk_dev,
				(struct mspm0_sys_clock *)config->clock_subsys,
				&clock_rate);
	if (ret < 0) {
		return ret;
	}

	DL_UART_Main_configBaudRate(config->regs,
				    clock_rate,
				    config->current_speed);

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

	if (DL_UART_Main_receiveDataCheck(config->regs, c) == false) {
		return -1;
	}

	return 0;
}

static void uart_mspm0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_transmitDataBlocking(config->regs, c);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_mspm0_err_check(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;

	switch (data->pending_interrupt) {
	case DL_UART_MAIN_IIDX_BREAK_ERROR:
		return UART_BREAK;
	case DL_UART_MAIN_IIDX_FRAMING_ERROR:
		return UART_ERROR_FRAMING;
	default:
		return 0;
	}
}

#define UART_MSPM0_TX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_TX |		\
				  DL_UART_MAIN_INTERRUPT_EOT_DONE)
#define UART_MSPM0_RX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_RX)

static int uart_mspm0_fifo_fill(const struct device *dev,
				const uint8_t *tx_data, int size)
{
	const struct uart_mspm0_config *config = dev->config;

	return (int)DL_UART_Main_fillTXFIFO(config->regs,
					    (uint8_t *)tx_data, size);
}

static int uart_mspm0_fifo_read(const struct device *dev,
				uint8_t *rx_data, const int size)
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
	struct uart_mspm0_data *data = dev->data;

	return (data->pending_interrupt &
		(DL_UART_MAIN_IIDX_TX | DL_UART_MAIN_IIDX_EOT_DONE))
		&& !DL_UART_Main_isTXFIFOFull(config->regs) ? 1 : 0;
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
	struct uart_mspm0_data *data = dev->data;

	return (data->pending_interrupt & DL_UART_MAIN_IIDX_RX) &&
		!DL_UART_Main_isRXFIFOEmpty(config->regs) ? 1 : 0;
}

static int uart_mspm0_irq_is_pending(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;

	return data->pending_interrupt != DL_UART_MAIN_IIDX_NO_INTERRUPT;
}

static int uart_mspm0_irq_update(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;
	const struct uart_mspm0_config *config = dev->config;

	data->pending_interrupt =
		DL_UART_Main_getPendingInterrupt(config->regs);

	return 1;
}

static void uart_mspm0_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	/* Set callback function and data */
	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

#define UART_MSPM0_ERROR_INTERRUPTS		\
	(DL_UART_MAIN_INTERRUPT_BREAK_ERROR |	\
	 DL_UART_MAIN_INTERRUPT_FRAMING_ERROR)

static void uart_mspm0_irq_error_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_enableInterrupt(config->regs, UART_MSPM0_ERROR_INTERRUPTS);
}

static void uart_mspm0_irq_error_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_ERROR_INTERRUPTS);
}

static void uart_mspm0_isr(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *const dev_data = dev->data;
	uint32_t int_status;

	/* Perform callback if defined */
	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}

	/* Unilaterally clearing the interrupt status */
	int_status = DL_UART_Main_getEnabledInterruptStatus(config->regs,
				UART_MSPM0_TX_INTERRUPTS | UART_MSPM0_RX_INTERRUPTS);
	DL_UART_Main_clearInterruptStatus(config->regs, int_status);

	dev_data->pending_interrupt = DL_UART_MAIN_IIDX_NO_INTERRUPT;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_mspm0_driver_api) = {
	.poll_in = uart_mspm0_poll_in,
	.poll_out = uart_mspm0_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.err_check = uart_mspm0_err_check,
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
	.irq_err_enable = uart_mspm0_irq_error_enable,
	.irq_err_disable = uart_mspm0_irq_error_disable,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define MSP_UART_IRQ_DEFINE(inst)                                                               \
	static void uart_mspm0_##inst##_irq_register(const struct device *dev)                  \
	{                                                                                       \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), uart_mspm0_isr,    \
			    DEVICE_DT_INST_GET(inst), 0);                                       \
		irq_enable(DT_INST_IRQN(inst));                                                 \
	}
#else
#define MSP_UART_IRQ_DEFINE(inst)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define MSPM0_MAIN_CLK_DIV(n)  CONCAT(DL_UART_MAIN_CLOCK_DIVIDE_RATIO_, DT_INST_PROP(n, clk_div))

#define MSPM0_UART_INIT_FN(index)								\
												\
	PINCTRL_DT_INST_DEFINE(index);								\
												\
	static const struct mspm0_sys_clock mspm0_uart_sys_clock##index =			\
		MSPM0_CLOCK_SUBSYS_FN(index);							\
												\
	MSP_UART_IRQ_DEFINE(index);								\
												\
	static const struct uart_mspm0_config uart_mspm0_cfg_##index = {			\
		.regs = (UART_Regs *)DT_INST_REG_ADDR(index),					\
		.current_speed = DT_INST_PROP(index, current_speed),				\
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),				\
		.clock_subsys = &mspm0_uart_sys_clock##index,					\
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,					\
			   (.irq_config_func = uart_mspm0_##index##_irq_register,))		\
		};										\
												\
	static struct uart_mspm0_data uart_mspm0_data_##index = {				\
		.uart_clockconfig = {								\
			.clockSel = MSPM0_CLOCK_PERIPH_REG_MASK(DT_INST_CLOCKS_CELL(index, clk)), \
			.divideRatio = MSPM0_MAIN_CLK_DIV(index),				\
		 },										\
		.uart_config = {.mode = DL_UART_MAIN_MODE_NORMAL,				\
				.direction = DL_UART_MAIN_DIRECTION_TX_RX,			\
				.flowControl = (DT_INST_PROP(index, hw_flow_control)		\
						? DL_UART_MAIN_FLOW_CONTROL_RTS_CTS		\
						: DL_UART_MAIN_FLOW_CONTROL_NONE),		\
				.parity = DL_UART_MAIN_PARITY_NONE,				\
				.wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,			\
				.stopBits = DL_UART_MAIN_STOP_BITS_ONE,				\
				},								\
		};										\
												\
	DEVICE_DT_INST_DEFINE(index, &uart_mspm0_init, NULL, &uart_mspm0_data_##index,		\
			      &uart_mspm0_cfg_##index, PRE_KERNEL_1,				\
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_UART_INIT_FN)
