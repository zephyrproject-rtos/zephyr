/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for UART port on STM32 family processor.
 *
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <soc.h>
#include <init.h>
#include <uart.h>
#include <clock_control.h>
#include <dma.h>
#include <linker/sections.h>
#include <clock_control/stm32_clock_control.h>
#include "uart_stm32.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_stm32);

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct uart_stm32_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct uart_stm32_data * const)(dev)->driver_data)
#define UART_STRUCT(dev)					\
	((USART_TypeDef *)(DEV_CFG(dev))->uconf.base)

static void uart_stm32_usart_set_baud_rate(struct device *dev,
					   u32_t clock_rate, u32_t baud_rate)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetBaudRate(UartInstance,
			     clock_rate,
#ifdef USART_PRESC_PRESCALER
			     LL_USART_PRESCALER_DIV1,
#endif
#ifdef USART_CR1_OVER8
			     LL_USART_OVERSAMPLING_16,
#endif
			     baud_rate);
}

#ifdef CONFIG_LPUART_1
static void uart_stm32_lpuart_set_baud_rate(struct device *dev,
					    u32_t clock_rate, u32_t baud_rate)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_LPUART_SetBaudRate(UartInstance,
			      clock_rate,
#ifdef USART_PRESC_PRESCALER
			      LL_USART_PRESCALER_DIV1,
#endif
			      baud_rate);
}
#endif	/* CONFIG_LPUART_1 */

static inline void uart_stm32_set_baudrate(struct device *dev, u32_t baud_rate)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
#ifdef CONFIG_LPUART_1
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
#endif
	u32_t clock_rate;

	/* Get clock rate */
	clock_control_get_rate(data->clock,
			       (clock_control_subsys_t *)&config->pclken,
			       &clock_rate);

#ifdef CONFIG_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		uart_stm32_lpuart_set_baud_rate(dev, clock_rate, baud_rate);
	} else {
		uart_stm32_usart_set_baud_rate(dev, clock_rate, baud_rate);
	}
#else
	uart_stm32_usart_set_baud_rate(dev, clock_rate, baud_rate);
#endif
}

static inline void uart_stm32_set_parity(struct device *dev, u32_t parity)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

#ifdef CONFIG_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		LL_LPUART_SetParity(UartInstance, parity);
	} else {
		LL_USART_SetParity(UartInstance, parity);
	}
#else
	LL_USART_SetParity(UartInstance, parity);
#endif	/* CONFIG_LPUART_1 */
}

static inline u32_t uart_stm32_get_parity(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

#ifdef CONFIG_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		return LL_LPUART_GetParity(UartInstance);
	} else {
		return LL_USART_GetParity(UartInstance);
	}
#else
	return LL_USART_GetParity(UartInstance);
#endif	/* CONFIG_LPUART_1 */
}

static inline void uart_stm32_set_stopbits(struct device *dev, u32_t stopbits)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

#ifdef CONFIG_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		LL_LPUART_SetStopBitsLength(UartInstance, stopbits);
	} else {
		LL_USART_SetStopBitsLength(UartInstance, stopbits);
	}
#else
	LL_USART_SetStopBitsLength(UartInstance, stopbits);
#endif	/* CONFIG_LPUART_1 */
}

static inline u32_t uart_stm32_get_stopbits(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

#ifdef CONFIG_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		return LL_LPUART_GetStopBitsLength(UartInstance);
	} else {
		return LL_USART_GetStopBitsLength(UartInstance);
	}
#else
	return LL_USART_GetStopBitsLength(UartInstance);
#endif	/* CONFIG_LPUART_1 */
}

static inline void uart_stm32_set_databits(struct device *dev, u32_t databits)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

#ifdef CONFIG_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		LL_LPUART_SetDataWidth(UartInstance, databits);
	} else {
		LL_USART_SetDataWidth(UartInstance, databits);
	}
#else
	LL_USART_SetDataWidth(UartInstance, databits);
#endif	/* CONFIG_LPUART_1 */
}

static inline u32_t uart_stm32_get_databits(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

#ifdef CONFIG_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		return LL_LPUART_GetDataWidth(UartInstance);
	} else {
		return LL_USART_GetDataWidth(UartInstance);
	}
#else
	return LL_USART_GetDataWidth(UartInstance);
#endif	/* CONFIG_LPUART_1 */
}

static inline u32_t uart_stm32_cfg2ll_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return LL_USART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return LL_USART_PARITY_EVEN;
	case UART_CFG_PARITY_NONE:
	default:
		return LL_USART_PARITY_NONE;
	}
}

