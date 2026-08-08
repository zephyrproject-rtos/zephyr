/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_uart

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma/dma_bee.h>
#include <zephyr/drivers/dma.h>
#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_gdma.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_gdma.h>
#endif
#endif

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_uart.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_uart.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_bee, CONFIG_UART_LOG_LEVEL);

struct uart_bee_config {
	UART_TypeDef *uart;
	uint16_t clkid;
	uint8_t rx_threshold;
	bool hw_flow_ctrl;
	const struct pinctrl_dev_config *pcfg;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
#endif
};

#ifdef CONFIG_UART_ASYNC_API
struct uart_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	uint8_t src_addr_increment;
	uint8_t dst_addr_increment;
	int fifo_threshold;
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

struct uart_bee_data {
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
#endif
};

/* clang-format off */
static const struct {
	uint16_t div;
	uint16_t ovsr;
	uint16_t ovsr_adj;
	uint32_t baudrate;
} uart_bee_baudrate_table[] = {
	{271, 10, 0x24A, 9600},
	{150, 8, 0x3EF, 19200},
	{20, 12, 0x252, 115200},
	{11, 10, 0x3BB, 230400},
	{11, 9, 0x084, 256000},
	{7, 9, 0x3EF, 384000},
	{6, 9, 0x0AA, 460800},
	{3, 9, 0x0AA, 921600},
	{4, 5, 0, 1000000},
	{2, 5, 0, 2000000},
	{1, 8, 0x292, 3000000},
};
/* clang-format on */

static int uart_bee_cfg2idx_baudrate(uint32_t baudrate)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(uart_bee_baudrate_table); i++) {
		if (uart_bee_baudrate_table[i].baudrate == baudrate) {
			return i;
		}
	}

	return -ENOTSUP;
}

static int uart_bee_cfg2mac_data_bits(enum uart_config_data_bits data_bits)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_7:
		return UART_WORD_LENGTH_7BIT;
	case UART_CFG_DATA_BITS_8:
		return UART_WORD_LENGTH_8BIT;
	default:
		return -ENOTSUP;
	}
}

static int uart_bee_cfg2mac_stopbits(enum uart_config_stop_bits stop_bits)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1:
		return UART_STOP_BITS_1;
	case UART_CFG_STOP_BITS_2:
		return UART_STOP_BITS_2;
	default:
		return -ENOTSUP;
	}
}

static int uart_bee_cfg2mac_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		return UART_PARITY_NO_PARTY;
	case UART_CFG_PARITY_ODD:
		return UART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return UART_PARITY_EVEN;
	default:
		return -ENOTSUP;
	}
}

static int uart_bee_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	UART_InitTypeDef uart_init_struct;

	int baudrate_idx;
	int wordlen;
	int stopbits;
	int parity;

	baudrate_idx = uart_bee_cfg2idx_baudrate(cfg->baudrate);
	if (baudrate_idx < 0) {
		LOG_DBG("Unsupported baudrate: %d", cfg->baudrate);
		return -ENOTSUP;
	}

	wordlen = uart_bee_cfg2mac_data_bits(cfg->data_bits);
	if (wordlen < 0) {
		LOG_DBG("Unsupported data_bits: %d", cfg->data_bits);
		return -ENOTSUP;
	}

	stopbits = uart_bee_cfg2mac_stopbits(cfg->stop_bits);
	if (stopbits < 0) {
		LOG_DBG("Unsupported stop_bits: %d", cfg->stop_bits);
		return -ENOTSUP;
	}

	parity = uart_bee_cfg2mac_parity(cfg->parity);
	if (parity < 0) {
		LOG_DBG("Unsupported parity: %d", cfg->parity);
		return -ENOTSUP;
	}

	LOG_DBG("baudrate_idx=%d, wordlen=%d, stopbits=%d, parity=%d, "
		"hw_flow_ctrl=%d",
		baudrate_idx, wordlen, stopbits, parity, config->hw_flow_ctrl);

	UART_StructInit(&uart_init_struct);
	uart_init_struct.UART_Div = uart_bee_baudrate_table[baudrate_idx].div;
	uart_init_struct.UART_Ovsr = uart_bee_baudrate_table[baudrate_idx].ovsr;
	uart_init_struct.UART_OvsrAdj = uart_bee_baudrate_table[baudrate_idx].ovsr_adj;
	uart_init_struct.UART_IdleTime = UART_RX_IDLE_1BYTE;
	uart_init_struct.UART_WordLen = wordlen;
	uart_init_struct.UART_StopBits = stopbits;
	uart_init_struct.UART_Parity = parity;
	uart_init_struct.UART_HardwareFlowControl = cfg->flow_ctrl;
	uart_init_struct.UART_RxThdLevel = config->rx_threshold;
	uart_init_struct.UART_TxThdLevel = UART_TX_FIFO_SIZE / 2;

