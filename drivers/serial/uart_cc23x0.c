/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_uart

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/atomic.h>

#include <errno.h>

#include <driverlib/uart.h>
#include <driverlib/clkctl.h>

#include <inc/hw_memmap.h>

#ifdef CONFIG_UART_CC23X0_DMA_DRIVEN
#define UART_CC23_REG_GET(base, offset) ((base) + (offset))
/*
 * For each DMA channel, burst transfer and single transfer request signals
 * are not mutually exclusive, and both can be asserted at the same time.
 * For example, when there is more data than the watermark level in the
 * TX (or RX) FIFO, the burst transfer request and the single transfer
 * requests are asserted.
 * When a burst request is detected, the DMA controller transfers the number
 * of items that is the lesser of the arbitration size or the number of items
 * remaining in the transfer. Therefore, the arbitration size must be the same
 * as the number of data items that the peripheral can accommodate when making
 * a burst request. Since UART, which uses a mix of single or burst requests,
 * can generate a burst request based on the FIFO trigger level (1/2 full),
 * the burst length is set to half the FIFO size.
 */
#define UART_CC23_BURST_LEN 4
#endif

struct uart_cc23x0_config {
	uint32_t reg;
	uint32_t sys_clk_freq;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_CC23X0_DMA_DRIVEN
	const struct device *dma_dev;
	uint8_t dma_channel_tx;
	uint8_t dma_trigsrc_tx;
	uint8_t dma_channel_rx;
	uint8_t dma_trigsrc_rx;
#endif
};

enum uart_cc23x0_pm_locks {
	UART_CC23X0_PM_LOCK_TX,
	UART_CC23X0_PM_LOCK_RX,
	UART_CC23X0_PM_LOCK_COUNT,
};

struct uart_cc23x0_data {
	struct uart_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_CC23X0_DMA_DRIVEN
	const struct device *dev;

	uart_callback_t async_callback;
	void *async_user_data;

	struct k_work_delayable tx_timeout_work;
	const uint8_t *tx_buf;
	size_t tx_len;

	uint8_t *rx_buf;
	size_t rx_len;
	size_t rx_processed_len;
	uint8_t *rx_next_buf;
	size_t rx_next_len;
#endif /* CONFIG_UART_CC23X0_DMA_DRIVEN */
#ifdef CONFIG_PM
	ATOMIC_DEFINE(pm_lock, UART_CC23X0_PM_LOCK_COUNT);
#endif
};

static inline void uart_cc23x0_pm_policy_state_lock_get(struct uart_cc23x0_data *data,
							enum uart_cc23x0_pm_locks pm_lock_type)
{
#ifdef CONFIG_PM_DEVICE
	if (!atomic_test_and_set_bit(data->pm_lock, pm_lock_type)) {
		pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif
}

static inline void uart_cc23x0_pm_policy_state_lock_put(struct uart_cc23x0_data *data,
							enum uart_cc23x0_pm_locks pm_lock_type)
{
#ifdef CONFIG_PM_DEVICE
	if (atomic_test_and_clear_bit(data->pm_lock, pm_lock_type)) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	}
#endif
}

static int uart_cc23x0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_cc23x0_config *config = dev->config;

	if (!UARTCharAvailable(config->reg)) {
		return -1;
	}

	*c = UARTGetCharNonBlocking(config->reg);

	return 0;
}

static void uart_cc23x0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_cc23x0_config *config = dev->config;

	UARTPutChar(config->reg, c);

#ifdef CONFIG_PM_DEVICE
	/* Wait for character to be transmitted to ensure CPU
	 * does not enter standby when UART is busy
	 */
	while (UARTBusy(config->reg)) {
	}
#endif
}

static int uart_cc23x0_err_check(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;
	uint32_t flags = UARTGetRxError(config->reg);
	int error = 0;

	error |= (flags & UART_RXERROR_FRAMING) ? UART_ERROR_FRAMING : 0;
	error |= (flags & UART_RXERROR_PARITY) ? UART_ERROR_PARITY : 0;
	error |= (flags & UART_RXERROR_BREAK) ? UART_BREAK : 0;
	error |= (flags & UART_RXERROR_OVERRUN) ? UART_ERROR_OVERRUN : 0;

	UARTClearRxError(config->reg);

	return error;
}