static inline enum uart_config_parity uart_stm32_ll2cfg_parity(u32_t parity)
{
	switch (parity) {
	case LL_USART_PARITY_ODD:
		return UART_CFG_PARITY_ODD;
	case LL_USART_PARITY_EVEN:
		return UART_CFG_PARITY_EVEN;
	case LL_USART_PARITY_NONE:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static inline u32_t uart_stm32_cfg2ll_stopbits(enum uart_config_stop_bits sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef LL_USART_STOPBITS_0_5
	case UART_CFG_STOP_BITS_0_5:
		return LL_USART_STOPBITS_0_5;
#endif	/* LL_USART_STOPBITS_0_5 */
	case UART_CFG_STOP_BITS_1:
		return LL_USART_STOPBITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef LL_USART_STOPBITS_1_5
	case UART_CFG_STOP_BITS_1_5:
		return LL_USART_STOPBITS_1_5;
#endif	/* LL_USART_STOPBITS_1_5 */
	case UART_CFG_STOP_BITS_2:
	default:
		return LL_USART_STOPBITS_2;
	}
}

static inline enum uart_config_stop_bits uart_stm32_ll2cfg_stopbits(u32_t sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef LL_USART_STOPBITS_0_5
	case LL_USART_STOPBITS_0_5:
		return UART_CFG_STOP_BITS_0_5;
#endif	/* LL_USART_STOPBITS_0_5 */
	case LL_USART_STOPBITS_1:
		return UART_CFG_STOP_BITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef LL_USART_STOPBITS_1_5
	case LL_USART_STOPBITS_1_5:
		return UART_CFG_STOP_BITS_1_5;
#endif	/* LL_USART_STOPBITS_1_5 */
	case LL_USART_STOPBITS_2:
	default:
		return UART_CFG_STOP_BITS_2;
	}
}

static inline u32_t uart_stm32_cfg2ll_databits(enum uart_config_data_bits db)
{
	switch (db) {
/* Some MCU's don't support 7B datawidth */
#ifdef LL_USART_DATAWIDTH_7B
	case UART_CFG_DATA_BITS_7:
		return LL_USART_DATAWIDTH_7B;
#endif	/* LL_USART_DATAWIDTH_7B */
	case UART_CFG_DATA_BITS_8:
	default:
		return LL_USART_DATAWIDTH_8B;
	}
}

static inline enum uart_config_data_bits uart_stm32_ll2cfg_databits(u32_t db)
{
	switch (db) {
/* Some MCU's don't support 7B datawidth */
#ifdef LL_USART_DATAWIDTH_7B
	case LL_USART_DATAWIDTH_7B:
		return UART_CFG_DATA_BITS_7;
#endif	/* LL_USART_DATAWIDTH_7B */
	case LL_USART_DATAWIDTH_8B:
	default:
		return UART_CFG_DATA_BITS_8;
	}
}

static int uart_stm32_configure(struct device *dev,
				const struct uart_config *cfg)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	const u32_t parity = uart_stm32_cfg2ll_parity(cfg->parity);
	const u32_t stopbits = uart_stm32_cfg2ll_stopbits(cfg->stop_bits);
	const u32_t databits = uart_stm32_cfg2ll_databits(cfg->data_bits);

	/* Hardware doesn't support mark or space parity */
	if ((UART_CFG_PARITY_MARK == cfg->parity) ||
	    (UART_CFG_PARITY_SPACE == cfg->parity)) {
		return -ENOTSUP;
	}

#if defined(LL_USART_STOPBITS_0_5) && defined(CONFIG_LPUART_1)
	if (IS_LPUART_INSTANCE(UartInstance) &&
	    UART_CFG_STOP_BITS_0_5 == cfg->stop_bits) {
		return -ENOTSUP;
	}
#else
	if (UART_CFG_STOP_BITS_0_5 == cfg->stop_bits) {
		return -ENOTSUP;
	}
#endif

#if defined(LL_USART_STOPBITS_1_5) && defined(CONFIG_LPUART_1)
	if (IS_LPUART_INSTANCE(UartInstance) &&
	    UART_CFG_STOP_BITS_1_5 == cfg->stop_bits) {
		return -ENOTSUP;
	}
#else
	if (UART_CFG_STOP_BITS_1_5 == cfg->stop_bits) {
		return -ENOTSUP;
	}