#ifdef CONFIG_UART_ASYNC_API
	uart_init_struct.UART_DmaEn = UART_DMA_ENABLE;

	if (data->dma_tx.dma_dev != NULL) {
		uart_init_struct.UART_TxWaterLevel = 16 - data->dma_tx.dma_cfg.dest_burst_length;
	}

	if (data->dma_rx.dma_dev != NULL) {
		uart_init_struct.UART_RxWaterLevel = data->dma_rx.dma_cfg.source_burst_length;
	}
#endif

	(void)clock_control_off(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&config->clkid);
	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&config->clkid);

	UART_Init(uart, &uart_init_struct);

	data->uart_config = *cfg;
	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_bee_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_bee_data *data = dev->data;

	*cfg = data->uart_config;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_bee_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	if (!UART_GetFlagStatus(uart, UART_FLAG_RX_DATA_AVA)) {
		return -1;
	}
	LOG_DBG("c=%c", *c);

	*c = (unsigned char)UART_ReceiveByte(uart);

	return 0;
}

static void uart_bee_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	while (!(UART_GetTxFIFODataLen(uart) < UART_TX_FIFO_SIZE)) {
	}

	UART_SendByte(uart, (uint8_t)c);

	while (UART_GetTxFIFODataLen(uart)) {
	}
}

static int uart_bee_err_check(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	uint32_t err = 0U;

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_OVERRUN)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_PARITY_ERR)) {
		err |= UART_ERROR_PARITY;
	}

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_FRAME_ERR)) {
		err |= UART_ERROR_FRAMING;
	}

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_BREAK_ERR)) {
		err |= UART_BREAK;
	}

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_bee_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	uint8_t num_tx = 0U;
	unsigned int key;

	if (!(UART_GetTxFIFODataLen(uart) < UART_TX_FIFO_SIZE)) {
		return num_tx;
	}

	key = irq_lock();

	while ((size - num_tx > 0) && (UART_GetTxFIFODataLen(uart) < UART_TX_FIFO_SIZE)) {
		UART_SendByte(uart, (uint8_t)tx_data[num_tx++]);
	}

	irq_unlock(key);

	LOG_DBG("num_tx=%d", num_tx);

	return num_tx;
}

static int uart_bee_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) && UART_GetFlagStatus(uart, UART_FLAG_RX_DATA_AVA)) {
		rx_data[num_rx++] = UART_ReceiveByte(uart);
	}

	LOG_DBG("num_rx=%d", num_rx);

	return num_rx;
}

static void uart_bee_irq_tx_enable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data = dev->data;

	data->tx_int_en = true;
	UART_INTConfig(uart, UART_INT_TX_FIFO_EMPTY, ENABLE);
}

static void uart_bee_irq_tx_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data = dev->data;

	data->tx_int_en = false;
	UART_INTConfig(uart, UART_INT_TX_FIFO_EMPTY, DISABLE);
}

static int uart_bee_irq_tx_ready(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	return UART_GetFlagStatus(uart, UART_FLAG_TX_EMPTY) && data->tx_int_en;
}

static int uart_bee_irq_tx_complete(const struct device *dev)
{
	return uart_bee_irq_tx_ready(dev);
}

