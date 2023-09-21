/*
 * Copyright (c) 2017,2021 NXP
 * Copyright (c) 2020 Softube
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_lpuart

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/pinctrl.h>
#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#endif
#include <zephyr/logging/log.h>

#include <fsl_lpuart.h>

LOG_MODULE_REGISTER(uart_mcux_lpuart, LOG_LEVEL_ERR);

#ifdef CONFIG_UART_ASYNC_API
struct lpuart_dma_config {
	const struct device *dma_dev;
	const uint32_t dma_channel;
	struct dma_config dma_cfg;
};
#endif /* CONFIG_UART_ASYNC_API */

struct mcux_lpuart_config {
	LPUART_Type *base;
	const struct device *clock_dev;
	const struct pinctrl_dev_config *pincfg;
	clock_control_subsys_t clock_subsys;
	uint32_t baud_rate;
	uint8_t flow_ctrl;
	uint8_t parity;
	bool rs485_de_active_low;
	bool loopback_en;
#ifdef CONFIG_UART_MCUX_LPUART_ISR_SUPPORT
	void (*irq_config_func)(const struct device *dev);
#endif
#ifdef CONFIG_UART_ASYNC_API
	const struct lpuart_dma_config rx_dma_config;
	const struct lpuart_dma_config tx_dma_config;
#endif /* CONFIG_UART_ASYNC_API */
};

#ifdef CONFIG_UART_ASYNC_API
struct mcux_lpuart_rx_dma_params {
	struct dma_block_config active_dma_block;
	uint8_t *buf;
	size_t buf_len;
	size_t offset;
	size_t counter;
	struct k_work_delayable timeout_work;
	size_t timeout_us;
};

struct mcux_lpuart_tx_dma_params {
	struct dma_block_config active_dma_block;
	const uint8_t *buf;
	size_t buf_len;
	struct k_work_delayable timeout_work;
	size_t timeout_us;
};

struct mcux_lpuart_async_data {
	const struct device *uart_dev;
	struct mcux_lpuart_tx_dma_params tx_dma_params;
	struct mcux_lpuart_rx_dma_params rx_dma_params;
	uint8_t *next_rx_buffer;
	size_t next_rx_buffer_len;
	uart_callback_t user_callback;
	void *user_data;
};
#endif

struct mcux_lpuart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
#ifdef CONFIG_PM
	bool pm_state_lock_on;
	bool tx_poll_stream_on;
	bool tx_int_stream_on;
#endif /* CONFIG_PM */
#ifdef CONFIG_UART_ASYNC_API
	struct mcux_lpuart_async_data async;
#endif
	struct uart_config uart_config;
};

#ifdef CONFIG_PM
static void mcux_lpuart_pm_policy_state_lock_get(const struct device *dev)
{
	struct mcux_lpuart_data *data = dev->data;

	if (!data->pm_state_lock_on) {
		data->pm_state_lock_on = true;
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void mcux_lpuart_pm_policy_state_lock_put(const struct device *dev)
{
	struct mcux_lpuart_data *data = dev->data;

	if (data->pm_state_lock_on) {
		data->pm_state_lock_on = false;
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}
#endif /* CONFIG_PM */

static int mcux_lpuart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kLPUART_RxDataRegFullFlag) {
		*c = LPUART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void mcux_lpuart_poll_out(const struct device *dev, unsigned char c)
{
	const struct mcux_lpuart_config *config = dev->config;
	unsigned int key;
#ifdef CONFIG_PM
	struct mcux_lpuart_data *data = dev->data;
#endif

	while (!(LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag)) {
	}
	/* Lock interrupts while we send data */
	key = irq_lock();
#ifdef CONFIG_PM
	/*
	 * We must keep the part from entering lower power mode until the
	 * transmission completes. Set the power constraint, and enable
	 * the transmission complete interrupt so we know when transmission is
	 * completed.
	 */
	if (!data->tx_poll_stream_on && !data->tx_int_stream_on) {
		data->tx_poll_stream_on = true;
		mcux_lpuart_pm_policy_state_lock_get(dev);
		/* Enable TC interrupt */
		LPUART_EnableInterrupts(config->base,
			kLPUART_TransmissionCompleteInterruptEnable);

	}
#endif /* CONFIG_PM */

	LPUART_WriteByte(config->base, c);
	irq_unlock(key);
}

static int mcux_lpuart_err_check(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
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

	if (flags & kLPUART_NoiseErrorFlag) {
		err |= UART_ERROR_PARITY;
	}

	LPUART_ClearStatusFlags(config->base, kLPUART_RxOverrunFlag |
					      kLPUART_ParityErrorFlag |
					      kLPUART_FramingErrorFlag |
						  kLPUART_NoiseErrorFlag);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int mcux_lpuart_fifo_fill(const struct device *dev,
				 const uint8_t *tx_data,
				 int len)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag)) {

		LPUART_WriteByte(config->base, tx_data[num_tx++]);
	}
	return num_tx;
}

static int mcux_lpuart_fifo_read(const struct device *dev, uint8_t *rx_data,
				 const int len)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_RxDataRegFullFlag)) {

		rx_data[num_rx++] = LPUART_ReadByte(config->base);
	}

	return num_rx;
}

