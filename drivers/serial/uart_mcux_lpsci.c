/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
	u32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

struct mcux_lpsci_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int mcux_lpsci_poll_in(struct device *dev, unsigned char *c)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t flags = LPSCI_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kLPSCI_RxDataRegFullFlag) {
		*c = LPSCI_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void mcux_lpsci_poll_out(struct device *dev, unsigned char c)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;

	while (!(LPSCI_GetStatusFlags(config->base)
		& kLPSCI_TxDataRegEmptyFlag)) {
	}

	LPSCI_WriteByte(config->base, c);
}

static int mcux_lpsci_err_check(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t flags = LPSCI_GetStatusFlags(config->base);
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
static int mcux_lpsci_fifo_fill(struct device *dev, const u8_t *tx_data,
				int len)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (LPSCI_GetStatusFlags(config->base)
		& kLPSCI_TxDataRegEmptyFlag)) {

		LPSCI_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int mcux_lpsci_fifo_read(struct device *dev, u8_t *rx_data,
				const int len)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (LPSCI_GetStatusFlags(config->base)
		& kLPSCI_RxDataRegFullFlag)) {

		rx_data[num_rx++] = LPSCI_ReadByte(config->base);
	}

	return num_rx;
}

static void mcux_lpsci_irq_tx_enable(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t mask = kLPSCI_TxDataRegEmptyInterruptEnable;

	LPSCI_EnableInterrupts(config->base, mask);
}

static void mcux_lpsci_irq_tx_disable(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t mask = kLPSCI_TxDataRegEmptyInterruptEnable;

	LPSCI_DisableInterrupts(config->base, mask);
}

static int mcux_lpsci_irq_tx_complete(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t flags = LPSCI_GetStatusFlags(config->base);

	return (flags & kLPSCI_TxDataRegEmptyFlag) != 0U;
}

static int mcux_lpsci_irq_tx_ready(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t mask = kLPSCI_TxDataRegEmptyInterruptEnable;

	return (LPSCI_GetEnabledInterrupts(config->base) & mask)
		&& mcux_lpsci_irq_tx_complete(dev);
}

static void mcux_lpsci_irq_rx_enable(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t mask = kLPSCI_RxDataRegFullInterruptEnable;

	LPSCI_EnableInterrupts(config->base, mask);
}

static void mcux_lpsci_irq_rx_disable(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t mask = kLPSCI_RxDataRegFullInterruptEnable;

	LPSCI_DisableInterrupts(config->base, mask);
}

static int mcux_lpsci_irq_rx_full(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t flags = LPSCI_GetStatusFlags(config->base);

	return (flags & kLPSCI_RxDataRegFullFlag) != 0U;
}

static int mcux_lpsci_irq_rx_ready(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t mask = kLPSCI_RxDataRegFullInterruptEnable;

	return (LPSCI_GetEnabledInterrupts(config->base) & mask)
		&& mcux_lpsci_irq_rx_full(dev);
}

static void mcux_lpsci_irq_err_enable(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t mask = kLPSCI_NoiseErrorInterruptEnable |
			kLPSCI_FramingErrorInterruptEnable |
			kLPSCI_ParityErrorInterruptEnable;

	LPSCI_EnableInterrupts(config->base, mask);
}

static void mcux_lpsci_irq_err_disable(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	u32_t mask = kLPSCI_NoiseErrorInterruptEnable |
			kLPSCI_FramingErrorInterruptEnable |
			kLPSCI_ParityErrorInterruptEnable;

	LPSCI_DisableInterrupts(config->base, mask);
}

static int mcux_lpsci_irq_is_pending(struct device *dev)
{
	return (mcux_lpsci_irq_tx_ready(dev)
		|| mcux_lpsci_irq_rx_ready(dev));
}

static int mcux_lpsci_irq_update(struct device *dev)
{
	return 1;
}

static void mcux_lpsci_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct mcux_lpsci_data *data = dev->driver_data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void mcux_lpsci_isr(void *arg)
{
	struct device *dev = arg;
	struct mcux_lpsci_data *data = dev->driver_data;

	if (data->callback) {
		data->callback(data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int mcux_lpsci_init(struct device *dev)
{
	const struct mcux_lpsci_config *config = dev->config->config_info;
	lpsci_config_t uart_config;
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

#ifdef CONFIG_UART_MCUX_LPSCI_0

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void mcux_lpsci_config_func_0(struct device *dev);
#endif

static const struct mcux_lpsci_config mcux_lpsci_0_config = {
	.base = UART0,
	.clock_name = DT_ALIAS_UART_0_CLOCK_CONTROLLER,
	.clock_subsys =
		(clock_control_subsys_t)DT_ALIAS_UART_0_CLOCK_NAME,
	.baud_rate = DT_ALIAS_UART_0_CURRENT_SPEED,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = mcux_lpsci_config_func_0,
#endif
};

static struct mcux_lpsci_data mcux_lpsci_0_data;

DEVICE_AND_API_INIT(uart_0, DT_ALIAS_UART_0_LABEL,
		    &mcux_lpsci_init,
		    &mcux_lpsci_0_data, &mcux_lpsci_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_lpsci_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void mcux_lpsci_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_UART_0_IRQ_0,
		    DT_ALIAS_UART_0_IRQ_0_PRIORITY,
		    mcux_lpsci_isr, DEVICE_GET(uart_0), 0);

	irq_enable(DT_ALIAS_UART_0_IRQ_0);
}
#endif

#endif /* CONFIG_UART_MCUX_LPSCI_0 */