static void uart_bee_irq_rx_enable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data = dev->data;

	data->rx_int_en = true;
	UART_INTConfig(uart, UART_INT_RD_AVA, ENABLE);
	UART_INTConfig(uart, UART_INT_RX_IDLE, ENABLE);
}

static void uart_bee_irq_rx_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data = dev->data;

	data->rx_int_en = false;
	UART_INTConfig(uart, UART_INT_RD_AVA, DISABLE);
	UART_INTConfig(uart, UART_INT_RX_IDLE, DISABLE);
}

static int uart_bee_irq_rx_ready(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	return UART_GetFlagStatus(uart, UART_FLAG_RX_DATA_AVA);
}

static void uart_bee_irq_err_enable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	UART_INTConfig(uart, UART_INT_RX_LINE_STS, ENABLE);
}

static void uart_bee_irq_err_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	UART_INTConfig(uart, UART_INT_RX_LINE_STS, DISABLE);
}

static int uart_bee_irq_is_pending(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	return ((UART_GetFlagStatus(uart, UART_FLAG_TX_EMPTY) && data->tx_int_en) ||
		(UART_GetFlagStatus(uart, UART_INT_RD_AVA) && data->rx_int_en));
}

static void uart_bee_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_bee_data *data = dev->data;

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
int uart_bee_line_ctrl_set(const struct device *dev, uint32_t ctrl, uint32_t val)
{
	LOG_ERR("Unsupported line_ctrl_set function");
	return -ENOTSUP;
}

int uart_bee_line_ctrl_get(const struct device *dev, uint32_t ctrl, uint32_t *val)
{
	LOG_ERR("Unsupported line_ctrl_get function");
	return -ENOTSUP;
}
#endif

#ifdef CONFIG_UART_DRV_CMD
int uart_bee_drv_cmd(const struct device *dev, uint32_t cmd, uint32_t p)
{
	LOG_ERR("Unsupported drv_cmd function");
	return -ENOTSUP;
}
#endif
#ifdef CONFIG_UART_ASYNC_API
static void uart_bee_dma_replace_buffer(const struct device *dev);
static int uart_bee_async_rx_disable(const struct device *dev);

static void uart_bee_dma_tx_control(UART_TypeDef *uart, bool en)
{
#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	UART_TxDmaCmd(uart, en);
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
	if (en) {
		uart->MISCR |= BIT(1);
	} else {
		uart->MISCR &= ~BIT(1);
	}
#endif
}

static void uart_bee_dma_rx_control(UART_TypeDef *uart, bool en)
{
#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	UART_RxDmaCmd(uart, en);
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
	if (en) {
		uart->MISCR |= BIT(2);
	} else {
		uart->MISCR &= ~BIT(2);
	}
#endif
}