static void mcux_lpuart_irq_tx_enable(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;
#ifdef CONFIG_PM
	struct mcux_lpuart_data *data = dev->data;
	unsigned int key;
#endif

#ifdef CONFIG_PM
	key = irq_lock();
	data->tx_poll_stream_on = false;
	data->tx_int_stream_on = true;
	/* Transmission complete interrupt no longer required */
	LPUART_DisableInterrupts(config->base,
		kLPUART_TransmissionCompleteInterruptEnable);
	/* Do not allow system to sleep while UART tx is ongoing */
	mcux_lpuart_pm_policy_state_lock_get(dev);
#endif
	LPUART_EnableInterrupts(config->base, mask);
#ifdef CONFIG_PM
	irq_unlock(key);
#endif
}

static void mcux_lpuart_irq_tx_disable(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;
#ifdef CONFIG_PM
	struct mcux_lpuart_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
#endif

	LPUART_DisableInterrupts(config->base, mask);
#ifdef CONFIG_PM
	data->tx_int_stream_on = false;
	/*
	 * If transmission IRQ is no longer enabled,
	 * transmission is complete. Release pm constraint.
	 */
	mcux_lpuart_pm_policy_state_lock_put(dev);
	irq_unlock(key);
#endif
}

static int mcux_lpuart_irq_tx_complete(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_TransmissionCompleteFlag) != 0U;
}

static int mcux_lpuart_irq_tx_ready(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& (flags & kLPUART_TxDataRegEmptyFlag);
}

static void mcux_lpuart_irq_rx_enable(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
	LPUART_EnableRx(config->base, true);
}

static void mcux_lpuart_irq_rx_disable(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_EnableRx(config->base, false);
	LPUART_DisableInterrupts(config->base, mask);
}

static int mcux_lpuart_irq_rx_full(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_RxDataRegFullFlag) != 0U;
}

static int mcux_lpuart_irq_rx_pending(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& mcux_lpuart_irq_rx_full(dev);
}

static void mcux_lpuart_irq_err_enable(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void mcux_lpuart_irq_err_disable(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int mcux_lpuart_irq_is_pending(const struct device *dev)
{
	return (mcux_lpuart_irq_tx_ready(dev)
		|| mcux_lpuart_irq_rx_pending(dev));
}

static int mcux_lpuart_irq_update(const struct device *dev)
{
	return 1;
}

static void mcux_lpuart_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct mcux_lpuart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async.user_callback = NULL;
	data->async.user_data = NULL;
#endif
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


#ifdef CONFIG_UART_ASYNC_API
static inline void async_timer_start(struct k_work_delayable *work, size_t timeout_us)
{
	if ((timeout_us != SYS_FOREVER_US) && (timeout_us != 0)) {
		LOG_DBG("async timer started for %d us", timeout_us);
		k_work_reschedule(work, K_USEC(timeout_us));
	}
}

static void async_user_callback(const struct device *dev, struct uart_event *evt)
{
	const struct mcux_lpuart_data *data = dev->data;

	if (data->async.user_callback) {
		data->async.user_callback(dev, evt, data->async.user_data);
	}
}

static void async_evt_tx_done(struct device *dev)
{
	struct mcux_lpuart_data *data = dev->data;

	(void)k_work_cancel_delayable(&data->async.tx_dma_params.timeout_work);

	LOG_DBG("TX done: %d", data->async.tx_dma_params.buf_len);
	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = data->async.tx_dma_params.buf,
		.data.tx.len = data->async.tx_dma_params.buf_len
	};

	/* Reset TX Buffer */
	data->async.tx_dma_params.buf = NULL;
	data->async.tx_dma_params.buf_len = 0U;

	async_user_callback(dev, &event);
}

static void async_evt_rx_rdy(const struct device *dev)
{
	struct mcux_lpuart_data *data = dev->data;
	struct mcux_lpuart_rx_dma_params *dma_params = &data->async.rx_dma_params;

	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = dma_params->buf,
		.data.rx.len = dma_params->counter - dma_params->offset,
		.data.rx.offset = dma_params->offset
	};

	LOG_DBG("RX Ready: (len: %d off: %d buf: %x)", event.data.rx.len, event.data.rx.offset,
		(uint32_t)event.data.rx.buf);

	/* Update the current pos for new data */
	dma_params->offset = dma_params->counter;

	/* Only send event for new data */
	if (event.data.rx.len > 0) {
		async_user_callback(dev, &event);
	}
}

