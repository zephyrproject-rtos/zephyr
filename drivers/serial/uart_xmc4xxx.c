/*
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	infineon_xmc4xxx_uart

#include <xmc_uart.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

#define MAX_FIFO_SIZE 64
#define USIC_IRQ_MIN  84
#define USIC_IRQ_MAX  101
#define IRQS_PER_USIC 6

#define CURRENT_BUFFER 0
#define NEXT_BUFFER 1

struct uart_xmc4xxx_config {
	XMC_USIC_CH_t *uart;
	const struct pinctrl_dev_config *pcfg;
	uint8_t input_src;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
	uint8_t irq_num_tx;
	uint8_t irq_num_rx;
#endif
	uint8_t fifo_start_offset;
	uint8_t fifo_tx_size;
	uint8_t fifo_rx_size;
};

#ifdef CONFIG_UART_ASYNC_API
struct uart_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	uint8_t *buffer;
	size_t buffer_len;
	size_t offset;
	size_t counter;
	int32_t timeout;
	struct k_work_delayable timeout_work;
};
#endif

struct uart_xmc4xxx_data {
	XMC_UART_CH_CONFIG_t config;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uint8_t service_request_tx;
	uint8_t service_request_rx;
#endif
#if defined(CONFIG_UART_ASYNC_API)
	const struct device *dev;
	uart_callback_t async_cb;
	void *async_user_data;
	struct uart_dma_stream dma_rx;
	struct uart_dma_stream dma_tx;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_len;
#endif
};

static int uart_xmc4xxx_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	bool fifo_empty;

	if (config->fifo_rx_size > 0) {
		fifo_empty = XMC_USIC_CH_RXFIFO_IsEmpty(config->uart);
	} else {
		fifo_empty = !XMC_USIC_CH_GetReceiveBufferStatus(config->uart);
	}
	if (fifo_empty) {
		return -1;
	}

	*c = (unsigned char)XMC_UART_CH_GetReceivedData(config->uart);

	return 0;
}

static void uart_xmc4xxx_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	/* XMC_UART_CH_Transmit() only blocks for UART to finish transmitting */
	/* when fifo is not used */
	while (config->fifo_tx_size > 0 && XMC_USIC_CH_TXFIFO_IsFull(config->uart)) {
	}
	XMC_UART_CH_Transmit(config->uart, c);
}