#endif

	/* Driver doesn't support 5 or 6 databits and potentially 7 */
	if ((UART_CFG_DATA_BITS_5 == cfg->data_bits) ||
	    (UART_CFG_DATA_BITS_6 == cfg->data_bits)
#ifndef LL_USART_DATAWIDTH_7B
	    || (UART_CFG_DATA_BITS_7 == cfg->data_bits)
#endif /* LL_USART_DATAWIDTH_7B */
		) {
		return -ENOTSUP;
	}

	/* Driver currently doesn't support line ctrl */
	if (UART_CFG_FLOW_CTRL_NONE != cfg->flow_ctrl) {
		return -ENOTSUP;
	}

	LL_USART_Disable(UartInstance);

	if (parity != uart_stm32_get_parity(dev)) {
		uart_stm32_set_parity(dev, parity);
	}

	if (stopbits != uart_stm32_get_stopbits(dev)) {
		uart_stm32_set_stopbits(dev, stopbits);
	}

	if (databits != uart_stm32_get_databits(dev)) {
		uart_stm32_set_databits(dev, databits);
	}

	if (cfg->baudrate != data->baud_rate) {
		uart_stm32_set_baudrate(dev, cfg->baudrate);
		data->baud_rate = cfg->baudrate;
	}

	LL_USART_Enable(UartInstance);
	return 0;
};

static int uart_stm32_config_get(struct device *dev, struct uart_config *cfg)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	cfg->baudrate = data->baud_rate;
	cfg->parity = uart_stm32_ll2cfg_parity(uart_stm32_get_parity(dev));
	cfg->stop_bits = uart_stm32_ll2cfg_stopbits(
		uart_stm32_get_stopbits(dev));
	cfg->data_bits = uart_stm32_ll2cfg_databits(
		uart_stm32_get_databits(dev));
	cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	return 0;
}

static int uart_stm32_poll_in(struct device *dev, unsigned char *c)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Clear overrun error flag */
	if (LL_USART_IsActiveFlag_ORE(UartInstance)) {
		LL_USART_ClearFlag_ORE(UartInstance);
	}

	if (!LL_USART_IsActiveFlag_RXNE(UartInstance)) {
		return -1;
	}

	*c = (unsigned char)LL_USART_ReceiveData8(UartInstance);

	return 0;
}

static void uart_stm32_poll_out(struct device *dev,
					unsigned char c)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Wait for TXE flag to be raised */
	while (!LL_USART_IsActiveFlag_TXE(UartInstance))
		;

	LL_USART_ClearFlag_TC(UartInstance);

	LL_USART_TransmitData8(UartInstance, (u8_t)c);
}

static inline void __uart_stm32_get_clock(struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);

	data->clock = clk;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_stm32_fifo_fill(struct device *dev, const u8_t *tx_data,
				  int size)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	u8_t num_tx = 0U;

	while ((size - num_tx > 0) &&
	       LL_USART_IsActiveFlag_TXE(UartInstance)) {
		/* TXE flag will be cleared with byte write to DR register */

		/* Send a character (8bit , parity none) */
		LL_USART_TransmitData8(UartInstance, tx_data[num_tx++]);
	}

	return num_tx;
}

static int uart_stm32_fifo_read(struct device *dev, u8_t *rx_data,
				  const int size)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	u8_t num_rx = 0U;

	while ((size - num_rx > 0) &&
	       LL_USART_IsActiveFlag_RXNE(UartInstance)) {
#if defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32F4X) \
	|| defined(CONFIG_SOC_SERIES_STM32F2X)
		/* Clear the interrupt */
		LL_USART_ClearFlag_RXNE(UartInstance);
#endif

		/* Receive a character (8bit , parity none) */
		rx_data[num_rx++] = LL_USART_ReceiveData8(UartInstance);

		/* Clear overrun error flag */
		if (LL_USART_IsActiveFlag_ORE(UartInstance)) {
			LL_USART_ClearFlag_ORE(UartInstance);
		}

	}
	return num_rx;
}

static void uart_stm32_irq_tx_enable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableIT_TC(UartInstance);
}

static void uart_stm32_irq_tx_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableIT_TC(UartInstance);
}

static int uart_stm32_irq_tx_ready(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_TXE(UartInstance);
}

static int uart_stm32_irq_tx_complete(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_TXE(UartInstance);
}

static void uart_stm32_irq_rx_enable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableIT_RXNE(UartInstance);
}

static void uart_stm32_irq_rx_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableIT_RXNE(UartInstance);
}