static void async_evt_rx_buf_request(const struct device *dev)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(dev, &evt);
}

static void async_evt_rx_buf_release(const struct device *dev)
{
	struct mcux_lpuart_data *data = (struct mcux_lpuart_data *)dev->data;
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->async.rx_dma_params.buf,
	};

	async_user_callback(dev, &evt);
	data->async.rx_dma_params.buf = NULL;
	data->async.rx_dma_params.buf_len = 0U;
	data->async.rx_dma_params.offset = 0U;
	data->async.rx_dma_params.counter = 0U;
}

static void mcux_lpuart_async_rx_flush(const struct device *dev)
{
	struct dma_status status;
	struct mcux_lpuart_data *data = dev->data;
	const struct mcux_lpuart_config *config = dev->config;

	const int get_status_result = dma_get_status(config->rx_dma_config.dma_dev,
						     config->rx_dma_config.dma_channel,
						     &status);

	if (get_status_result == 0) {
		const size_t rx_rcv_len = data->async.rx_dma_params.buf_len -
					  status.pending_length;

		if (rx_rcv_len > data->async.rx_dma_params.counter) {
			data->async.rx_dma_params.counter = rx_rcv_len;
			async_evt_rx_rdy(dev);
		}
	} else {
		LOG_ERR("Error getting DMA status");
	}
}

static int mcux_lpuart_rx_disable(const struct device *dev)
{
	LOG_INF("Disabling UART RX DMA");
	const struct mcux_lpuart_config *config = dev->config;
	struct mcux_lpuart_data *data = (struct mcux_lpuart_data *)dev->data;
	LPUART_Type *lpuart = config->base;
	const unsigned int key = irq_lock();

	LPUART_EnableRx(lpuart, false);
	(void)k_work_cancel_delayable(&data->async.rx_dma_params.timeout_work);
	LPUART_DisableInterrupts(lpuart, kLPUART_IdleLineInterruptEnable);
	LPUART_ClearStatusFlags(lpuart, kLPUART_IdleLineFlag);
	LPUART_EnableRxDMA(lpuart, false);

	/* No active RX buffer, cannot disable */
	if (!data->async.rx_dma_params.buf) {
		LOG_ERR("No buffers to release from RX DMA!");
	} else {
		mcux_lpuart_async_rx_flush(dev);
		async_evt_rx_buf_release(dev);
		if (data->async.next_rx_buffer != NULL) {
			data->async.rx_dma_params.buf = data->async.next_rx_buffer;
			data->async.rx_dma_params.buf_len = data->async.next_rx_buffer_len;
			data->async.next_rx_buffer = NULL;
			data->async.next_rx_buffer_len = 0;
			/* Release the next buffer as well */
			async_evt_rx_buf_release(dev);
		}
	}
	const int ret = dma_stop(config->rx_dma_config.dma_dev,
				 config->rx_dma_config.dma_channel);

	if (ret != 0) {
		LOG_ERR("Error stopping rx DMA. Reason: %x", ret);
	}
	LOG_DBG("RX: Disabled");
	struct uart_event disabled_event = {
		.type = UART_RX_DISABLED
	};

	async_user_callback(dev, &disabled_event);
	irq_unlock(key);
	return ret;
}

static void prepare_rx_dma_block_config(const struct device *dev)
{
	struct mcux_lpuart_data *data = (struct mcux_lpuart_data *)dev->data;
	const struct mcux_lpuart_config *config = dev->config;
	LPUART_Type *lpuart = config->base;
	struct mcux_lpuart_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;

	assert(rx_dma_params->buf != NULL);
	assert(rx_dma_params->buf_len > 0);

	struct dma_block_config *head_block_config = &rx_dma_params->active_dma_block;

	head_block_config->dest_address = (uint32_t)rx_dma_params->buf;
	head_block_config->source_address = LPUART_GetDataRegisterAddress(lpuart);
	head_block_config->block_size = rx_dma_params->buf_len;
	head_block_config->dest_scatter_en = true;
}

static int configure_and_start_rx_dma(
	const struct mcux_lpuart_config *config, struct mcux_lpuart_data *data,
	LPUART_Type *lpuart)
{
	LOG_DBG("Configuring and Starting UART RX DMA");
	int ret = dma_config(config->rx_dma_config.dma_dev,
			     config->rx_dma_config.dma_channel,
			     (struct dma_config *)&config->rx_dma_config.dma_cfg);

	if (ret != 0) {
		LOG_ERR("Failed to Configure RX DMA: err: %d", ret);
		return ret;
	}
	ret = dma_start(config->rx_dma_config.dma_dev, config->rx_dma_config.dma_channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DMA(Rx) Ch %d(%d)",
			config->rx_dma_config.dma_channel,
			ret);
	}
	LPUART_EnableRxDMA(lpuart, true);
	return ret;
}