#if defined(CONFIG_UART_ASYNC_API)
static inline void async_timer_start(struct k_work_delayable *work, int32_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void disable_tx_events(const struct uart_xmc4xxx_config *config)
{
	if (config->fifo_tx_size > 0) {
		XMC_USIC_CH_TXFIFO_DisableEvent(config->uart,
					       XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	} else {
		XMC_USIC_CH_DisableEvent(config->uart, XMC_USIC_CH_EVENT_TRANSMIT_SHIFT);
	}
}
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
static void enable_tx_events(const struct uart_xmc4xxx_config *config)
{
	if (config->fifo_tx_size > 0) {
		/* wait till the fifo has at least 1 byte free */
		while (XMC_USIC_CH_TXFIFO_IsFull(config->uart)) {
		}
		XMC_USIC_CH_TXFIFO_EnableEvent(config->uart,
					       XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	} else {
		XMC_USIC_CH_EnableEvent(config->uart, XMC_USIC_CH_EVENT_TRANSMIT_SHIFT);
	}
}

#define NVIC_ICPR_BASE 0xe000e280u
static void clear_pending_interrupt(int irq_num)
{
	uint32_t *clearpend = (uint32_t *)(NVIC_ICPR_BASE) + irq_num / 32;

	irq_num = irq_num & 0x1f;
	/* writing zero has not effect, i.e. we only clear irq_num */
	*clearpend = BIT(irq_num);
}

static void uart_xmc4xxx_isr(void *arg)
{
	const struct device *dev = arg;
	struct uart_xmc4xxx_data *data = dev->data;

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif

#if defined(CONFIG_UART_ASYNC_API)
	const struct uart_xmc4xxx_config *config = dev->config;
	unsigned int key = irq_lock();

	if (data->dma_rx.buffer_len) {
		/* We only need to trigger this irq once to start timer */
		/* event. Everything else is handled by the timer callback and dma_rx_callback. */
		/* Note that we can't simply disable the event that triggers this irq, since the */
		/* same service_request gets routed to the dma. Thus we disable the nvic irq */
		/* below. Any pending irq must be cleared before irq_enable() is called. */
		irq_disable(config->irq_num_rx);

		async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
	}
	irq_unlock(key);
#endif
}

static void uart_xmc4xxx_configure_service_requests(const struct device *dev)
{
	struct uart_xmc4xxx_data *data = dev->data;
	const struct uart_xmc4xxx_config *config = dev->config;

	__ASSERT(config->irq_num_tx >= USIC_IRQ_MIN && config->irq_num_tx <= USIC_IRQ_MAX,
		 "Invalid irq number\n");
	data->service_request_tx = (config->irq_num_tx - USIC_IRQ_MIN) % IRQS_PER_USIC;

	if (config->fifo_tx_size > 0) {
		XMC_USIC_CH_TXFIFO_SetInterruptNodePointer(
			config->uart, XMC_USIC_CH_TXFIFO_INTERRUPT_NODE_POINTER_STANDARD,
			data->service_request_tx);
	} else {
		XMC_USIC_CH_SetInterruptNodePointer(
			config->uart, XMC_USIC_CH_INTERRUPT_NODE_POINTER_TRANSMIT_SHIFT,
			data->service_request_tx);
	}

	__ASSERT(config->irq_num_rx >= USIC_IRQ_MIN && config->irq_num_rx <= USIC_IRQ_MAX,
		 "Invalid irq number\n");
	data->service_request_rx = (config->irq_num_rx - USIC_IRQ_MIN) % IRQS_PER_USIC;

	if (config->fifo_rx_size > 0) {
		XMC_USIC_CH_RXFIFO_SetInterruptNodePointer(
			config->uart, XMC_USIC_CH_RXFIFO_INTERRUPT_NODE_POINTER_STANDARD,
			data->service_request_rx);
		XMC_USIC_CH_RXFIFO_SetInterruptNodePointer(
			config->uart, XMC_USIC_CH_RXFIFO_INTERRUPT_NODE_POINTER_ALTERNATE,
			data->service_request_rx);
	} else {
		XMC_USIC_CH_SetInterruptNodePointer(config->uart,
						    XMC_USIC_CH_INTERRUPT_NODE_POINTER_RECEIVE,
						    data->service_request_rx);
		XMC_USIC_CH_SetInterruptNodePointer(
			config->uart, XMC_USIC_CH_INTERRUPT_NODE_POINTER_ALTERNATE_RECEIVE,
			data->service_request_rx);
	}
}

static int uart_xmc4xxx_irq_tx_ready(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	if (config->fifo_tx_size > 0) {
		return !XMC_USIC_CH_TXFIFO_IsFull(config->uart);
	} else {
		return XMC_USIC_CH_GetTransmitBufferStatus(config->uart) ==
			XMC_USIC_CH_TBUF_STATUS_IDLE;
	}
}

static void uart_xmc4xxx_irq_rx_disable(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	if (config->fifo_rx_size > 0) {
		XMC_USIC_CH_RXFIFO_DisableEvent(config->uart,
						XMC_USIC_CH_RXFIFO_EVENT_CONF_STANDARD |
						XMC_USIC_CH_RXFIFO_EVENT_CONF_ALTERNATE);
	} else {
		XMC_USIC_CH_DisableEvent(config->uart, XMC_USIC_CH_EVENT_STANDARD_RECEIVE |
						       XMC_USIC_CH_EVENT_ALTERNATIVE_RECEIVE);
	}
}
static void uart_xmc4xxx_irq_rx_enable(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	uint32_t recv_status;

	/* re-enable the IRQ as it may have been disabled during async_rx */
	clear_pending_interrupt(config->irq_num_rx);
	irq_enable(config->irq_num_rx);

	if (config->fifo_rx_size > 0) {
		XMC_USIC_CH_RXFIFO_Flush(config->uart);
		XMC_USIC_CH_RXFIFO_SetSizeTriggerLimit(config->uart, config->fifo_rx_size, 0);
#if CONFIG_UART_XMC4XXX_RX_FIFO_INT_TRIGGER
		config->uart->RBCTR |= BIT(USIC_CH_RBCTR_SRBTEN_Pos);
#endif
		XMC_USIC_CH_RXFIFO_EnableEvent(config->uart,
					       XMC_USIC_CH_RXFIFO_EVENT_CONF_STANDARD |
					       XMC_USIC_CH_RXFIFO_EVENT_CONF_ALTERNATE);
	} else {
		/* flush out any received bytes while the uart rx irq was disabled */
		recv_status = XMC_USIC_CH_GetReceiveBufferStatus(config->uart);
		if (recv_status & USIC_CH_RBUFSR_RDV0_Msk) {
			XMC_UART_CH_GetReceivedData(config->uart);
		}
		if (recv_status & USIC_CH_RBUFSR_RDV1_Msk) {
			XMC_UART_CH_GetReceivedData(config->uart);
		}

		XMC_USIC_CH_EnableEvent(config->uart, XMC_USIC_CH_EVENT_STANDARD_RECEIVE |
						      XMC_USIC_CH_EVENT_ALTERNATIVE_RECEIVE);
	}
}
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)

static int uart_xmc4xxx_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	int i = 0;

	for (i = 0; i < len; i++) {
		bool fifo_full;

		XMC_UART_CH_Transmit(config->uart, tx_data[i]);
		if (config->fifo_tx_size == 0) {
			return 1;
		}

		fifo_full = XMC_USIC_CH_TXFIFO_IsFull(config->uart);
		if (fifo_full) {
			return i + 1;
		}
	}
	return i;
}

static int uart_xmc4xxx_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	int i;

	for (i = 0; i < size; i++) {
		bool fifo_empty;

		if (config->fifo_rx_size > 0) {
			fifo_empty = XMC_USIC_CH_RXFIFO_IsEmpty(config->uart);
		} else {
			fifo_empty = !XMC_USIC_CH_GetReceiveBufferStatus(config->uart);
		}
		if (fifo_empty) {
			break;
		}
		rx_data[i] = XMC_UART_CH_GetReceivedData(config->uart);
	}
	return i;
}

