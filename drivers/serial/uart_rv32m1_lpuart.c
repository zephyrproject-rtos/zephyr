/*
 * Copyright (c) 2018 Foundries.io
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_lpuart

#include <errno.h>
#include <device.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
#include <fsl_lpuart.h>
#include <soc.h>

struct rv32m1_lpuart_config {
	LPUART_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	clock_ip_name_t clock_ip_name;
	uint32_t clock_ip_src;
	uint32_t baud_rate;
	uint8_t hw_flow_control;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

struct rv32m1_lpuart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int rv32m1_lpuart_poll_in(struct device *dev, unsigned char *c)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kLPUART_RxDataRegFullFlag) {
		*c = LPUART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void rv32m1_lpuart_poll_out(struct device *dev, unsigned char c)
{
	const struct rv32m1_lpuart_config *config = dev->config;

	while (!(LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag)) {
	}

	LPUART_WriteByte(config->base, c);
}

static int rv32m1_lpuart_err_check(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);
	int err = 0;

	if (flags & kLPUART_RxOverrunFlag) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & kLPUART_ParityErrorFlag) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & kLPUART_FramingErrorFlag) {
		err |= UART_ERROR_FRAMING;
	}

	LPUART_ClearStatusFlags(config->base, kLPUART_RxOverrunFlag |
					      kLPUART_ParityErrorFlag |
					      kLPUART_FramingErrorFlag);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int rv32m1_lpuart_fifo_fill(struct device *dev, const uint8_t *tx_data,
			       int len)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag)) {

		LPUART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int rv32m1_lpuart_fifo_read(struct device *dev, uint8_t *rx_data,
			       const int len)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_RxDataRegFullFlag)) {

		rx_data[num_rx++] = LPUART_ReadByte(config->base);
	}

	return num_rx;
}

static void rv32m1_lpuart_irq_tx_enable(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void rv32m1_lpuart_irq_tx_disable(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int rv32m1_lpuart_irq_tx_complete(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_TxDataRegEmptyFlag) != 0U;
}

static int rv32m1_lpuart_irq_tx_ready(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& rv32m1_lpuart_irq_tx_complete(dev);
}

static void rv32m1_lpuart_irq_rx_enable(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void rv32m1_lpuart_irq_rx_disable(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int rv32m1_lpuart_irq_rx_full(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_RxDataRegFullFlag) != 0U;
}

static int rv32m1_lpuart_irq_rx_ready(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& rv32m1_lpuart_irq_rx_full(dev);
}

static void rv32m1_lpuart_irq_err_enable(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void rv32m1_lpuart_irq_err_disable(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int rv32m1_lpuart_irq_is_pending(struct device *dev)
{
	return (rv32m1_lpuart_irq_tx_ready(dev)
		|| rv32m1_lpuart_irq_rx_ready(dev));
}

static int rv32m1_lpuart_irq_update(struct device *dev)
{
	return 1;
}

static void rv32m1_lpuart_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct rv32m1_lpuart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void rv32m1_lpuart_isr(void *arg)
{
	struct device *dev = arg;
	struct rv32m1_lpuart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int rv32m1_lpuart_init(struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	lpuart_config_t uart_config;
	struct device *clock_dev;
	uint32_t clock_freq;

	/* set clock source */
	/* TODO: Don't change if another core has configured */
	CLOCK_SetIpSrc(config->clock_ip_name, config->clock_ip_src);

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPUART_GetDefaultConfig(&uart_config);
	uart_config.enableTx = true;
	uart_config.enableRx = true;
	if (config->hw_flow_control) {
		uart_config.enableRxRTS = true;
		uart_config.enableTxCTS = true;
	}
	uart_config.baudRate_Bps = config->baud_rate;

	LPUART_Init(config->base, &uart_config, clock_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api rv32m1_lpuart_driver_api = {
	.poll_in = rv32m1_lpuart_poll_in,
	.poll_out = rv32m1_lpuart_poll_out,
	.err_check = rv32m1_lpuart_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = rv32m1_lpuart_fifo_fill,
	.fifo_read = rv32m1_lpuart_fifo_read,
	.irq_tx_enable = rv32m1_lpuart_irq_tx_enable,
	.irq_tx_disable = rv32m1_lpuart_irq_tx_disable,
	.irq_tx_complete = rv32m1_lpuart_irq_tx_complete,
	.irq_tx_ready = rv32m1_lpuart_irq_tx_ready,
	.irq_rx_enable = rv32m1_lpuart_irq_rx_enable,
	.irq_rx_disable = rv32m1_lpuart_irq_rx_disable,
	.irq_rx_ready = rv32m1_lpuart_irq_rx_ready,
	.irq_err_enable = rv32m1_lpuart_irq_err_enable,
	.irq_err_disable = rv32m1_lpuart_irq_err_disable,
	.irq_is_pending = rv32m1_lpuart_irq_is_pending,
	.irq_update = rv32m1_lpuart_irq_update,
	.irq_callback_set = rv32m1_lpuart_irq_callback_set,
#endif
};

#define RV32M1_LPUART_DECLARE_CFG(n, IRQ_FUNC_INIT)			\
	static const struct rv32m1_lpuart_config rv32m1_lpuart_##n##_cfg = {\
		.base = (LPUART_Type *)DT_INST_REG_ADDR(n),		\
		.clock_name = DT_INST_CLOCKS_LABEL(n),			\
		.clock_subsys =						\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.clock_ip_name = INST_DT_CLOCK_IP_NAME(n),		\
		.clock_ip_src = kCLOCK_IpSrcFircAsync,			\
		.baud_rate = DT_INST_PROP(n, current_speed),		\
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control),	\
		IRQ_FUNC_INIT						\
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define RV32M1_LPUART_CONFIG_FUNC(n)					\
	static void rv32m1_lpuart_config_func_##n(struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), 0, rv32m1_lpuart_isr,	\
			    DEVICE_GET(uart_0), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define RV32M1_LPUART_IRQ_CFG_FUNC_INIT(n)				\
	.irq_config_func = rv32m1_lpuart_config_func_##n,
#define RV32M1_LPUART_INIT_CFG(n)					\
	RV32M1_LPUART_DECLARE_CFG(n, RV32M1_LPUART_IRQ_CFG_FUNC_INIT(n))
#else
#define RV32M1_LPUART_CONFIG_FUNC(n)
#define RV32M1_LPUART_IRQ_CFG_FUNC_INIT
#define RV32M1_LPUART_INIT_CFG(n)					\
	RV32M1_LPUART_DECLARE_CFG(n, RV32M1_LPUART_IRQ_CFG_FUNC_INIT)
#endif

#define RV32M1_LPUART_INIT(n)						\
	static struct rv32m1_lpuart_data rv32m1_lpuart_##n##_data;	\
									\
	static const struct rv32m1_lpuart_config rv32m1_lpuart_##n##_cfg;\
									\
	DEVICE_AND_API_INIT(uart_##n, DT_INST_LABEL(n),			\
			    &rv32m1_lpuart_init,			\
			    &rv32m1_lpuart_##n##_data,			\
			    &rv32m1_lpuart_##n##_cfg,			\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &rv32m1_lpuart_driver_api);			\
									\
	RV32M1_LPUART_CONFIG_FUNC(n)					\
									\
	RV32M1_LPUART_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(RV32M1_LPUART_INIT)