static int uart_mcux_lpuart_dma_replace_rx_buffer(const struct device *dev)
{
	struct mcux_lpuart_data *data = (struct mcux_lpuart_data *)dev->data;
	const struct mcux_lpuart_config *config = dev->config;
	LPUART_Type *lpuart = config->base;

	LOG_DBG("Replacing RX buffer, new length: %d", data->async.next_rx_buffer_len);
	/* There must be a buffer to replace this one with */
	assert(data->async.next_rx_buffer != NULL);
	assert(data->async.next_rx_buffer_len != 0U);
	const int success = dma_reload(config->rx_dma_config.dma_dev,
				       config->rx_dma_config.dma_channel,
				       LPUART_GetDataRegisterAddress(lpuart),
				       (uint32_t)data->async.next_rx_buffer,
				       data->async.next_rx_buffer_len);

	if (success != 0) {
		LOG_ERR("Error %d reloading DMA with next RX buffer", success);
	}
	return success;
}

static void dma_callback(const struct device *dma_dev, void *callback_arg, uint32_t channel,
			 int dma_status)
{
	struct device *dev = (struct device *)callback_arg;
	const struct mcux_lpuart_config *config = dev->config;
	LPUART_Type *lpuart = config->base;
	struct mcux_lpuart_data *data = (struct mcux_lpuart_data *)dev->data;

	LOG_DBG("DMA call back on channel %d", channel);
	struct dma_status status;
	const int get_status_result = dma_get_status(dma_dev, channel, &status);

	if (get_status_result < 0) {
		LOG_ERR("error on status get: %d", get_status_result);
	} else {
		LOG_DBG("DMA Status: b: %d dir: %d len_remain: %d", status.busy, status.dir,
			status.pending_length);
	}

	if (dma_status < 0) {
		LOG_ERR("Got error : %d", dma_status);
	}


	if (channel == config->tx_dma_config.dma_channel) {
		LOG_DBG("TX Channel");
		LPUART_EnableTxDMA(lpuart, false);
		async_evt_tx_done(dev);
	} else if (channel == config->rx_dma_config.dma_channel) {
		LOG_DBG("RX Channel");
		struct mcux_lpuart_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;

		/* The RX Event indicates DMA transfer is complete and full buffer is available. */
		rx_dma_params->counter = rx_dma_params->buf_len;

		LOG_DBG("Current Buf (%x) full, swapping to new buf: %x",
			(uint32_t)rx_dma_params->buf,
			(uint32_t)data->async.next_rx_buffer);
		async_evt_rx_rdy(dev);
		async_evt_rx_buf_release(dev);

		rx_dma_params->buf = data->async.next_rx_buffer;
		rx_dma_params->buf_len = data->async.next_rx_buffer_len;
		data->async.next_rx_buffer = NULL;
		data->async.next_rx_buffer_len = 0U;

		/* A new buffer was available (and already loaded into the DMA engine) */
		if (rx_dma_params->buf != NULL &&
		    rx_dma_params->buf_len > 0) {
			/* Request the next buffer */
			async_evt_rx_buf_request(dev);
		} else {
			/* Buffer full without valid next buffer, disable RX DMA */
			LOG_INF("Disabled RX DMA, no valid next buffer ");
			mcux_lpuart_rx_disable(dev);
		}
	} else {
		LOG_ERR("Got unexpected DMA Channel: %d", channel);
	}
}

static int mcux_lpuart_callback_set(const struct device *dev, uart_callback_t callback,
				    void *user_data)
{
	struct mcux_lpuart_data *data = dev->data;

	data->async.user_callback = callback;
	data->async.user_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->callback = NULL;
	data->cb_data = NULL;
#endif

	return 0;
}

