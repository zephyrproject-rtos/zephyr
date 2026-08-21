/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Realtek Ameba UART interface
 */
#define DT_DRV_COMPAT realtek_ameba_uart

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/cache.h>
#include "dma_ameba_gdma.h"
#include <zephyr/drivers/dma.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_ameba, CONFIG_UART_LOG_LEVEL);

#ifdef CONFIG_UART_ASYNC_API

struct uart_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	uint8_t *buffer;
	size_t buffer_length;
	size_t offset;
	volatile size_t counter;
	int32_t timeout;
	struct k_work_delayable timeout_work;
	bool enabled;
};
#endif

struct uart_ameba_config {
	UART_TypeDef *uart;
	const struct device *clock_dev;
	const struct pinctrl_dev_config *pcfg;
	const clock_control_subsys_t clock_subsys;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
#endif
};

/* driver data */
struct uart_ameba_data {
	const struct device *dev;
	struct uart_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
	bool tx_int_en;
	bool rx_int_en;
#endif
#ifdef CONFIG_UART_ASYNC_API
	uart_callback_t async_cb;
	void *async_user_data;
	struct uart_dma_stream dma_rx;
	struct uart_dma_stream dma_tx;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_len;
#if !defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	uint32_t last_dat_cnt;
	uint32_t hang_cnt;
#endif
#endif
};

static int uart_ameba_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	if (!UART_Readable(uart)) {
		return -1;
	}

	UART_CharGet(uart, (uint8_t *)c);

	return 0;
}

static void uart_ameba_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	while (!UART_Writable(uart)) {
	}

	UART_CharPut(uart, (uint8_t)c);
}

static int uart_ameba_err_check(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	uint32_t lsr = UART_LineStatusGet(uart);
	uint32_t err = 0U;

	/* Clear error flags first */
	if (lsr & UART_ALL_RX_ERR) {
		UART_INT_Clear(uart, RUART_BIT_RLSICF);
	}

	/* Check for errors */
	if (lsr & RUART_BIT_OVR_ERR) {
		err |= UART_ERROR_OVERRUN;
	}

	if (lsr & RUART_BIT_PAR_ERR) {
		err |= UART_ERROR_PARITY;
	}

	if (lsr & RUART_BIT_FRM_ERR) {
		err |= UART_ERROR_FRAMING;
	}

	if (lsr & RUART_BIT_BREAK_INT) {
		err |= UART_BREAK;
	}

	return err;
}

static int rtl_cfg_parity(UART_InitTypeDef *uart_struct, uint32_t parity)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		uart_struct->Parity = RUART_PARITY_DISABLE;
		break;
	case UART_CFG_PARITY_ODD:
		uart_struct->Parity = RUART_PARITY_ENABLE;
		uart_struct->ParityType = RUART_ODD_PARITY;
		uart_struct->StickParity = RUART_STICK_PARITY_DISABLE;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_struct->Parity = RUART_PARITY_ENABLE;
		uart_struct->ParityType = RUART_EVEN_PARITY;
		uart_struct->StickParity = RUART_STICK_PARITY_DISABLE;
		break;
	case UART_CFG_PARITY_MARK:
		uart_struct->Parity = RUART_PARITY_ENABLE;
		uart_struct->ParityType = RUART_ODD_PARITY;
		uart_struct->StickParity = RUART_STICK_PARITY_ENABLE;
		break;
	case UART_CFG_PARITY_SPACE:
		uart_struct->Parity = RUART_PARITY_ENABLE;
		uart_struct->ParityType = RUART_EVEN_PARITY;
		uart_struct->StickParity = RUART_STICK_PARITY_ENABLE;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rtl_cfg_data_bits(UART_InitTypeDef *uart_struct, uint32_t data_bits)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_7:
		uart_struct->WordLen = RUART_WLS_7BITS;
		break;
	case UART_CFG_DATA_BITS_8:
		uart_struct->WordLen = RUART_WLS_8BITS;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rtl_cfg_stop_bits(UART_InitTypeDef *uart_struct, uint32_t stop_bits)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_struct->StopBit = RUART_STOP_BIT_1;
		break;
	case UART_CFG_STOP_BITS_2:
		uart_struct->StopBit = RUART_STOP_BIT_2;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rtl_cfg_flow_ctrl(UART_InitTypeDef *uart_struct, uint32_t flow_ctrl)
{
	switch (flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uart_struct->FlowControl = DISABLE;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		uart_struct->FlowControl = ENABLE;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int uart_ameba_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	int ret;

	UART_InitTypeDef UART_InitStruct;
	UART_TypeDef *uart = config->uart;

	UART_StructInit(&UART_InitStruct);

	ret = rtl_cfg_parity(&UART_InitStruct, cfg->parity);
	if (ret < 0) {
		LOG_ERR("Unsupported UART Parity Type (%d)", ret);
		return ret;
	}

	ret = rtl_cfg_data_bits(&UART_InitStruct, cfg->data_bits);
	if (ret < 0) {
		LOG_ERR("Unsupported UART Data Bit (%d)", ret);
		return ret;
	}

	ret = rtl_cfg_stop_bits(&UART_InitStruct, cfg->stop_bits);
	if (ret < 0) {
		LOG_ERR("Unsupported UART Stop Bit (%d)", ret);
		return ret;
	}

	ret = rtl_cfg_flow_ctrl(&UART_InitStruct, cfg->flow_ctrl);
	if (ret < 0) {
		LOG_ERR("Unsupported UART Flow Control (%d)", ret);
		return ret;
	}

	UART_Init(uart, &UART_InitStruct);
	UART_SetBaud(uart, cfg->baudrate);
	UART_RxCmd(uart, ENABLE);

	data->uart_config = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_ameba_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_ameba_data *data = dev->data;

	*cfg = data->uart_config;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_ameba_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	uint8_t num_tx = 0U;
	unsigned int key;

	if (!UART_Writable(uart)) {
		return num_tx;
	}

	/* Lock interrupts to prevent nested interrupts or thread switch */

	key = irq_lock();

	while ((len - num_tx > 0) && UART_Writable(uart)) {
		UART_CharPut(uart, (uint8_t)tx_data[num_tx++]);
	}

	irq_unlock(key);

	return num_tx;
}

static int uart_ameba_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) && UART_Readable(uart)) {
		UART_CharGet(uart, (uint8_t *)(rx_data + num_rx++));
	}

	/* Clear timeout int flag */
	if (UART_LineStatusGet(uart) & RUART_BIT_TIMEOUT_INT) {
		UART_INT_Clear(uart, RUART_BIT_TOICF);
	}

	return num_rx;
}