static int uart_bee_async_callback_set(const struct device *dev, uart_callback_t callback,
				       void *user_data)
{
	struct uart_bee_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static inline void async_user_callback(struct uart_bee_data *data, struct uart_event *event)
{
	if (data->async_cb) {
		data->async_cb(data->dev, event, data->async_user_data);
	}
}

static inline void async_evt_tx_done(struct uart_bee_data *data)
{
	struct uart_event event = {.type = UART_TX_DONE,
				   .data.tx.buf = data->dma_tx.buffer,
				   .data.tx.len = data->dma_tx.counter};

	/* Reset the TX buffer */
	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_tx_abort(struct uart_bee_data *data)
{
	struct uart_event event = {.type = UART_TX_ABORTED,
				   .data.tx.buf = data->dma_tx.buffer,
				   .data.tx.len = data->dma_tx.counter};

	/* Reset the TX buffer */
	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_rx_rdy(struct uart_bee_data *data)
{
	struct uart_event event = {.type = UART_RX_RDY,
				   .data.rx.buf = data->dma_rx.buffer,
				   .data.rx.len = data->dma_rx.counter,
				   .data.rx.offset = 0};

	/* Send event only for new data */
	if (event.data.rx.len > 0) {
		async_user_callback(data, &event);
	}
}

static inline void async_evt_rx_buf_request(struct uart_bee_data *data)
{
	struct uart_event event = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(data, &event);
}

static inline void async_evt_rx_buf_release(struct uart_bee_data *data)
{
	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->dma_rx.buffer,
	};

	async_user_callback(data, &event);
}

static inline void async_evt_rx_disable(struct uart_bee_data *data)
{
	struct uart_event event = {
		.type = UART_RX_DISABLED,
	};

	async_user_callback(data, &event);
}

static inline void async_evt_rx_err(struct uart_bee_data *data, int err_code)
{
	struct uart_event event = {.type = UART_RX_STOPPED,
				   .data.rx_stop.reason = err_code,
				   .data.rx_stop.data.len = data->dma_rx.counter,
				   .data.rx_stop.data.offset = 0,
				   .data.rx_stop.data.buf = data->dma_rx.buffer};

	async_user_callback(data, &event);
}

static inline void async_timer_start(struct k_work_delayable *work, int32_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		LOG_DBG("Async timer started for %d us", timeout);

		/* Start the timeout timer */
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static inline void uart_bee_dma_tx_enable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	uart_bee_dma_tx_control(uart, true);
}

static inline void uart_bee_dma_tx_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	uart_bee_dma_tx_control(uart, false);
}

static inline void uart_bee_dma_rx_enable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	uart_bee_dma_rx_control(uart, true);

	data->dma_rx.enabled = true;
}

static inline void uart_bee_dma_rx_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	uart_bee_dma_rx_control(uart, false);

	data->dma_rx.enabled = false;
}

void uart_bee_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct uart_bee_data *data = uart_dev->data;
	struct dma_status stat;

	/* Disable the UART TX DMA requests */
	uart_bee_dma_tx_disable(uart_dev);

	/* Stop the TX timeout timer */
	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);

	/* Get the DMA TX length */
	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = data->dma_tx.buffer_length - stat.pending_length;
	}

	LOG_DBG("channel=%d, status=%d, dma_tx.counter=%d", channel, status, data->dma_tx.counter);

	data->dma_tx.buffer_length = 0;

	/* Stop the TX DMA */
	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	/* Generate TX_DONE event when TX is done */
	async_evt_tx_done(data);
}

static void uart_bee_dma_replace_buffer(const struct device *dev)
{
	struct uart_bee_data *data = dev->data;

	LOG_DBG("Replacing RX buffer: %d", data->rx_next_buffer_len);

	/* Replace the RX DMA buffer and reload the RX DMA */
	data->dma_rx.blk_cfg.block_size = data->rx_next_buffer_len;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)(data->rx_next_buffer);

	dma_reload(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
		   data->dma_rx.blk_cfg.source_address, data->dma_rx.blk_cfg.dest_address,
		   data->dma_rx.blk_cfg.block_size);

	dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
}

static void uart_bee_dma_rx_calc_counter(struct uart_bee_data *data)
{
	struct dma_status stat;
	/* Get the DMA RX length */
	if (!dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &stat)) {
		data->dma_rx.counter = data->dma_rx.buffer_length - stat.pending_length;
	}
}

static void uart_bee_rx_proceed_next_or_disable(const struct device *dev)
{
	struct uart_bee_data *data = dev->data;

	if (data->rx_next_buffer) {
		/* Directly reload the RX DMA buffer and start DMA to minimize overhead, preventing
		 * data loss caused by UART RX FIFO overflow.
		 */
		uart_bee_dma_replace_buffer(dev);
		/* Generate RX_RDY event with the received data */
		async_evt_rx_rdy(data);
		/* Generate RX_BUF_RELEASED event to release every passed buffer */
		async_evt_rx_buf_release(data);
		data->dma_rx.buffer = data->rx_next_buffer;
		data->dma_rx.buffer_length = data->rx_next_buffer_len;
		data->rx_next_buffer = 0;
		data->rx_next_buffer_len = 0;
		/* Generate RX_BUF_REQUEST event to get the next buffer */
		async_evt_rx_buf_request(data);
	} else {
		/* No next buffer available. Proceed directly to the disable flow and report the
		 * received data.
		 */
		uart_bee_async_rx_disable(dev);
	}
}