static int uart_stm32_irq_rx_ready(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_RXNE(UartInstance);
}

static void uart_stm32_irq_err_enable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Enable FE, ORE interruptions */
	LL_USART_EnableIT_ERROR(UartInstance);
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Enable Line break detection */
	if (IS_UART_LIN_INSTANCE(UartInstance)) {
		LL_USART_EnableIT_LBD(UartInstance);
	}
#endif
	/* Enable parity error interruption */
	LL_USART_EnableIT_PE(UartInstance);
}

static void uart_stm32_irq_err_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Disable FE, ORE interruptions */
	LL_USART_DisableIT_ERROR(UartInstance);
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Disable Line break detection */
	if (IS_UART_LIN_INSTANCE(UartInstance)) {
		LL_USART_DisableIT_LBD(UartInstance);
	}
#endif
	/* Disable parity error interruption */
	LL_USART_DisableIT_PE(UartInstance);
}

static int uart_stm32_irq_is_pending(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return ((LL_USART_IsActiveFlag_RXNE(UartInstance) &&
		 LL_USART_IsEnabledIT_RXNE(UartInstance)) ||
		(LL_USART_IsActiveFlag_TC(UartInstance) &&
		 LL_USART_IsEnabledIT_TC(UartInstance)));
}

static int uart_stm32_irq_update(struct device *dev)
{
	return 1;
}

static void uart_stm32_irq_callback_set(struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API

static void user_callback(struct uart_stm32_data *data,
			  struct uart_event *event)
{
	if (data != NULL && data->async_cb) {
		data->async_cb(event, data->async_user_data);
	}
}

static int uart_stm32_callback_set(struct device *dev,
				  uart_callback_t callback,
				  void *user_data)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static void async_rx_rdy_evt(void *priv_data)
{
	struct uart_stm32_data *data = priv_data;

	struct uart_event event;
	size_t rx_cnt = data->rx_counter;

	event.type = UART_RX_RDY;
	event.data.rx.buf = data->rx_buffer;
	event.data.rx.len = rx_cnt;
	event.data.rx.offset = data->rx_offset;
	user_callback(priv_data, &event);
}

static void async_rx_err_evt(void *priv_data, int err_code)
{
	struct uart_stm32_data *data = priv_data;
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = err_code,
		.data.rx_stop.data.len = data->rx_counter,
		.data.rx_stop.data.offset = 0,
		.data.rx_stop.data.buf = data->rx_buffer
	};

	user_callback(data, &event);
}

static void async_tx_done_evt(void *priv_data)
{
	struct uart_stm32_data *data = priv_data;

	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = data->tx_buffer,
		.data.tx.len = data->tx_counter
	};

	user_callback(data, &event);

	/* Reset tx buffer */
	data->tx_buffer_length = 0;
	data->tx_counter = 0;


}


static int uart_stm32_dma_tx_enable(struct device *dev, bool enable)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	if (enable) {
		LL_USART_EnableDMAReq_TX(UartInstance);
	} else {
		LL_USART_DisableDMAReq_TX(UartInstance);
	}

	bool isEnabled = LL_USART_IsEnabledDMAReq_TX(UartInstance);

	return isEnabled == enable;
}

static inline int is_dma_rx_enabled(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsEnabledDMAReq_RX(UartInstance);
}

static int uart_stm32_dma_rx_enable(struct device *dev, bool enable)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	if (enable) {
		LL_USART_EnableDMAReq_RX(UartInstance);
	} else {
		LL_USART_DisableDMAReq_RX(UartInstance);
	}

	bool isEnabled = is_dma_rx_enabled(dev);

	return isEnabled == enable;
}

static int uart_stm32_dma_reload(struct device *dev, int id)
{
	int ret = -EINVAL;

	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);

	struct dma_block_config *dma_cfg = NULL;
	int channel_id = -1;

	if (id == config->channel_rx) {
		/* Reload RX buffer */
		dma_cfg = &data->rx_blk_cfg;
		channel_id = config->channel_rx;
		ret = 0;
	} else if (id == config->channel_tx) {
		dma_cfg = &data->tx_blk_cfg;
		channel_id = config->channel_tx;
		ret = 0;
	}

	if (ret == 0) {
		dma_reload(data->dma_dev, channel_id,
			   dma_cfg->source_address,
			   dma_cfg->dest_address,
			   dma_cfg->block_size);
	}
	return ret;
}


