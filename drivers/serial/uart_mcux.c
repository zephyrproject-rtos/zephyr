/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
#include <fsl_uart.h>
#include <soc.h>

struct uart_mcux_config {
	UART_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	u32_t baud_rate;
	u8_t hw_flow_control;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

struct uart_mcux_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int uart_mcux_poll_in(struct device *dev, unsigned char *c)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t flags = UART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kUART_RxDataRegFullFlag) {
		*c = UART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void uart_mcux_poll_out(struct device *dev, unsigned char c)
{
	const struct uart_mcux_config *config = dev->config->config_info;

	while (!(UART_GetStatusFlags(config->base) & kUART_TxDataRegEmptyFlag)) {
	}

	UART_WriteByte(config->base, c);
}

static int uart_mcux_err_check(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t flags = UART_GetStatusFlags(config->base);
	int err = 0;

	if (flags & kUART_RxOverrunFlag) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & kUART_ParityErrorFlag) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & kUART_FramingErrorFlag) {
		err |= UART_ERROR_FRAMING;
	}

	UART_ClearStatusFlags(config->base, kUART_RxOverrunFlag |
					    kUART_ParityErrorFlag |
					    kUART_FramingErrorFlag);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_mcux_fifo_fill(struct device *dev, const u8_t *tx_data,
			       int len)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (UART_GetStatusFlags(config->base) & kUART_TxDataRegEmptyFlag)) {

		UART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int uart_mcux_fifo_read(struct device *dev, u8_t *rx_data,
			       const int len)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (UART_GetStatusFlags(config->base) & kUART_RxDataRegFullFlag)) {

		rx_data[num_rx++] = UART_ReadByte(config->base);
	}

	return num_rx;
}

static void uart_mcux_irq_tx_enable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t mask = kUART_TxDataRegEmptyInterruptEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_tx_disable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t mask = kUART_TxDataRegEmptyInterruptEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int uart_mcux_irq_tx_complete(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t flags = UART_GetStatusFlags(config->base);

	return (flags & kUART_TxDataRegEmptyFlag) != 0U;
}

static int uart_mcux_irq_tx_ready(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t mask = kUART_TxDataRegEmptyInterruptEnable;

	return (UART_GetEnabledInterrupts(config->base) & mask)
		&& uart_mcux_irq_tx_complete(dev);
}

static void uart_mcux_irq_rx_enable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t mask = kUART_RxDataRegFullInterruptEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_rx_disable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t mask = kUART_RxDataRegFullInterruptEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int uart_mcux_irq_rx_full(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t flags = UART_GetStatusFlags(config->base);

	return (flags & kUART_RxDataRegFullFlag) != 0U;
}

static int uart_mcux_irq_rx_ready(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t mask = kUART_RxDataRegFullInterruptEnable;

	return (UART_GetEnabledInterrupts(config->base) & mask)
		&& uart_mcux_irq_rx_full(dev);
}

static void uart_mcux_irq_err_enable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t mask = kUART_NoiseErrorInterruptEnable |
			kUART_FramingErrorInterruptEnable |
			kUART_ParityErrorInterruptEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_err_disable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	u32_t mask = kUART_NoiseErrorInterruptEnable |
			kUART_FramingErrorInterruptEnable |
			kUART_ParityErrorInterruptEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int uart_mcux_irq_is_pending(struct device *dev)
{
	return uart_mcux_irq_tx_ready(dev) || uart_mcux_irq_rx_ready(dev);
}

static int uart_mcux_irq_update(struct device *dev)
{
	return 1;
}

