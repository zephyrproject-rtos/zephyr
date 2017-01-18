/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <uart.h>
#include <fsl_lpuart.h>
#include <fsl_clock.h>
#include <soc.h>

struct mcux_lpuart_config {
	LPUART_Type *base;
	clock_name_t clock_source;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

struct mcux_lpuart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t callback;
#endif
};

static int mcux_lpuart_poll_in(struct device *dev, unsigned char *c)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t flags = LPUART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kLPUART_RxDataRegFullFlag) {
		*c = LPUART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static unsigned char mcux_lpuart_poll_out(struct device *dev, unsigned char c)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;

	while (!(LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag))
		;

	LPUART_WriteByte(config->base, c);

	return c;
}

static int mcux_lpuart_err_check(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
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
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint8_t num_tx = 0;

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
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint8_t num_rx = 0;

	while ((len - num_rx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_RxDataRegFullFlag)) {

		rx_data[num_rx++] = LPUART_ReadByte(config->base);
	}

	return num_rx;
}

static void mcux_lpuart_irq_tx_enable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void mcux_lpuart_irq_tx_disable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int mcux_lpuart_irq_tx_empty(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_TxDataRegEmptyFlag) != 0;
}

static int mcux_lpuart_irq_tx_ready(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& mcux_lpuart_irq_tx_empty(dev);
}

static void mcux_lpuart_irq_rx_enable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void mcux_lpuart_irq_rx_disable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int mcux_lpuart_irq_rx_full(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_RxDataRegFullFlag) != 0;
}

static int mcux_lpuart_irq_rx_ready(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& mcux_lpuart_irq_rx_full(dev);
}

static void mcux_lpuart_irq_err_enable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void mcux_lpuart_irq_err_disable(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
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
				       uart_irq_callback_t cb)
{
	struct mcux_lpuart_data *data = dev->driver_data;

	data->callback = cb;
}

static void mcux_lpuart_isr(void *arg)
{
	struct device *dev = arg;
	struct mcux_lpuart_data *data = dev->driver_data;

	if (data->callback) {
		data->callback(dev);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int mcux_lpuart_init(struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config->config_info;
	lpuart_config_t uart_config;
	uint32_t clock_freq;

	clock_freq = CLOCK_GetFreq(config->clock_source);

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
	.irq_tx_empty = mcux_lpuart_irq_tx_empty,
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

#ifdef CONFIG_UART_MCUX_LPUART_0

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void mcux_lpuart_config_func_0(struct device *dev);
#endif

static const struct mcux_lpuart_config mcux_lpuart_0_config = {
	.base = LPUART0,
	.clock_source = LPUART0_CLK_SRC,
	.baud_rate = CONFIG_UART_MCUX_LPUART_0_BAUD_RATE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = mcux_lpuart_config_func_0,
#endif
};

static struct mcux_lpuart_data mcux_lpuart_0_data;

DEVICE_AND_API_INIT(uart_0, CONFIG_UART_MCUX_LPUART_0_NAME,
		    &mcux_lpuart_init,
		    &mcux_lpuart_0_data, &mcux_lpuart_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_lpuart_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void mcux_lpuart_config_func_0(struct device *dev)
{
	IRQ_CONNECT(IRQ_LPUART0, CONFIG_UART_MCUX_LPUART_0_IRQ_PRI,
		    mcux_lpuart_isr, DEVICE_GET(uart_0), 0);

	irq_enable(IRQ_LPUART0);
}
#endif

#endif /* CONFIG_UART_MCUX_LPUART_0 */