static void uart_xmc4xxx_irq_tx_enable(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	const struct uart_xmc4xxx_data *data = dev->data;

	clear_pending_interrupt(config->irq_num_tx);
	irq_enable(config->irq_num_tx);

	enable_tx_events(config);

	XMC_USIC_CH_TriggerServiceRequest(config->uart, data->service_request_tx);
}

static void uart_xmc4xxx_irq_tx_disable(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	if (config->fifo_tx_size > 0) {
		XMC_USIC_CH_TXFIFO_DisableEvent(config->uart,
						XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	} else {
		XMC_USIC_CH_DisableEvent(config->uart, XMC_USIC_CH_EVENT_TRANSMIT_SHIFT);
	}
}

static int uart_xmc4xxx_irq_rx_ready(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	if (config->fifo_rx_size > 0) {
		return !XMC_USIC_CH_RXFIFO_IsEmpty(config->uart);
	} else {
		return XMC_USIC_CH_GetReceiveBufferStatus(config->uart);
	}
}

static void uart_xmc4xxx_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb, void *user_data)
{
	struct uart_xmc4xxx_data *data = dev->data;

	data->user_cb = cb;
	data->user_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async_cb = NULL;
	data->async_user_data = NULL;
#endif
}

#define NVIC_ISPR_BASE 0xe000e200u
static int uart_xmc4xxx_irq_is_pending(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	uint32_t irq_num_tx = config->irq_num_tx;
	uint32_t irq_num_rx = config->irq_num_rx;
	bool tx_pending;
	bool rx_pending;
	uint32_t setpend;

	/* the NVIC_ISPR_BASE address stores info which interrupts are pending */
	/* bit 0 -> irq 0, bit 1 -> irq 1,...  */
	setpend = *((uint32_t *)(NVIC_ISPR_BASE) + irq_num_tx / 32);
	irq_num_tx = irq_num_tx & 0x1f; /* take modulo 32 */
	tx_pending = setpend & BIT(irq_num_tx);

	setpend = *((uint32_t *)(NVIC_ISPR_BASE) + irq_num_rx / 32);
	irq_num_rx = irq_num_rx & 0x1f; /* take modulo 32 */
	rx_pending = setpend & BIT(irq_num_rx);

	return tx_pending || rx_pending;
}
#endif

#if defined(CONFIG_UART_ASYNC_API)
static inline void async_evt_rx_buf_request(struct uart_xmc4xxx_data *data)
{
	struct uart_event evt = {.type = UART_RX_BUF_REQUEST};

	if (data->async_cb) {
		data->async_cb(data->dev, &evt, data->async_user_data);
	}
}

