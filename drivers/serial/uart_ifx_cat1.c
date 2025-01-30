/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for Infineon CAT1 MCU family.
 *
 */

#define DT_DRV_COMPAT infineon_cat1_uart

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <cyhal_uart.h>
#include <cyhal_utils_impl.h>
#include <cyhal_scb_common.h>

#include "cy_scb_uart.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_ifx_cat1, CONFIG_UART_LOG_LEVEL);

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#include <cyhal_dma.h>

extern int ifx_cat1_dma_ex_connect_digital(const struct device *dev, uint32_t channel,
					   cyhal_source_t source, cyhal_dma_input_t input);

struct ifx_cat1_dma_stream {
	const struct device *dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	uint8_t *buf;
	size_t buf_len;
	size_t offset;
	size_t counter;
	uint32_t timeout;
	size_t dma_transmitted_bytes;

	struct k_work_delayable timeout_work;
};

struct ifx_cat1_uart_async {
	const struct device *uart_dev;
	uart_callback_t cb;
	void *user_data;

	struct ifx_cat1_dma_stream dma_rx;
	struct ifx_cat1_dma_stream dma_tx;

	uint8_t *rx_next_buf;
	size_t rx_next_buf_len;
};

#define CURRENT_BUFFER 0
#define NEXT_BUFFER    1

#endif /* CONFIG_UART_ASYNC_API */

/* Data structure */
struct ifx_cat1_uart_data {
	cyhal_uart_t obj; /* UART CYHAL object */
	struct uart_config cfg;
	cyhal_resource_inst_t hw_resource;
	cyhal_clock_t clock;

#if CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb; /* Interrupt Callback */
	void *irq_cb_data;                    /* Interrupt Callback Arg */
#endif

#ifdef CONFIG_UART_ASYNC_API
	struct ifx_cat1_uart_async async;
#endif
};

/* Device config structure */
struct ifx_cat1_uart_config {
	const struct pinctrl_dev_config *pcfg;
	CySCB_Type *reg_addr;
	struct uart_config dt_cfg;
	uint8_t irq_priority;
};

/* Default Counter configuration structure */
static const cy_stc_scb_uart_config_t _cyhal_uart_default_config = {
	.uartMode = CY_SCB_UART_STANDARD,
	.enableMutliProcessorMode = false,
	.smartCardRetryOnNack = false,
	.irdaInvertRx = false,
	.irdaEnableLowPowerReceiver = false,
	.oversample = 12,
	.enableMsbFirst = false,
	.dataWidth = 8UL,
	.parity = CY_SCB_UART_PARITY_NONE,
	.stopBits = CY_SCB_UART_STOP_BITS_1,
	.enableInputFilter = false,
	.breakWidth = 11UL,
	.dropOnFrameError = false,
	.dropOnParityError = false,

	.receiverAddress = 0x0UL,
	.receiverAddressMask = 0x0UL,
	.acceptAddrInFifo = false,

	.enableCts = false,
	.ctsPolarity = CY_SCB_UART_ACTIVE_LOW,
#if defined(COMPONENT_CAT1A) || defined(COMPONENT_CAT1B)
	.rtsRxFifoLevel = 20UL,
#elif defined(COMPONENT_CAT2)
	.rtsRxFifoLevel = 3UL,
#endif
	.rtsPolarity = CY_SCB_UART_ACTIVE_LOW,

	/* Level triggers when at least one element is in FIFO */
	.rxFifoTriggerLevel = 0UL,
	.rxFifoIntEnableMask = 0x0UL,

	/* Level triggers when half-fifo is half empty */
	.txFifoTriggerLevel = (CY_SCB_FIFO_SIZE / 2 - 1),
	.txFifoIntEnableMask = 0x0UL
};

/* Helper API */
static cyhal_uart_parity_t _convert_uart_parity_z_to_cyhal(enum uart_config_parity parity)
{
	cyhal_uart_parity_t cyhal_parity;

	switch (parity) {
	case UART_CFG_PARITY_NONE:
		cyhal_parity = CYHAL_UART_PARITY_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		cyhal_parity = CYHAL_UART_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		cyhal_parity = CYHAL_UART_PARITY_EVEN;
		break;
	default:
		cyhal_parity = CYHAL_UART_PARITY_NONE;
	}
	return cyhal_parity;
}

static uint32_t _convert_uart_stop_bits_z_to_cyhal(enum uart_config_stop_bits stop_bits)
{
	uint32_t cyhal_stop_bits;

	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1:
		cyhal_stop_bits = 1u;
		break;

	case UART_CFG_STOP_BITS_2:
		cyhal_stop_bits = 2u;
		break;
	default:
		cyhal_stop_bits = 1u;
	}
	return cyhal_stop_bits;
}