void uart_bee_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct uart_bee_data *data = uart_dev->data;

	/* Get the DMA RX length */
	uart_bee_dma_rx_calc_counter(data);

	if (status < 0) {
		async_evt_rx_err(data, status);
		return;
	}

	/* Reload buffer or disable RX */
	uart_bee_rx_proceed_next_or_disable(uart_dev);

	/* Stop the RX timeout timer */
	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);
}

static int uart_bee_async_tx(const struct device *dev, const uint8_t *tx_data, size_t buf_size,
			     int32_t timeout)
{
	struct uart_bee_data *data = dev->data;
	int ret;

	LOG_DBG("bufsize=%d, timeout=%d", buf_size, timeout);

	if (data->dma_tx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->dma_tx.buffer_length != 0) {
		return -EBUSY;
	}

	data->dma_tx.buffer = (uint8_t *)tx_data;
	data->dma_tx.buffer_length = buf_size;
	data->dma_tx.timeout = timeout;

	/* Set source address */
	data->dma_tx.blk_cfg.source_address = (uint32_t)(data->dma_tx.buffer);
	data->dma_tx.blk_cfg.block_size = data->dma_tx.buffer_length;

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &data->dma_tx.dma_cfg);

	if (ret != 0) {
		LOG_ERR("DMA TX config error!");
		return -EINVAL;
	}

	/* Start the TX timeout timer */
	async_timer_start(&data->dma_tx.timeout_work, data->dma_tx.timeout);

	/* Enable the UART TX DMA requests */
	uart_bee_dma_tx_enable(dev);

	/* Start the TX dma */
	dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	return 0;
}

static int uart_bee_async_tx_abort(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	size_t tx_buffer_length = data->dma_tx.buffer_length;
	struct dma_status stat;

	if (tx_buffer_length == 0) {
		return -EFAULT;
	}

	/* Stop the TX timeout timer */
	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);

	/* Suspend the TX DMA to get the transmitted data length */
	dma_suspend(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	/* Get the transmitted data length */
	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_length - stat.pending_length;
	}

	/* Stop the TX dma */
	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	while (UART_GetTxFIFODataLen(uart)) {
	}

	/* Generate TX_ABORTED event with the TX is aborted */
	async_evt_tx_abort(data);

	return 0;
}

static int uart_bee_async_rx_enable(const struct device *dev, uint8_t *rx_buf, size_t buf_size,
				    int32_t timeout)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	int ret;

	LOG_DBG("buf_size=%d, timeout=%d", buf_size, timeout);

	/* Flush the UART RX FIFO to prevent processing stale data received before enabling. */
	uint32_t cnt = UART_GetRxFIFODataLen(uart);

	for (uint32_t i = 0; i < cnt; i++) {
		UART_ReceiveByte(uart);
	}

	if (data->dma_rx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->dma_rx.enabled) {
		LOG_WRN("RX was already enabled");
		return -EBUSY;
	}

	data->dma_rx.buffer = rx_buf;
	data->dma_rx.buffer_length = buf_size;
	data->dma_rx.timeout = timeout;

	data->rx_next_buffer = 0;
	data->rx_next_buffer_len = 0;

	/* Disable UART RX AVA interrupts to let DMA to handle it */
	UART_INTConfig(uart, UART_INT_RD_AVA, DISABLE);

	data->dma_rx.blk_cfg.block_size = buf_size;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)rx_buf;

	/* Configure the RX DMA with the passed buffer */
	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &data->dma_rx.dma_cfg);

	if (ret != 0) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EINVAL;
	}

	/* Enable the UART RX DMA requests */
	uart_bee_dma_rx_enable(dev);

	/* Enable UART_INT_RX_IDLE to define the end of a RX DMA transaction */
	UART_INTConfig(uart, UART_INT_RX_IDLE, DISABLE);
	UART_INTConfig(uart, UART_INT_RX_IDLE, ENABLE);

	dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	/* Generate RX_BUF_REQUEST event to get the next buffer */
	async_evt_rx_buf_request(data);

	return ret;
}