static int mcux_lpuart_tx(const struct device *dev, const uint8_t *buf, size_t len,
			  int32_t timeout_us)
{
	struct mcux_lpuart_data *data = dev->data;
	const struct mcux_lpuart_config *config = dev->config;
	LPUART_Type *lpuart = config->base;

	unsigned int key = irq_lock();

	/* Check for an ongiong transfer and abort if it is pending */
	struct dma_status status;
	const int get_status_result = dma_get_status(config->tx_dma_config.dma_dev,
						     config->tx_dma_config.dma_channel,
						     &status);

	if (get_status_result < 0 || status.busy) {
		irq_unlock(key);
		LOG_ERR("Unable to submit UART DMA Transfer.");
		return get_status_result < 0 ? get_status_result : -EBUSY;
	}

	int ret;

	LPUART_EnableTxDMA(lpuart, false);

	data->async.tx_dma_params.buf = buf;
	data->async.tx_dma_params.buf_len = len;
	data->async.tx_dma_params.active_dma_block.source_address = (uint32_t)buf;
	data->async.tx_dma_params.active_dma_block.dest_address =
		LPUART_GetDataRegisterAddress(lpuart);
	data->async.tx_dma_params.active_dma_block.block_size = len;
	data->async.tx_dma_params.active_dma_block.next_block = NULL;

	ret = dma_config(config->tx_dma_config.dma_dev,
			 config->tx_dma_config.dma_channel,
			 (struct dma_config *)&config->tx_dma_config.dma_cfg);

	if (ret == 0) {
		LOG_DBG("Starting UART DMA TX Ch %u", config->tx_dma_config.dma_channel);

		ret = dma_start(config->tx_dma_config.dma_dev,
				config->tx_dma_config.dma_channel);
		LPUART_EnableTxDMA(lpuart, true);
		if (ret != 0) {
			LOG_ERR("Failed to start DMA(Tx) Ch %d",
				config->tx_dma_config.dma_channel);
		}
		async_timer_start(&data->async.tx_dma_params.timeout_work, timeout_us);
	} else {
		LOG_ERR("Error configuring UART DMA: %x", ret);
	}
	irq_unlock(key);
	return ret;
}

static int mcux_lpuart_tx_abort(const struct device *dev)
{
	struct mcux_lpuart_data *data = dev->data;
	const struct mcux_lpuart_config *config = dev->config;
	LPUART_Type *lpuart = config->base;

	LPUART_EnableTxDMA(lpuart, false);
	(void)k_work_cancel_delayable(&data->async.tx_dma_params.timeout_work);
	struct dma_status status;
	const int get_status_result = dma_get_status(config->tx_dma_config.dma_dev,
						     config->tx_dma_config.dma_channel,
						     &status);

	if (get_status_result < 0) {
		LOG_ERR("Error querying TX DMA Status during abort.");
	}

	const size_t bytes_transmitted = (get_status_result == 0) ?
			 data->async.tx_dma_params.buf_len - status.pending_length : 0;

	const int ret = dma_stop(config->tx_dma_config.dma_dev, config->tx_dma_config.dma_channel);

	if (ret == 0) {
		struct uart_event tx_aborted_event = {
			.type = UART_TX_ABORTED,
			.data.tx.buf = data->async.tx_dma_params.buf,
			.data.tx.len = bytes_transmitted
		};
		async_user_callback(dev, &tx_aborted_event);
	}
	return ret;
}

static int mcux_lpuart_rx_enable(const struct device *dev, uint8_t *buf, const size_t len,
				 const int32_t timeout_us)
{
	LOG_DBG("Enabling UART RX DMA");
	struct mcux_lpuart_data *data = dev->data;
	const struct mcux_lpuart_config *config = dev->config;
	LPUART_Type *lpuart = config->base;

	struct mcux_lpuart_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;

	unsigned int key = irq_lock();
	struct dma_status status;
	const int get_status_result = dma_get_status(config->rx_dma_config.dma_dev,
						     config->rx_dma_config.dma_channel,
						     &status);

	if (get_status_result < 0 || status.busy) {
		LOG_ERR("Unable to start receive on UART.");
		irq_unlock(key);
		return get_status_result < 0 ? get_status_result : -EBUSY;
	}

	rx_dma_params->timeout_us = timeout_us;
	rx_dma_params->buf = buf;
	rx_dma_params->buf_len = len;

	LPUART_EnableInterrupts(config->base, kLPUART_IdleLineInterruptEnable);
	prepare_rx_dma_block_config(dev);
	const int ret = configure_and_start_rx_dma(config, data, lpuart);

	/* Request the next buffer for when this buffer is full for continuous reception */
	async_evt_rx_buf_request(dev);

	/* Clear these status flags as they can prevent the UART device from receiving data */
	LPUART_ClearStatusFlags(config->base, kLPUART_RxOverrunFlag |
					      kLPUART_ParityErrorFlag |
					      kLPUART_FramingErrorFlag |
						  kLPUART_NoiseErrorFlag);
	LPUART_EnableRx(lpuart, true);
	irq_unlock(key);
	return ret;
}

static int mcux_lpuart_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct mcux_lpuart_data *data = dev->data;

	assert(data->async.next_rx_buffer == NULL);
	assert(data->async.next_rx_buffer_len == 0);
	data->async.next_rx_buffer = buf;
	data->async.next_rx_buffer_len = len;
	uart_mcux_lpuart_dma_replace_rx_buffer(dev);

	return 0;
}