static void uart_mcux_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_mcux_data *data = dev->driver_data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_mcux_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_mcux_data *data = dev->driver_data;

	if (data->callback) {
		data->callback(data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_mcux_init(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config->config_info;
	uart_config_t uart_config;
	struct device *clock_dev;
	u32_t clock_freq;

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	UART_GetDefaultConfig(&uart_config);
	uart_config.enableTx = true;
	uart_config.enableRx = true;
	if (config->hw_flow_control) {
		uart_config.enableRxRTS = true;
		uart_config.enableTxCTS = true;
	}
	uart_config.baudRate_Bps = config->baud_rate;

	UART_Init(config->base, &uart_config, clock_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api uart_mcux_driver_api = {
	.poll_in = uart_mcux_poll_in,
	.poll_out = uart_mcux_poll_out,
	.err_check = uart_mcux_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_mcux_fifo_fill,
	.fifo_read = uart_mcux_fifo_read,
	.irq_tx_enable = uart_mcux_irq_tx_enable,
	.irq_tx_disable = uart_mcux_irq_tx_disable,
	.irq_tx_complete = uart_mcux_irq_tx_complete,
	.irq_tx_ready = uart_mcux_irq_tx_ready,
	.irq_rx_enable = uart_mcux_irq_rx_enable,
	.irq_rx_disable = uart_mcux_irq_rx_disable,
	.irq_rx_ready = uart_mcux_irq_rx_ready,
	.irq_err_enable = uart_mcux_irq_err_enable,
	.irq_err_disable = uart_mcux_irq_err_disable,
	.irq_is_pending = uart_mcux_irq_is_pending,
	.irq_update = uart_mcux_irq_update,
	.irq_callback_set = uart_mcux_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_MCUX_0

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_0(struct device *dev);
#endif

static const struct uart_mcux_config uart_mcux_0_config = {
	.base = UART0,
	.clock_name = DT_ALIAS_UART_0_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)DT_ALIAS_UART_0_CLOCK_NAME,
	.baud_rate = DT_ALIAS_UART_0_CURRENT_SPEED,
	.hw_flow_control = DT_ALIAS_UART_0_HW_FLOW_CONTROL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_mcux_config_func_0,
#endif
};

static struct uart_mcux_data uart_mcux_0_data;

DEVICE_AND_API_INIT(uart_0, DT_ALIAS_UART_0_LABEL,
		    &uart_mcux_init,
		    &uart_mcux_0_data, &uart_mcux_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_mcux_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_UART_0_IRQ_STATUS,
		    DT_ALIAS_UART_0_IRQ_STATUS_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_0), 0);

	irq_enable(DT_ALIAS_UART_0_IRQ_STATUS);

	IRQ_CONNECT(DT_ALIAS_UART_0_IRQ_ERROR,
		    DT_ALIAS_UART_0_IRQ_ERROR_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_0), 0);

	irq_enable(DT_ALIAS_UART_0_IRQ_ERROR);
}
#endif

#endif /* CONFIG_UART_MCUX_0 */

#ifdef CONFIG_UART_MCUX_1

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_1(struct device *dev);
#endif

static const struct uart_mcux_config uart_mcux_1_config = {
	.base = UART1,
	.clock_name = DT_ALIAS_UART_1_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)DT_ALIAS_UART_1_CLOCK_NAME,
	.baud_rate = DT_ALIAS_UART_1_CURRENT_SPEED,
	.hw_flow_control = DT_ALIAS_UART_1_HW_FLOW_CONTROL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_mcux_config_func_1,
#endif
};

static struct uart_mcux_data uart_mcux_1_data;

DEVICE_AND_API_INIT(uart_1, DT_ALIAS_UART_1_LABEL,
		    &uart_mcux_init,
		    &uart_mcux_1_data, &uart_mcux_1_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_mcux_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_1(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_UART_1_IRQ_STATUS,
		    DT_ALIAS_UART_1_IRQ_STATUS_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_1), 0);

	irq_enable(DT_ALIAS_UART_1_IRQ_STATUS);

	IRQ_CONNECT(DT_ALIAS_UART_1_IRQ_ERROR,
		    DT_ALIAS_UART_1_IRQ_ERROR_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_1), 0);

	irq_enable(DT_ALIAS_UART_1_IRQ_ERROR);
}
#endif

#endif /* CONFIG_UART_MCUX_1 */

#ifdef CONFIG_UART_MCUX_2

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_2(struct device *dev);
#endif

static const struct uart_mcux_config uart_mcux_2_config = {
	.base = UART2,
	.clock_name = DT_ALIAS_UART_2_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)DT_ALIAS_UART_2_CLOCK_NAME,
	.baud_rate = DT_ALIAS_UART_2_CURRENT_SPEED,
	.hw_flow_control = DT_ALIAS_UART_2_HW_FLOW_CONTROL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_mcux_config_func_2,
#endif
};

static struct uart_mcux_data uart_mcux_2_data;

DEVICE_AND_API_INIT(uart_2, DT_ALIAS_UART_2_LABEL,
		    &uart_mcux_init,
		    &uart_mcux_2_data, &uart_mcux_2_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_mcux_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_2(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_UART_2_IRQ_STATUS,
		    DT_ALIAS_UART_2_IRQ_STATUS_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_2), 0);

	irq_enable(DT_ALIAS_UART_2_IRQ_STATUS);

	IRQ_CONNECT(DT_ALIAS_UART_2_IRQ_ERROR,
		    DT_ALIAS_UART_2_IRQ_ERROR_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_2), 0);

	irq_enable(DT_ALIAS_UART_2_IRQ_ERROR);
}
#endif