static uint32_t _convert_uart_data_bits_z_to_cyhal(enum uart_config_data_bits data_bits)
{
	uint32_t cyhal_data_bits;

	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		cyhal_data_bits = 1u;
		break;

	case UART_CFG_DATA_BITS_6:
		cyhal_data_bits = 6u;
		break;

	case UART_CFG_DATA_BITS_7:
		cyhal_data_bits = 7u;
		break;

	case UART_CFG_DATA_BITS_8:
		cyhal_data_bits = 8u;
		break;

	case UART_CFG_DATA_BITS_9:
		cyhal_data_bits = 9u;
		break;

	default:
		cyhal_data_bits = 1u;
	}
	return cyhal_data_bits;
}

static int32_t _get_hw_block_num(CySCB_Type *reg_addr)
{
	extern const uint8_t _CYHAL_SCB_BASE_ADDRESS_INDEX[_SCB_ARRAY_SIZE];
	extern CySCB_Type *const _CYHAL_SCB_BASE_ADDRESSES[_SCB_ARRAY_SIZE];

	uint32_t i;

	for (i = 0u; i < _SCB_ARRAY_SIZE; i++) {
		if (_CYHAL_SCB_BASE_ADDRESSES[i] == reg_addr) {
			return _CYHAL_SCB_BASE_ADDRESS_INDEX[i];
		}
	}

	return -1;
}

uint32_t ifx_cat1_uart_get_num_in_tx_fifo(const struct device *dev)
{
	const struct ifx_cat1_uart_config *const config = dev->config;

	return Cy_SCB_GetNumInTxFifo(config->reg_addr);
}

bool ifx_cat1_uart_get_tx_active(const struct device *dev)
{
	const struct ifx_cat1_uart_config *const config = dev->config;

	return Cy_SCB_GetTxSrValid(config->reg_addr) ? true : false;
}

static int ifx_cat1_uart_poll_in(const struct device *dev, unsigned char *c)
{
	cy_rslt_t rec;
	struct ifx_cat1_uart_data *data = dev->data;

	rec = cyhal_uart_getc(&data->obj, c, 0u);

	return ((rec == CY_SCB_UART_RX_NO_DATA) ? -1 : 0);
}

static void ifx_cat1_uart_poll_out(const struct device *dev, unsigned char c)
{
	struct ifx_cat1_uart_data *data = dev->data;

	(void)cyhal_uart_putc(&data->obj, (uint32_t)c);
}

static int ifx_cat1_uart_err_check(const struct device *dev)
{
	struct ifx_cat1_uart_data *data = dev->data;
	uint32_t status = Cy_SCB_UART_GetRxFifoStatus(data->obj.base);
	int errors = 0;

	if (status & CY_SCB_UART_RX_OVERFLOW) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (status & CY_SCB_UART_RX_ERR_PARITY) {
		errors |= UART_ERROR_PARITY;
	}

	if (status & CY_SCB_UART_RX_ERR_FRAME) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int ifx_cat1_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	__ASSERT_NO_MSG(cfg != NULL);

	cy_rslt_t result;
	struct ifx_cat1_uart_data *data = dev->data;

	cyhal_uart_cfg_t uart_cfg = {
		.data_bits = _convert_uart_data_bits_z_to_cyhal(cfg->data_bits),
		.stop_bits = _convert_uart_stop_bits_z_to_cyhal(cfg->stop_bits),
		.parity = _convert_uart_parity_z_to_cyhal(cfg->parity)};

	/* Store Uart Zephyr configuration (uart config) into data structure */
	data->cfg = *cfg;

	/* Configure parity, data and stop bits */
	result = cyhal_uart_configure(&data->obj, &uart_cfg);

	/* Configure the baud rate */
	if (result == CY_RSLT_SUCCESS) {
		result = cyhal_uart_set_baud(&data->obj, cfg->baudrate, NULL);
	}

	/* Set RTS/CTS flow control pins as NC so cyhal will skip initialization */
	data->obj.pin_cts = NC;
	data->obj.pin_rts = NC;

	/* Enable RTS/CTS flow control */
	if ((result == CY_RSLT_SUCCESS) && cfg->flow_ctrl) {
		result = cyhal_uart_enable_flow_control(&data->obj, true, true);
	}

	return (result == CY_RSLT_SUCCESS) ? 0 : -ENOTSUP;
};