static inline void async_evt_rx_release_buffer(struct uart_xmc4xxx_data *data, int buffer_type)
{
	struct uart_event event = {.type = UART_RX_BUF_RELEASED};

	if (buffer_type == NEXT_BUFFER && !data->rx_next_buffer) {
		return;
	}

	if (buffer_type == CURRENT_BUFFER && !data->dma_rx.buffer) {
		return;
	}

	if (buffer_type == NEXT_BUFFER) {
		event.data.rx_buf.buf = data->rx_next_buffer;
		data->rx_next_buffer = NULL;
		data->rx_next_buffer_len = 0;
	} else {
		event.data.rx_buf.buf = data->dma_rx.buffer;
		data->dma_rx.buffer = NULL;
		data->dma_rx.buffer_len = 0;
	}

	if (data->async_cb) {
		data->async_cb(data->dev, &event, data->async_user_data);
	}
}

static inline void async_evt_rx_stopped(struct uart_xmc4xxx_data *data,
					enum uart_rx_stop_reason reason)
{
	struct uart_event event = {.type = UART_RX_STOPPED, .data.rx_stop.reason = reason};
	struct uart_event_rx *rx = &event.data.rx_stop.data;
	struct dma_status stat;

	if (data->dma_rx.buffer_len == 0 || data->async_cb == NULL) {
		return;
	}

	rx->buf = data->dma_rx.buffer;
	if (dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &stat) == 0) {
		data->dma_rx.counter = data->dma_rx.buffer_len - stat.pending_length;
	}

	rx->len = data->dma_rx.counter - data->dma_rx.offset;
	rx->offset = data->dma_rx.counter;

	data->async_cb(data->dev, &event, data->async_user_data);
}

static inline void async_evt_rx_disabled(struct uart_xmc4xxx_data *data)
{
	struct uart_event event = {.type = UART_RX_DISABLED};

	data->dma_rx.buffer = NULL;
	data->dma_rx.buffer_len = 0;
	data->dma_rx.offset = 0;
	data->dma_rx.counter = 0;

	if (data->async_cb) {
		data->async_cb(data->dev, &event, data->async_user_data);
	}
}

static inline void async_evt_rx_rdy(struct uart_xmc4xxx_data *data)
{
	struct uart_event event = {.type = UART_RX_RDY,
				   .data.rx.buf = (uint8_t *)data->dma_rx.buffer,
				   .data.rx.len = data->dma_rx.counter - data->dma_rx.offset,
				   .data.rx.offset = data->dma_rx.offset};

	data->dma_rx.offset = data->dma_rx.counter;

	if (event.data.rx.len > 0 && data->async_cb) {
		data->async_cb(data->dev, &event, data->async_user_data);
	}
}

static inline void async_evt_tx_done(struct uart_xmc4xxx_data *data)
{
	struct uart_event event = {.type = UART_TX_DONE,
				   .data.tx.buf = data->dma_tx.buffer,
				   .data.tx.len = data->dma_tx.counter};

	data->dma_tx.buffer = NULL;
	data->dma_tx.buffer_len = 0;
	data->dma_tx.counter = 0;

	if (data->async_cb) {
		data->async_cb(data->dev, &event, data->async_user_data);
	}
}

static inline void async_evt_tx_abort(struct uart_xmc4xxx_data *data)
{
	struct uart_event event = {.type = UART_TX_ABORTED,
				   .data.tx.buf = data->dma_tx.buffer,
				   .data.tx.len = data->dma_tx.counter};

	data->dma_tx.buffer = NULL;
	data->dma_tx.buffer_len = 0;
	data->dma_tx.counter = 0;

	if (data->async_cb) {
		data->async_cb(data->dev, &event, data->async_user_data);
	}
}

static void uart_xmc4xxx_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *rx_stream =
		CONTAINER_OF(dwork, struct uart_dma_stream, timeout_work);
	struct uart_xmc4xxx_data *data = CONTAINER_OF(rx_stream, struct uart_xmc4xxx_data, dma_rx);
	struct dma_status stat;
	unsigned int key = irq_lock();

	if (data->dma_rx.buffer_len == 0) {
		irq_unlock(key);
		return;
	}

	if (dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &stat) == 0) {
		size_t rx_rcv_len = data->dma_rx.buffer_len - stat.pending_length;

		if (rx_rcv_len > data->dma_rx.offset) {
			data->dma_rx.counter = rx_rcv_len;
			async_evt_rx_rdy(data);
		}
	}
	irq_unlock(key);
	async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
}

