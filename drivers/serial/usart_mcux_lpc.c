/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief USART driver for LPC54XXX and LPC55xxx families.
 *
 * Note:
 * - The driver is implemented for only one device, multiple instances
 * will be implemented in the future.
 */

#include <errno.h>
#include <device.h>
#include <drivers/uart.h>
#include <fsl_usart.h>
#include <fsl_clock.h>
#include <soc.h>
#include <fsl_device_registers.h>

struct usart_mcux_lpc_config {
	USART_Type *base;
	u32_t clock_source;
	u32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

struct usart_mcux_lpc_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int usart_mcux_lpc_poll_in(struct device *dev, unsigned char *c)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t flags = USART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kUSART_RxFifoFullFlag) {
		*c = USART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void usart_mcux_lpc_poll_out(struct device *dev,
					     unsigned char c)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;

	/* Wait until space is available in TX FIFO */
	while (!(USART_GetStatusFlags(config->base) & kUSART_TxFifoEmptyFlag)) {
	}

	USART_WriteByte(config->base, c);
}

static int usart_mcux_lpc_err_check(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t flags = USART_GetStatusFlags(config->base);
	int err = 0;

	if (flags & kStatus_USART_RxRingBufferOverrun) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & kStatus_USART_ParityError) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & kStatus_USART_FramingError) {
		err |= UART_ERROR_FRAMING;
	}

	USART_ClearStatusFlags(config->base,
			       kStatus_USART_RxRingBufferOverrun |
			       kStatus_USART_ParityError |
			       kStatus_USART_FramingError);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int usart_mcux_lpc_fifo_fill(struct device *dev, const u8_t *tx_data,
			       int len)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (USART_GetStatusFlags(config->base)
		& kUSART_TxFifoNotFullFlag)) {

		USART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int usart_mcux_lpc_fifo_read(struct device *dev, u8_t *rx_data,
			       const int len)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (USART_GetStatusFlags(config->base)
		& kUSART_RxFifoNotEmptyFlag)) {

		rx_data[num_rx++] = USART_ReadByte(config->base);
	}

	return num_rx;
}

static void usart_mcux_lpc_irq_tx_enable(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t mask = kUSART_TxLevelInterruptEnable;

	USART_EnableInterrupts(config->base, mask);
}

static void usart_mcux_lpc_irq_tx_disable(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t mask = kUSART_TxLevelInterruptEnable;

	USART_DisableInterrupts(config->base, mask);
}

static int usart_mcux_lpc_irq_tx_complete(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t flags = USART_GetStatusFlags(config->base);

	return (flags & kUSART_TxFifoEmptyFlag) != 0U;
}

static int usart_mcux_lpc_irq_tx_ready(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t mask = kUSART_TxLevelInterruptEnable;

	return (USART_GetEnabledInterrupts(config->base) & mask)
		&& usart_mcux_lpc_irq_tx_complete(dev);
}

static void usart_mcux_lpc_irq_rx_enable(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t mask = kUSART_RxLevelInterruptEnable;

	USART_EnableInterrupts(config->base, mask);
}

static void usart_mcux_lpc_irq_rx_disable(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t mask = kUSART_RxLevelInterruptEnable;

	USART_DisableInterrupts(config->base, mask);
}

static int usart_mcux_lpc_irq_rx_full(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t flags = USART_GetStatusFlags(config->base);

	return (flags & kUSART_RxFifoNotEmptyFlag) != 0U;
}

static int usart_mcux_lpc_irq_rx_ready(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t mask = kUSART_RxLevelInterruptEnable;

	return (USART_GetEnabledInterrupts(config->base) & mask)
		&& usart_mcux_lpc_irq_rx_full(dev);
}

static void usart_mcux_lpc_irq_err_enable(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t mask = kStatus_USART_NoiseError |
			kStatus_USART_FramingError |
			kStatus_USART_ParityError;

	USART_EnableInterrupts(config->base, mask);
}