static void mcux_lpuart_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mcux_lpuart_rx_dma_params *rx_params = CONTAINER_OF(dwork,
								   struct mcux_lpuart_rx_dma_params,
								   timeout_work);
	struct mcux_lpuart_async_data *async_data = CONTAINER_OF(rx_params,
								 struct mcux_lpuart_async_data,
								 rx_dma_params);
	const struct device *dev = async_data->uart_dev;

	LOG_DBG("RX timeout");
	mcux_lpuart_async_rx_flush(dev);
}

static void mcux_lpuart_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mcux_lpuart_tx_dma_params *tx_params = CONTAINER_OF(dwork,
								   struct mcux_lpuart_tx_dma_params,
								   timeout_work);
	struct mcux_lpuart_async_data *async_data = CONTAINER_OF(tx_params,
								 struct mcux_lpuart_async_data,
								 tx_dma_params);
	const struct device *dev = async_data->uart_dev;

	LOG_DBG("TX timeout");
	(void)mcux_lpuart_tx_abort(dev);
}

#endif /* CONFIG_UART_ASYNC_API */

#if CONFIG_UART_MCUX_LPUART_ISR_SUPPORT
static void mcux_lpuart_isr(const struct device *dev)
{
	struct mcux_lpuart_data *data = dev->data;
	const struct mcux_lpuart_config *config = dev->config;
	const uint32_t status = LPUART_GetStatusFlags(config->base);

#if CONFIG_PM
	if (status & kLPUART_TransmissionCompleteFlag) {

		if (data->tx_poll_stream_on) {
			/* Poll transmission complete. Allow system to sleep */
			LPUART_DisableInterrupts(config->base,
				kLPUART_TransmissionCompleteInterruptEnable);
			data->tx_poll_stream_on = false;
			mcux_lpuart_pm_policy_state_lock_put(dev);
		}
	}
#endif /* CONFIG_PM */

#if CONFIG_UART_INTERRUPT_DRIVEN
	if (data->callback) {
		data->callback(dev, data->cb_data);
	}

	if (status & kLPUART_RxOverrunFlag) {
		LPUART_ClearStatusFlags(config->base, kLPUART_RxOverrunFlag);
	}
#endif

#if CONFIG_UART_ASYNC_API
	if (status & kLPUART_IdleLineFlag) {
		async_timer_start(&data->async.rx_dma_params.timeout_work,
				  data->async.rx_dma_params.timeout_us);
		LPUART_ClearStatusFlags(config->base, kLPUART_IdleLineFlag);
	}
#endif /* CONFIG_UART_ASYNC_API */
}
#endif /* CONFIG_UART_MCUX_LPUART_ISR_SUPPORT */

static int mcux_lpuart_configure_init(const struct device *dev, const struct uart_config *cfg)
{
	const struct mcux_lpuart_config *config = dev->config;
	struct mcux_lpuart_data *data = dev->data;
	uint32_t clock_freq;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	lpuart_config_t uart_config;
	LPUART_GetDefaultConfig(&uart_config);

	/* Translate UART API enum to LPUART enum from HAL */
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_config.parityMode = kLPUART_ParityDisabled;
		break;
	case UART_CFG_PARITY_ODD:
		uart_config.parityMode = kLPUART_ParityOdd;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_config.parityMode = kLPUART_ParityEven;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->data_bits) {
#if defined(FSL_FEATURE_LPUART_HAS_7BIT_DATA_SUPPORT) && \
	FSL_FEATURE_LPUART_HAS_7BIT_DATA_SUPPORT
	case UART_CFG_DATA_BITS_7:
		uart_config.dataBitsCount  = kLPUART_SevenDataBits;
		break;
#endif
	case UART_CFG_DATA_BITS_8:
		uart_config.dataBitsCount  = kLPUART_EightDataBits;
		break;
	default:
		return -ENOTSUP;
	}

#if defined(FSL_FEATURE_LPUART_HAS_STOP_BIT_CONFIG_SUPPORT) && \
	FSL_FEATURE_LPUART_HAS_STOP_BIT_CONFIG_SUPPORT
	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_config.stopBitCount = kLPUART_OneStopBit;
		break;
	case UART_CFG_STOP_BITS_2:
		uart_config.stopBitCount = kLPUART_TwoStopBit;
		break;
	default:
		return -ENOTSUP;
	}
#endif

#if defined(FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT) && \
	FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT
	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
	case UART_CFG_FLOW_CTRL_RS485:
		uart_config.enableTxCTS = false;
		uart_config.enableRxRTS = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		uart_config.enableTxCTS = true;
		uart_config.enableRxRTS = true;
		break;
	default:
		return -ENOTSUP;
	}
#endif

	uart_config.baudRate_Bps = cfg->baudrate;
	uart_config.enableRx = true;
	/* Tx will be enabled manually after set tx-rts */
	uart_config.enableTx = false;