static void uart_stm32_dma_tx_cb(void *client_data, u32_t id, int ret_code)
{
	struct device *uart_dev = client_data;
	struct uart_stm32_data *data = DEV_DATA(uart_dev);

	unsigned int key = irq_lock();

	k_delayed_work_cancel(&data->tx_timeout_work);

	data->tx_buffer_length = 0;
	if (ret_code >= 0) {
		data->tx_counter = ret_code; /* remaining data */
	}

	async_tx_done_evt(data);

	irq_unlock(key);
}


static void uart_stm32_dma_rx_cb(void *client_data, u32_t id, int ret_code)
{
	struct device *dev = client_data;
	struct uart_stm32_data *data = DEV_DATA(dev);


	if (!data->rx_buffer_length || !is_dma_rx_enabled(dev)) {
		async_rx_err_evt(data, UART_ERROR_OVERRUN);
		return;
	}

	if (ret_code < 0) {
		async_rx_err_evt(data, ret_code);
		return;
	}

	unsigned int key = irq_lock();

	k_delayed_work_cancel(&data->rx_timeout_work);

	int pos = data->rx_buffer_length - ret_code;

	if (pos > data->rx_offset) {
		data->rx_counter = pos - data->rx_offset;
		async_rx_rdy_evt(data);
	} else if (pos < data->rx_offset) {
		data->rx_counter = data->rx_buffer_length - data->rx_offset;
		async_rx_rdy_evt(data);
	}

	data->rx_offset = pos;

	if (data->rx_offset == data->rx_buffer_length) {
		data->rx_offset = 0;
	}

	if (data->rx_enabled) {
		/* Start the timer again */
		if (data->rx_timeout != 0 && data->rx_timeout != K_FOREVER) {
			/* start RX timer */
			k_delayed_work_submit(&data->rx_timeout_work,
					data->rx_timeout);
		}
	}

	irq_unlock(key);
}

static int uart_stm32_tx(struct device *dev, const u8_t *tx_data,
			size_t buf_size, u32_t timeout)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	if (data->dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->tx_buffer_length != 0) {
		return -EBUSY;
	}

	data->tx_buffer = tx_data;
	data->tx_buffer_length = buf_size;
	data->tx_timeout = timeout;

	/* disable TX interrupt since DMA will handle it */
	LL_USART_DisableIT_TC(UartInstance);

	/* set source address */
	data->tx_blk_cfg.source_address = (u32_t)data->tx_buffer;
	data->tx_blk_cfg.block_size = data->tx_buffer_length;

	int ret = uart_stm32_dma_reload(dev, config->channel_tx);

	if (ret != 0) {
		LOG_ERR("dma reload error!");
		return -EINVAL;
	}

	if (dma_start(data->dma_dev, config->channel_tx)) {
		LOG_ERR("UART err: TX DMA start failed!");
		return -1;
	}

	if (timeout != 0 && timeout != K_FOREVER) {
		/* start TX timer */
		data->tx_timeout = timeout;
		k_delayed_work_submit(&data->tx_timeout_work,
				data->tx_timeout);
	}

	/* Enable TX DMA requests */
	return uart_stm32_dma_tx_enable(dev, true);
}

static int uart_stm32_rx_enable(struct device *dev, u8_t *rx_buf,
				size_t buf_size, u32_t timeout)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	if (data->dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->rx_enabled) {
		LOG_INF("RX was already enabled");
		return 0;

	}

	LOG_INF("DMA rx: %d(%d,%d)", config->dma_slot,
			config->channel_tx, config->channel_rx);

	data->rx_offset = 0;
	data->rx_buffer = rx_buf;
	data->rx_buffer_length = buf_size;
	data->rx_counter = 0;
	data->rx_timeout = timeout;

	/* Disable RX inerrupts to let DMA to handle it */
	LL_USART_DisableIT_RXNE(UartInstance);

	/* Enable IRQ IDLE to define the end of a
	 * RX DMA transaction.
	 */
	LL_USART_EnableIT_IDLE(UartInstance);

	data->rx_blk_cfg.block_size = buf_size;
	data->rx_blk_cfg.dest_address = (u32_t)data->rx_buffer;

	int ret = uart_stm32_dma_reload(dev, config->channel_rx);

	if (ret != 0) {
		LOG_ERR("dma reload error!");
		return -EINVAL;
	}


	if (dma_start(data->dma_dev, config->channel_rx)) {
		LOG_ERR("UART ERR: RX DMA start failed!");
		return -1;
	}

	/* Enable RX DMA requests */
	ret = uart_stm32_dma_rx_enable(dev, true);
	data->rx_enabled = ret == 0 ? true : false;

	return ret;
}