static void uart_ameba_irq_tx_enable(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	uint32_t sts;

	sts = irq_disable_save();

	data->tx_int_en = true;
	UART_INTConfig(uart, RUART_BIT_ETBEI, ENABLE);

	/* Enable IRQ Interrupts according to Previous Status. */
	irq_enable_restore(sts);
}

static void uart_ameba_irq_tx_disable(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	uint32_t sts;

	sts = irq_disable_save();

	data->tx_int_en = false;
	UART_INTConfig(uart, RUART_BIT_ETBEI, DISABLE);

	/* Enable IRQ Interrupts according to Previous Status. */
	irq_enable_restore(sts);
}

static int uart_ameba_irq_tx_ready(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	return (UART_LineStatusGet(uart) & RUART_BIT_TX_EMPTY) && data->tx_int_en;
}

static int uart_ameba_irq_tx_complete(const struct device *dev)
{
	return uart_ameba_irq_tx_ready(dev);
}

static void uart_ameba_irq_rx_enable(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	data->rx_int_en = true;
	UART_INTConfig(uart, RUART_BIT_ERBI | RUART_BIT_ETOI, ENABLE);
}

static void uart_ameba_irq_rx_disable(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	data->rx_int_en = false;
	UART_INTConfig(uart, RUART_BIT_ERBI | RUART_BIT_ETOI, DISABLE);
}

static int uart_ameba_irq_rx_ready(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	return (UART_LineStatusGet(uart) & (RUART_BIT_RXFIFO_INT | RUART_BIT_TIMEOUT_INT)) &&
	       data->rx_int_en;
}

static void uart_ameba_irq_err_enable(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	UART_INTConfig(uart, RUART_BIT_ELSI, ENABLE);
}

static void uart_ameba_irq_err_disable(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	UART_INTConfig(uart, RUART_BIT_ELSI, DISABLE);
}

static int uart_ameba_irq_is_pending(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	return ((UART_LineStatusGet(uart) & RUART_BIT_TX_EMPTY) && data->tx_int_en) ||
	       ((UART_LineStatusGet(uart) & (RUART_BIT_RXFIFO_INT | RUART_BIT_TIMEOUT_INT)) &&
		data->rx_int_en);
}

static void uart_ameba_irq_update(const struct device *dev)
{
}

static void uart_ameba_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_ameba_data *data = dev->data;

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static inline void async_user_callback(struct uart_ameba_data *data, struct uart_event *event)
{
	if (data->async_cb) {
		data->async_cb(data->dev, event, data->async_user_data);
	}
}

static inline void async_evt_rx_rdy(struct uart_ameba_data *data)
{
	LOG_DBG("rx_rdy: (%d %d)", data->dma_rx.offset, data->dma_rx.counter);

	struct uart_event event = {.type = UART_RX_RDY,
				   .data.rx.buf = data->dma_rx.buffer,
				   .data.rx.len = data->dma_rx.counter - data->dma_rx.offset,
				   .data.rx.offset = data->dma_rx.offset};

	/* update the current pos for new data */
	data->dma_rx.offset = data->dma_rx.counter;

	/* send event only for new data */
	if (event.data.rx.len > 0) {
		async_user_callback(data, &event);
	}
}

static inline void async_evt_rx_err(struct uart_ameba_data *data, int err_code)
{
	LOG_DBG("rx error: %d", err_code);

	struct uart_event event = {.type = UART_RX_STOPPED,
				   .data.rx_stop.reason = err_code,
				   .data.rx_stop.data.len = data->dma_rx.counter,
				   .data.rx_stop.data.offset = 0,
				   .data.rx_stop.data.buf = data->dma_rx.buffer};

	async_user_callback(data, &event);
}