static int ifx_cat1_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	ARG_UNUSED(dev);

	struct ifx_cat1_uart_data *const data = dev->data;

	if (cfg == NULL) {
		return -EINVAL;
	}

	*cfg = data->cfg;
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* Uart event callback for Interrupt driven mode */
static void _uart_event_callback_irq_mode(void *arg, cyhal_uart_event_t event)
{
	ARG_UNUSED(event);

	const struct device *dev = (const struct device *)arg;
	struct ifx_cat1_uart_data *const data = dev->data;

	if (data->irq_cb != NULL) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

/* Fill FIFO with data */
static int ifx_cat1_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	size_t _size = (size_t)size;

	(void)cyhal_uart_write(&data->obj, (uint8_t *)tx_data, &_size);
	return (int)_size;
}

/* Read data from FIFO */
static int ifx_cat1_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	size_t _size = (size_t)size;

	(void)cyhal_uart_read(&data->obj, rx_data, &_size);
	return (int)_size;
}

/* Enable TX interrupt */
static void ifx_cat1_uart_irq_tx_enable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj, (cyhal_uart_event_t)CYHAL_UART_IRQ_TX_EMPTY,
				config->irq_priority, 1);
}

/* Disable TX interrupt */
static void ifx_cat1_uart_irq_tx_disable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj, (cyhal_uart_event_t)CYHAL_UART_IRQ_TX_EMPTY,
				config->irq_priority, 0);
}

/* Check if UART TX buffer can accept a new char */
static int ifx_cat1_uart_irq_tx_ready(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	uint32_t mask = Cy_SCB_GetTxInterruptStatusMasked(data->obj.base);

	return (((mask & (CY_SCB_UART_TX_NOT_FULL | SCB_INTR_TX_EMPTY_Msk)) != 0u) ? 1 : 0);
}

/* Check if UART TX block finished transmission */
static int ifx_cat1_uart_irq_tx_complete(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;

	return (int)!(cyhal_uart_is_tx_active(&data->obj));
}

/* Enable RX interrupt */
static void ifx_cat1_uart_irq_rx_enable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj, (cyhal_uart_event_t)CYHAL_UART_IRQ_RX_NOT_EMPTY,
				config->irq_priority, 1);
}

/* Disable TX interrupt */
static void ifx_cat1_uart_irq_rx_disable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj, (cyhal_uart_event_t)CYHAL_UART_IRQ_RX_NOT_EMPTY,
				config->irq_priority, 0);
}

/* Check if UART RX buffer has a received char */
static int ifx_cat1_uart_irq_rx_ready(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;

	return cyhal_uart_readable(&data->obj) ? 1 : 0;
}

/* Enable Error interrupts */
static void ifx_cat1_uart_irq_err_enable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(
		&data->obj, (cyhal_uart_event_t)(CYHAL_UART_IRQ_TX_ERROR | CYHAL_UART_IRQ_RX_ERROR),
		config->irq_priority, 1);
}

/* Disable Error interrupts */
static void ifx_cat1_uart_irq_err_disable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(
		&data->obj, (cyhal_uart_event_t)(CYHAL_UART_IRQ_TX_ERROR | CYHAL_UART_IRQ_RX_ERROR),
		config->irq_priority, 0);
}

/* Check if any IRQs is pending */
static int ifx_cat1_uart_irq_is_pending(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	uint32_t intcause = Cy_SCB_GetInterruptCause(data->obj.base);

	return (int)(intcause & (CY_SCB_TX_INTR | CY_SCB_RX_INTR));
}

/* Start processing interrupts in ISR.
 * This function should be called the first thing in the ISR. Calling
 * uart_irq_rx_ready(), uart_irq_tx_ready(), uart_irq_tx_complete()
 * allowed only after this.
 */
static int ifx_cat1_uart_irq_update(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	int status = 1;

	if (((ifx_cat1_uart_irq_is_pending(dev) & CY_SCB_RX_INTR) != 0u) &&
	    (Cy_SCB_UART_GetNumInRxFifo(data->obj.base) == 0u)) {
		status = 0;
	}

	return status;
}

static void ifx_cat1_uart_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct ifx_cat1_uart_data *data = dev->data;
	cyhal_uart_t *uart_obj = &data->obj;

	/* Store user callback info */
	data->irq_cb = cb;
	data->irq_cb_data = cb_data;

	/* Register a uart general callback handler  */
	cyhal_uart_register_callback(uart_obj, _uart_event_callback_irq_mode, (void *)dev);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static int ifx_cat1_uart_async_callback_set(const struct device *dev, uart_callback_t callback,
					    void *user_data)
{
	struct ifx_cat1_uart_data *const data = dev->data;

	data->async.cb = callback;
	data->async.user_data = user_data;
	data->async.dma_tx.dma_cfg.user_data = (void *)dev;

	return 0;
}