static int uart_cc23x0_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_cc23x0_config *config = dev->config;
	struct uart_cc23x0_data *data = dev->data;
	uint32_t line_ctrl = 0;
	bool flow_ctrl;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		line_ctrl |= UART_CONFIG_PAR_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		line_ctrl |= UART_CONFIG_PAR_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		line_ctrl |= UART_CONFIG_PAR_EVEN;
		break;
	case UART_CFG_PARITY_MARK:
		line_ctrl |= UART_CONFIG_PAR_ONE;
		break;
	case UART_CFG_PARITY_SPACE:
		line_ctrl |= UART_CONFIG_PAR_ZERO;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		line_ctrl |= UART_CONFIG_STOP_ONE;
		break;
	case UART_CFG_STOP_BITS_2:
		line_ctrl |= UART_CONFIG_STOP_TWO;
		break;
	case UART_CFG_STOP_BITS_0_5:
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		line_ctrl |= UART_CONFIG_WLEN_5;
		break;
	case UART_CFG_DATA_BITS_6:
		line_ctrl |= UART_CONFIG_WLEN_6;
		break;
	case UART_CFG_DATA_BITS_7:
		line_ctrl |= UART_CONFIG_WLEN_7;
		break;
	case UART_CFG_DATA_BITS_8:
		line_ctrl |= UART_CONFIG_WLEN_8;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		flow_ctrl = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		flow_ctrl = true;
		break;
	case UART_CFG_FLOW_CTRL_DTR_DSR:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	/* Disables UART before setting control registers */
	UARTConfigSetExpClk(config->reg, config->sys_clk_freq, cfg->baudrate, line_ctrl);

	if (flow_ctrl) {
		UARTEnableCTS(config->reg);
		UARTEnableRTS(config->reg);
	} else {
		UARTDisableCTS(config->reg);
		UARTDisableRTS(config->reg);
	}

	/* Re-enable UART */
	UARTEnable(config->reg);

	/* Make use of the FIFO to reduce chances of data being lost */
	UARTEnableFifo(config->reg);

	data->uart_config = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_cc23x0_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_cc23x0_data *data = dev->data;

	*cfg = data->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_cc23x0_fifo_fill(const struct device *dev, const uint8_t *buf, int len)
{
	const struct uart_cc23x0_config *config = dev->config;
	int n = 0;

	while (n < len) {
		if (!UARTSpaceAvailable(config->reg)) {
			break;
		}
		UARTPutCharNonBlocking(config->reg, buf[n]);
		n++;
	}

	return n;
}

static int uart_cc23x0_fifo_read(const struct device *dev, uint8_t *buf, const int len)
{
	const struct uart_cc23x0_config *config = dev->config;
	int c, n;

	n = 0;
	while (n < len) {
		if (!UARTCharAvailable(config->reg)) {
			break;
		}
		c = UARTGetCharNonBlocking(config->reg);
		buf[n++] = c;
	}

	return n;
}

static void uart_cc23x0_irq_tx_enable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	/* When TX IRQ is enabled, it is implicit that we are expecting to transmit
	 * using the UART, hence we should no longer go into standby
	 */
	uart_cc23x0_pm_policy_state_lock_get(dev->data, UART_CC23X0_PM_LOCK_TX);

	UARTEnableInt(config->reg, UART_INT_TX);
}

static void uart_cc23x0_irq_tx_disable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	UARTDisableInt(config->reg, UART_INT_TX);

	uart_cc23x0_pm_policy_state_lock_put(dev->data, UART_CC23X0_PM_LOCK_TX);
}

static int uart_cc23x0_irq_tx_ready(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTSpaceAvailable(config->reg) ? 1 : 0;
}

static void uart_cc23x0_irq_rx_enable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	/* When RX IRQ is enabled, it is implicit that we are expecting to receive
	 * from the UART, hence we can no longer go into standby
	 */
	uart_cc23x0_pm_policy_state_lock_get(dev->data, UART_CC23X0_PM_LOCK_RX);

	/* Trigger the ISR on both RX and Receive Timeout. This is to allow
	 * the use of the hardware FIFOs for more efficient operation
	 */
	UARTEnableInt(config->reg, UART_INT_RX | UART_INT_RT);
}