static inline void async_evt_tx_done(struct uart_ameba_data *data)
{
	LOG_DBG("tx done: %d", data->dma_tx.counter);

	struct uart_event event = {.type = UART_TX_DONE,
				   .data.tx.buf = data->dma_tx.buffer,
				   .data.tx.len = data->dma_tx.counter};

	/* Reset tx buffer */
	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_tx_abort(struct uart_ameba_data *data)
{
	LOG_DBG("tx abort: %d", data->dma_tx.counter);

	struct uart_event event = {.type = UART_TX_ABORTED,
				   .data.tx.buf = data->dma_tx.buffer,
				   .data.tx.len = data->dma_tx.counter};

	/* Reset tx buffer */
	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_rx_buf_request(struct uart_ameba_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(data, &evt);
}

static inline void async_evt_rx_buf_release(struct uart_ameba_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->dma_rx.buffer,
	};

	async_user_callback(data, &evt);
}

static inline void async_evt_rx_disabled(struct uart_ameba_data *data)
{
#if !defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	data->last_dat_cnt = 0x0;
	data->hang_cnt = 0;
#endif
	struct uart_event evt = {
		.type = UART_RX_DISABLED,
	};

	async_user_callback(data, &evt);
}

static inline void async_timer_start(struct k_work_delayable *work, int32_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		/* start timer */
		LOG_DBG("async timer started for %d us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

#if defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
static inline void async_get_dma_data(uint32_t dma_channel, uint32_t timeout)
{
	uint32_t count = 0;

	if (GDMA_ChnlFIFOIsEmpty(0x0, dma_channel) == TRUE) {
		return;
	}

	GDMA_Suspend(0x0, dma_channel);

	while (GDMA_ChannelIsActive(0x0, dma_channel)) {
		if (count++ == timeout) {
			break;
		}
	}

	GDMA_Resume(0x0, dma_channel);

	if (count > timeout) {
		LOG_ERR("Data is hang in GDMA FIFO");
	}
}
#endif

static void uart_ameba_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				 int status)
{
	const struct device *uart_dev = user_data;
	struct uart_ameba_data *data = uart_dev->data;
	const struct uart_ameba_config *config = uart_dev->config;
	UART_TypeDef *uart = config->uart;
	struct dma_status stat;
	unsigned int key = irq_lock();

	LOG_DBG("tx %dB done", data->dma_tx.buffer_length);

	/* Disable TX */
	UART_TXDMACmd(uart, DISABLE);

	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);

	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = data->dma_tx.buffer_length - stat.pending_length;
	}

	data->dma_tx.buffer_length = 0;

	irq_unlock(key);

	if (dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel)) {
		LOG_ERR("TX DMA stop failed");
	}

	/* Generate TX_DONE event when transmission is done */
	async_evt_tx_done(data);
}

static void uart_ameba_dma_replace_buffer(const struct device *dev)
{
	const struct device *uart_dev = dev;
	struct uart_ameba_data *data = uart_dev->data;
	int ret;

	LOG_DBG("Replacing RX buffer: %d", data->rx_next_buffer_len);

	/* reload DMA */
	data->dma_rx.offset = 0;
	data->dma_rx.counter = 0;
	data->dma_rx.buffer = data->rx_next_buffer;
	data->dma_rx.buffer_length = data->rx_next_buffer_len;
	data->dma_rx.blk_cfg.block_size = data->dma_rx.buffer_length;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)(data->dma_rx.buffer);
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	sys_cache_data_flush_range((void *)data->dma_rx.buffer, data->dma_rx.buffer_length);
	ret = dma_reload(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
			 data->dma_rx.blk_cfg.source_address, data->dma_rx.blk_cfg.dest_address,
			 data->dma_rx.blk_cfg.block_size);
	if (ret) {
		LOG_ERR("RX DMA reload failed: %d", ret);
		async_evt_rx_err(data, ret);
		return;
	}

	ret = dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
	if (ret) {
		LOG_ERR("RX DMA start failed: %d", ret);
		async_evt_rx_err(data, ret);
		return;
	}

	/* Request next buffer */
	async_evt_rx_buf_request(data);

#if !defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
#endif
}

static void uart_ameba_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				 int status)
{
	const struct device *uart_dev = user_data;
	struct uart_ameba_data *data = uart_dev->data;
	const struct uart_ameba_config *config = uart_dev->config;
	UART_TypeDef *uart = config->uart;

	LOG_DBG("rx %dB done", data->dma_rx.buffer_length);

	if (status < 0) {
		async_evt_rx_err(data, status);
		return;
	}

#if !defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);
#endif

	UART_RxByteCntClear(uart);

	/* true since this functions occurs when buffer if full */
	data->dma_rx.counter = data->dma_rx.buffer_length;

	sys_cache_data_invd_range((void *)data->dma_rx.buffer, data->dma_rx.counter);
	async_evt_rx_rdy(data);

	async_evt_rx_buf_release(data);

	if (data->rx_next_buffer != NULL) {
		/* replace the buffer when the current is full and not the same as the next one. */
		uart_ameba_dma_replace_buffer(uart_dev);
	} else {
		LOG_DBG("no next buffer");
		data->dma_rx.enabled = false;
#if defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
		UART_INT_Clear(uart, RUART_BIT_TOICF | RUART_BIT_RETICF);
		UART_INTConfig(uart, RUART_BIT_ERETI | RUART_BIT_ETOI, DISABLE);
#endif
		async_evt_rx_disabled(data);
	}
}

