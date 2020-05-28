/*
 * Copyright (c) 2017, NXP
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_uart

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
	struct uart_config uart_cfg;
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

static int uart_mcux_configure(struct device *dev,
			       const struct uart_config *cfg)
{
	const struct uart_mcux_config *config = dev->config;
	uart_config_t uart_config;
	struct device *clock_dev;
	uint32_t clock_freq;
	status_t retval;

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
	uart_config.baudRate_Bps = cfg->baudrate;

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
#if defined(FSL_FEATURE_UART_HAS_STOP_BIT_CONFIG_SUPPORT) && \
FSL_FEATURE_UART_HAS_STOP_BIT_CONFIG_SUPPORT
		uart_config.stopBitCount = kUART_OneStopBit;
		break;
	case UART_CFG_STOP_BITS_2:
		uart_config.stopBitCount = kUART_TwoStopBit;
#endif
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uart_config.enableRxRTS = false;
		uart_config.enableTxCTS = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		uart_config.enableRxRTS = true;
		uart_config.enableTxCTS = true;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_config.parityMode = kUART_ParityDisabled;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_config.parityMode = kUART_ParityEven;
		break;
	case UART_CFG_PARITY_ODD:
		uart_config.parityMode = kUART_ParityOdd;
		break;
	default:
		return -ENOTSUP;
	}

	retval = UART_Init(config->base, &uart_config, clock_freq);
	if (retval != kStatus_Success) {
		return -EINVAL;
	}

	return 0;
}

static int uart_mcux_poll_in(struct device *dev, unsigned char *c)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t flags = UART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kUART_RxDataRegFullFlag) {
		*c = UART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void uart_mcux_poll_out(struct device *dev, unsigned char c)
{
	const struct uart_mcux_config *config = dev->config;

	while (!(UART_GetStatusFlags(config->base) & kUART_TxDataRegEmptyFlag)) {
	}

	UART_WriteByte(config->base, c);
}

static int uart_mcux_err_check(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t flags = UART_GetStatusFlags(config->base);
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
static int uart_mcux_fifo_fill(struct device *dev, const uint8_t *tx_data,
			       int len)
{
	const struct uart_mcux_config *config = dev->config;
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (UART_GetStatusFlags(config->base) & kUART_TxDataRegEmptyFlag)) {

		UART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int uart_mcux_fifo_read(struct device *dev, uint8_t *rx_data,
			       const int len)
{
	const struct uart_mcux_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (UART_GetStatusFlags(config->base) & kUART_RxDataRegFullFlag)) {

		rx_data[num_rx++] = UART_ReadByte(config->base);
	}

	return num_rx;
}

static void uart_mcux_irq_tx_enable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_TxDataRegEmptyInterruptEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_tx_disable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_TxDataRegEmptyInterruptEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int uart_mcux_irq_tx_complete(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t flags = UART_GetStatusFlags(config->base);

	return (flags & kUART_TransmissionCompleteFlag) != 0U;
}

static int uart_mcux_irq_tx_ready(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_TxDataRegEmptyInterruptEnable;
	uint32_t flags = UART_GetStatusFlags(config->base);

	return (UART_GetEnabledInterrupts(config->base) & mask)
		&& (flags & kUART_TxDataRegEmptyFlag);
}

static void uart_mcux_irq_rx_enable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_RxDataRegFullInterruptEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_rx_disable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_RxDataRegFullInterruptEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int uart_mcux_irq_rx_full(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t flags = UART_GetStatusFlags(config->base);

	return (flags & kUART_RxDataRegFullFlag) != 0U;
}

static int uart_mcux_irq_rx_ready(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_RxDataRegFullInterruptEnable;

	return (UART_GetEnabledInterrupts(config->base) & mask)
		&& uart_mcux_irq_rx_full(dev);
}

static void uart_mcux_irq_err_enable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_NoiseErrorInterruptEnable |
			kUART_FramingErrorInterruptEnable |
			kUART_ParityErrorInterruptEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_err_disable(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_NoiseErrorInterruptEnable |
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
	struct uart_mcux_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_mcux_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_mcux_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_mcux_init(struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	int err;

	err = uart_mcux_configure(dev, &config->uart_cfg);
	if (err != 0) {
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api uart_mcux_driver_api = {
	.poll_in = uart_mcux_poll_in,
	.poll_out = uart_mcux_poll_out,
	.err_check = uart_mcux_err_check,
	.configure = uart_mcux_configure,
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

#define UART_MCUX_DECLARE_CFG(n, IRQ_FUNC_INIT)				\
static const struct uart_mcux_config uart_mcux_##n##_config = {		\
	.base = (UART_Type *)DT_INST_REG_ADDR(n),			\
	.clock_name = DT_INST_CLOCKS_LABEL(n),				\
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
	.uart_cfg = {							\
		.stop_bits = UART_CFG_STOP_BITS_1,			\
		.data_bits = UART_CFG_DATA_BITS_8,			\
		.baudrate  = DT_INST_PROP(n, current_speed),		\
		.parity    = UART_CFG_PARITY_NONE,			\
		.flow_ctrl = DT_INST_PROP(n, hw_flow_control) ?		\
			UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE,\
	},								\
	IRQ_FUNC_INIT							\
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_MCUX_CONFIG_FUNC(n)					\
	static void uart_mcux_config_func_##n(struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, status, irq),	\
			    DT_INST_IRQ_BY_NAME(n, status, priority),	\
			    uart_mcux_isr, DEVICE_GET(uart_##n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_NAME(n, status, irq));	\
									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, error, irq),		\
			    DT_INST_IRQ_BY_NAME(n, error, priority),	\
			    uart_mcux_isr, DEVICE_GET(uart_##n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_NAME(n, error, irq));		\
	}
#define UART_MCUX_IRQ_CFG_FUNC_INIT(n)					\
	.irq_config_func = uart_mcux_config_func_##n
#define UART_MCUX_INIT_CFG(n)						\
	UART_MCUX_DECLARE_CFG(n, UART_MCUX_IRQ_CFG_FUNC_INIT(n))
#else
#define UART_MCUX_CONFIG_FUNC(n)
#define UART_MCUX_IRQ_CFG_FUNC_INIT
#define UART_MCUX_INIT_CFG(n)						\
	UART_MCUX_DECLARE_CFG(n, UART_MCUX_IRQ_CFG_FUNC_INIT)
#endif

#define UART_MCUX_INIT(n)						\
									\
	static struct uart_mcux_data uart_mcux_##n##_data;		\
									\
	static const struct uart_mcux_config uart_mcux_##n##_config;	\
									\
	DEVICE_AND_API_INIT(uart_##n, DT_INST_LABEL(n),			\
			    &uart_mcux_init,				\
			    &uart_mcux_##n##_data,			\
			    &uart_mcux_##n##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &uart_mcux_driver_api);			\
									\
	UART_MCUX_CONFIG_FUNC(n)					\
									\
	UART_MCUX_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_MCUX_INIT)