/* Async DMA helper */
static int ifx_cat1_uart_async_dma_config_buffer(const struct device *dev, bool tx)
{
	int ret;
	struct ifx_cat1_uart_data *const data = dev->data;
	struct ifx_cat1_dma_stream *dma_stream = tx ? &data->async.dma_tx : &data->async.dma_rx;

	/* Configure byte mode */
	dma_stream->blk_cfg.block_size = dma_stream->buf_len;

	if (tx) {
		dma_stream->blk_cfg.source_address = (uint32_t)dma_stream->buf;
	} else {
		dma_stream->blk_cfg.dest_address = (uint32_t)dma_stream->buf;
	}

	ret = dma_config(dma_stream->dev, dma_stream->dma_channel, &dma_stream->dma_cfg);

	if (!ret) {
		ret = dma_start(dma_stream->dev, dma_stream->dma_channel);
	}

	return ret;
}

static int ifx_cat1_uart_async_tx(const struct device *dev, const uint8_t *tx_data,
				  size_t tx_data_size, int32_t timeout)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct device *dev_dma = data->async.dma_tx.dev;
	int err;

	if (dev_dma == NULL) {
		err = -ENODEV;
		goto exit;
	}

	if (tx_data == NULL || tx_data_size == 0) {
		err = -EINVAL;
		goto exit;
	}

	/* Store information about data buffer need to send */
	data->async.dma_tx.buf = (uint8_t *)tx_data;
	data->async.dma_tx.buf_len = tx_data_size;
	data->async.dma_tx.blk_cfg.block_size = 0;
	data->async.dma_tx.dma_transmitted_bytes = 0;

	/* Configure dma to transfer */
	err = ifx_cat1_uart_async_dma_config_buffer(dev, true);
	if (err) {
		LOG_ERR("Error Tx DMA configure (%d)", err);
		goto exit;
	}

	/* Configure timeout */
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		k_work_reschedule(&data->async.dma_tx.timeout_work, K_USEC(timeout));
	}

exit:
	return err;
}

static int ifx_cat1_uart_async_tx_abort(const struct device *dev)
{
	struct ifx_cat1_uart_data *data = dev->data;
	struct uart_event evt = {0};
	struct dma_status stat;
	int err = 0;
	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async.dma_tx.timeout_work);

	err = dma_stop(data->async.dma_tx.dev, data->async.dma_tx.dma_channel);
	if (err) {
		LOG_ERR("Error stopping Tx DMA (%d)", err);
		goto unlock;
	}

	err = dma_get_status(data->async.dma_tx.dev, data->async.dma_tx.dma_channel, &stat);
	if (err) {
		LOG_ERR("Error stopping Tx DMA (%d)", err);
		goto unlock;
	}

	evt.type = UART_TX_ABORTED;
	evt.data.tx.buf = data->async.dma_tx.buf;
	evt.data.tx.len = 0;

	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

unlock:
	irq_unlock(key);
	return err;
}

static void dma_callback_tx_done(const struct device *dma_dev, void *arg, uint32_t channel,
				 int status)
{
	const struct device *uart_dev = (void *)arg;
	struct ifx_cat1_uart_data *const data = uart_dev->data;
	unsigned int key = irq_lock();

	if (status == 0) {

		k_work_cancel_delayable(&data->async.dma_tx.timeout_work);
		dma_stop(data->async.dma_tx.dev, data->async.dma_tx.dma_channel);

		struct uart_event evt = {.type = UART_TX_DONE,
					 .data.tx.buf = data->async.dma_tx.buf,
					 .data.tx.len = data->async.dma_tx.buf_len};

		data->async.dma_tx.buf = NULL;
		data->async.dma_tx.buf_len = 0;

		if (data->async.cb) {
			data->async.cb(uart_dev, &evt, data->async.user_data);
		}

	} else {
		/* DMA error */
		dma_stop(data->async.dma_tx.dev, data->async.dma_tx.dma_channel);
	}
	irq_unlock(key);
}

static void ifx_cat1_uart_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ifx_cat1_dma_stream *dma_tx =
		CONTAINER_OF(dwork, struct ifx_cat1_dma_stream, timeout_work);
	struct ifx_cat1_uart_async *async =
		CONTAINER_OF(dma_tx, struct ifx_cat1_uart_async, dma_tx);

	(void)ifx_cat1_uart_async_tx_abort(async->uart_dev);
}