static int uart_ameba_async_callback_set(const struct device *dev, uart_callback_t callback,
					 void *user_data)
{
	struct uart_ameba_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static int uart_ameba_async_tx(const struct device *dev, const uint8_t *tx_data, size_t buf_size,
			       int32_t timeout)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	int ret;

	if (data->dma_tx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->dma_tx.buffer_length != 0) {
		return -EBUSY;
	}

	data->dma_tx.buffer = (uint8_t *)tx_data;
	data->dma_tx.timeout = timeout;

	LOG_DBG("tx: l=%d", buf_size);

	/* Configure GDMA transfer */
	if (((buf_size & 0x03) == 0) && (((uint32_t)(tx_data) & 0x03) == 0)) {
		/* 4-bytes aligned, move 4 bytes each transfer */
		data->dma_tx.dma_cfg.source_burst_length = 1;
		data->dma_tx.dma_cfg.source_data_size = 4;
	} else {
		/* Not 4-byte aligned: fall back to 1-byte-wide transfers */
		data->dma_tx.dma_cfg.source_burst_length = 4;
		data->dma_tx.dma_cfg.source_data_size = 1;
	}

	/* set source address */
	data->dma_tx.blk_cfg.source_address = (uint32_t)(data->dma_tx.buffer);
	data->dma_tx.blk_cfg.block_size = buf_size;

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &data->dma_tx.dma_cfg);

	if (ret != 0) {
		LOG_ERR("dma tx config error!");
		return -EINVAL;
	}

	data->dma_tx.buffer_length = buf_size;

	/* Start TX timer */
	async_timer_start(&data->dma_tx.timeout_work, data->dma_tx.timeout);

	/* Enable TX DMA requests */
	UART_TXDMAConfig(uart, data->dma_tx.dma_cfg.dest_burst_length);
	UART_TXDMACmd(uart, ENABLE);

	sys_cache_data_flush_range((void *)data->dma_tx.buffer, buf_size);
	if (dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel)) {
		LOG_ERR("UART err: TX DMA start failed!");
		return -EFAULT;
	}

	return 0;
}

static int uart_ameba_async_tx_abort(const struct device *dev)
{
	struct uart_ameba_data *data = dev->data;
	size_t tx_buffer_length = data->dma_tx.buffer_length;
	struct dma_status stat;

	if (tx_buffer_length == 0) {
		return -EALREADY;
	}

	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);

	if (dma_suspend(data->dma_tx.dma_dev, data->dma_tx.dma_channel)) {
		LOG_ERR("TX DMA suspend failed");
	} else if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_length - stat.pending_length;
	}

	if (dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel)) {
		LOG_ERR("TX DMA stop failed");
	}
	async_evt_tx_abort(data);

	return 0;
}

static int uart_ameba_async_rx_enable(const struct device *dev, uint8_t *rx_buf, size_t buf_size,
				      int32_t timeout)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	int ret;
	uint8_t rc = 0;

	if (data->dma_rx.dma_dev == NULL) {
		LOG_ERR("no dma device");
		return -ENODEV;
	}

	if (data->dma_rx.enabled) {
		LOG_WRN("RX was already enabled");
		return -EBUSY;
	}

	/* read out all the data in the FIFO */
	while (UART_Readable(uart)) {
		UART_CharGet(uart, &rc);
	}

	UART_RxByteCntClear(uart);

	data->dma_rx.offset = 0;
	data->dma_rx.buffer = rx_buf;
	data->dma_rx.buffer_length = buf_size;
	data->dma_rx.counter = 0;
#if defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	data->dma_rx.timeout = timeout;
#else
	/* 5 can be modified, 3 in uart_ameba_async_rx_timeout should be modified as well */
	data->dma_rx.timeout = timeout / 5;
#endif
	data->dma_rx.dma_cfg.dest_burst_length = 1;
	/* note: fixed before g2, including g2 */
	data->dma_rx.dma_cfg.dest_data_size = 4;
	data->dma_rx.blk_cfg.block_size = buf_size;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)data->dma_rx.buffer;
	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &data->dma_rx.dma_cfg);
	if (ret != 0) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EINVAL;
	}

	UART_INTConfig(uart, RUART_BIT_ERBI, DISABLE);

#if defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		/* timeout in us */
		if (data->uart_config.baudrate >= 1000) {
			UART_RxToThreConfig(uart,
					    data->uart_config.baudrate / 1000 * timeout / 1000);
		} else {
			UART_RxToThreConfig(uart, data->uart_config.baudrate * timeout / 1000000);
		}
		/* timeout in us, rx clk 40MHz */
		UART_REToThreConfig(uart, 40 * timeout);

		UART_INT_Clear(uart, RUART_BIT_TOICF | RUART_BIT_RETICF);
		UART_INTConfig(uart, RUART_BIT_ETOI | RUART_BIT_ERETI, ENABLE);
	} else {
		UART_INTConfig(uart, RUART_BIT_ETOI | RUART_BIT_ERETI, DISABLE);
	}