static int uart_xmc4xxx_async_tx_abort(const struct device *dev)
{
	struct uart_xmc4xxx_data *data = dev->data;
	struct dma_status stat;
	size_t tx_buffer_len;
	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->dma_tx.timeout_work);
	tx_buffer_len = data->dma_tx.buffer_len;

	if (tx_buffer_len == 0) {
		irq_unlock(key);
		return -EINVAL;
	}

	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_len - stat.pending_length;
	}

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	disable_tx_events(dev->config);
	async_evt_tx_abort(data);

	irq_unlock(key);

	return 0;
}

static void uart_xmc4xxx_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *tx_stream =
		CONTAINER_OF(dwork, struct uart_dma_stream, timeout_work);
	struct uart_xmc4xxx_data *data = CONTAINER_OF(tx_stream, struct uart_xmc4xxx_data, dma_tx);

	uart_xmc4xxx_async_tx_abort(data->dev);
}

static int uart_xmc4xxx_async_init(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	struct uart_xmc4xxx_data *data = dev->data;

	data->dev = dev;

	if (data->dma_rx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_rx.dma_dev)) {
			return -ENODEV;
		}

		k_work_init_delayable(&data->dma_rx.timeout_work, uart_xmc4xxx_async_rx_timeout);
		if (config->fifo_rx_size > 0) {
			data->dma_rx.blk_cfg.source_address = (uint32_t)&config->uart->OUTR;
		} else {
			data->dma_rx.blk_cfg.source_address = (uint32_t)&config->uart->RBUF;
		}

		data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
		data->dma_rx.dma_cfg.user_data = (void *)dev;
	}

	if (data->dma_tx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_tx.dma_dev)) {
			return -ENODEV;
		}

		k_work_init_delayable(&data->dma_tx.timeout_work, uart_xmc4xxx_async_tx_timeout);

		if (config->fifo_tx_size > 0) {
			data->dma_tx.blk_cfg.dest_address = (uint32_t)&config->uart->IN[0];
		} else {
			data->dma_tx.blk_cfg.dest_address = (uint32_t)&config->uart->TBUF[0];
		}

		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
		data->dma_tx.dma_cfg.user_data = (void *)dev;
	}

	return 0;
}

static int uart_xmc4xxx_async_callback_set(const struct device *dev, uart_callback_t callback,
					   void *user_data)
{
	struct uart_xmc4xxx_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->user_cb = NULL;
	data->user_data = NULL;
#endif

	return 0;
}

static int uart_xmc4xxx_async_tx(const struct device *dev, const uint8_t *tx_data, size_t buf_size,
				 int32_t timeout)
{
	struct uart_xmc4xxx_data *data = dev->data;
	const struct uart_xmc4xxx_config *config = dev->config;
	int ret;

	/* Assume threads are pre-emptive so this call cannot be interrupted */
	/* by uart_xmc4xxx_async_tx_abort */
	if (data->dma_tx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (tx_data == NULL || buf_size == 0) {
		return -EINVAL;
	}

	/* No need to lock irq. Isr uart_xmc4xxx_dma_tx_cb() will only trigger if */
	/* dma_tx.buffer_len != 0 */
	if (data->dma_tx.buffer_len != 0) {
		return -EBUSY;
	}

	data->dma_tx.buffer = (uint8_t *)tx_data;
	data->dma_tx.buffer_len = buf_size;
	data->dma_tx.timeout = timeout;

	/* set source address */
	data->dma_tx.blk_cfg.source_address = (uint32_t)data->dma_tx.buffer;
	data->dma_tx.blk_cfg.block_size = data->dma_tx.buffer_len;

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &data->dma_tx.dma_cfg);
	if (ret < 0) {
		return ret;
	}

	/* make sure the tx is not transmitting */
	while (!uart_xmc4xxx_irq_tx_ready(dev)) {
	};

	/* Tx irq is not used in async mode so disable it */
	irq_disable(config->irq_num_tx);
	enable_tx_events(config);
	XMC_USIC_CH_TriggerServiceRequest(config->uart, data->service_request_tx);

	async_timer_start(&data->dma_tx.timeout_work, data->dma_tx.timeout);

	return dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
}