static void uart_cc23x0_irq_rx_disable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	UARTDisableInt(config->reg, UART_INT_RX | UART_INT_RT);

	uart_cc23x0_pm_policy_state_lock_put(dev->data, UART_CC23X0_PM_LOCK_RX);
}

static int uart_cc23x0_irq_tx_complete(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTBusy(config->reg) ? 0 : 1;
}

static int uart_cc23x0_irq_rx_ready(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTCharAvailable(config->reg) ? 1 : 0;
}

static void uart_cc23x0_irq_err_enable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTEnableInt(config->reg, UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE);
}

static void uart_cc23x0_irq_err_disable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTDisableInt(config->reg, UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE);
}

static int uart_cc23x0_irq_is_pending(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	/* Read masked interrupt status */
	uint32_t status = UARTIntStatus(config->reg, true);

	return status ? 1 : 0;
}

static int uart_cc23x0_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void uart_cc23x0_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct uart_cc23x0_data *data = dev->data;

	data->callback = cb;
	data->user_data = user_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if CONFIG_UART_CC23X0_DMA_DRIVEN

static int uart_cc23x0_async_callback_set(const struct device *dev, uart_callback_t callback,
					  void *user_data)
{
	struct uart_cc23x0_data *data = dev->data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async_callback = NULL;
	data->async_user_data = NULL;
#else
	data->async_callback = callback;
	data->async_user_data = user_data;
#endif

	return 0;
}

static int uart_cc23x0_async_tx(const struct device *dev, const uint8_t *buf, size_t len,
				int32_t timeout)
{
	const struct uart_cc23x0_config *config = dev->config;
	struct uart_cc23x0_data *data = dev->data;
	unsigned int key;
	int ret;

	struct dma_block_config block_cfg_tx = {
		.source_address = (uint32_t)buf,
		.dest_address = UART_CC23_REG_GET(config->reg, UART_O_DR),
		.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.block_size = len,
	};

	struct dma_config dma_cfg_tx = {
		.dma_slot = config->dma_trigsrc_tx,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.block_count = 1,
		.head_block = &block_cfg_tx,
		.source_data_size = 1,
		.dest_data_size = 1,
		.source_burst_length = UART_CC23_BURST_LEN,
		.dma_callback = NULL,
		.user_data = NULL,
	};

	key = irq_lock();

	if (data->tx_len) {
		irq_unlock(key);
		return -EBUSY;
	}

	data->tx_buf = buf;
	data->tx_len = len;

	irq_unlock(key);

	/* Resume DMA (TX) */
	ret = pm_device_runtime_get(config->dma_dev);
	if (ret) {
		return ret;
	}

	ret = dma_config(config->dma_dev, config->dma_channel_tx, &dma_cfg_tx);
	if (ret) {
		return ret;
	}

	/* Disable DMA trigger */
	UARTDisableDMA(config->reg, UART_DMA_TX);

	/* Schedule timeout work */
	if (timeout != SYS_FOREVER_US) {
		k_work_reschedule(&data->tx_timeout_work, K_USEC(timeout));
	}

	/* Start DMA channel */
	ret = dma_start(config->dma_dev, config->dma_channel_tx);
	if (ret) {
		return ret;
	}

	/* Lock PM */
	uart_cc23x0_pm_policy_state_lock_get(data, UART_CC23X0_PM_LOCK_TX);

	/* Enable DMA trigger to start the transfer */
	UARTEnableDMA(config->reg, UART_DMA_TX);

	return 0;
}