#endif

	/* Enable RX DMA requests */
	UART_RXDMAConfig(uart, data->dma_rx.dma_cfg.source_burst_length);
	UART_RXDMACmd(uart, ENABLE);

	sys_cache_data_flush_range((void *)data->dma_rx.buffer, buf_size);
	if (dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel)) {
		LOG_ERR("UART ERR: RX DMA start failed!");
		UART_RXDMACmd(uart, DISABLE);
		return -EFAULT;
	}

	data->dma_rx.enabled = true;

	/* Request next buffer */
	async_evt_rx_buf_request(data);

#if !defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
	UART_RxByteCntClear(uart);
	data->last_dat_cnt = 0;
	data->hang_cnt = 0;
#endif

	LOG_DBG("async rx enabled");

	return ret;
}

static int uart_ameba_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_ameba_data *data = dev->data;

	LOG_DBG("replace buffer (%d)", len);
	data->rx_next_buffer = buf;
	data->rx_next_buffer_len = len;

	return 0;
}

static int uart_ameba_async_rx_disable(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	if (!data->dma_rx.enabled) {
		async_evt_rx_disabled(data);
		return -EALREADY;
	}

#if defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	UART_INT_Clear(uart, RUART_BIT_TOICF | RUART_BIT_RETICF);
	UART_INTConfig(uart, RUART_BIT_ETOI | RUART_BIT_ERETI, DISABLE);
#else
	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);
#endif

	/* release current buffer */
	async_evt_rx_buf_release(data);

	/* in case rx has not completed */
	GDMA_Abort(0x0, data->dma_rx.dma_channel);
	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	UART_RXDMACmd(uart, DISABLE);
	data->dma_rx.enabled = false;

	/* release next buffer */
	if (data->rx_next_buffer) {
		struct uart_event rx_next_buf_release_evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->rx_next_buffer,
		};
		async_user_callback(data, &rx_next_buf_release_evt);
	}

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* When async rx is disabled, enable interruptible instance of uart to function normally */
	LOG_DBG("rx: disabled");

	async_evt_rx_disabled(data);

	return 0;
}

static void uart_ameba_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *tx_stream =
		CONTAINER_OF(dwork, struct uart_dma_stream, timeout_work);
	struct uart_ameba_data *data = CONTAINER_OF(tx_stream, struct uart_ameba_data, dma_tx);
	const struct device *dev = data->dev;

	uart_ameba_async_tx_abort(dev);

	LOG_DBG("tx: async timeout");
}

#if !defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
static void uart_ameba_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *rx_stream =
		CONTAINER_OF(dwork, struct uart_dma_stream, timeout_work);
	struct uart_ameba_data *data = CONTAINER_OF(rx_stream, struct uart_ameba_data, dma_rx);
	const struct device *dev = data->dev;
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	uint32_t ch = data->dma_rx.dma_channel;
	uint32_t timeout = 0;
	uint32_t byte_got;
	int ret;

	LOG_DBG("rx timeout");

	byte_got = UART_RxByteCntGet(uart);

	if (data->last_dat_cnt != byte_got) {
		data->last_dat_cnt = byte_got;
		data->hang_cnt = 0;
	} else if (data->hang_cnt++ >= 3) {
		/* 3 can be modified according to 5 in uart_ameba_async_rx_enable */
		data->hang_cnt = 0;
		/* rx idle for some time */
		LOG_DBG("hang");

		/* get remaining data in DMA FIFO */
		if (GDMA_ChnlFIFOIsEmpty(0x0, ch) == FALSE) {

			GDMA_Suspend(0x0, ch);

			while (GDMA_ChannelIsActive(0x0, ch)) {
				if (timeout++ >= 500) {
					break;
				}
			}
			GDMA_Resume(0x0, ch);

			/* If the ch is still active after timeout, resume is required */
			if (timeout >= 500) {
				LOG_ERR("Data hangs in GDMA FIFO");
				return;
			}
		}

		if (byte_got > data->dma_rx.offset) {
			data->dma_rx.counter = byte_got;
			LOG_DBG("rx cnt turns %u", data->dma_rx.counter);
		}

		/* get remaining data in UART RX FIFO */
		if (UART_Readable(uart) == 0) {
			sys_cache_data_invd_range(
				(void *)(data->dma_rx.buffer + data->dma_rx.offset),
				data->dma_rx.counter - data->dma_rx.offset);
			/* report rx ready event only if rx new data */
			async_evt_rx_rdy(data);
		} else {
			sys_cache_data_invd_range(
				(void *)(data->dma_rx.buffer + data->dma_rx.offset),
				data->dma_rx.counter - data->dma_rx.offset);
			GDMA_Abort(0x0, ch);
			if (dma_stop(data->dma_rx.dma_dev, ch)) {
				LOG_ERR("RX DMA stop failed");
			}
			UART_RXDMACmd(uart, DISABLE);
			while (UART_Readable(uart)) {
				UART_CharGet(uart, data->dma_rx.buffer + data->dma_rx.counter);
				data->dma_rx.counter++;
			}
			byte_got = UART_RxByteCntGet(uart);

			__ASSERT(data->dma_rx.counter == byte_got,
				 "rx cnt mismatch: counter=%u byte_got=%u",
				 data->dma_rx.counter, byte_got);

			/* report rx ready event only if rx new data */
			async_evt_rx_rdy(data);

			/* reload DMA */
			if ((data->dma_rx.counter != data->dma_rx.buffer_length) &&
			    (data->dma_rx.buffer_length != 0)) {
				data->dma_rx.buffer += data->dma_rx.counter;
				data->dma_rx.blk_cfg.block_size =
					data->dma_rx.buffer_length - data->dma_rx.counter;
				data->dma_rx.buffer_length = data->dma_rx.blk_cfg.block_size;
				data->dma_rx.blk_cfg.dest_address = (uint32_t)(data->dma_rx.buffer);
				data->dma_rx.offset = 0;
				data->dma_rx.counter = 0;
				data->rx_next_buffer = NULL;
				data->rx_next_buffer_len = 0;

				LOG_DBG("reload: rx %dB to 0x%x", data->dma_rx.blk_cfg.block_size,
					data->dma_rx.blk_cfg.dest_address);

				sys_cache_data_flush_range((void *)data->dma_rx.buffer,
							   data->dma_rx.buffer_length);
				ret = dma_reload(data->dma_rx.dma_dev, ch,
						 data->dma_rx.blk_cfg.source_address,
						 data->dma_rx.blk_cfg.dest_address,
						 data->dma_rx.blk_cfg.block_size);
				if (ret) {
					LOG_ERR("RX DMA reload failed: %d", ret);
					async_evt_rx_err(data, ret);
					return;
				}

				ret = dma_start(data->dma_rx.dma_dev, ch);
				if (ret) {
					LOG_ERR("RX DMA start failed: %d", ret);
					async_evt_rx_err(data, ret);
					return;
				}
				UART_RxByteCntClear(uart);
				data->last_dat_cnt = 0;
				UART_RXDMACmd(uart, ENABLE);
			}
		}
	}

	async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
}
#endif