static int uart_xmc4xxx_async_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
					int32_t timeout)
{
	struct uart_xmc4xxx_data *data = dev->data;
	int ret;

	if (data->dma_rx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->dma_rx.buffer_len != 0) {
		return -EBUSY;
	}

	uart_xmc4xxx_irq_rx_disable(dev);

	data->dma_rx.buffer = buf;
	data->dma_rx.buffer_len = len;
	data->dma_rx.timeout = timeout;

	data->dma_rx.blk_cfg.dest_address = (uint32_t)data->dma_rx.buffer;
	data->dma_rx.blk_cfg.block_size = data->dma_rx.buffer_len;

	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &data->dma_rx.dma_cfg);
	if (ret < 0) {
		return ret;
	}

	/* Request buffers before enabling rx. It's unlikely, but we may not */
	/* request a new buffer in time (for example if receive buffer size is one byte). */
	async_evt_rx_buf_request(data);
	uart_xmc4xxx_irq_rx_enable(dev);

	return dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
}

static void uart_xmc4xxx_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				   int status)
{
	const struct device *dev_uart = user_data;
	struct uart_xmc4xxx_data *data = dev_uart->data;
	unsigned int key;
	int ret;

	__ASSERT_NO_MSG(channel == data->dma_rx.dma_channel);
	key = irq_lock();
	k_work_cancel_delayable(&data->dma_rx.timeout_work);

	if (status < 0) {
		async_evt_rx_stopped(data, UART_ERROR_OVERRUN);
		uart_xmc4xxx_irq_rx_disable(dev_uart);
		dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		async_evt_rx_release_buffer(data, CURRENT_BUFFER);
		async_evt_rx_release_buffer(data, NEXT_BUFFER);
		async_evt_rx_disabled(data);
		goto done;
	}

	if (data->dma_rx.buffer_len == 0) {
		goto done;
	}

	data->dma_rx.counter = data->dma_rx.buffer_len;
	async_evt_rx_rdy(data);

	async_evt_rx_release_buffer(data, CURRENT_BUFFER);

	if (!data->rx_next_buffer) {
		uart_xmc4xxx_irq_rx_disable(dev_uart);
		dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		async_evt_rx_disabled(data);
		goto done;
	}

	data->dma_rx.buffer = data->rx_next_buffer;
	data->dma_rx.buffer_len = data->rx_next_buffer_len;
	data->dma_rx.offset = 0;
	data->dma_rx.counter = 0;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	ret = dma_reload(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
			 data->dma_rx.blk_cfg.source_address, (uint32_t)data->dma_rx.buffer,
			 data->dma_rx.buffer_len);

	if (ret < 0) {
		uart_xmc4xxx_irq_rx_disable(dev_uart);
		dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		async_evt_rx_release_buffer(data, CURRENT_BUFFER);
		async_evt_rx_disabled(data);
		goto done;
	}

	dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	async_evt_rx_buf_request(data);
	async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
done:
	irq_unlock(key);
}

static int uart_xmc4xxx_async_rx_disable(const struct device *dev)
{
	struct uart_xmc4xxx_data *data = dev->data;
	struct dma_status stat;
	unsigned int key;

	k_work_cancel_delayable(&data->dma_rx.timeout_work);

	key = irq_lock();

	if (data->dma_rx.buffer_len == 0) {
		__ASSERT_NO_MSG(data->dma_rx.buffer == NULL);
		irq_unlock(key);
		return -EINVAL;
	}

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
	uart_xmc4xxx_irq_rx_disable(dev);

	if (dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &stat) == 0) {
		size_t rx_rcv_len = data->dma_rx.buffer_len - stat.pending_length;

		if (rx_rcv_len > data->dma_rx.offset) {
			data->dma_rx.counter = rx_rcv_len;
			async_evt_rx_rdy(data);
		}
	}

	async_evt_rx_release_buffer(data, CURRENT_BUFFER);
	async_evt_rx_release_buffer(data, NEXT_BUFFER);
	async_evt_rx_disabled(data);

	irq_unlock(key);

	return 0;
}