static void usart_mcux_lpc_irq_err_disable(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	u32_t mask = kStatus_USART_NoiseError |
			kStatus_USART_FramingError |
			kStatus_USART_ParityError;

	USART_DisableInterrupts(config->base, mask);
}

static int usart_mcux_lpc_irq_is_pending(struct device *dev)
{
	return (usart_mcux_lpc_irq_tx_ready(dev)
		|| usart_mcux_lpc_irq_rx_ready(dev));
}

static int usart_mcux_lpc_irq_update(struct device *dev)
{
	return 1;
}

static void usart_mcux_lpc_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct usart_mcux_lpc_data *data = dev->driver_data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void usart_mcux_lpc_isr(void *arg)
{
	struct device *dev = arg;
	struct usart_mcux_lpc_data *data = dev->driver_data;

	if (data->callback) {
		data->callback(data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static int usart_mcux_lpc_init(struct device *dev)
{
	const struct usart_mcux_lpc_config *config = dev->config->config_info;
	usart_config_t usart_config;
	u32_t clock_freq;

	clock_freq = CLOCK_GetFlexCommClkFreq(config->clock_source);

	USART_GetDefaultConfig(&usart_config);
	usart_config.enableTx = true;
	usart_config.enableRx = true;
	usart_config.baudRate_Bps = config->baud_rate;

	USART_Init(config->base, &usart_config, clock_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api usart_mcux_lpc_driver_api = {
	.poll_in = usart_mcux_lpc_poll_in,
	.poll_out = usart_mcux_lpc_poll_out,
	.err_check = usart_mcux_lpc_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_mcux_lpc_fifo_fill,
	.fifo_read = usart_mcux_lpc_fifo_read,
	.irq_tx_enable = usart_mcux_lpc_irq_tx_enable,
	.irq_tx_disable = usart_mcux_lpc_irq_tx_disable,
	.irq_tx_complete = usart_mcux_lpc_irq_tx_complete,
	.irq_tx_ready = usart_mcux_lpc_irq_tx_ready,
	.irq_rx_enable = usart_mcux_lpc_irq_rx_enable,
	.irq_rx_disable = usart_mcux_lpc_irq_rx_disable,
	.irq_rx_ready = usart_mcux_lpc_irq_rx_ready,
	.irq_err_enable = usart_mcux_lpc_irq_err_enable,
	.irq_err_disable = usart_mcux_lpc_irq_err_disable,
	.irq_is_pending = usart_mcux_lpc_irq_is_pending,
	.irq_update = usart_mcux_lpc_irq_update,
	.irq_callback_set = usart_mcux_lpc_irq_callback_set,
#endif
};

#ifdef CONFIG_USART_MCUX_LPC_0
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void usart_mcux_lpc_config_func_0(struct device *dev);
#endif

static const struct usart_mcux_lpc_config usart_mcux_lpc_0_config = {
	.base = (USART_Type *)DT_USART_MCUX_LPC_0_BASE_ADDRESS,
	.clock_source = 0,
	.baud_rate = DT_USART_MCUX_LPC_0_BAUD_RATE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = usart_mcux_lpc_config_func_0,
#endif
};

static struct usart_mcux_lpc_data usart_mcux_lpc_0_data;

DEVICE_AND_API_INIT(usart_0, DT_USART_MCUX_LPC_0_NAME,
		    &usart_mcux_lpc_init,
		    &usart_mcux_lpc_0_data, &usart_mcux_lpc_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &usart_mcux_lpc_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void usart_mcux_lpc_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_USART_MCUX_LPC_0_IRQ,
		    DT_USART_MCUX_LPC_0_IRQ_PRI,
		    usart_mcux_lpc_isr, DEVICE_GET(usart_0), 0);

	irq_enable(DT_USART_MCUX_LPC_0_IRQ);
}
#endif

#endif /* CONFIG_USART_MCUX_LPC_0 */