static inline void async_evt_rx_rdy(struct ifx_cat1_uart_data *data)
{
	struct uart_event event = {.type = UART_RX_RDY,
				   .data.rx.buf = (uint8_t *)data->async.dma_rx.buf,
				   .data.rx.len =
					   data->async.dma_rx.counter - data->async.dma_rx.offset,
				   .data.rx.offset = data->async.dma_rx.offset};

	data->async.dma_rx.offset = data->async.dma_rx.counter;

	if (event.data.rx.len > 0 && data->async.cb) {
		data->async.cb(data->async.uart_dev, &event, data->async.user_data);
	}
}

static inline void async_evt_rx_buf_request(struct ifx_cat1_uart_data *data)
{
	struct uart_event evt = {.type = UART_RX_BUF_REQUEST};

	if (data->async.cb) {
		data->async.cb(data->async.uart_dev, &evt, data->async.user_data);
	}
}

static inline void async_evt_rx_release_buffer(struct ifx_cat1_uart_data *data, int buffer_type)
{
	struct uart_event event = {.type = UART_RX_BUF_RELEASED};

	if (buffer_type == NEXT_BUFFER && !data->async.rx_next_buf) {
		return;
	}

	if (buffer_type == CURRENT_BUFFER && !data->async.dma_rx.buf) {
		return;
	}

	if (buffer_type == NEXT_BUFFER) {
		event.data.rx_buf.buf = data->async.rx_next_buf;
		data->async.rx_next_buf = NULL;
		data->async.rx_next_buf_len = 0;
	} else {
		event.data.rx_buf.buf = data->async.dma_rx.buf;
		data->async.dma_rx.buf = NULL;
		data->async.dma_rx.buf_len = 0;
	}

	if (data->async.cb) {
		data->async.cb(data->async.uart_dev, &event, data->async.user_data);
	}
}

static inline void async_evt_rx_disabled(struct ifx_cat1_uart_data *data)
{
	struct uart_event event = {.type = UART_RX_DISABLED};

	data->async.dma_rx.buf = NULL;
	data->async.dma_rx.buf_len = 0;
	data->async.dma_rx.offset = 0;
	data->async.dma_rx.counter = 0;

	if (data->async.cb) {
		data->async.cb(data->async.uart_dev, &event, data->async.user_data);
	}
}

static inline void async_evt_rx_stopped(struct ifx_cat1_uart_data *data,
					enum uart_rx_stop_reason reason)
{
	struct uart_event event = {.type = UART_RX_STOPPED, .data.rx_stop.reason = reason};
	struct uart_event_rx *rx = &event.data.rx_stop.data;
	struct dma_status stat;

	if (data->async.dma_rx.buf_len == 0 || data->async.cb == NULL) {
		return;
	}

	rx->buf = data->async.dma_rx.buf;

	if (dma_get_status(data->async.dma_rx.dev, data->async.dma_rx.dma_channel, &stat) == 0) {
		data->async.dma_rx.counter = data->async.dma_rx.buf_len - stat.pending_length;
	}
	rx->len = data->async.dma_rx.counter - data->async.dma_rx.offset;
	rx->offset = data->async.dma_rx.counter;

	data->async.cb(data->async.uart_dev, &event, data->async.user_data);
}

static int ifx_cat1_uart_async_rx_enable(const struct device *dev, uint8_t *rx_data,
					 size_t rx_data_size, int32_t timeout)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	struct dma_status dma_status = {0};
	int err = 0;
	unsigned int key = irq_lock();

	if (data->async.dma_rx.dev == NULL) {
		return -ENODEV;
	}

	if (data->async.dma_rx.buf_len != 0) {
		return -EBUSY;
	}

	/* Store information about data buffer need to send */
	data->async.dma_rx.buf = (uint8_t *)rx_data;
	data->async.dma_rx.buf_len = rx_data_size;
	data->async.dma_rx.blk_cfg.block_size = 0;
	data->async.dma_rx.dma_transmitted_bytes = 0;
	data->async.dma_rx.timeout = timeout;

	/* Request buffers before enabling rx */
	async_evt_rx_buf_request(data);

	/* Configure dma to transfer */
	err = ifx_cat1_uart_async_dma_config_buffer(dev, false);
	if (err) {
		LOG_ERR("Error Rx DMA configure (%d)", err);
		goto unlock;
	}
	err = dma_get_status(data->async.dma_rx.dev, data->async.dma_rx.dma_channel, &dma_status);
	if (err) {
		return err;
	}

	if (dma_status.busy) {
		return -EBUSY;
	}

	/* Configure timeout */
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		k_work_reschedule(&data->async.dma_rx.timeout_work, K_USEC(timeout));
	}

unlock:
	irq_unlock(key);
	return err;
}