#endif /* CONFIG_UART_MCUX_2 */

#ifdef CONFIG_UART_MCUX_3

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_3(struct device *dev);
#endif

static const struct uart_mcux_config uart_mcux_3_config = {
	.base = UART3,
	.clock_name = DT_ALIAS_UART_3_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)DT_ALIAS_UART_3_CLOCK_NAME,
	.baud_rate = DT_ALIAS_UART_3_CURRENT_SPEED,
	.hw_flow_control = DT_ALIAS_UART_3_HW_FLOW_CONTROL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_mcux_config_func_3,
#endif
};

static struct uart_mcux_data uart_mcux_3_data;

DEVICE_AND_API_INIT(uart_3, DT_ALIAS_UART_3_LABEL,
		    &uart_mcux_init,
		    &uart_mcux_3_data, &uart_mcux_3_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_mcux_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_3(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_UART_3_IRQ_STATUS,
		    DT_ALIAS_UART_3_IRQ_STATUS_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_3), 0);

	irq_enable(DT_ALIAS_UART_3_IRQ_STATUS);

	IRQ_CONNECT(DT_ALIAS_UART_3_IRQ_ERROR,
		    DT_ALIAS_UART_3_IRQ_ERROR_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_3), 0);

	irq_enable(DT_ALIAS_UART_3_IRQ_ERROR);
}
#endif

#endif /* CONFIG_UART_MCUX_3 */

#ifdef CONFIG_UART_MCUX_4

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_4(struct device *dev);
#endif

static const struct uart_mcux_config uart_mcux_4_config = {
	.base = UART4,
	.clock_name = DT_ALIAS_UART_4_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)DT_ALIAS_UART_4_CLOCK_NAME,
	.baud_rate = DT_ALIAS_UART_4_CURRENT_SPEED,
	.hw_flow_control = DT_ALIAS_UART_4_HW_FLOW_CONTROL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_mcux_config_func_4,
#endif
};

static struct uart_mcux_data uart_mcux_4_data;

DEVICE_AND_API_INIT(uart_4, DT_ALIAS_UART_4_LABEL,
		    &uart_mcux_init,
		    &uart_mcux_4_data, &uart_mcux_4_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_mcux_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_4(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_UART_4_IRQ_STATUS,
		    DT_ALIAS_UART_4_IRQ_STATUS_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_4), 0);

	irq_enable(DT_ALIAS_UART_4_IRQ_STATUS);

	IRQ_CONNECT(DT_ALIAS_UART_4_IRQ_ERROR,
		    DT_ALIAS_UART_4_IRQ_ERROR_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_4), 0);

	irq_enable(DT_ALIAS_UART_4_IRQ_ERROR);
}
#endif

#endif /* CONFIG_UART_MCUX_4 */

#ifdef CONFIG_UART_MCUX_5

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_5(struct device *dev);
#endif

static const struct uart_mcux_config uart_mcux_5_config = {
	.base = UART5,
	.clock_name = DT_ALIAS_UART_5_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)DT_ALIAS_UART_5_CLOCK_NAME,
	.baud_rate = DT_ALIAS_UART_5_CURRENT_SPEED,
	.hw_flow_control = DT_ALIAS_UART_5_HW_FLOW_CONTROL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_mcux_config_func_5,
#endif
};

static struct uart_mcux_data uart_mcux_5_data;

DEVICE_AND_API_INIT(uart_5, DT_ALIAS_UART_5_LABEL,
		    &uart_mcux_init,
		    &uart_mcux_5_data, &uart_mcux_5_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_mcux_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_mcux_config_func_5(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_UART_5_IRQ_STATUS,
		    DT_ALIAS_UART_5_IRQ_STATUS_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_5), 0);

	irq_enable(DT_ALIAS_UART_5_IRQ_STATUS);

	IRQ_CONNECT(DT_ALIAS_UART_5_IRQ_ERROR,
		    DT_ALIAS_UART_5_IRQ_ERROR_PRIORITY,
		    uart_mcux_isr, DEVICE_GET(uart_5), 0);

	irq_enable(DT_ALIAS_UART_5_IRQ_ERROR);
}
#endif

#endif /* CONFIG_UART_MCUX_5 */
