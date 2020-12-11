/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_lpsci

#include <errno.h>
#include <device.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
#include <fsl_lpsci.h>
#include <soc.h>

struct mcux_lpsci_config {
	UART0_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct mcux_lpsci_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int mcux_lpsci_poll_in(const struct device *dev, unsigned char *c)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t flags = LPSCI_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kLPSCI_RxDataRegFullFlag) {
		*c = LPSCI_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void mcux_lpsci_poll_out(const struct device *dev, unsigned char c)
{
	const struct mcux_lpsci_config *config = dev->config;

	while (!(LPSCI_GetStatusFlags(config->base)
		& kLPSCI_TxDataRegEmptyFlag)) {
	}

	LPSCI_WriteByte(config->base, c);
}

static int mcux_lpsci_err_check(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t flags = LPSCI_GetStatusFlags(config->base);
	int err = 0;

	if (flags & kLPSCI_RxOverrunFlag) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & kLPSCI_ParityErrorFlag) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & kLPSCI_FramingErrorFlag) {
		err |= UART_ERROR_FRAMING;
	}

	LPSCI_ClearStatusFlags(config->base, kLPSCI_RxOverrunFlag |
					      kLPSCI_ParityErrorFlag |
					      kLPSCI_FramingErrorFlag);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int mcux_lpsci_fifo_fill(const struct device *dev,
				const uint8_t *tx_data,
				int len)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (LPSCI_GetStatusFlags(config->base)
		& kLPSCI_TxDataRegEmptyFlag)) {

		LPSCI_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int mcux_lpsci_fifo_read(const struct device *dev, uint8_t *rx_data,
				const int len)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (LPSCI_GetStatusFlags(config->base)
		& kLPSCI_RxDataRegFullFlag)) {

		rx_data[num_rx++] = LPSCI_ReadByte(config->base);
	}

	return num_rx;
}

static void mcux_lpsci_irq_tx_enable(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t mask = kLPSCI_TxDataRegEmptyInterruptEnable;

	LPSCI_EnableInterrupts(config->base, mask);
}

static void mcux_lpsci_irq_tx_disable(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t mask = kLPSCI_TxDataRegEmptyInterruptEnable;

	LPSCI_DisableInterrupts(config->base, mask);
}

static int mcux_lpsci_irq_tx_complete(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t flags = LPSCI_GetStatusFlags(config->base);

	return (flags & kLPSCI_TransmissionCompleteFlag) != 0U;
}

static int mcux_lpsci_irq_tx_ready(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t mask = kLPSCI_TxDataRegEmptyInterruptEnable;
	uint32_t flags = LPSCI_GetStatusFlags(config->base);

	return (LPSCI_GetEnabledInterrupts(config->base) & mask)
		&& (flags & kLPSCI_TxDataRegEmptyFlag);
}

static void mcux_lpsci_irq_rx_enable(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t mask = kLPSCI_RxDataRegFullInterruptEnable;

	LPSCI_EnableInterrupts(config->base, mask);
}

static void mcux_lpsci_irq_rx_disable(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t mask = kLPSCI_RxDataRegFullInterruptEnable;

	LPSCI_DisableInterrupts(config->base, mask);
}

static int mcux_lpsci_irq_rx_full(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t flags = LPSCI_GetStatusFlags(config->base);

	return (flags & kLPSCI_RxDataRegFullFlag) != 0U;
}

static int mcux_lpsci_irq_rx_ready(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t mask = kLPSCI_RxDataRegFullInterruptEnable;

	return (LPSCI_GetEnabledInterrupts(config->base) & mask)
		&& mcux_lpsci_irq_rx_full(dev);
}

static void mcux_lpsci_irq_err_enable(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t mask = kLPSCI_NoiseErrorInterruptEnable |
			kLPSCI_FramingErrorInterruptEnable |
			kLPSCI_ParityErrorInterruptEnable;

	LPSCI_EnableInterrupts(config->base, mask);
}