static void dma_callback_rx_rdy(const struct device *dma_dev, void *arg, uint32_t channel,
				int status)
{
	const struct device *uart_dev = (void *)arg;
	struct ifx_cat1_uart_data *const data = uart_dev->data;
	unsigned int key = irq_lock();

	if (status == 0) {
		/* All data are sent, call user callback */

		k_work_cancel_delayable(&data->async.dma_rx.timeout_work);
		data->async.dma_rx.counter = data->async.dma_rx.buf_len;

		async_evt_rx_rdy(data);
		async_evt_rx_release_buffer(data, CURRENT_BUFFER);

		data->async.dma_rx.buf = NULL;
		data->async.dma_rx.buf_len = 0;
		data->async.dma_rx.blk_cfg.block_size = 0;
		data->async.dma_rx.dma_transmitted_bytes = 0;

		if (!data->async.rx_next_buf) {
			dma_stop(data->async.dma_rx.dev, data->async.dma_rx.dma_channel);
			async_evt_rx_disabled(data);
			goto unlock;
		}

		data->async.dma_rx.buf = data->async.rx_next_buf;
		data->async.dma_rx.buf_len = data->async.rx_next_buf_len;
		data->async.dma_rx.offset = 0;
		data->async.dma_rx.counter = 0;
		data->async.rx_next_buf = NULL;
		data->async.rx_next_buf_len = 0;

		ifx_cat1_uart_async_dma_config_buffer(uart_dev, false);

		async_evt_rx_buf_request(data);

		if ((data->async.dma_rx.timeout != SYS_FOREVER_US) &&
		    (data->async.dma_rx.timeout != 0)) {
			k_work_reschedule(&data->async.dma_rx.timeout_work,
					  K_USEC(data->async.dma_rx.timeout));
		}

	} else {
		/* DMA error */
		dma_stop(data->async.dma_rx.dev, data->async.dma_rx.dma_channel);

		async_evt_rx_stopped(data, UART_ERROR_OVERRUN);
		async_evt_rx_release_buffer(data, CURRENT_BUFFER);
		async_evt_rx_release_buffer(data, NEXT_BUFFER);
		async_evt_rx_disabled(data);
		goto unlock;
	}
unlock:
	irq_unlock(key);
}

static void ifx_cat1_uart_async_rx_timeout(struct k_work *work)
{

	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ifx_cat1_dma_stream *dma_rx =
		CONTAINER_OF(dwork, struct ifx_cat1_dma_stream, timeout_work);
	struct ifx_cat1_uart_async *async =
		CONTAINER_OF(dma_rx, struct ifx_cat1_uart_async, dma_rx);
	struct ifx_cat1_uart_data *data = CONTAINER_OF(async, struct ifx_cat1_uart_data, async);

	struct dma_status stat;
	unsigned int key = irq_lock();

	if (dma_rx->buf_len == 0) {
		irq_unlock(key);
		return;
	}
	if (dma_get_status(dma_rx->dev, dma_rx->dma_channel, &stat) == 0) {
		size_t rx_rcv_len = dma_rx->buf_len - stat.pending_length;

		if ((rx_rcv_len > 0) && (rx_rcv_len == dma_rx->counter)) {
			dma_rx->counter = rx_rcv_len;
			async_evt_rx_rdy(data);
		} else {
			dma_rx->counter = rx_rcv_len;
		}
	}
	irq_unlock(key);

	if ((dma_rx->timeout != SYS_FOREVER_US) && (dma_rx->timeout != 0)) {
		k_work_reschedule(&dma_rx->timeout_work, K_USEC(dma_rx->timeout));
	}
}

static int ifx_cat1_uart_async_rx_disable(const struct device *dev)
{
	struct ifx_cat1_uart_data *data = dev->data;
	struct dma_status stat;
	unsigned int key;

	k_work_cancel_delayable(&data->async.dma_rx.timeout_work);

	key = irq_lock();

	if (data->async.dma_rx.buf_len == 0) {
		__ASSERT_NO_MSG(data->async.dma_rx.buf == NULL);
		irq_unlock(key);
		return -EINVAL;
	}

	dma_stop(data->async.dma_rx.dev, data->async.dma_rx.dma_channel);

	if (dma_get_status(data->async.dma_rx.dev, data->async.dma_rx.dma_channel, &stat) == 0) {
		size_t rx_rcv_len = data->async.dma_rx.buf_len - stat.pending_length;

		if (rx_rcv_len > data->async.dma_rx.offset) {
			data->async.dma_rx.counter = rx_rcv_len;
			async_evt_rx_rdy(data);
		}
	}
	async_evt_rx_release_buffer(data, CURRENT_BUFFER);
	async_evt_rx_release_buffer(data, NEXT_BUFFER);
	async_evt_rx_disabled(data);

	irq_unlock(key);
	return 0;
}