static int uart_ameba_async_init(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	data->dev = dev;

	if (data->dma_rx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_rx.dma_dev)) {
			return -ENODEV;
		}
	}

	if (data->dma_tx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_tx.dma_dev)) {
			return -ENODEV;
		}
	}

	if (data->dma_tx.dma_dev == NULL && data->dma_rx.dma_dev == NULL) {
		return 0;
	}

	atomic_set_bit(((struct dma_context *)data->dma_rx.dma_dev->data)->atomic,
		       data->dma_rx.dma_channel);
	atomic_set_bit(((struct dma_context *)data->dma_tx.dma_dev->data)->atomic,
		       data->dma_tx.dma_channel);

	/* Disable both TX and RX DMA requests */
	UART_TXDMACmd(uart, DISABLE);
	UART_RXDMACmd(uart, DISABLE);
	data->dma_rx.enabled = false;

#if !defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	k_work_init_delayable(&data->dma_rx.timeout_work, uart_ameba_async_rx_timeout);
#endif
	k_work_init_delayable(&data->dma_tx.timeout_work, uart_ameba_async_tx_timeout);

	/* Save DT-initialised slot values before memset wipes them. */
	uint32_t rx_slot = data->dma_rx.dma_cfg.dma_slot;
	uint32_t tx_slot = data->dma_tx.dma_cfg.dma_slot;

	/* Configure dma rx config */
	memset(&data->dma_rx.blk_cfg, 0, sizeof(data->dma_rx.blk_cfg));
	memset(&data->dma_rx.dma_cfg, 0, sizeof(data->dma_rx.dma_cfg));
	data->dma_rx.dma_cfg.dma_slot = rx_slot;
	data->dma_rx.blk_cfg.source_address = (uint32_t)(&(uart->RBR_OR_UART_THR));
	data->dma_rx.blk_cfg.dest_address = 0; /* dest not ready */
	data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	/* RX disable circular buffer */
	data->dma_rx.blk_cfg.source_reload_en = 0;
	data->dma_rx.blk_cfg.dest_reload_en = 0;
	data->dma_rx.blk_cfg.flow_control_mode = 0;

	data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
	data->dma_rx.dma_cfg.user_data = (void *)dev;
	data->dma_rx.dma_cfg.dma_callback = uart_ameba_dma_rx_cb;
	data->dma_rx.dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	data->dma_rx.dma_cfg.error_callback_dis = 1;
	data->dma_rx.dma_cfg.source_handshake = 0;
	data->dma_rx.dma_cfg.dest_handshake = 1;
	data->dma_rx.dma_cfg.channel_priority = 1;
	data->dma_rx.dma_cfg.block_count = 1;
	data->dma_rx.dma_cfg.source_data_size = 1;
	data->dma_rx.dma_cfg.source_burst_length = 4;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* Configure dma tx config */
	memset(&data->dma_tx.blk_cfg, 0, sizeof(data->dma_tx.blk_cfg));
	memset(&data->dma_tx.dma_cfg, 0, sizeof(data->dma_tx.dma_cfg));
	data->dma_tx.dma_cfg.dma_slot = tx_slot;
	data->dma_tx.blk_cfg.source_address = 0; /* not ready */
	data->dma_tx.blk_cfg.dest_address = (uint32_t)(&(uart->RBR_OR_UART_THR));
	data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma_tx.blk_cfg.source_reload_en = 0;
	data->dma_tx.blk_cfg.dest_reload_en = 0;
	data->dma_tx.blk_cfg.flow_control_mode = 0;

	data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
	data->dma_tx.dma_cfg.user_data = (void *)dev;
	data->dma_tx.dma_cfg.dma_callback = uart_ameba_dma_tx_cb;
	data->dma_tx.dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	data->dma_tx.dma_cfg.error_callback_dis = 1;
	data->dma_tx.dma_cfg.source_handshake = 1;
	data->dma_tx.dma_cfg.dest_handshake = 0;
	data->dma_tx.dma_cfg.channel_priority = 1;
	data->dma_tx.dma_cfg.block_count = 1;
	data->dma_tx.dma_cfg.dest_data_size = 1;
	data->dma_tx.dma_cfg.dest_burst_length = 4;

	return 0;
}
#endif /* CONFIG_UART_ASYNC_API */