static void mcux_lpsci_irq_err_disable(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	uint32_t mask = kLPSCI_NoiseErrorInterruptEnable |
			kLPSCI_FramingErrorInterruptEnable |
			kLPSCI_ParityErrorInterruptEnable;

	LPSCI_DisableInterrupts(config->base, mask);
}

static int mcux_lpsci_irq_is_pending(const struct device *dev)
{
	return (mcux_lpsci_irq_tx_ready(dev)
		|| mcux_lpsci_irq_rx_ready(dev));
}

static int mcux_lpsci_irq_update(const struct device *dev)
{
	return 1;
}

static void mcux_lpsci_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct mcux_lpsci_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void mcux_lpsci_isr(const struct device *dev)
{
	struct mcux_lpsci_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int mcux_lpsci_init(const struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config;
	lpsci_config_t uart_config;
	const struct device *clock_dev;
	uint32_t clock_freq;

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPSCI_GetDefaultConfig(&uart_config);
	uart_config.enableTx = true;
	uart_config.enableRx = true;
	uart_config.baudRate_Bps = config->baud_rate;

	LPSCI_Init(config->base, &uart_config, clock_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api mcux_lpsci_driver_api = {
	.poll_in = mcux_lpsci_poll_in,
	.poll_out = mcux_lpsci_poll_out,
	.err_check = mcux_lpsci_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = mcux_lpsci_fifo_fill,
	.fifo_read = mcux_lpsci_fifo_read,
	.irq_tx_enable = mcux_lpsci_irq_tx_enable,
	.irq_tx_disable = mcux_lpsci_irq_tx_disable,
	.irq_tx_complete = mcux_lpsci_irq_tx_complete,
	.irq_tx_ready = mcux_lpsci_irq_tx_ready,
	.irq_rx_enable = mcux_lpsci_irq_rx_enable,
	.irq_rx_disable = mcux_lpsci_irq_rx_disable,
	.irq_rx_ready = mcux_lpsci_irq_rx_ready,
	.irq_err_enable = mcux_lpsci_irq_err_enable,
	.irq_err_disable = mcux_lpsci_irq_err_disable,
	.irq_is_pending = mcux_lpsci_irq_is_pending,
	.irq_update = mcux_lpsci_irq_update,
	.irq_callback_set = mcux_lpsci_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define MCUX_LPSCI_CONFIG_FUNC(n)					\
	static void mcux_lpsci_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    mcux_lpsci_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define MCUX_LPSCI_IRQ_CFG_FUNC_INIT(n)					\
	.irq_config_func = mcux_lpsci_config_func_##n
#define MCUX_LPSCI_INIT_CFG(n)						\
	MCUX_LPSCI_DECLARE_CFG(n, MCUX_LPSCI_IRQ_CFG_FUNC_INIT(n))
#else
#define MCUX_LPSCI_CONFIG_FUNC(n)
#define MCUX_LPSCI_IRQ_CFG_FUNC_INIT
#define MCUX_LPSCI_INIT_CFG(n)						\
	MCUX_LPSCI_DECLARE_CFG(n, MCUX_LPSCI_IRQ_CFG_FUNC_INIT)
#endif

#define MCUX_LPSCI_DECLARE_CFG(n, IRQ_FUNC_INIT)			\
static const struct mcux_lpsci_config mcux_lpsci_##n##_config = {	\
	.base = (UART0_Type *)DT_INST_REG_ADDR(n),			\
	.clock_name = DT_INST_CLOCKS_LABEL(n),				\
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
	.baud_rate = DT_INST_PROP(n, current_speed),			\
	IRQ_FUNC_INIT							\
}

#define MCUX_LPSCI_INIT(n)						\
									\
	static struct mcux_lpsci_data mcux_lpsci_##n##_data;		\
									\
	static const struct mcux_lpsci_config mcux_lpsci_##n##_config;	\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &mcux_lpsci_init,				\
			    device_pm_control_nop,			\
			    &mcux_lpsci_##n##_data,			\
			    &mcux_lpsci_##n##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &mcux_lpsci_driver_api);			\
									\
	MCUX_LPSCI_CONFIG_FUNC(n)					\
									\
	MCUX_LPSCI_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(MCUX_LPSCI_INIT)
