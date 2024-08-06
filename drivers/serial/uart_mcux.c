/*
 * Copyright 2017, 2024 NXP
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_uart

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_uart.h>
#include <soc.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>

struct uart_mcux_config {
	UART_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
	const struct pinctrl_dev_config *pincfg;
};

struct uart_mcux_data {
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int uart_mcux_configure(const struct device *dev,
			       const struct uart_config *cfg)
{
	const struct uart_mcux_config *config = dev->config;
	struct uart_mcux_data *data = dev->data;
	uart_config_t uart_config;
	uint32_t clock_freq;
	status_t retval;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
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

#if defined(FSL_FEATURE_UART_HAS_MODEM_SUPPORT) && FSL_FEATURE_UART_HAS_MODEM_SUPPORT
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
#endif

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

	data->uart_cfg = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_mcux_config_get(const struct device *dev,
				struct uart_config *cfg)
{
	struct uart_mcux_data *data = dev->data;

	*cfg = data->uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_mcux_poll_in(const struct device *dev, unsigned char *c)
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

static void uart_mcux_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_mcux_config *config = dev->config;

	while (!(UART_GetStatusFlags(config->base) & kUART_TxDataRegEmptyFlag)) {
	}

	UART_WriteByte(config->base, c);
}

static int uart_mcux_err_check(const struct device *dev)
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
static int uart_mcux_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data,
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

static int uart_mcux_fifo_read(const struct device *dev, uint8_t *rx_data,
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

static void uart_mcux_irq_tx_enable(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_TxDataRegEmptyInterruptEnable;
	pm_device_busy_set(dev);
	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_tx_disable(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_TxDataRegEmptyInterruptEnable;
	pm_device_busy_clear(dev);
	UART_DisableInterrupts(config->base, mask);
}

static int uart_mcux_irq_tx_complete(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t flags = UART_GetStatusFlags(config->base);

	return (flags & kUART_TransmissionCompleteFlag) != 0U;
}

static int uart_mcux_irq_tx_ready(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_TxDataRegEmptyInterruptEnable;
	uint32_t flags = UART_GetStatusFlags(config->base);

	return (UART_GetEnabledInterrupts(config->base) & mask)
		&& (flags & kUART_TxDataRegEmptyFlag);
}

static void uart_mcux_irq_rx_enable(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_RxDataRegFullInterruptEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_rx_disable(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_RxDataRegFullInterruptEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int uart_mcux_irq_rx_full(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t flags = UART_GetStatusFlags(config->base);

	return (flags & kUART_RxDataRegFullFlag) != 0U;
}

static int uart_mcux_irq_rx_pending(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_RxDataRegFullInterruptEnable;

	return (UART_GetEnabledInterrupts(config->base) & mask)
		&& uart_mcux_irq_rx_full(dev);
}

static void uart_mcux_irq_err_enable(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_NoiseErrorInterruptEnable |
			kUART_FramingErrorInterruptEnable |
			kUART_ParityErrorInterruptEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void uart_mcux_irq_err_disable(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	uint32_t mask = kUART_NoiseErrorInterruptEnable |
			kUART_FramingErrorInterruptEnable |
			kUART_ParityErrorInterruptEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int uart_mcux_irq_is_pending(const struct device *dev)
{
	return uart_mcux_irq_tx_ready(dev) || uart_mcux_irq_rx_pending(dev);
}

static int uart_mcux_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_mcux_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_mcux_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_mcux_isr(const struct device *dev)
{
	struct uart_mcux_data *data = dev->data;
	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_mcux_init(const struct device *dev)
{
	const struct uart_mcux_config *config = dev->config;
	struct uart_mcux_data *data = dev->data;
	int err;

	err = uart_mcux_configure(dev, &data->uart_cfg);
	if (err != 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int uart_mcux_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_mcux_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		clock_control_on(config->clock_dev, config->clock_subsys);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		clock_control_off(config->clock_dev, config->clock_subsys);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
		return 0;
	default:
		return -ENOTSUP;
	}
	return 0;
}
#endif /*CONFIG_PM_DEVICE*/

static const struct uart_driver_api uart_mcux_driver_api = {
	.poll_in = uart_mcux_poll_in,
	.poll_out = uart_mcux_poll_out,
	.err_check = uart_mcux_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_mcux_configure,
	.config_get = uart_mcux_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_mcux_fifo_fill,
	.fifo_read = uart_mcux_fifo_read,
	.irq_tx_enable = uart_mcux_irq_tx_enable,
	.irq_tx_disable = uart_mcux_irq_tx_disable,
	.irq_tx_complete = uart_mcux_irq_tx_complete,
	.irq_tx_ready = uart_mcux_irq_tx_ready,
	.irq_rx_enable = uart_mcux_irq_rx_enable,
	.irq_rx_disable = uart_mcux_irq_rx_disable,
	.irq_rx_ready = uart_mcux_irq_rx_full,
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
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	IRQ_FUNC_INIT							\
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_MCUX_CONFIG_FUNC(n)					\
	static void uart_mcux_config_func_##n(const struct device *dev)	\
	{								\
		UART_MCUX_IRQ(n, status);	\
		UART_MCUX_IRQ(n, error);	\
	}

#define UART_MCUX_IRQ_INIT(n, name)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, name, irq),	\
			    DT_INST_IRQ_BY_NAME(n, name, priority),	\
			    uart_mcux_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_NAME(n, name, irq));	\
	} while (false)

#define UART_MCUX_IRQ(n, name)						\
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(n, name),		\
		    (UART_MCUX_IRQ_INIT(n, name)), ())

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
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static struct uart_mcux_data uart_mcux_##n##_data = {		\
		.uart_cfg = {						\
			.stop_bits = UART_CFG_STOP_BITS_1,		\
			.data_bits = UART_CFG_DATA_BITS_8,		\
			.baudrate  = DT_INST_PROP(n, current_speed),	\
			.parity    = UART_CFG_PARITY_NONE,		\
			.flow_ctrl = DT_INST_PROP(n, hw_flow_control) ?	\
				UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE,\
		},							\
	};								\
									\
	static const struct uart_mcux_config uart_mcux_##n##_config;	\
	PM_DEVICE_DT_INST_DEFINE(n, uart_mcux_pm_action);\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    uart_mcux_init,				\
			    PM_DEVICE_DT_INST_GET(n),			\
			    &uart_mcux_##n##_data,			\
			    &uart_mcux_##n##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &uart_mcux_driver_api);			\
									\
	UART_MCUX_CONFIG_FUNC(n)					\
									\
	UART_MCUX_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_MCUX_INIT)