static int ifx_cat1_uart_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct ifx_cat1_uart_data *data = dev->data;
	unsigned int key;
	int ret = 0;

	key = irq_lock();

	if (data->async.dma_rx.buf_len == 0U) {
		ret = -EACCES;
		goto unlock;
	}

	if (data->async.rx_next_buf_len != 0U) {
		ret = -EBUSY;
		goto unlock;
	}

	data->async.rx_next_buf = buf;
	data->async.rx_next_buf_len = len;

unlock:
	irq_unlock(key);
	return ret;
}

#endif /*CONFIG_UART_ASYNC_API */

static int ifx_cat1_uart_init(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;
	cy_rslt_t result;
	int ret;

	cyhal_uart_configurator_t uart_init_cfg = {
		.resource = &data->hw_resource,
		.config = &_cyhal_uart_default_config,
		.clock = &data->clock,
		.gpios = {.pin_tx = NC, .pin_rts = NC, .pin_cts = NC},
	};

	/* Dedicate SCB HW resource */
	data->hw_resource.type = CYHAL_RSC_SCB;
	data->hw_resource.block_num = _get_hw_block_num(config->reg_addr);

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Allocates clock for selected IP block */
	result = _cyhal_utils_allocate_clock(&data->clock, &data->hw_resource,
					     CYHAL_CLOCK_BLOCK_PERIPHERAL_16BIT, true);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Assigns a programmable divider to a selected IP block */
	en_clk_dst_t clk_idx = _cyhal_scb_get_clock_index(uart_init_cfg.resource->block_num);

	result = _cyhal_utils_peri_pclk_assign_divider(clk_idx, uart_init_cfg.clock);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Initialize the UART peripheral */
	result = cyhal_uart_init_cfg(&data->obj, &uart_init_cfg);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Perform initial Uart configuration */
	data->obj.is_clock_owned = true;
	ret = ifx_cat1_uart_configure(dev, &config->dt_cfg);

#ifdef CONFIG_UART_ASYNC_API
	data->async.uart_dev = dev;
	if (data->async.dma_rx.dev != NULL) {
		cyhal_source_t uart_source;

		if (!device_is_ready(data->async.dma_rx.dev)) {
			return -ENODEV;
		}

		data->async.dma_rx.blk_cfg.source_address =
			(uint32_t)(&config->reg_addr->RX_FIFO_RD);
		data->async.dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->async.dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->async.dma_rx.dma_cfg.head_block = &data->async.dma_rx.blk_cfg;
		data->async.dma_rx.dma_cfg.user_data = (void *)dev;
		data->async.dma_rx.dma_cfg.dma_callback = dma_callback_rx_rdy;

		if (cyhal_uart_enable_output(&data->obj,
					     CYHAL_UART_OUTPUT_TRIGGER_RX_FIFO_LEVEL_REACHED,
					     &uart_source)) {
			return -ENOTSUP;
		}

		if (ifx_cat1_dma_ex_connect_digital(data->async.dma_rx.dev,
						    data->async.dma_rx.dma_channel, uart_source,
						    CYHAL_DMA_INPUT_TRIGGER_ALL_ELEMENTS)) {
			return -ENOTSUP;
		}

		Cy_SCB_SetRxFifoLevel(config->reg_addr, 0);
	}

	if (data->async.dma_tx.dev != NULL) {
		cyhal_source_t uart_source;

		if (!device_is_ready(data->async.dma_tx.dev)) {
			return -ENODEV;
		}

		data->async.dma_tx.blk_cfg.dest_address = (uint32_t)(&config->reg_addr->TX_FIFO_WR);
		data->async.dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->async.dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->async.dma_tx.dma_cfg.head_block = &data->async.dma_tx.blk_cfg;
		data->async.dma_tx.dma_cfg.user_data = (void *)dev;
		data->async.dma_tx.dma_cfg.dma_callback = dma_callback_tx_done;

		if (cyhal_uart_enable_output(&data->obj,
					     CYHAL_UART_OUTPUT_TRIGGER_TX_FIFO_LEVEL_REACHED,
					     &uart_source)) {
			return -ENOTSUP;
		}

		if (ifx_cat1_dma_ex_connect_digital(data->async.dma_tx.dev,
						    data->async.dma_tx.dma_channel, uart_source,
						    CYHAL_DMA_INPUT_TRIGGER_ALL_ELEMENTS)) {
			return -ENOTSUP;
		}
		Cy_SCB_SetTxFifoLevel(config->reg_addr, 1);
	}

	k_work_init_delayable(&data->async.dma_tx.timeout_work, ifx_cat1_uart_async_tx_timeout);
	k_work_init_delayable(&data->async.dma_rx.timeout_work, ifx_cat1_uart_async_rx_timeout);

#endif /* CONFIG_UART_ASYNC_API */

	return ret;
}