#ifdef CONFIG_UART_ASYNC_API
	uart_config.rxIdleType = kLPUART_IdleTypeStopBit;
	uart_config.rxIdleConfig = kLPUART_IdleCharacter1;
	data->async.next_rx_buffer = NULL;
	data->async.next_rx_buffer_len = 0;
	data->async.uart_dev = dev;
	k_work_init_delayable(&data->async.rx_dma_params.timeout_work,
			      mcux_lpuart_async_rx_timeout);
	k_work_init_delayable(&data->async.tx_dma_params.timeout_work,
			      mcux_lpuart_async_tx_timeout);

	/* Disable the UART Receiver until the async API provides a buffer to
	 * to receive into with rx_enable
	 */
	uart_config.enableRx = false;

#endif /* CONFIG_UART_ASYNC_API */

	LPUART_Init(config->base, &uart_config, clock_freq);

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) {
		/* Set the LPUART into RS485 mode (tx driver enable using RTS) */
		config->base->MODIR |= LPUART_MODIR_TXRTSE(true);
		if (!config->rs485_de_active_low) {
			config->base->MODIR |= LPUART_MODIR_TXRTSPOL(1);
		}
	}
	/* Now can enable tx */
	config->base->CTRL |= LPUART_CTRL_TE(true);


	if (config->loopback_en) {
		/* Set the LPUART into loopback mode */
		config->base->CTRL |= LPUART_CTRL_LOOPS_MASK;
		config->base->CTRL &= ~LPUART_CTRL_RSRC_MASK;
	}

	/* update internal uart_config */
	data->uart_config = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int mcux_lpuart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct mcux_lpuart_data *data = dev->data;
	*cfg = data->uart_config;
	return 0;
}

static int mcux_lpuart_configure(const struct device *dev,
				 const struct uart_config *cfg)
{
	const struct mcux_lpuart_config *config = dev->config;

	/* disable LPUART */
	LPUART_Deinit(config->base);

	int ret = mcux_lpuart_configure_init(dev, cfg);
	if (ret) {
		return ret;
	}

	/* wait for hardware init */
	k_sleep(K_MSEC(1));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int mcux_lpuart_init(const struct device *dev)
{
	const struct mcux_lpuart_config *config = dev->config;
	struct mcux_lpuart_data *data = dev->data;
	struct uart_config *uart_api_config = &data->uart_config;
	int err;

	uart_api_config->baudrate = config->baud_rate;
	uart_api_config->parity = config->parity;
	uart_api_config->stop_bits = UART_CFG_STOP_BITS_1;
	uart_api_config->data_bits = UART_CFG_DATA_BITS_8;
	uart_api_config->flow_ctrl = config->flow_ctrl;

	/* set initial configuration */
	mcux_lpuart_configure_init(dev, uart_api_config);
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_UART_MCUX_LPUART_ISR_SUPPORT
	config->irq_config_func(dev);
#endif

#ifdef CONFIG_PM
	data->pm_state_lock_on = false;
	data->tx_poll_stream_on = false;
	data->tx_int_stream_on = false;
#endif

	return 0;
}

static const struct uart_driver_api mcux_lpuart_driver_api = {
	.poll_in = mcux_lpuart_poll_in,
	.poll_out = mcux_lpuart_poll_out,
	.err_check = mcux_lpuart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = mcux_lpuart_configure,
	.config_get = mcux_lpuart_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = mcux_lpuart_fifo_fill,
	.fifo_read = mcux_lpuart_fifo_read,
	.irq_tx_enable = mcux_lpuart_irq_tx_enable,
	.irq_tx_disable = mcux_lpuart_irq_tx_disable,
	.irq_tx_complete = mcux_lpuart_irq_tx_complete,
	.irq_tx_ready = mcux_lpuart_irq_tx_ready,
	.irq_rx_enable = mcux_lpuart_irq_rx_enable,
	.irq_rx_disable = mcux_lpuart_irq_rx_disable,
	.irq_rx_ready = mcux_lpuart_irq_rx_full,
	.irq_err_enable = mcux_lpuart_irq_err_enable,
	.irq_err_disable = mcux_lpuart_irq_err_disable,
	.irq_is_pending = mcux_lpuart_irq_is_pending,
	.irq_update = mcux_lpuart_irq_update,
	.irq_callback_set = mcux_lpuart_irq_callback_set,
#endif
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = mcux_lpuart_callback_set,
	.tx = mcux_lpuart_tx,
	.tx_abort = mcux_lpuart_tx_abort,
	.rx_enable = mcux_lpuart_rx_enable,
	.rx_buf_rsp = mcux_lpuart_rx_buf_rsp,
	.rx_disable = mcux_lpuart_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};


#ifdef CONFIG_UART_MCUX_LPUART_ISR_SUPPORT
#define MCUX_LPUART_IRQ_INSTALL(n, i)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq),		\
			    DT_INST_IRQ_BY_IDX(n, i, priority),		\
			    mcux_lpuart_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));		\
	} while (false)