static int uart_cc23x0_tx_halt(struct uart_cc23x0_data *data)
{
	const struct uart_cc23x0_config *config = data->dev->config;
	struct dma_status status;
	struct uart_event evt;
	size_t total_len;
	unsigned int key;
	int ret;

	key = irq_lock();

	total_len = data->tx_len;

	evt.type = UART_TX_ABORTED;
	evt.data.tx.buf = data->tx_buf;
	evt.data.tx.len = 0;

	data->tx_buf = NULL;
	data->tx_len = 0;

	dma_stop(config->dma_dev, config->dma_channel_tx);

	irq_unlock(key);

	if (dma_get_status(config->dma_dev, config->dma_channel_tx, &status) == 0) {
		evt.data.tx.len = total_len - status.pending_length;
	}

	if (total_len) {
		if (data->async_callback) {
			data->async_callback(data->dev, &evt, data->async_user_data);
		}

		/* Unlock PM */
		uart_cc23x0_pm_policy_state_lock_put(data, UART_CC23X0_PM_LOCK_TX);

		/* Suspend DMA (TX) */
		ret = pm_device_runtime_put(config->dma_dev);
		if (ret) {
			return ret;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static void uart_cc23x0_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_cc23x0_data *data = CONTAINER_OF(dwork, struct uart_cc23x0_data,
						     tx_timeout_work);

	uart_cc23x0_tx_halt(data);
}

static int uart_cc23x0_async_tx_abort(const struct device *dev)
{
	struct uart_cc23x0_data *data = dev->data;

	k_work_cancel_delayable(&data->tx_timeout_work);

	return uart_cc23x0_tx_halt(data);
}

static int uart_cc23x0_async_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
				       int32_t timeout)
{
	const struct uart_cc23x0_config *config = dev->config;
	struct uart_cc23x0_data *data = dev->data;
	struct uart_event evt;
	unsigned int key;
	int ret;

	struct dma_block_config block_cfg_rx = {
		.source_address = UART_CC23_REG_GET(config->reg, UART_O_DR),
		.dest_address = (uint32_t)buf,
		.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.block_size = len,
	};

	struct dma_config dma_cfg_rx = {
		.dma_slot = config->dma_trigsrc_rx,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.block_count = 1,
		.head_block = &block_cfg_rx,
		.source_data_size = 1,
		.dest_data_size = 1,
		.source_burst_length = UART_CC23_BURST_LEN,
		.dma_callback = NULL,
		.user_data = NULL,
	};

	if (timeout != SYS_FOREVER_US) {
		return -ENOTSUP;
	}

	key = irq_lock();

	if (data->rx_len) {
		ret = -EBUSY;
		goto unlock;
	}

	/* Resume DMA (RX) */
	ret = pm_device_runtime_get(config->dma_dev);
	if (ret) {
		goto unlock;
	}

	ret = dma_config(config->dma_dev, config->dma_channel_rx, &dma_cfg_rx);
	if (ret) {
		goto unlock;
	}

	/* Disable DMA trigger */
	UARTDisableDMA(config->reg, UART_DMA_RX);

	/* Start DMA channel */
	ret = dma_start(config->dma_dev, config->dma_channel_rx);
	if (ret) {
		goto unlock;
	}

	/* Lock PM */
	uart_cc23x0_pm_policy_state_lock_get(data, UART_CC23X0_PM_LOCK_RX);

	/* Enable DMA trigger to start the transfer */
	UARTEnableDMA(config->reg, UART_DMA_RX);

	data->rx_buf = buf;
	data->rx_len = len;
	data->rx_processed_len = 0;

	/* Request next buffer */
	if (data->async_callback) {
		evt.type = UART_RX_BUF_REQUEST;

		data->async_callback(dev, &evt, data->async_user_data);
	}

unlock:
	irq_unlock(key);

	return ret;
}

static int uart_cc23x0_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_cc23x0_data *data = dev->data;
	unsigned int key;
	int ret = 0;

	key = irq_lock();

	if (data->rx_len == 0) {
		ret = -EACCES;
		goto unlock;
	}

	if (data->rx_next_len) {
		ret = -EBUSY;
		goto unlock;
	}

	data->rx_next_buf = buf;
	data->rx_next_len = len;

unlock:
	irq_unlock(key);

	return ret;
}