static int uart_ameba_init(const struct device *dev)
{
	const struct uart_ameba_config *config = dev->config;
	struct uart_ameba_data *data = dev->data;

	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(config->clock_dev, config->clock_subsys)) {
		LOG_ERR("Could not enable UART clock");
		return -EIO;
	}

	ret = uart_ameba_configure(dev, &data->uart_config);

	if (ret < 0) {
		LOG_ERR("Error configuring UART (%d)", ret);
		return ret;
	}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_ASYNC_API
	return uart_ameba_async_init(dev);
#endif
	return 0;
}

#ifdef CONFIG_UART_ASYNC_API

#define UART_DMA_CHANNEL_INIT(index, dir)                                                          \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = AMEBA_DMA_CONFIG(index, dir, 1, uart_ameba_dma_##dir##_cb),

#define UART_DMA_CHANNEL(index, dir)                                                               \
	.dma_##dir = {COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),                      \
				  (UART_DMA_CHANNEL_INIT(index, dir)), (NULL))},
#else
#define UART_DMA_CHANNEL(index, dir)
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define AMEBA_UART_IRQ_HANDLER_DECL(n)                                                             \
	static void uart_ameba_irq_config_func_##n(const struct device *dev);
#define AMEBA_UART_IRQ_HANDLER(n)                                                                  \
	static void uart_ameba_irq_config_func_##n(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_ameba_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define AMEBA_UART_IRQ_HANDLER_FUNC(n) .irq_config_func = uart_ameba_irq_config_func_##n,
#else
#define AMEBA_UART_IRQ_HANDLER_DECL(n) /* Not used */
#define AMEBA_UART_IRQ_HANDLER(n)      /* Not used */
#define AMEBA_UART_IRQ_HANDLER_FUNC(n) /* Not used */
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)