#define MCUX_LPUART_IRQ_INIT(n) .irq_config_func = mcux_lpuart_config_func_##n,
#define MCUX_LPUART_IRQ_DEFINE(n)						\
	static void mcux_lpuart_config_func_##n(const struct device *dev)	\
	{									\
		MCUX_LPUART_IRQ_INSTALL(n, 0);				\
									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 1),			\
			   (MCUX_LPUART_IRQ_INSTALL(n, 1);))		\
	}
#else
#define MCUX_LPUART_IRQ_INIT(n)
#define MCUX_LPUART_IRQ_DEFINE(n)
#endif /* CONFIG_UART_MCUX_LPUART_ISR_SUPPORT */

#ifdef CONFIG_UART_ASYNC_API
#define TX_DMA_CONFIG(id)								       \
	.tx_dma_config = {								       \
		.dma_dev =								       \
			DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, tx)),		       \
		.dma_channel =								       \
			DT_INST_DMAS_CELL_BY_NAME(id, tx, mux),				       \
		.dma_cfg = {								       \
			.source_burst_length = 1,					       \
			.dest_burst_length = 1,						       \
			.source_data_size = 1,						       \
			.dest_data_size = 1,						       \
			.complete_callback_en = 1,					       \
			.error_callback_en = 1,						       \
			.block_count = 1,						       \
			.head_block =							       \
				&mcux_lpuart_##id##_data.async.tx_dma_params.active_dma_block, \
			.channel_direction = MEMORY_TO_PERIPHERAL,			       \
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(				       \
				id, tx, source),					       \
			.dma_callback = dma_callback,					       \
			.user_data = (void *)DEVICE_DT_INST_GET(id)			       \
		},									       \
	},
#define RX_DMA_CONFIG(id)								       \
	.rx_dma_config = {								       \
		.dma_dev =								       \
			DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, rx)),		       \
		.dma_channel =								       \
			DT_INST_DMAS_CELL_BY_NAME(id, rx, mux),				       \
		.dma_cfg = {								       \
			.source_burst_length = 1,					       \
			.dest_burst_length = 1,						       \
			.source_data_size = 1,						       \
			.dest_data_size = 1,						       \
			.complete_callback_en = 1,					       \
			.error_callback_en = 1,						       \
			.block_count = 1,						       \
			.head_block =							       \
				&mcux_lpuart_##id##_data.async.rx_dma_params.active_dma_block, \
			.channel_direction = PERIPHERAL_TO_MEMORY,			       \
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(				       \
				id, rx, source),					       \
			.dma_callback = dma_callback,					       \
			.user_data = (void *)DEVICE_DT_INST_GET(id)			       \
		},									       \
	},
#else
#define RX_DMA_CONFIG(n)
#define TX_DMA_CONFIG(n)
#endif /* CONFIG_UART_ASYNC_API */

#define FLOW_CONTROL(n) \
	DT_INST_PROP(n, hw_flow_control)   \
		? UART_CFG_FLOW_CTRL_RTS_CTS     \
		: DT_INST_PROP(n, nxp_rs485_mode)\
				? UART_CFG_FLOW_CTRL_RS485   \
				: UART_CFG_FLOW_CTRL_NONE

#define LPUART_MCUX_DECLARE_CFG(n)                                      \
static const struct mcux_lpuart_config mcux_lpuart_##n##_config = {     \
	.base = (LPUART_Type *) DT_INST_REG_ADDR(n),                          \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                   \
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
	.baud_rate = DT_INST_PROP(n, current_speed),                          \
	.flow_ctrl = FLOW_CONTROL(n),                                         \
	.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),       \
	.rs485_de_active_low = DT_INST_PROP(n, nxp_rs485_de_active_low),      \
	.loopback_en = DT_INST_PROP(n, nxp_loopback),                         \
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                          \
	MCUX_LPUART_IRQ_INIT(n) \
	RX_DMA_CONFIG(n)        \
	TX_DMA_CONFIG(n)        \
};

#define LPUART_MCUX_INIT(n)						\
									\
	static struct mcux_lpuart_data mcux_lpuart_##n##_data;		\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
	MCUX_LPUART_IRQ_DEFINE(n)					\
									\
	LPUART_MCUX_DECLARE_CFG(n)					\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &mcux_lpuart_init,				\
			    NULL,					\
			    &mcux_lpuart_##n##_data,			\
			    &mcux_lpuart_##n##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &mcux_lpuart_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(LPUART_MCUX_INIT)