static void uart_cc23x0_notify_rx_processed(struct uart_cc23x0_data *data,
					    size_t processed)
{
	struct uart_event evt;

	if (!data->async_callback || data->rx_processed_len == processed) {
		return;
	}

	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->rx_buf;
	evt.data.rx.offset = data->rx_processed_len;
	evt.data.rx.len = processed - data->rx_processed_len;

	data->rx_processed_len = processed;

	data->async_callback(data->dev, &evt, data->async_user_data);
}

static int uart_cc23x0_async_rx_disable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;
	struct uart_cc23x0_data *data = dev->data;
	struct dma_status status;
	struct uart_event evt;
	size_t rx_processed;
	unsigned int key;
	int ret = 0;

	key = irq_lock();

	if (data->rx_len == 0) {
		ret = -EINVAL;
		goto unlock;
	}

	dma_stop(config->dma_dev, config->dma_channel_rx);

	/* Unlock PM */
	uart_cc23x0_pm_policy_state_lock_put(data, UART_CC23X0_PM_LOCK_RX);

	if (dma_get_status(config->dma_dev, config->dma_channel_rx, &status) == 0 &&
	    status.pending_length) {
		rx_processed = data->rx_len - status.pending_length;

		uart_cc23x0_notify_rx_processed(data, rx_processed);
	}

	/* Suspend DMA (RX) */
	ret = pm_device_runtime_put(config->dma_dev);
	if (ret) {
		goto unlock;
	}

	if (data->async_callback) {
		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = data->rx_buf;

		data->async_callback(dev, &evt, data->async_user_data);
	}

	data->rx_buf = NULL;
	data->rx_len = 0;

	if (data->rx_next_len) {
		if (data->async_callback) {
			evt.type = UART_RX_BUF_RELEASED;
			evt.data.rx_buf.buf = data->rx_next_buf;

			data->async_callback(dev, &evt, data->async_user_data);
		}

		data->rx_next_buf = NULL;
		data->rx_next_len = 0;
	}

	if (data->async_callback) {
		evt.type = UART_RX_DISABLED;

		data->async_callback(dev, &evt, data->async_user_data);
	}

unlock:
	irq_unlock(key);

	return ret;
}

#endif /* CONFIG_UART_CC23X0_DMA_DRIVEN */

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_CC23X0_DMA_DRIVEN