static int uart_bee_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_bee_data *data = dev->data;

	LOG_DBG("buf=0x%p len=%d", buf, len);

	data->rx_next_buffer = buf;
	data->rx_next_buffer_len = len;

	return 0;
}

static int uart_bee_async_rx_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	struct dma_status stat;

	if (!data->dma_rx.enabled) {
		async_evt_rx_disable(data);
		return -EFAULT;
	}

	UART_INTConfig(uart, UART_INT_RX_IDLE, DISABLE);

	/* Disable the UART RX DMA requests */
	uart_bee_dma_rx_disable(dev);

	/* Get the received DMA length before stopping the RX DMA */
	if (!dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &stat)) {
		data->dma_rx.counter = data->dma_rx.buffer_length - stat.pending_length;
	}

	/* Stop the RX DMA */
	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	/* Stop the RX timeout timer */
	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

	/* Generate RX_RDY event with the received data */
	async_evt_rx_rdy(data);

	/* Generate RX_BUF_RELEASED event to release every passed buffer */
	while (data->dma_rx.buffer) {
		async_evt_rx_buf_release(data);
		data->dma_rx.buffer = data->rx_next_buffer;
		data->dma_rx.buffer_length = 0;
		data->rx_next_buffer = NULL;
		data->rx_next_buffer_len = 0;
	}

	/* Generate RX_DISABLED event when RX is disabled */
	async_evt_rx_disable(data);

	return 0;
}

static void uart_bee_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *tx_stream =
		CONTAINER_OF(dwork, struct uart_dma_stream, timeout_work);
	struct uart_bee_data *data = CONTAINER_OF(tx_stream, struct uart_bee_data, dma_tx);
	const struct device *dev = data->dev;

	uart_bee_async_tx_abort(dev);
}

static void uart_bee_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *rx_stream =
		CONTAINER_OF(dwork, struct uart_dma_stream, timeout_work);
	struct uart_bee_data *data = CONTAINER_OF(rx_stream, struct uart_bee_data, dma_rx);
	const struct device *dev = data->dev;

	/* Get the DMA RX length */
	uart_bee_dma_rx_calc_counter(data);

	if (data->dma_rx.counter) {
		/* Reload buffer or disable RX */
		uart_bee_rx_proceed_next_or_disable(dev);
	}
}

static int uart_bee_async_init(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
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

	/* Disable both UART TX and UART RX DMA requests */
	uart_bee_dma_rx_disable(dev);
	uart_bee_dma_tx_disable(dev);

	k_work_init_delayable(&data->dma_rx.timeout_work, uart_bee_async_rx_timeout);
	k_work_init_delayable(&data->dma_tx.timeout_work, uart_bee_async_tx_timeout);

	/* Configure DMA RX config */
	memset(&data->dma_rx.blk_cfg, 0, sizeof(data->dma_rx.blk_cfg));

	data->dma_rx.blk_cfg.source_address = UART_RX_FIFO_ADDR(uart);
	data->dma_rx.blk_cfg.dest_address = 0; /* dest not ready */

	data->dma_rx.blk_cfg.source_addr_adj = data->dma_rx.src_addr_increment;
	data->dma_rx.blk_cfg.dest_addr_adj = data->dma_rx.dst_addr_increment;

	/* Disable the RX circular buffer */
	data->dma_rx.blk_cfg.source_reload_en = 0;
	data->dma_rx.blk_cfg.dest_reload_en = 0;

	data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
	data->dma_rx.dma_cfg.user_data = (void *)dev;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* Configure DMA TX config */
	memset(&data->dma_tx.blk_cfg, 0, sizeof(data->dma_tx.blk_cfg));

	data->dma_tx.blk_cfg.dest_address = UART_TX_FIFO_ADDR(uart);
	data->dma_tx.blk_cfg.source_address = 0; /* not ready */

	data->dma_tx.blk_cfg.source_addr_adj = data->dma_tx.src_addr_increment;
	data->dma_tx.blk_cfg.dest_addr_adj = data->dma_tx.dst_addr_increment;

	data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
	data->dma_tx.dma_cfg.user_data = (void *)dev;

	return 0;
}