static void uart_xmc4xxx_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				   int status)
{
	const struct device *dev_uart = user_data;
	struct uart_xmc4xxx_data *data = dev_uart->data;
	size_t tx_buffer_len = data->dma_tx.buffer_len;
	struct dma_status stat;

	if (status != 0) {
		return;
	}

	__ASSERT_NO_MSG(channel == data->dma_tx.dma_channel);

	k_work_cancel_delayable(&data->dma_tx.timeout_work);

	if (tx_buffer_len == 0) {
		return;
	}

	if (!dma_get_status(data->dma_tx.dma_dev, channel, &stat)) {
		data->dma_tx.counter = tx_buffer_len - stat.pending_length;
	}

	async_evt_tx_done(data);
	/* if the callback doesn't doesn't do a chained uart_tx write, then stop the dma */
	if (data->dma_tx.buffer == NULL) {
		dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
		disable_tx_events(dev_uart->config);
	}
}

static int uart_xmc4xxx_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_xmc4xxx_data *data = dev->data;
	unsigned int key;
	int ret = 0;

	key = irq_lock();

	if (data->dma_rx.buffer_len == 0U) {
		ret = -EACCES;
		goto done;
	}

	if (data->rx_next_buffer_len != 0U) {
		ret = -EBUSY;
		goto done;
	}

	data->rx_next_buffer = buf;
	data->rx_next_buffer_len = len;

done:
	irq_unlock(key);
	return ret;
}

#endif

static int uart_xmc4xxx_init(const struct device *dev)
{
	int ret;
	const struct uart_xmc4xxx_config *config = dev->config;
	struct uart_xmc4xxx_data *data = dev->data;
	uint8_t fifo_offset = config->fifo_start_offset;

	data->config.data_bits = 8U;
	data->config.stop_bits = 1U;

	XMC_UART_CH_Init(config->uart, &(data->config));

	if (config->fifo_tx_size > 0) {
		/* fifos need to be aligned on fifo size */
		fifo_offset = ROUND_UP(fifo_offset, BIT(config->fifo_tx_size));
		XMC_USIC_CH_TXFIFO_Configure(config->uart, fifo_offset, config->fifo_tx_size, 1);
		fifo_offset += BIT(config->fifo_tx_size);
	}

	if (config->fifo_rx_size > 0) {
		/* fifos need to be aligned on fifo size */
		fifo_offset = ROUND_UP(fifo_offset, BIT(config->fifo_rx_size));
		XMC_USIC_CH_RXFIFO_Configure(config->uart, fifo_offset, config->fifo_rx_size, 0);
		fifo_offset += BIT(config->fifo_rx_size);
	}

	if (fifo_offset > MAX_FIFO_SIZE) {
		return -EINVAL;
	}

	/* Connect UART RX to logical 1. It is connected to proper pin after pinctrl is applied */
	XMC_UART_CH_SetInputSource(config->uart, XMC_UART_CH_INPUT_RXD, 0x7);

	/* Start the UART before pinctrl, because the USIC is driving the TX line */
	/* low in off state */
	XMC_UART_CH_Start(config->uart);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	/* Connect UART RX to the target pin */
	XMC_UART_CH_SetInputSource(config->uart, XMC_UART_CH_INPUT_RXD,
				   config->input_src);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	config->irq_config_func(dev);
	uart_xmc4xxx_configure_service_requests(dev);
#endif

#if defined(CONFIG_UART_ASYNC_API)
	ret = uart_xmc4xxx_async_init(dev);
#endif

	return ret;
}

static DEVICE_API(uart, uart_xmc4xxx_driver_api) = {
	.poll_in = uart_xmc4xxx_poll_in,
	.poll_out = uart_xmc4xxx_poll_out,
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	.fifo_fill = uart_xmc4xxx_fifo_fill,
	.fifo_read = uart_xmc4xxx_fifo_read,
	.irq_tx_enable = uart_xmc4xxx_irq_tx_enable,
	.irq_tx_disable = uart_xmc4xxx_irq_tx_disable,
	.irq_tx_ready = uart_xmc4xxx_irq_tx_ready,
	.irq_rx_enable = uart_xmc4xxx_irq_rx_enable,
	.irq_rx_disable = uart_xmc4xxx_irq_rx_disable,
	.irq_rx_ready = uart_xmc4xxx_irq_rx_ready,
	.irq_callback_set = uart_xmc4xxx_irq_callback_set,
	.irq_is_pending = uart_xmc4xxx_irq_is_pending,
#endif
#if defined(CONFIG_UART_ASYNC_API)
	.callback_set = uart_xmc4xxx_async_callback_set,
	.tx = uart_xmc4xxx_async_tx,
	.tx_abort = uart_xmc4xxx_async_tx_abort,
	.rx_enable = uart_xmc4xxx_async_rx_enable,
	.rx_buf_rsp = uart_xmc4xxx_rx_buf_rsp,
	.rx_disable = uart_xmc4xxx_async_rx_disable,
#endif
};