static void uart_cc23x0_isr(const struct device *dev)
{
	struct uart_cc23x0_data *data = dev->data;
#if CONFIG_UART_CC23X0_DMA_DRIVEN
	const struct uart_cc23x0_config *config = dev->config;
	struct uart_event evt;
	unsigned int key;
	uint32_t int_status = UARTIntStatus(config->reg, true);
#endif

#if CONFIG_UART_INTERRUPT_DRIVEN
	if (data->callback) {
		data->callback(dev, data->user_data);
	}
#endif

#if CONFIG_UART_CC23X0_DMA_DRIVEN
	/*
	 * When a peripheral channel is used (which is the case here for UART),
	 * the DMA transfer completion is signaled on the peripheral's interrupt only.
	 * It is not signaled on the DMA dedicated interrupt.
	 */
	if (int_status & UART_INT_TXDMADONE) {
		k_work_cancel_delayable(&data->tx_timeout_work);

		key = irq_lock();

		if (data->tx_len && data->async_callback) {
			evt.type = UART_TX_DONE;
			evt.data.tx.buf = data->tx_buf;
			evt.data.tx.len = data->tx_len;

			data->async_callback(dev, &evt, data->async_user_data);
		}

		data->tx_buf = NULL;
		data->tx_len = 0;

		/* Unlock PM */
		uart_cc23x0_pm_policy_state_lock_put(data, UART_CC23X0_PM_LOCK_TX);

		/* Suspend DMA (TX) */
		pm_device_runtime_put(config->dma_dev);

		irq_unlock(key);

		UARTClearInt(config->reg, UART_INT_TXDMADONE);
	}

	if (int_status & UART_INT_RXDMADONE) {
		key = irq_lock();

		uart_cc23x0_notify_rx_processed(data, data->rx_len);

		if (data->async_callback) {
			evt.type = UART_RX_BUF_RELEASED;
			evt.data.rx.buf = data->rx_buf;

			data->async_callback(dev, &evt, data->async_user_data);
		}

		if (data->rx_next_len == 0) {
			/* If no next buffer, end the transfer */
			data->rx_buf = NULL;
			data->rx_len = 0;

			if (data->async_callback) {
				evt.type = UART_RX_DISABLED;

				data->async_callback(dev, &evt, data->async_user_data);
			}

			/* Unlock PM */
			uart_cc23x0_pm_policy_state_lock_put(data, UART_CC23X0_PM_LOCK_RX);

			/* Suspend DMA (RX) */
			pm_device_runtime_put(config->dma_dev);
		} else {
			/* Otherwise, load next buffer and start the transfer */
			data->rx_buf = data->rx_next_buf;
			data->rx_len = data->rx_next_len;
			data->rx_next_buf = NULL;
			data->rx_next_len = 0;
			data->rx_processed_len = 0;

			dma_reload(config->dma_dev, config->dma_channel_rx,
				   (uint32_t)UART_CC23_REG_GET(config->reg, UART_O_DR),
				   (uint32_t)data->rx_buf, data->rx_len);

			dma_start(config->dma_dev, config->dma_channel_rx);

			/* Request a new buffer */
			if (data->async_callback) {
				evt.type = UART_RX_BUF_REQUEST;

				data->async_callback(dev, &evt, data->async_user_data);
			}
		}

		irq_unlock(key);

		UARTClearInt(config->reg, UART_INT_RXDMADONE);
	}
#endif
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_CC23X0_DMA_DRIVEN */

static DEVICE_API(uart, uart_cc23x0_driver_api) = {
	.poll_in = uart_cc23x0_poll_in,
	.poll_out = uart_cc23x0_poll_out,
	.err_check = uart_cc23x0_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_cc23x0_configure,
	.config_get = uart_cc23x0_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_cc23x0_fifo_fill,
	.fifo_read = uart_cc23x0_fifo_read,
	.irq_tx_enable = uart_cc23x0_irq_tx_enable,
	.irq_tx_disable = uart_cc23x0_irq_tx_disable,
	.irq_tx_ready = uart_cc23x0_irq_tx_ready,
	.irq_rx_enable = uart_cc23x0_irq_rx_enable,
	.irq_rx_disable = uart_cc23x0_irq_rx_disable,
	.irq_tx_complete = uart_cc23x0_irq_tx_complete,
	.irq_rx_ready = uart_cc23x0_irq_rx_ready,
	.irq_err_enable = uart_cc23x0_irq_err_enable,
	.irq_err_disable = uart_cc23x0_irq_err_disable,
	.irq_is_pending = uart_cc23x0_irq_is_pending,
	.irq_update = uart_cc23x0_irq_update,
	.irq_callback_set = uart_cc23x0_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#if CONFIG_UART_CC23X0_DMA_DRIVEN
	.callback_set = uart_cc23x0_async_callback_set,
	.tx = uart_cc23x0_async_tx,
	.tx_abort = uart_cc23x0_async_tx_abort,
	.rx_enable = uart_cc23x0_async_rx_enable,
	.rx_buf_rsp = uart_cc23x0_async_rx_buf_rsp,
	.rx_disable = uart_cc23x0_async_rx_disable,
#endif /* CONFIG_UART_CC23X0_DMA_DRIVEN */
};

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_CC23X0_DMA_DRIVEN
#define UART_CC23X0_IRQ_CFG(n)                                                                     \
	do {                                                                                       \
		UARTClearInt(config->reg, UART_INT_RX);                                            \
		UARTClearInt(config->reg, UART_INT_RT);                                            \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_cc23x0_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	} while (false)

#else
#define UART_CC23X0_IRQ_CFG(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_CC23X0_DMA_DRIVEN */

#if CONFIG_UART_INTERRUPT_DRIVEN
#define UART_CC23X0_INT_FIELDS .callback = NULL, .user_data = NULL,
#else
#define UART_CC23X0_INT_FIELDS
#endif

#ifdef CONFIG_UART_CC23X0_DMA_DRIVEN
#define UART_CC23X0_DMA_INIT(n)						\
	.dma_dev = DEVICE_DT_GET(TI_CC23X0_DT_INST_DMA_CTLR(n, tx)),	\
	.dma_channel_tx = TI_CC23X0_DT_INST_DMA_CHANNEL(n, tx),		\
	.dma_trigsrc_tx = TI_CC23X0_DT_INST_DMA_TRIGSRC(n, tx),		\
	.dma_channel_rx = TI_CC23X0_DT_INST_DMA_CHANNEL(n, rx),		\
	.dma_trigsrc_rx = TI_CC23X0_DT_INST_DMA_TRIGSRC(n, rx),
#else
#define UART_CC23X0_DMA_INIT(n)
#endif

static int uart_cc23x0_init_common(const struct device *dev)
{
#ifdef CONFIG_UART_CC23X0_DMA_DRIVEN
	const struct uart_cc23x0_config *config = dev->config;
#endif
	struct uart_cc23x0_data *data = dev->data;

	CLKCTLEnable(CLKCTL_BASE, CLKCTL_UART0);

#ifdef CONFIG_UART_CC23X0_DMA_DRIVEN
	if (!device_is_ready(config->dma_dev)) {
		return -ENODEV;
	}

	UARTEnableInt(config->reg, UART_INT_TXDMADONE | UART_INT_RXDMADONE);

	k_work_init_delayable(&data->tx_timeout_work, uart_cc23x0_async_tx_timeout);

	data->dev = dev;
#endif

#ifdef CONFIG_PM_DEVICE
	atomic_clear_bit(data->pm_lock, UART_CC23X0_PM_LOCK_RX);
	atomic_clear_bit(data->pm_lock, UART_CC23X0_PM_LOCK_TX);
#endif

	/* Configure and enable UART */
	return uart_cc23x0_configure(dev, &data->uart_config);
}

static int uart_cc23x0_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_cc23x0_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		UARTDisable(config->reg);
		CLKCTLDisable(CLKCTL_BASE, CLKCTL_UART0);
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		return uart_cc23x0_init_common(dev);
	default:
		return -ENOTSUP;
	}
}