#endif /* CONFIG_UART_ASYNC_API */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)

#ifdef CONFIG_UART_ASYNC_API
static void uart_bee_handle_async_rx_idle(const struct device *dev)
{
	struct uart_bee_data *data = dev->data;

	if (!data->dma_rx.dma_dev) {
		return;
	}

	/* Get the DMA RX length */
	uart_bee_dma_rx_calc_counter(data);

	if (data->dma_rx.counter) {
		if (data->dma_rx.timeout == 0) {
			/* Reload buffer or disable RX */
			uart_bee_rx_proceed_next_or_disable(dev);
		} else {
			/* Start the RX timeout timer */
			async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
		}
	}
}
#endif

static void uart_bee_isr(const struct device *dev)
{
	struct uart_bee_data *data = dev->data;
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	UART_GetIID(uart);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_IDLE)) {
		UART_INTConfig(uart, UART_INT_RX_IDLE, DISABLE);
		UART_INTConfig(uart, UART_INT_RX_IDLE, ENABLE);
#ifdef CONFIG_UART_ASYNC_API
		uart_bee_handle_async_rx_idle(dev);
#endif
	}

#ifdef CONFIG_UART_ASYNC_API
	/* Clear errors */
	uart_bee_err_check(dev);
#endif /* CONFIG_UART_ASYNC_API */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

static int uart_bee_init(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	int err;

	data->dev = dev;

	/* Configure pinmux  */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&config->clkid);

	/* Configure peripheral  */
	err = uart_bee_configure(dev, &data->uart_config);
	if (err) {
		return err;
	}

	/* Enable nvic */
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_ASYNC_API
	return uart_bee_async_init(dev);
#else
	return 0;
#endif
}

static DEVICE_API(uart, uart_bee_driver_api) = {
	.poll_in = uart_bee_poll_in,
	.poll_out = uart_bee_poll_out,
	.err_check = uart_bee_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_bee_configure,
	.config_get = uart_bee_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_bee_fifo_fill,
	.fifo_read = uart_bee_fifo_read,
	.irq_tx_enable = uart_bee_irq_tx_enable,
	.irq_tx_disable = uart_bee_irq_tx_disable,
	.irq_tx_ready = uart_bee_irq_tx_ready,
	.irq_tx_complete = uart_bee_irq_tx_complete,
	.irq_rx_enable = uart_bee_irq_rx_enable,
	.irq_rx_disable = uart_bee_irq_rx_disable,
	.irq_rx_ready = uart_bee_irq_rx_ready,
	.irq_err_enable = uart_bee_irq_err_enable,
	.irq_err_disable = uart_bee_irq_err_disable,
	.irq_is_pending = uart_bee_irq_is_pending,
	.irq_callback_set = uart_bee_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_bee_async_callback_set,
	.tx = uart_bee_async_tx,
	.tx_abort = uart_bee_async_tx_abort,
	.rx_enable = uart_bee_async_rx_enable,
	.rx_buf_rsp = uart_bee_async_rx_buf_rsp,
	.rx_disable = uart_bee_async_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = uart_bee_line_ctrl_set,
	.line_ctrl_get = uart_bee_line_ctrl_get,
#endif

#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd = uart_bee_drv_cmd,
#endif
};

#ifdef CONFIG_UART_ASYNC_API