static int uart_stm32_tx_abort(struct device *dev)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);

	if (data->tx_buffer_length == 0) {
		return -EFAULT;
	}

	k_delayed_work_cancel(&data->tx_timeout_work);

	data->tx_counter = dma_pending(data->dma_dev, config->channel_tx);

	dma_stop(data->dma_dev, config->channel_tx);

	async_tx_done_evt(data);

	return 0;
}

static int uart_stm32_rx_disable(struct device *dev)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);

	if (data->rx_buffer_length == 0) {
		return -EFAULT;
	}

	k_delayed_work_cancel(&data->rx_timeout_work);

	dma_stop(data->dma_dev, config->channel_rx);

	data->rx_enabled = false;

	struct uart_event disabled_event = {
		.type = UART_RX_DISABLED
	};

	user_callback(data, &disabled_event);

	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->rx_buffer
	};
	user_callback(data, &event);

	return 0;
}

static void uart_async_rx_timeout(struct k_work *work)
{
	struct uart_stm32_data *data = CONTAINER_OF(work,
				struct uart_stm32_data, rx_timeout_work);

	struct device *dev = CONTAINER_OF(data, struct device, driver_data);

	uart_stm32_rx_disable(dev);
}

static void uart_async_tx_timeout(struct k_work *work)
{
	struct uart_stm32_data *data = CONTAINER_OF(work,
				struct uart_stm32_data, tx_timeout_work);

	struct device *dev = CONTAINER_OF(data, struct device, driver_data);

	uart_stm32_tx_abort(dev);
}

static int uart_stm32_rx_buf_rsp(struct device *dev, u8_t *buf, size_t len)
{
	/* STM32 UART doesn't use double buffers */
	return -EINVAL;
}

#endif /* CONFIG_UART_ASYNC_API */


#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
static void uart_stm32_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_stm32_data *data = DEV_DATA(dev);

#ifdef CONFIG_UART_ASYNC_API
	const struct uart_stm32_config *config = DEV_CFG(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	if (LL_USART_IsEnabledIT_IDLE(UartInstance)
	   && LL_USART_IsActiveFlag_IDLE(UartInstance)) {
		LL_USART_ClearFlag_IDLE(UartInstance);
		int remain = dma_pending(data->dma_dev, config->channel_rx);

		uart_stm32_dma_rx_cb(dev, config->channel_rx, remain);
	}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(data->user_data);
	}
#endif
}

#endif

static const struct uart_driver_api uart_stm32_driver_api = {
	.poll_in = uart_stm32_poll_in,
	.poll_out = uart_stm32_poll_out,
	.configure = uart_stm32_configure,
	.config_get = uart_stm32_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_stm32_fifo_fill,
	.fifo_read = uart_stm32_fifo_read,
	.irq_tx_enable = uart_stm32_irq_tx_enable,
	.irq_tx_disable = uart_stm32_irq_tx_disable,
	.irq_tx_ready = uart_stm32_irq_tx_ready,
	.irq_tx_complete = uart_stm32_irq_tx_complete,
	.irq_rx_enable = uart_stm32_irq_rx_enable,
	.irq_rx_disable = uart_stm32_irq_rx_disable,
	.irq_rx_ready = uart_stm32_irq_rx_ready,
	.irq_err_enable = uart_stm32_irq_err_enable,
	.irq_err_disable = uart_stm32_irq_err_disable,
	.irq_is_pending = uart_stm32_irq_is_pending,
	.irq_update = uart_stm32_irq_update,
	.irq_callback_set = uart_stm32_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set     = uart_stm32_callback_set,
	.tx               = uart_stm32_tx,
	.tx_abort         = uart_stm32_tx_abort,
	.rx_enable        = uart_stm32_rx_enable,
	.rx_disable       = uart_stm32_rx_disable,
	.rx_buf_rsp       = uart_stm32_rx_buf_rsp,
#endif /* CONFIG_UART_ASYNC_API */

};