static DEVICE_API(uart, ifx_cat1_uart_driver_api) = {
	.poll_in = ifx_cat1_uart_poll_in,
	.poll_out = ifx_cat1_uart_poll_out,
	.err_check = ifx_cat1_uart_err_check,

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = ifx_cat1_uart_configure,
	.config_get = ifx_cat1_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = ifx_cat1_uart_fifo_fill,
	.fifo_read = ifx_cat1_uart_fifo_read,
	.irq_tx_enable = ifx_cat1_uart_irq_tx_enable,
	.irq_tx_disable = ifx_cat1_uart_irq_tx_disable,
	.irq_tx_ready = ifx_cat1_uart_irq_tx_ready,
	.irq_rx_enable = ifx_cat1_uart_irq_rx_enable,
	.irq_rx_disable = ifx_cat1_uart_irq_rx_disable,
	.irq_tx_complete = ifx_cat1_uart_irq_tx_complete,
	.irq_rx_ready = ifx_cat1_uart_irq_rx_ready,
	.irq_err_enable = ifx_cat1_uart_irq_err_enable,
	.irq_err_disable = ifx_cat1_uart_irq_err_disable,
	.irq_is_pending = ifx_cat1_uart_irq_is_pending,
	.irq_update = ifx_cat1_uart_irq_update,
	.irq_callback_set = ifx_cat1_uart_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if CONFIG_UART_ASYNC_API
	.callback_set = ifx_cat1_uart_async_callback_set,
	.tx = ifx_cat1_uart_async_tx,
	.rx_enable = ifx_cat1_uart_async_rx_enable,
	.tx_abort = ifx_cat1_uart_async_tx_abort,
	.rx_buf_rsp = ifx_cat1_uart_async_rx_buf_rsp,
	.rx_disable = ifx_cat1_uart_async_rx_disable,
#endif /*CONFIG_UART_ASYNC_API*/

};

#if defined(CONFIG_UART_ASYNC_API)
#define UART_DMA_CHANNEL_INIT(index, dir, ch_dir, src_data_size, dst_data_size)                    \
	.dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                               \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = {                                                                               \
		.channel_direction = ch_dir,                                                       \
		.source_data_size = src_data_size,                                                 \
		.dest_data_size = dst_data_size,                                                   \
		.source_burst_length = 0,                                                          \
		.dest_burst_length = 0,                                                            \
		.block_count = 1,                                                                  \
		.complete_callback_en = 0,                                                         \
	},

#define UART_DMA_CHANNEL(index, dir, ch_dir, src_data_size, dst_data_size)                         \
	.async.dma_##dir = {COND_CODE_1(                                                           \
		DT_INST_DMAS_HAS_NAME(index, dir),                                                 \
		(UART_DMA_CHANNEL_INIT(index, dir, ch_dir, src_data_size, dst_data_size)),         \
		(NULL))},

#else
#define UART_DMA_CHANNEL(index, dir, ch_dir, src_data_size, dst_data_size)
#endif /* CONFIG_UART_ASYNC_API */

#define INFINEON_CAT1_UART_INIT(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct ifx_cat1_uart_data ifx_cat1_uart##n##_data = {                               \
		UART_DMA_CHANNEL(n, tx, MEMORY_TO_PERIPHERAL, 1, 1)                                \
			UART_DMA_CHANNEL(n, rx, PERIPHERAL_TO_MEMORY, 1, 1)};                      \
                                                                                                   \
	static struct ifx_cat1_uart_config ifx_cat1_uart##n##_cfg = {                              \
		.dt_cfg.baudrate = DT_INST_PROP(n, current_speed),                                 \
		.dt_cfg.parity = DT_INST_ENUM_IDX(n, parity),             \
		.dt_cfg.stop_bits = DT_INST_ENUM_IDX(n, stop_bits),       \
		.dt_cfg.data_bits = DT_INST_ENUM_IDX(n, data_bits),       \
		.dt_cfg.flow_ctrl = DT_INST_PROP(n, hw_flow_control),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.reg_addr = (CySCB_Type *)DT_INST_REG_ADDR(n),                                     \
		.irq_priority = DT_INST_IRQ(n, priority)};                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_uart_init, NULL, &ifx_cat1_uart##n##_data,              \
			      &ifx_cat1_uart##n##_cfg, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &ifx_cat1_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_UART_INIT)