#define UART_DMA_CHANNEL_INIT(index, dir)                                                          \
	.dma_dev = DEVICE_DT_GET(BEE_DMA_CTLR(index, dir)),                                        \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg =                                                                                 \
		{                                                                                  \
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(index, dir, slot),                   \
			.channel_direction =                                                       \
				BEE_DMA_CONFIG_DIRECTION(BEE_DMA_CHANNEL_CONFIG(index, dir)),      \
			.channel_priority =                                                        \
				BEE_DMA_CONFIG_PRIORITY(BEE_DMA_CHANNEL_CONFIG(index, dir)),       \
			.source_data_size = BEE_DMA_CONFIG_SOURCE_DATA_SIZE(                       \
				BEE_DMA_CHANNEL_CONFIG(index, dir)),                               \
			.dest_data_size = BEE_DMA_CONFIG_DESTINATION_DATA_SIZE(                    \
				BEE_DMA_CHANNEL_CONFIG(index, dir)),                               \
			.source_burst_length =                                                     \
				BEE_DMA_CONFIG_SOURCE_MSIZE(BEE_DMA_CHANNEL_CONFIG(index, dir)),   \
			.dest_burst_length = BEE_DMA_CONFIG_DESTINATION_MSIZE(                     \
				BEE_DMA_CHANNEL_CONFIG(index, dir)),                               \
			.block_count = 1,                                                          \
			.cyclic = false,                                                           \
			.complete_callback_en = 1,                                                 \
			.dma_callback = uart_bee_dma_##dir##_cb,                                   \
	},                                                                                         \
	.src_addr_increment = BEE_DMA_CONFIG_SOURCE_ADDR_INC(BEE_DMA_CHANNEL_CONFIG(index, dir)),  \
	.dst_addr_increment =                                                                      \
		BEE_DMA_CONFIG_DESTINATION_ADDR_INC(BEE_DMA_CHANNEL_CONFIG(index, dir)),

#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define BEE_UART_IRQ_HANDLER_DECL(index)                                                           \
	static void uart_bee_irq_config_func_##index(const struct device *dev);
#define BEE_UART_IRQ_HANDLER(index)                                                                \
	static void uart_bee_irq_config_func_##index(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), uart_bee_isr,       \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}
#else
#define BEE_UART_IRQ_HANDLER_DECL(index) /* Not used */
#define BEE_UART_IRQ_HANDLER(index)      /* Not used */
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define BEE_UART_IRQ_HANDLER_FUNC(index) .irq_config_func = uart_bee_irq_config_func_##index,
#else
#define BEE_UART_IRQ_HANDLER_FUNC(index) /* Not used */
#endif

#ifdef CONFIG_UART_ASYNC_API
#define UART_DMA_CHANNEL(index, dir)                                                               \
	.dma_##dir = {COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),                               \
				  (UART_DMA_CHANNEL_INIT(index, dir)), (NULL))},
#else
#define UART_DMA_CHANNEL(index, dir)
#endif

#define BEE_UART_INIT(index)                                                                       \
	BEE_UART_IRQ_HANDLER_DECL(index)                                                           \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct uart_bee_config uart_bee_cfg_##index = {                               \
		.uart = (UART_TypeDef *)DT_INST_REG_ADDR(index),                                   \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.rx_threshold = DT_INST_PROP_OR(index, rx_threshold, 10),                          \
		.hw_flow_ctrl = DT_INST_PROP_OR(index, flow_ctrl, false),                          \
		BEE_UART_IRQ_HANDLER_FUNC(index)};                                                 \
                                                                                                   \
	static struct uart_bee_data uart_bee_data_##index = {                                      \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity =                                                          \
					DT_INST_ENUM_IDX_OR(index, parity, UART_CFG_PARITY_NONE),  \
				.stop_bits = DT_INST_ENUM_IDX_OR(index, stop_bits,                 \
								 UART_CFG_STOP_BITS_1),            \
				.data_bits = DT_INST_ENUM_IDX_OR(index, data_bits,                 \
								 UART_CFG_DATA_BITS_8),            \
				.flow_ctrl = DT_INST_PROP(index, hw_flow_control),                 \
			},                                                                         \
		UART_DMA_CHANNEL(index, rx) UART_DMA_CHANNEL(index, tx)};                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &uart_bee_init, NULL, &uart_bee_data_##index,                 \
			      &uart_bee_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &uart_bee_driver_api);                                               \
                                                                                                   \
	BEE_UART_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(BEE_UART_INIT)