#ifdef CONFIG_UART_ASYNC_API
static int uart_stm32_async_init(struct device *dev)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	const char *dma_dev_name = CONFIG_DMA_1_NAME;

	if (config->pclken.bus == STM32_CLOCK_BUS_APB2) {
		dma_dev_name = CONFIG_DMA_2_NAME;
	}

	data->dma_dev = device_get_binding(dma_dev_name);
	if (data->dma_dev == NULL) {
		LOG_ERR("%s device not found", dma_dev_name);
		return -ENODEV;
	}
	LOG_INF("DMA device %s opened", dma_dev_name);

	/* Disable both TX and RX DMA requests */
	uart_stm32_dma_rx_enable(dev, false);
	uart_stm32_dma_tx_enable(dev, false);

	k_delayed_work_init(&data->rx_timeout_work, uart_async_rx_timeout);
	k_delayed_work_init(&data->tx_timeout_work, uart_async_tx_timeout);

	/* Configure dma rx config */
	memset(&data->rx_blk_cfg, 0, sizeof(data->rx_blk_cfg));
	data->rx_blk_cfg.source_address = LL_USART_DMA_GetRegAddr(UartInstance);

	struct dma_config dma_rxcfg = {
		.dma_slot = config->dma_slot,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.source_data_size = 0,
		.dest_data_size = 0,
		.source_burst_length = 4,
		.dest_burst_length = 4,
		.complete_callback_en = 0,
		.error_callback_en = 1,
		.block_count = 1,
		.channel_priority = 1,
		.head_block = &data->rx_blk_cfg,
		.dma_callback = uart_stm32_dma_rx_cb,
		.callback_arg = dev,
		.buf_cfg = DMA_BUF_CIRCULAR,
	};

	if (dma_config(data->dma_dev, config->channel_rx, &dma_rxcfg)) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EIO;
	}

	/* Configure dma tx config */
	memset(&data->tx_blk_cfg, 0, sizeof(data->tx_blk_cfg));
	data->tx_blk_cfg.dest_address = LL_USART_DMA_GetRegAddr(UartInstance);
	struct dma_config dma_txcfg = {
		.dma_slot = config->dma_slot,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = 0,
		.dest_data_size = 0,
		.source_burst_length = 4,
		.dest_burst_length = 4,
		.complete_callback_en = 0,
		.error_callback_en = 1,
		.block_count = 1,
		.channel_priority = 1,
		.head_block = &data->tx_blk_cfg,
		.dma_callback = uart_stm32_dma_tx_cb,
		.callback_arg = dev,
	};

	if (dma_config(data->dma_dev, config->channel_tx, &dma_txcfg)) {
		LOG_ERR("UART err: TX DMA config failed!");
		return -EIO;
	}

	return 0;
}
#endif

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_stm32_init(struct device *dev)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	__uart_stm32_get_clock(dev);
	/* enable clock */
	if (clock_control_on(data->clock,
			(clock_control_subsys_t *)&config->pclken) != 0) {
		return -EIO;
	}

	LL_USART_Disable(UartInstance);

	/* TX/RX direction */
	LL_USART_SetTransferDirection(UartInstance,
				      LL_USART_DIRECTION_TX_RX);

	/* 8 data bit, 1 start bit, 1 stop bit, no parity */
	LL_USART_ConfigCharacter(UartInstance,
				 LL_USART_DATAWIDTH_8B,
				 LL_USART_PARITY_NONE,
				 LL_USART_STOPBITS_1);

	/* Set the default baudrate */
	uart_stm32_set_baudrate(dev, data->baud_rate);

	LL_USART_Enable(UartInstance);

#ifdef USART_ISR_TEACK
	/* Wait until TEACK flag is set */
	while (!(LL_USART_IsActiveFlag_TEACK(UartInstance)))
		;
#endif /* !USART_ISR_TEACK */

#ifdef USART_ISR_REACK
	/* Wait until REACK flag is set */
	while (!(LL_USART_IsActiveFlag_REACK(UartInstance)))
		;
#endif /* !USART_ISR_REACK */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	config->uconf.irq_config_func(dev);
#endif

	int ret = 0;
#ifdef CONFIG_UART_ASYNC_API
	ret = uart_stm32_async_init(dev);
#endif

	return ret;
}


#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define STM32_UART_IRQ_HANDLER_DECL(name)				\
	static void uart_stm32_irq_config_func_##name(struct device *dev)
#define STM32_UART_IRQ_HANDLER_FUNC(name)				\
	.irq_config_func = uart_stm32_irq_config_func_##name,