static void uart_ameba_isr(const struct device *dev)
{
	struct uart_ameba_data *data = dev->data;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
#if defined(CONFIG_SERIAL_ASYNC_WITH_ERETI)
	const struct uart_ameba_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	uint32_t ch = data->dma_rx.dma_channel;
	uint32_t byte_got = UART_RxByteCntGet(uart);
	int ret;

	if (UART_LineStatusGet(uart) & RUART_BIT_RE_TIMEOUT_INT) {
		UART_INT_Clear(uart, RUART_BIT_RETICF);

		LOG_DBG("fifo empty timeout, read %dB from fifo totally", byte_got);

		/* get remaining data in DMA FIFO */
		async_get_dma_data(ch, 500);

		if (byte_got > data->dma_rx.offset) {
			data->dma_rx.counter = byte_got;
			LOG_DBG("rx cnt turns %d", data->dma_rx.counter);
		}

		sys_cache_data_invd_range(
			(void *)(data->dma_rx.buffer + data->dma_rx.offset),
			data->dma_rx.counter - data->dma_rx.offset);
		/* report rx ready event only if rx new data */
		async_evt_rx_rdy(data);

		/* NOTE: UART_RXDMA will be disabled automatically when RE_TIMEOUT arises!!! */
		UART_RXDMACmd(uart, ENABLE);
	}

	if (UART_LineStatusGet(uart) & RUART_BIT_TIMEOUT_INT) {
		UART_INT_Clear(uart, RUART_BIT_TOICF);

		LOG_DBG("remain data timeout, read %dB from fifo totally", byte_got);

		/* get remaining data in DMA FIFO */
		async_get_dma_data(ch, 500);

		if (byte_got > data->dma_rx.offset) {
			data->dma_rx.counter = byte_got;
			LOG_DBG("rx cnt turns %d", data->dma_rx.counter);
		}

		sys_cache_data_invd_range(
			(void *)(data->dma_rx.buffer + data->dma_rx.offset),
			data->dma_rx.counter - data->dma_rx.offset);
		GDMA_Abort(0x0, ch);
		if (dma_stop(data->dma_rx.dma_dev, ch)) {
			LOG_ERR("RX DMA stop failed");
		}
		UART_RXDMACmd(uart, DISABLE);

		/* get remaining data in UART RX FIFO */
		while (UART_Readable(uart)) {
			UART_CharGet(uart, data->dma_rx.buffer + data->dma_rx.counter);
			LOG_DBG(">> %x", *(data->dma_rx.buffer + data->dma_rx.counter));
			data->dma_rx.counter++;
		}

		byte_got = UART_RxByteCntGet(uart);

		__ASSERT(data->dma_rx.counter == byte_got,
			 "rx cnt mismatch: counter=%u byte_got=%u",
			 data->dma_rx.counter, byte_got);

		/* report rx ready event */
		async_evt_rx_rdy(data);

		/* reload DMA */
		if ((data->dma_rx.counter != data->dma_rx.buffer_length) &&
		    (data->dma_rx.buffer_length != 0)) {
			data->dma_rx.buffer += data->dma_rx.counter;
			data->dma_rx.blk_cfg.block_size =
				data->dma_rx.buffer_length - data->dma_rx.counter;
			data->dma_rx.buffer_length = data->dma_rx.blk_cfg.block_size;
			data->dma_rx.blk_cfg.dest_address = (uint32_t)(data->dma_rx.buffer);
			data->dma_rx.offset = 0;
			data->dma_rx.counter = 0;
			data->rx_next_buffer = NULL;
			data->rx_next_buffer_len = 0;

			LOG_DBG("reload: rx %dB to 0x%x", data->dma_rx.blk_cfg.block_size,
				data->dma_rx.blk_cfg.dest_address);

			sys_cache_data_flush_range((void *)data->dma_rx.buffer,
						   data->dma_rx.buffer_length);
			ret = dma_reload(data->dma_rx.dma_dev, ch,
					 data->dma_rx.blk_cfg.source_address,
					 data->dma_rx.blk_cfg.dest_address,
					 data->dma_rx.blk_cfg.block_size);
			if (ret) {
				LOG_ERR("RX DMA reload failed: %d", ret);
				return;
			}

			ret = dma_start(data->dma_rx.dma_dev, ch);
			if (ret) {
				LOG_ERR("RX DMA start failed: %d", ret);
				return;
			}
			UART_RxByteCntClear(uart);
			UART_RXDMACmd(uart, ENABLE);
		}
	}
#else
	(void)data;
#endif /* CONFIG_SERIAL_ASYNC_WITH_ERETI */
#endif /* CONFIG_UART_ASYNC_API */

	/* Clear errors */
	uart_ameba_err_check(dev);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

static DEVICE_API(uart, uart_ameba_api) = {
	.poll_in = uart_ameba_poll_in,
	.poll_out = uart_ameba_poll_out,
	.err_check = uart_ameba_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ameba_configure,
	.config_get = uart_ameba_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_ameba_fifo_fill,
	.fifo_read = uart_ameba_fifo_read,
	.irq_tx_enable = uart_ameba_irq_tx_enable,
	.irq_tx_disable = uart_ameba_irq_tx_disable,
	.irq_tx_ready = uart_ameba_irq_tx_ready,
	.irq_rx_enable = uart_ameba_irq_rx_enable,
	.irq_rx_disable = uart_ameba_irq_rx_disable,
	.irq_tx_complete = uart_ameba_irq_tx_complete,
	.irq_rx_ready = uart_ameba_irq_rx_ready,
	.irq_err_enable = uart_ameba_irq_err_enable,
	.irq_err_disable = uart_ameba_irq_err_disable,
	.irq_is_pending = uart_ameba_irq_is_pending,
	.irq_update = uart_ameba_irq_update,
	.irq_callback_set = uart_ameba_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_ameba_async_callback_set,
	.tx = uart_ameba_async_tx,
	.tx_abort = uart_ameba_async_tx_abort,
	.rx_enable = uart_ameba_async_rx_enable,
	.rx_buf_rsp = uart_ameba_async_rx_buf_rsp,
	.rx_disable = uart_ameba_async_rx_disable,
#endif /*CONFIG_UART_ASYNC_API*/
};

#define AMEBA_UART_INIT(n)                                                                         \
	AMEBA_UART_IRQ_HANDLER_DECL(n)                                                             \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct uart_ameba_config uart_ameba_config##n = {                             \
		.uart = (UART_TypeDef *)DT_INST_REG_ADDR(n),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, idx),               \
		AMEBA_UART_IRQ_HANDLER_FUNC(n)};                                                   \
                                                                                                   \
	static struct uart_ameba_data uart_ameba_data_##n = {                                      \
		.uart_config = {.baudrate = DT_INST_PROP(n, current_speed),                        \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE},                             \
		UART_DMA_CHANNEL(n, rx) UART_DMA_CHANNEL(n, tx)};                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &uart_ameba_init, NULL, &uart_ameba_data_##n,                     \
			      &uart_ameba_config##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &uart_ameba_api);                                                    \
                                                                                                   \
	AMEBA_UART_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(AMEBA_UART_INIT);