#ifdef CONFIG_UART_ASYNC_API
#define UART_DMA_CHANNEL_INIT(index, dir, ch_dir, src_burst, dst_burst)                            \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = {                                                                               \
		.dma_slot = DT_INST_DMAS_CELL_BY_NAME(index, dir, config),                         \
		.channel_direction = ch_dir,                                                       \
		.channel_priority = DT_INST_DMAS_CELL_BY_NAME(index, dir, priority),               \
		.source_data_size = 1,                                                             \
		.dest_data_size = 1,                                                               \
		.source_burst_length = src_burst,                                                  \
		.dest_burst_length = dst_burst,                                                    \
		.block_count = 1,                                                                  \
		.dma_callback = uart_xmc4xxx_dma_##dir##_cb,                                       \
	},

#define UART_DMA_CHANNEL(index, dir, ch_dir, src_burst, dst_burst)                                 \
	.dma_##dir = {COND_CODE_1(                                                                 \
		DT_INST_DMAS_HAS_NAME(index, dir),                                                 \
		(UART_DMA_CHANNEL_INIT(index, dir, ch_dir, src_burst, dst_burst)), (NULL))},
#else
#define UART_DMA_CHANNEL(index, dir, ch_dir, src_burst, dst_burst)
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define XMC4XXX_IRQ_HANDLER(index)                                                         \
static void uart_xmc4xxx_irq_setup_##index(const struct device *dev)                       \
{                                                                                          \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tx, irq),                                   \
		    DT_INST_IRQ_BY_NAME(index, tx, priority), uart_xmc4xxx_isr,            \
		    DEVICE_DT_INST_GET(index), 0);                                         \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rx, irq),                                   \
		    DT_INST_IRQ_BY_NAME(index, rx, priority), uart_xmc4xxx_isr,            \
		    DEVICE_DT_INST_GET(index), 0);                                         \
	irq_enable(DT_INST_IRQ_BY_NAME(index, tx, irq));                                   \
	irq_enable(DT_INST_IRQ_BY_NAME(index, rx, irq));                                   \
}

#define XMC4XXX_IRQ_STRUCT_INIT(index)                                                     \
	.irq_config_func = uart_xmc4xxx_irq_setup_##index,                                 \
	.irq_num_tx = DT_INST_IRQ_BY_NAME(index, tx, irq),                                 \
	.irq_num_rx = DT_INST_IRQ_BY_NAME(index, rx, irq),

#else
#define XMC4XXX_IRQ_HANDLER(index)
#define XMC4XXX_IRQ_STRUCT_INIT(index)
#endif

#define XMC4XXX_INIT(index)						\
PINCTRL_DT_INST_DEFINE(index);						\
XMC4XXX_IRQ_HANDLER(index)						\
static struct uart_xmc4xxx_data xmc4xxx_data_##index = {		\
	.config.baudrate = DT_INST_PROP(index, current_speed),		\
	UART_DMA_CHANNEL(index, tx, MEMORY_TO_PERIPHERAL, 8, 1)         \
	UART_DMA_CHANNEL(index, rx, PERIPHERAL_TO_MEMORY, 1, 8)         \
};									\
									\
static const struct uart_xmc4xxx_config xmc4xxx_config_##index = {	\
	.uart = (XMC_USIC_CH_t *)DT_INST_REG_ADDR(index),		\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.input_src = DT_INST_ENUM_IDX(index, input_src),		\
XMC4XXX_IRQ_STRUCT_INIT(index)						\
	.fifo_start_offset = DT_INST_PROP(index, fifo_start_offset),    \
	.fifo_tx_size = DT_INST_ENUM_IDX(index, fifo_tx_size),          \
	.fifo_rx_size = DT_INST_ENUM_IDX(index, fifo_rx_size),          \
};									\
									\
	DEVICE_DT_INST_DEFINE(index, uart_xmc4xxx_init,			\
			    NULL,					\
			    &xmc4xxx_data_##index,			\
			    &xmc4xxx_config_##index, PRE_KERNEL_1,	\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &uart_xmc4xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XMC4XXX_INIT)