#define STM32_UART_IRQ_HANDLER(name)					\
static void uart_stm32_irq_config_func_##name(struct device *dev)	\
{									\
	IRQ_CONNECT(DT_##name##_IRQ,					\
		DT_UART_STM32_##name##_IRQ_PRI,			\
		uart_stm32_isr, DEVICE_GET(uart_stm32_##name),	\
		0);							\
	irq_enable(DT_##name##_IRQ);					\
}
#else
#define STM32_UART_IRQ_HANDLER_DECL(name)
#define STM32_UART_IRQ_HANDLER_FUNC(name)
#define STM32_UART_IRQ_HANDLER(name)
#endif

#ifdef CONFIG_UART_ASYNC_API
#define STM32_UART_DMA_CFG(name)					\
	.dma_slot   = DT_UART_STM32_##name##_DMA_SLOT,			\
	.channel_tx = DT_UART_STM32_##name##_DMA_TX,			\
	.channel_rx = DT_UART_STM32_##name##_DMA_RX,
#else
#define STM32_UART_DMA_CFG(name)
#endif

#define STM32_UART_INIT(name)						\
STM32_UART_IRQ_HANDLER_DECL(name);					\
									\
static const struct uart_stm32_config uart_stm32_cfg_##name = {		\
	.uconf = {							\
		.base = (u8_t *)DT_UART_STM32_##name##_BASE_ADDRESS,\
		STM32_UART_IRQ_HANDLER_FUNC(name)			\
	},								\
	.pclken = { .bus = DT_UART_STM32_##name##_CLOCK_BUS,	\
		    .enr = DT_UART_STM32_##name##_CLOCK_BITS	\
	},								\
	STM32_UART_DMA_CFG(name)					\
};									\
									\
static struct uart_stm32_data uart_stm32_data_##name = {		\
	.baud_rate = DT_UART_STM32_##name##_BAUD_RATE			\
};									\
									\
DEVICE_AND_API_INIT(uart_stm32_##name, DT_UART_STM32_##name##_NAME,	\
		    &uart_stm32_init,					\
		    &uart_stm32_data_##name, &uart_stm32_cfg_##name,	\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &uart_stm32_driver_api);				\
									\
STM32_UART_IRQ_HANDLER(name)


#ifdef CONFIG_UART_1
STM32_UART_INIT(USART_1)
#endif	/* CONFIG_UART_1 */

#ifdef CONFIG_UART_2
STM32_UART_INIT(USART_2)
#endif	/* CONFIG_UART_2 */

#ifdef CONFIG_UART_3
STM32_UART_INIT(USART_3)
#endif	/* CONFIG_UART_3 */

#ifdef CONFIG_UART_6
STM32_UART_INIT(USART_6)
#endif /* CONFIG_UART_6 */

/*
 * STM32F0 and STM32L0 series differ from other STM32 series by some
 * peripheral names (UART vs USART).
 */
#if defined(CONFIG_SOC_SERIES_STM32F0X) || defined(CONFIG_SOC_SERIES_STM32L0X)

#ifdef CONFIG_UART_4
STM32_UART_INIT(USART_4)
#endif /* CONFIG_UART_4 */

#ifdef CONFIG_UART_5
STM32_UART_INIT(USART_5)
#endif /* CONFIG_UART_5 */

/* Following devices are not available in L0 series (for now)
 * But keeping them simplifies ifdefery and won't harm
 */

#ifdef CONFIG_UART_7
STM32_UART_INIT(USART_7)
#endif /* CONFIG_UART_7 */

#ifdef CONFIG_UART_8
STM32_UART_INIT(USART_8)
#endif /* CONFIG_UART_8 */

#else

#ifdef CONFIG_UART_4
STM32_UART_INIT(UART_4)
#endif /* CONFIG_UART_4 */

#ifdef CONFIG_UART_5
STM32_UART_INIT(UART_5)
#endif /* CONFIG_UART_5 */

#ifdef CONFIG_UART_7
STM32_UART_INIT(UART_7)
#endif /* CONFIG_UART_7 */

#ifdef CONFIG_UART_8
STM32_UART_INIT(UART_8)
#endif /* CONFIG_UART_8 */

#ifdef CONFIG_UART_9
STM32_UART_INIT(UART_9)
#endif /* CONFIG_UART_9 */

#ifdef CONFIG_UART_10
STM32_UART_INIT(UART_10)
#endif /* CONFIG_UART_10 */

#endif

#if defined(CONFIG_SOC_SERIES_STM32L4X) || defined(CONFIG_SOC_SERIES_STM32L0X)
#ifdef CONFIG_LPUART_1
STM32_UART_INIT(LPUART_1)
#endif /* CONFIG_LPUART_1 */
#endif
