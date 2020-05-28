/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_lpuart

#include <errno.h>
#include <soc.h>
#include <fsl_lpuart.h>
#include <device.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>

struct mcux_lpuart_config {
	LPUART_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

struct mcux_lpuart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int mcux_lpuart_poll_in(struct device *dev, unsigned char *c)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kLPUART_RxDataRegFullFlag) {
		*c = LPUART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void mcux_lpuart_poll_out(struct device *dev, unsigned char c)
{
	const struct mcux_lpuart_config *config = dev->config;

	while (!(LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag)) {
	}

	LPUART_WriteByte(config->base, c);
}

static int mcux_lpuart_err_check(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
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
static int mcux_lpuart_fifo_fill(struct device *dev, const uint8_t *tx_data,
			       int len)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag)) {

		LPUART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int mcux_lpuart_fifo_read(struct device *dev, uint8_t *rx_data,
			       const int len)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_RxDataRegFullFlag)) {

		rx_data[num_rx++] = LPUART_ReadByte(config->base);
	}

	return num_rx;
}

static void mcux_lpuart_irq_tx_enable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void mcux_lpuart_irq_tx_disable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int mcux_lpuart_irq_tx_complete(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_TransmissionCompleteFlag) != 0U;
}

static int mcux_lpuart_irq_tx_ready(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& (flags & kLPUART_TxDataRegEmptyFlag);
}

static void mcux_lpuart_irq_rx_enable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void mcux_lpuart_irq_rx_disable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int mcux_lpuart_irq_rx_full(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_RxDataRegFullFlag) != 0U;
}

static int mcux_lpuart_irq_rx_ready(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& mcux_lpuart_irq_rx_full(dev);
}

static void mcux_lpuart_irq_err_enable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void mcux_lpuart_irq_err_disable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int mcux_lpuart_irq_is_pending(struct device *dev)
{
	return (mcux_lpuart_irq_tx_ready(dev)
		|| mcux_lpuart_irq_rx_ready(dev));
}

static int mcux_lpuart_irq_update(struct device *dev)
{
	return 1;
}

static void mcux_lpuart_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct mcux_lpuart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void mcux_lpuart_isr(void *arg)
{
	struct device *dev = arg;
	struct mcux_lpuart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int mcux_lpuart_init(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	lpuart_config_t uart_config;
	struct device *clock_dev;
	uint32_t clock_freq;

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
	uart_config.baudRate_Bps = config->baud_rate;

	LPUART_Init(config->base, &uart_config, clock_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api mcux_lpuart_driver_api = {
	.poll_in = mcux_lpuart_poll_in,
	.poll_out = mcux_lpuart_poll_out,
	.err_check = mcux_lpuart_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = mcux_lpuart_fifo_fill,
	.fifo_read = mcux_lpuart_fifo_read,
	.irq_tx_enable = mcux_lpuart_irq_tx_enable,
	.irq_tx_disable = mcux_lpuart_irq_tx_disable,
	.irq_tx_complete = mcux_lpuart_irq_tx_complete,
	.irq_tx_ready = mcux_lpuart_irq_tx_ready,
	.irq_rx_enable = mcux_lpuart_irq_rx_enable,
	.irq_rx_disable = mcux_lpuart_irq_rx_disable,
	.irq_rx_ready = mcux_lpuart_irq_rx_ready,
	.irq_err_enable = mcux_lpuart_irq_err_enable,
	.irq_err_disable = mcux_lpuart_irq_err_disable,
	.irq_is_pending = mcux_lpuart_irq_is_pending,
	.irq_update = mcux_lpuart_irq_update,
	.irq_callback_set = mcux_lpuart_irq_callback_set,
#endif
};


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define MCUX_LPUART_IRQ_INIT(n, i)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq),		\
			    DT_INST_IRQ_BY_IDX(n, i, priority),		\
			    mcux_lpuart_isr, DEVICE_GET(uart_##n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));		\
	} while (0)
#define LPUART_MCUX_CONFIG_FUNC(n)					\
	static void mcux_lpuart_config_func_##n(struct device *dev)	\
	{								\
		MCUX_LPUART_IRQ_INIT(n, 0);				\
									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 1),			\
			   (MCUX_LPUART_IRQ_INIT(n, 1);))		\
	}
#define LPUART_MCUX_IRQ_CFG_FUNC_INIT(n)				\
	.irq_config_func = mcux_lpuart_config_func_##n
#define LPUART_MCUX_INIT_CFG(n)						\
	LPUART_MCUX_DECLARE_CFG(n, LPUART_MCUX_IRQ_CFG_FUNC_INIT(n))
#else
#define LPUART_MCUX_CONFIG_FUNC(n)
#define LPUART_MCUX_IRQ_CFG_FUNC_INIT
#define LPUART_MCUX_INIT_CFG(n)						\
	LPUART_MCUX_DECLARE_CFG(n, LPUART_MCUX_IRQ_CFG_FUNC_INIT)
#endif

#define LPUART_MCUX_DECLARE_CFG(n, IRQ_FUNC_INIT)			\
static const struct mcux_lpuart_config mcux_lpuart_##n##_config = {	\
	.base = (LPUART_Type *) DT_INST_REG_ADDR(n),			\
	.clock_name = DT_INST_CLOCKS_LABEL(n),				\
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
	.baud_rate = DT_INST_PROP(n, current_speed),			\
	IRQ_FUNC_INIT							\
}

#define LPUART_MCUX_INIT(n)						\
									\
	static struct mcux_lpuart_data mcux_lpuart_##n##_data;		\
									\
	static const struct mcux_lpuart_config mcux_lpuart_##n##_config;\
									\
	DEVICE_AND_API_INIT(uart_##n, DT_INST_LABEL(n),			\
			    &mcux_lpuart_init,				\
			    &mcux_lpuart_##n##_data,			\
			    &mcux_lpuart_##n##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &mcux_lpuart_driver_api);			\
									\
	LPUART_MCUX_CONFIG_FUNC(n)					\
									\
	LPUART_MCUX_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(LPUART_MCUX_INIT)