#define UART_CC23X0_DEVICE_DEFINE(n)                                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_cc23x0_init_##n, PM_DEVICE_DT_INST_GET(n),                   \
			      &uart_cc23x0_data_##n,  &uart_cc23x0_config_##n,                     \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_cc23x0_driver_api)

#define UART_CC23X0_INIT_FUNC(n)                                                                   \
	static int uart_cc23x0_init_##n(const struct device *dev)                                  \
	{                                                                                          \
		const struct uart_cc23x0_config *config = dev->config;                             \
		int ret;                                                                           \
                                                                                                   \
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);                    \
		if (ret) {                                                                         \
			return ret;                                                                \
		}                                                                                  \
                                                                                                   \
		/* Enable interrupts */                                                            \
		UART_CC23X0_IRQ_CFG(n);                                                            \
                                                                                                   \
		return pm_device_driver_init(dev, uart_cc23x0_pm_action);                          \
	}

#define UART_CC23X0_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	PM_DEVICE_DT_INST_DEFINE(n, uart_cc23x0_pm_action);                                        \
	UART_CC23X0_INIT_FUNC(n);                                                                  \
                                                                                                   \
	static struct uart_cc23x0_config uart_cc23x0_config_##n = {                                \
		.reg = DT_INST_REG_ADDR(n),                                                        \
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		UART_CC23X0_DMA_INIT(n)                                                            \
		};                                                                                 \
                                                                                                   \
	static struct uart_cc23x0_data uart_cc23x0_data_##n = {                                    \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(n, current_speed),                        \
				.parity = DT_INST_ENUM_IDX(n, parity),                             \
				.stop_bits = DT_INST_ENUM_IDX(n, stop_bits),                       \
				.data_bits = DT_INST_ENUM_IDX(n, data_bits),                       \
				.flow_ctrl = DT_INST_PROP(n, hw_flow_control),                     \
			},                                                                         \
		UART_CC23X0_INT_FIELDS                                                             \
		};                                                                                 \
	UART_CC23X0_DEVICE_DEFINE(n);

DT_INST_FOREACH_STATUS_OKAY(UART_CC23X0_INIT)
