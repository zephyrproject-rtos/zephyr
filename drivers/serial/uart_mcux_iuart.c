/*
 * Copyright (c) 2019, Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_iuart

#include <device.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
#include <errno.h>
#include <fsl_uart.h>
#include <soc.h>

struct mcux_iuart_config {
	UART_Type *base;
	const char *clock_name;
	clock_control_subsys_t clock_subsys;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

struct mcux_iuart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#define DEV_CFG(dev)						\
	((const struct mcux_iuart_config * const)(dev)->config)

static int mcux_iuart_poll_in(struct device *dev, unsigned char *c)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	int ret = -1;

	if (UART_GetStatusFlag(config->base, kUART_RxDataReadyFlag)) {
		*c = UART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void mcux_iuart_poll_out(struct device *dev, unsigned char c)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);

	while (!(UART_GetStatusFlag(config->base, kUART_TxReadyFlag))) {
	}

	UART_WriteByte(config->base, c);
}

static int mcux_iuart_err_check(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	int err = 0;

	if (UART_GetStatusFlag(config->base, kUART_RxOverrunFlag)) {
		err |= UART_ERROR_OVERRUN;
		UART_ClearStatusFlag(config->base, kUART_RxOverrunFlag);
	}

	if (UART_GetStatusFlag(config->base, kUART_ParityErrorFlag)) {
		err |= UART_ERROR_PARITY;
		UART_ClearStatusFlag(config->base, kUART_ParityErrorFlag);
	}

	if (UART_GetStatusFlag(config->base, kUART_FrameErrorFlag)) {
		err |= UART_ERROR_FRAMING;
		UART_ClearStatusFlag(config->base, kUART_FrameErrorFlag);
	}

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int mcux_iuart_fifo_fill(struct device *dev, const uint8_t *tx_data,
			       int len)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (UART_GetStatusFlag(config->base, kUART_TxEmptyFlag))) {

		UART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int mcux_iuart_fifo_read(struct device *dev, uint8_t *rx_data,
			       const int len)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (UART_GetStatusFlag(config->base, kUART_RxDataReadyFlag))) {

		rx_data[num_rx++] = UART_ReadByte(config->base);
	}

	return num_rx;
}

static void mcux_iuart_irq_tx_enable(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);

	UART_EnableInterrupts(config->base, kUART_TxEmptyEnable);
}

static void mcux_iuart_irq_tx_disable(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);

	UART_DisableInterrupts(config->base, kUART_TxEmptyEnable);
}

static int mcux_iuart_irq_tx_complete(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);

	return (UART_GetStatusFlag(config->base, kUART_TxEmptyFlag)) != 0U;
}

static int mcux_iuart_irq_tx_ready(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uint32_t mask = kUART_TxEmptyEnable;

	return (UART_GetEnabledInterrupts(config->base) & mask)
		&& mcux_iuart_irq_tx_complete(dev);
}

static void mcux_iuart_irq_rx_enable(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uint32_t mask = kUART_RxDataReadyEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void mcux_iuart_irq_rx_disable(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uint32_t mask = kUART_RxDataReadyEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int mcux_iuart_irq_rx_full(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);

	return (UART_GetStatusFlag(config->base, kUART_RxDataReadyFlag)) != 0U;
}

static int mcux_iuart_irq_rx_ready(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uint32_t mask = kUART_RxDataReadyEnable;

	return (UART_GetEnabledInterrupts(config->base) & mask)
		&& mcux_iuart_irq_rx_full(dev);
}

static void mcux_iuart_irq_err_enable(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uint32_t mask = kUART_RxOverrunEnable | kUART_ParityErrorEnable |
			kUART_FrameErrorEnable;

	UART_EnableInterrupts(config->base, mask);
}

static void mcux_iuart_irq_err_disable(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uint32_t mask = kUART_RxOverrunEnable | kUART_ParityErrorEnable |
			kUART_FrameErrorEnable;

	UART_DisableInterrupts(config->base, mask);
}

static int mcux_iuart_irq_is_pending(struct device *dev)
{
	return mcux_iuart_irq_tx_ready(dev) || mcux_iuart_irq_rx_ready(dev);
}

static int mcux_iuart_irq_update(struct device *dev)
{
	return 1;
}

static void mcux_iuart_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct mcux_iuart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void mcux_iuart_isr(void *arg)
{
	struct device *dev = arg;
	struct mcux_iuart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int mcux_iuart_init(struct device *dev)
{
	const struct mcux_iuart_config *config = DEV_CFG(dev);
	uart_config_t uart_config;
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

	UART_GetDefaultConfig(&uart_config);
	uart_config.enableTx = true;
	uart_config.enableRx = true;
	uart_config.baudRate_Bps = config->baud_rate;

	UART_Init(config->base, &uart_config, clock_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api mcux_iuart_driver_api = {
	.poll_in = mcux_iuart_poll_in,
	.poll_out = mcux_iuart_poll_out,
	.err_check = mcux_iuart_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = mcux_iuart_fifo_fill,
	.fifo_read = mcux_iuart_fifo_read,
	.irq_tx_enable = mcux_iuart_irq_tx_enable,
	.irq_tx_disable = mcux_iuart_irq_tx_disable,
	.irq_tx_complete = mcux_iuart_irq_tx_complete,
	.irq_tx_ready = mcux_iuart_irq_tx_ready,
	.irq_rx_enable = mcux_iuart_irq_rx_enable,
	.irq_rx_disable = mcux_iuart_irq_rx_disable,
	.irq_rx_ready = mcux_iuart_irq_rx_ready,
	.irq_err_enable = mcux_iuart_irq_err_enable,
	.irq_err_disable = mcux_iuart_irq_err_disable,
	.irq_is_pending = mcux_iuart_irq_is_pending,
	.irq_update = mcux_iuart_irq_update,
	.irq_callback_set = mcux_iuart_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define MCUX_IUART_IRQ_INIT(n, i)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq),		\
			    DT_INST_IRQ_BY_IDX(n, i, priority),		\
			    mcux_iuart_isr, DEVICE_GET(uart_##n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));		\
	} while (0)
#define IUART_MCUX_CONFIG_FUNC(n)					\
	static void mcux_iuart_config_func_##n(struct device *dev)	\
	{								\
		MCUX_IUART_IRQ_INIT(n, 0);				\
									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 1),			\
			   (MCUX_IUART_IRQ_INIT(n, 1);))		\
	}
#define IUART_MCUX_IRQ_CFG_FUNC_INIT(n)				\
	.irq_config_func = mcux_iuart_config_func_##n
#define IUART_MCUX_INIT_CFG(n)						\
	IUART_MCUX_DECLARE_CFG(n, IUART_MCUX_IRQ_CFG_FUNC_INIT(n))
#else
#define IUART_MCUX_CONFIG_FUNC(n)
#define IUART_MCUX_IRQ_CFG_FUNC_INIT
#define IUART_MCUX_INIT_CFG(n)						\
	IUART_MCUX_DECLARE_CFG(n, IUART_MCUX_IRQ_CFG_FUNC_INIT)
#endif

#define IUART_MCUX_DECLARE_CFG(n, IRQ_FUNC_INIT)			\
static const struct mcux_iuart_config mcux_iuart_##n##_config = {	\
	.base = (UART_Type *) DT_INST_REG_ADDR(n),			\
	.clock_name = DT_INST_CLOCKS_LABEL(n),				\
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
	.baud_rate = DT_INST_PROP(n, current_speed),			\
	IRQ_FUNC_INIT							\
}

#define IUART_MCUX_INIT(n)						\
									\
	static struct mcux_iuart_data mcux_iuart_##n##_data;		\
									\
	static const struct mcux_iuart_config mcux_iuart_##n##_config;\
									\
	DEVICE_AND_API_INIT(uart_##n, DT_INST_LABEL(n),			\
			    &mcux_iuart_init,				\
			    &mcux_iuart_##n##_data,			\
			    &mcux_iuart_##n##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &mcux_iuart_driver_api);			\
									\
	IUART_MCUX_CONFIG_FUNC(n)					\
									\
	IUART_MCUX_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(IUART_MCUX_INIT)
