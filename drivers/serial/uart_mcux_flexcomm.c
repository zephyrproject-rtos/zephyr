/*
 * Copyright 2017, 2022-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_usart

/** @file
 * @brief UART driver for MCUX Flexcomm USART.
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/pm.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <fsl_usart.h>
#include <soc.h>
#include <fsl_device_registers.h>
#include <zephyr/drivers/pinctrl.h>
#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#include <fsl_inputmux.h>
#endif

#define FC_UART_IS_WAKEUP (IS_ENABLED(CONFIG_PM) && DT_ANY_INST_HAS_BOOL_STATUS_OKAY(wakeup_source))

#ifdef CONFIG_UART_ASYNC_API
struct mcux_flexcomm_uart_dma_config {
	const struct device *dev;
	DMA_Type *base;
	uint8_t channel;
	struct dma_config cfg;
};
#endif

struct mcux_flexcomm_config {
	USART_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t baud_rate;
	uint8_t parity;
#ifdef CONFIG_UART_MCUX_FLEXCOMM_ISR_SUPPORT
	void (*irq_config_func)(const struct device *dev);
#endif
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_UART_ASYNC_API
	struct mcux_flexcomm_uart_dma_config tx_dma;
	struct mcux_flexcomm_uart_dma_config rx_dma;
	void (*rx_timeout_func)(struct k_work *work);
	void (*tx_timeout_func)(struct k_work *work);
#endif
#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
	void (*pm_unlock_work_fn)(struct k_work *);
#endif
#if FC_UART_IS_WAKEUP
	void (*wakeup_cfg)(void);
	clock_control_subsys_t lp_clock_subsys;
#endif
};

#if CONFIG_UART_ASYNC_API
struct mcux_flexcomm_uart_tx_data {
	const uint8_t *xfer_buf;
	size_t xfer_len;
	struct dma_block_config active_block;
	struct k_work_delayable timeout_work;
};

struct mcux_flexcomm_uart_rx_data {
	uint8_t *xfer_buf;
	size_t xfer_len;
	struct dma_block_config active_block;
	uint8_t *next_xfer_buf;
	size_t next_xfer_len;
	struct k_work_delayable timeout_work;
	int32_t timeout;
	size_t count;
	size_t offset;
};
#endif

struct mcux_flexcomm_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_callback;
	void *irq_cb_data;
#endif
#ifdef CONFIG_UART_ASYNC_API
	uart_callback_t async_callback;
	void *async_cb_data;
	struct mcux_flexcomm_uart_tx_data tx_data;
	struct mcux_flexcomm_uart_rx_data rx_data;
#endif
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config uart_config;
#endif
#if FC_UART_IS_WAKEUP
	struct pm_notifier pm_handles;
	uint16_t old_brg;
	uint8_t old_osr;
#endif
#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
	bool pm_policy_state_lock;
	struct k_work pm_lock_work;
#endif
};

#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
static void mcux_flexcomm_pm_policy_state_lock_get(const struct device *dev)
{
	struct mcux_flexcomm_data *data = dev->data;

	if (!data->pm_policy_state_lock) {
		data->pm_policy_state_lock = true;
		pm_policy_device_power_lock_get(dev);
	}
}

static void mcux_flexcomm_pm_unlock_if_idle(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;

	if (config->base->STAT & USART_STAT_TXIDLE_MASK) {
		data->pm_policy_state_lock = false;
		pm_policy_device_power_lock_put(dev);
	} else {
		/* can't block systemn workqueue so keep re-submitting until it's done */
		k_work_submit(&data->pm_lock_work);
	}
}

static void mcux_flexcomm_pm_policy_state_lock_put(const struct device *dev)
{
	struct mcux_flexcomm_data *data = dev->data;

	if (data->pm_policy_state_lock) {
		/* we can't block on TXidle mask in IRQ context so offload */
		k_work_submit(&data->pm_lock_work);
	}
}
#endif /* CONFIG_PM_POLICY_DEVICE_CONSTRAINTS */

static int mcux_flexcomm_poll_in(const struct device *dev, unsigned char *c)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t flags = USART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kUSART_RxFifoNotEmptyFlag) {
		*c = USART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void mcux_flexcomm_poll_out(const struct device *dev,
					     unsigned char c)
{
	const struct mcux_flexcomm_config *config = dev->config;

	/* Wait until space is available in TX FIFO, as per API description:
	 * This routine checks if the transmitter is full.
	 * When the transmitter is not full, it writes a character to the data register.
	 * It waits and blocks the calling thread otherwise.
	 */
	while (!(USART_GetStatusFlags(config->base) & kUSART_TxFifoNotFullFlag)) {
	}

	USART_WriteByte(config->base, c);

	/* Wait for the transfer to complete, as per API description:
	 * This function is a blocking call. It blocks the calling thread until the character
	 * is sent.
	 */
	while (!(USART_GetStatusFlags(config->base) & kUSART_TxFifoEmptyFlag)) {
	}
}

static int mcux_flexcomm_err_check(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t flags = USART_GetStatusFlags(config->base);
	int err = 0;

	if (flags & kUSART_RxError) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & kUSART_ParityErrorFlag) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & kUSART_FramingErrorFlag) {
		err |= UART_ERROR_FRAMING;
	}

	USART_ClearStatusFlags(config->base,
			       kUSART_RxError |
			       kUSART_ParityErrorFlag |
			       kUSART_FramingErrorFlag);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int mcux_flexcomm_fifo_fill(const struct device *dev,
				   const uint8_t *tx_data,
				   int len)
{
	const struct mcux_flexcomm_config *config = dev->config;
	int num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (USART_GetStatusFlags(config->base)
		& kUSART_TxFifoNotFullFlag)) {

		USART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int mcux_flexcomm_fifo_read(const struct device *dev, uint8_t *rx_data,
				   const int len)
{
	const struct mcux_flexcomm_config *config = dev->config;
	int num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (USART_GetStatusFlags(config->base)
		& kUSART_RxFifoNotEmptyFlag)) {

		rx_data[num_rx++] = USART_ReadByte(config->base);
	}

	return num_rx;
}

static void mcux_flexcomm_irq_tx_enable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_TxLevelInterruptEnable;

	/* Indicates that this device started a transaction that should
	 * not be interrupted by putting the SoC in states that would
	 * interfere with this transfer.
	 */
#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
	mcux_flexcomm_pm_policy_state_lock_get(dev);
#endif

	USART_EnableInterrupts(config->base, mask);
}

static void mcux_flexcomm_irq_tx_disable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_TxLevelInterruptEnable;

#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
	mcux_flexcomm_pm_policy_state_lock_put(dev);
#endif

	USART_DisableInterrupts(config->base, mask);
}

static int mcux_flexcomm_irq_tx_complete(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;

	return (config->base->STAT & USART_STAT_TXIDLE_MASK) != 0;
}

static int mcux_flexcomm_irq_tx_ready(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_TxLevelInterruptEnable;
	uint32_t flags = USART_GetStatusFlags(config->base);

	return (USART_GetEnabledInterrupts(config->base) & mask)
		&& (flags & kUSART_TxFifoEmptyFlag);
}

static void mcux_flexcomm_irq_rx_enable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_RxLevelInterruptEnable;

	USART_EnableInterrupts(config->base, mask);
}

static void mcux_flexcomm_irq_rx_disable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_RxLevelInterruptEnable;

	USART_DisableInterrupts(config->base, mask);
}

static int mcux_flexcomm_irq_rx_full(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t flags = USART_GetStatusFlags(config->base);

	return (flags & kUSART_RxFifoNotEmptyFlag) != 0U;
}

static int mcux_flexcomm_irq_rx_pending(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_RxLevelInterruptEnable;

	return (USART_GetEnabledInterrupts(config->base) & mask)
		&& mcux_flexcomm_irq_rx_full(dev);
}

static void mcux_flexcomm_irq_err_enable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_NoiseErrorInterruptEnable |
			kUSART_FramingErrorInterruptEnable |
			kUSART_ParityErrorInterruptEnable;

	USART_EnableInterrupts(config->base, mask);
}

static void mcux_flexcomm_irq_err_disable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_NoiseErrorInterruptEnable |
			kUSART_FramingErrorInterruptEnable |
			kUSART_ParityErrorInterruptEnable;

	USART_DisableInterrupts(config->base, mask);
}

static int mcux_flexcomm_irq_is_pending(const struct device *dev)
{
	return (mcux_flexcomm_irq_tx_ready(dev)
		|| mcux_flexcomm_irq_rx_pending(dev));
}

static int mcux_flexcomm_irq_update(const struct device *dev)
{
	return 1;
}

static void mcux_flexcomm_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb,
					   void *cb_data)
{
	struct mcux_flexcomm_data *data = dev->data;

	data->irq_callback = cb;
	data->irq_cb_data = cb_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async_callback = NULL;
	data->async_cb_data = NULL;
#endif
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int mcux_flexcomm_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	struct uart_config *uart_config = &data->uart_config;
	usart_config_t usart_config;
	usart_parity_mode_t parity_mode;
	usart_stop_bit_count_t stop_bits;
	usart_data_len_t data_bits = kUSART_8BitsPerChar;
	bool nine_bit_mode = false;
	uint32_t clock_freq;

	/* Set up structure to reconfigure UART */
	USART_GetDefaultConfig(&usart_config);

	/* Set parity */
	if (cfg->parity == UART_CFG_PARITY_ODD) {
		parity_mode = kUSART_ParityOdd;
	} else if (cfg->parity == UART_CFG_PARITY_EVEN) {
		parity_mode = kUSART_ParityEven;
	} else if (cfg->parity == UART_CFG_PARITY_NONE) {
		parity_mode = kUSART_ParityDisabled;
	} else {
		return -ENOTSUP;
	}
	usart_config.parityMode = parity_mode;

	/* Set baudrate */
	usart_config.baudRate_Bps = cfg->baudrate;

	/* Set stop bits */
	if (cfg->stop_bits == UART_CFG_STOP_BITS_1) {
		stop_bits = kUSART_OneStopBit;
	} else if (cfg->stop_bits == UART_CFG_STOP_BITS_2) {
		stop_bits = kUSART_TwoStopBit;
	} else {
		return -ENOTSUP;
	}
	usart_config.stopBitCount = stop_bits;

	/* Set data bits */
	if (cfg->data_bits == UART_CFG_DATA_BITS_5 ||
	    cfg->data_bits == UART_CFG_DATA_BITS_6) {
		return -ENOTSUP;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_7) {
		data_bits = kUSART_7BitsPerChar;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_8) {
		data_bits = kUSART_8BitsPerChar;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_9) {
		nine_bit_mode = true;
	} else {
		return -EINVAL;
	}
	usart_config.bitCountPerChar = data_bits;

	/* Set flow control */
	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE) {
		usart_config.enableHardwareFlowControl = false;
	} else if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		usart_config.enableHardwareFlowControl = true;
	} else {
		return -ENOTSUP;
	}

	/* Wait for USART to finish transmission and turn off */
	USART_Deinit(config->base);

	/* Get UART clock frequency */
	clock_control_get_rate(config->clock_dev,
		config->clock_subsys, &clock_freq);

	/* Handle 9 bit mode */
	USART_Enable9bitMode(config->base, nine_bit_mode);

	/* Reconfigure UART */
	USART_Init(config->base, &usart_config, clock_freq);

	/* Update driver device data */
	uart_config->parity = cfg->parity;
	uart_config->baudrate = cfg->baudrate;
	uart_config->stop_bits = cfg->stop_bits;
	uart_config->data_bits = cfg->data_bits;
	uart_config->flow_ctrl = cfg->flow_ctrl;

	return 0;
}

static int mcux_flexcomm_uart_config_get(const struct device *dev,
					struct uart_config *cfg)
{
	struct mcux_flexcomm_data *data = dev->data;
	*cfg = data->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_ASYNC_API
/* This function is called by this driver to notify user callback of events */
static void async_user_callback(const struct device *dev,
					      struct uart_event *evt)
{
	const struct mcux_flexcomm_data *data = dev->data;

	if (data->async_callback) {
		data->async_callback(dev, evt, data->async_cb_data);
	}
}

static int mcux_flexcomm_uart_callback_set(const struct device *dev,
					   uart_callback_t callback,
					   void *user_data)
{
	struct mcux_flexcomm_data *data = dev->data;

	data->async_callback = callback;
	data->async_cb_data = user_data;


#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->irq_callback = NULL;
	data->irq_cb_data = NULL;
#endif

	return 0;
}

static int mcux_flexcomm_uart_tx(const struct device *dev, const uint8_t *buf,
				 size_t len, int32_t timeout)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	int ret = 0;

	if (config->tx_dma.dev == NULL) {
		return -ENODEV;
	}

	unsigned int key = irq_lock();

	/* Getting DMA status to tell if channel is busy or not set up */
	struct dma_status status;

	ret = dma_get_status(config->tx_dma.dev, config->tx_dma.channel, &status);

	if (ret < 0) {
		irq_unlock(key);
		return ret;
	}

	/* There is an ongoing transfer */
	if (status.busy) {
		irq_unlock(key);
		return -EBUSY;
	}

	/* Disable TX DMA requests for uart while setting up */
	USART_EnableTxDMA(config->base, false);

	/* Set up the dma channel/transfer */
	data->tx_data.xfer_buf = buf;
	data->tx_data.xfer_len = len;
	data->tx_data.active_block.source_address = (uint32_t)buf;
	data->tx_data.active_block.dest_address = (uint32_t) &config->base->FIFOWR;
	data->tx_data.active_block.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->tx_data.active_block.block_size = len;
	data->tx_data.active_block.next_block = NULL;

	ret = dma_config(config->tx_dma.dev, config->tx_dma.channel,
				(struct dma_config *) &config->tx_dma.cfg);
	if (ret) {
		irq_unlock(key);
		return ret;
	}

	/* Enable interrupt for when TX fifo is empty (all data transmitted) */
	config->base->FIFOINTENSET |= USART_FIFOINTENSET_TXLVL_MASK;

	/* Enable TX DMA requests */
	USART_EnableTxDMA(config->base, true);

	/* Do not allow the system to suspend until the transmission has completed */
#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
	mcux_flexcomm_pm_policy_state_lock_get(dev);
#endif

	/* Trigger the DMA to start transfer */
	ret = dma_start(config->tx_dma.dev, config->tx_dma.channel);
	if (ret) {
		irq_unlock(key);
#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
		mcux_flexcomm_pm_policy_state_lock_put(dev);
#endif
	return ret;
	}

	/* Schedule a TX abort for @param timeout */
	if (timeout != SYS_FOREVER_US) {
		k_work_schedule(&data->tx_data.timeout_work, K_USEC(timeout));
	}

	irq_unlock(key);

	return ret;
}

static int mcux_flexcomm_uart_tx_abort(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	int ret = 0;

	/* First disable DMA requests from UART to prevent transfer
	 * status change during the abort routine
	 */
	USART_EnableTxDMA(config->base, false);

	/* In case there is no transfer to abort */
	if (data->tx_data.xfer_len == 0) {
		return -EFAULT;
	}

	/* In case a user called this function, do not abort twice */
	(void)k_work_cancel_delayable(&data->tx_data.timeout_work);

	/* Getting dma status to use to calculate bytes sent */
	struct dma_status status = {0};

	ret = dma_get_status(config->tx_dma.dev, config->tx_dma.channel, &status);
	if (ret < 0) {
		return ret;
	}

	/* Done with the DMA transfer, can stop it now */
	ret = dma_stop(config->tx_dma.dev, config->tx_dma.channel);
	if (ret) {
		return ret;
	}

	/* Define TX abort event before resetting driver variables */
	size_t sent_len = data->tx_data.xfer_len - status.pending_length;
	const uint8_t *aborted_buf = data->tx_data.xfer_buf;
	struct uart_event tx_abort_event = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = aborted_buf,
		.data.tx.len = sent_len
	};

	/* Driver data needs reset since there is no longer an ongoing
	 * transfer, this should before the user callback, not after,
	 * just in case the user callback calls tx again
	 */
	data->tx_data.xfer_len = 0;
	data->tx_data.xfer_buf = NULL;

	async_user_callback(dev, &tx_abort_event);

	return ret;
}

static int mcux_flexcomm_uart_rx_enable(const struct device *dev, uint8_t *buf,
					const size_t len, const int32_t timeout)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	int ret = 0;

	if (config->rx_dma.dev == NULL) {
		return -ENODEV;
	}

	/* Getting DMA status to tell if channel is busy or not set up */
	struct dma_status status;

	ret = dma_get_status(config->rx_dma.dev, config->rx_dma.channel, &status);

	if (ret < 0) {
		return ret;
	}

	/* There is an ongoing transfer */
	if (status.busy) {
		return -EBUSY;
	}

	/* Disable RX DMA requests for uart while setting up */
	USART_EnableRxDMA(config->base, false);

	/* Set up the dma channel/transfer */
	data->rx_data.xfer_buf = buf;
	data->rx_data.xfer_len = len;
	data->rx_data.active_block.dest_address = (uint32_t)data->rx_data.xfer_buf;
	data->rx_data.active_block.source_address = (uint32_t) &config->base->FIFORD;
	data->tx_data.active_block.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->rx_data.active_block.block_size = data->rx_data.xfer_len;

	ret = dma_config(config->rx_dma.dev, config->rx_dma.channel,
				(struct dma_config *) &config->rx_dma.cfg);
	if (ret) {
		return ret;
	}

	data->rx_data.timeout = timeout;

	/* Enable RX DMA requests from UART */
	USART_EnableRxDMA(config->base, true);

	/* Enable start bit detected interrupt, this is the only
	 * way for the flexcomm uart to support the Zephyr Async API.
	 * This is only needed if using a timeout.
	 */
	if (timeout != SYS_FOREVER_US) {
		config->base->INTENSET |= USART_INTENSET_STARTEN_MASK;
	}

	/* Trigger the DMA to start transfer */
	ret = dma_start(config->rx_dma.dev, config->rx_dma.channel);
	if (ret) {
		return ret;
	}

	/* Request next buffer */
	struct uart_event rx_buf_request = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(dev, &rx_buf_request);

	return ret;
}

static void flexcomm_uart_rx_update(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;

	struct dma_status status;

	(void)dma_get_status(config->rx_dma.dev, config->rx_dma.channel, &status);

	/* Calculate how many bytes have been received by RX DMA */
	size_t total_rx_receive_len = data->rx_data.xfer_len - status.pending_length;

	/* Generate RX ready event if there has been new data received */
	if (total_rx_receive_len > data->rx_data.offset) {

		data->rx_data.count = total_rx_receive_len - data->rx_data.offset;
		struct uart_event rx_rdy_event = {
			.type = UART_RX_RDY,
			.data.rx.buf = data->rx_data.xfer_buf,
			.data.rx.len = data->rx_data.count,
			.data.rx.offset = data->rx_data.offset,
		};

		async_user_callback(dev, &rx_rdy_event);
	}

	/* The data is no longer new, update buffer tracking variables */
	data->rx_data.offset += data->rx_data.count;
	data->rx_data.count = 0;

}

static int mcux_flexcomm_uart_rx_disable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	int ret = 0;

	/* This bit can be used to check if RX is already disabled
	 * because it is the bit changed by enabling and disabling DMA
	 * requests, and in this driver, RX DMA requests should only be
	 * disabled when the rx function is disabled other than when
	 * setting up in uart_rx_enable.
	 */
	if (!(config->base->FIFOCFG & USART_FIFOCFG_DMARX_MASK)) {
		return -EFAULT;
	}

	/* In case a user called this function, don't disable twice */
	(void)k_work_cancel_delayable(&data->rx_data.timeout_work);


	/* Disable RX requests to pause DMA first and measure what happened,
	 * Can't stop yet because DMA pending length is needed to
	 * calculate how many bytes have been received
	 */
	USART_EnableRxDMA(config->base, false);

	/* Check if RX data received and generate rx ready event if so */
	flexcomm_uart_rx_update(dev);

	/* Notify DMA driver to stop transfer only after RX data handled */
	ret = dma_stop(config->rx_dma.dev, config->rx_dma.channel);
	if (ret) {
		return ret;
	}

	/* Generate buffer release event for current buffer */
	struct uart_event current_buffer_release_event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->rx_data.xfer_buf,
	};

	async_user_callback(dev, &current_buffer_release_event);

	/* Generate buffer release event for next buffer */
	if (data->rx_data.next_xfer_buf) {
		struct uart_event next_buffer_release_event = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->rx_data.next_xfer_buf
		};

		async_user_callback(dev, &next_buffer_release_event);
	}

	/* Reset RX driver data */
	data->rx_data.xfer_buf = NULL;
	data->rx_data.xfer_len = 0;
	data->rx_data.next_xfer_buf = NULL;
	data->rx_data.next_xfer_len = 0;
	data->rx_data.offset = 0;
	data->rx_data.count = 0;

	/* Final event is the RX disable event */
	struct uart_event rx_disabled_event = {
		.type = UART_RX_DISABLED
	};

	async_user_callback(dev, &rx_disabled_event);

	return ret;
}

static int mcux_flexcomm_uart_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;

	/* There is already a next buffer scheduled */
	if (data->rx_data.next_xfer_buf != NULL || data->rx_data.next_xfer_len != 0) {
		return -EBUSY;
	}

	/* DMA requests are disabled, meaning the RX has been disabled */
	if (!(config->base->FIFOCFG & USART_FIFOCFG_DMARX_MASK)) {
		return -EACCES;
	}

	/* If everything is fine, schedule the new buffer */
	data->rx_data.next_xfer_buf = buf;
	data->rx_data.next_xfer_len = len;

	return 0;
}

/* This callback is from the TX DMA and consumed by this driver */
static void mcux_flexcomm_uart_dma_tx_callback(const struct device *dma_device, void *cb_data,
				uint32_t channel, int status)
{
	/* DMA callback data was configured during driver init as UART device ptr */
	struct device *dev = (struct device *)cb_data;

	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;

	unsigned int key = irq_lock();

	/* Turn off requests since we are aborting */
	USART_EnableTxDMA(config->base, false);

	/* Timeout did not happen */
	(void)k_work_cancel_delayable(&data->tx_data.timeout_work);

	irq_unlock(key);
}

/* This callback is from the RX DMA and consumed by this driver */
static void mcux_flexcomm_uart_dma_rx_callback(const struct device *dma_device, void *cb_data,
				uint32_t channel, int status)
{
	/* DMA callback data was configured during driver init as UART device ptr */
	struct device *dev = (struct device *)cb_data;

	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;

	/* Cancel timeout now that the transfer is complete */
	(void)k_work_cancel_delayable(&data->rx_data.timeout_work);

	/* Update user with received RX data if needed */
	flexcomm_uart_rx_update(dev);

	/* Release current buffer */
	struct uart_event current_buffer_release_event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->rx_data.xfer_buf,
	};

	async_user_callback(dev, &current_buffer_release_event);

	if (data->rx_data.next_xfer_buf) {
		/* Replace buffer in driver data */
		data->rx_data.xfer_buf = data->rx_data.next_xfer_buf;
		data->rx_data.xfer_len = data->rx_data.next_xfer_len;
		data->rx_data.next_xfer_buf = NULL;
		data->rx_data.next_xfer_len = 0;

		/* Reload DMA channel with new buffer */
		data->rx_data.active_block.block_size = data->rx_data.xfer_len;
		data->rx_data.active_block.dest_address = (uint32_t) data->rx_data.xfer_buf;
		dma_reload(config->rx_dma.dev, config->rx_dma.channel,
				data->rx_data.active_block.source_address,
				data->rx_data.active_block.dest_address,
				data->rx_data.active_block.block_size);

		/* Request next buffer */
		struct uart_event rx_buf_request = {
			.type = UART_RX_BUF_REQUEST,
		};

		async_user_callback(dev, &rx_buf_request);

		/* Start the new transfer */
		dma_start(config->rx_dma.dev, config->rx_dma.channel);

	} else {
		/* If there is no next available buffer then disable DMA */
		mcux_flexcomm_uart_rx_disable(dev);
	}

	/* Now that this transfer was finished, reset tracking variables */
	data->rx_data.count = 0;
	data->rx_data.offset = 0;
}

#if defined(CONFIG_SOC_SERIES_IMXRT5XX) || defined(CONFIG_SOC_SERIES_IMXRT6XX)
/*
 * This functions calculates the inputmux connection value
 * needed by INPUTMUX_EnableSignal to allow the UART's DMA
 * request to reach the DMA.
 */
static uint32_t fc_uart_calc_inmux_connection(uint8_t channel, DMA_Type *base)
{
	uint32_t chmux_avl = 0;
	uint32_t chmux_sel = 0;
	uint32_t chmux_val = 0;

#if defined(CONFIG_SOC_SERIES_IMXRT5XX)
	uint32_t chmux_sel_id = 0;

	if (base == (DMA_Type *)DMA0_BASE) {
		chmux_sel_id = DMA0_CHMUX_SEL0_ID;
	} else if (base == (DMA_Type *)DMA1_BASE) {
		chmux_sel_id = DMA1_CHMUX_SEL0_ID;
	}


	if (channel >= 16 && !(channel >= 24 && channel <= 27)) {
		chmux_avl = 1 << CHMUX_AVL_SHIFT;
	} else {
		chmux_avl = 0;
	}

	/* 1 for flexcomm */
	chmux_val = 1 << CHMUX_VAL_SHIFT;


	if (channel <= 15 || (channel >= 24 && channel <= 27)) {
		chmux_sel = 0;
	} else if (channel >= 16 && channel <= 23) {
		chmux_sel = (chmux_sel_id + 4 * (channel - 16))
				<< CHMUX_OFF_SHIFT;
	} else {
		chmux_sel = (chmux_sel_id + 4 * (channel - 20))
				<< CHMUX_OFF_SHIFT;
	}

#endif /* RT5xx */

	uint32_t req_en_id = 0;

	if (base == (DMA_Type *)DMA0_BASE) {
		req_en_id = DMA0_REQ_ENA0_ID;
	} else if (base == (DMA_Type *)DMA1_BASE) {
		req_en_id = DMA1_REQ_ENA0_ID;
	}


	uint32_t en_val;

	if (channel <= 31) {
		en_val = channel + (req_en_id << ENA_SHIFT);
	} else {
		en_val = (channel - 32) + ((req_en_id + 4) << ENA_SHIFT);
	}


	uint32_t ret = en_val + chmux_avl + chmux_val + chmux_sel;

	return ret;
}
#endif /* RT 3-digit */


static int flexcomm_uart_async_init(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;

	if (config->rx_dma.dev == NULL ||
		config->tx_dma.dev == NULL) {
		return -ENODEV;
	}

	if (!device_is_ready(config->rx_dma.dev) ||
		!device_is_ready(config->tx_dma.dev)) {
		return -ENODEV;
	}

	/* Disable DMA requests */
	USART_EnableTxDMA(config->base, false);
	USART_EnableRxDMA(config->base, false);

	/* Route DMA requests */
#if defined(CONFIG_SOC_SERIES_IMXRT5XX) || defined(CONFIG_SOC_SERIES_IMXRT6XX)
	/* RT 3 digit uses input mux to route DMA requests from
	 * the UART peripheral to a hardware designated DMA channel
	 */
	INPUTMUX_Init(INPUTMUX);
	INPUTMUX_EnableSignal(INPUTMUX,
		fc_uart_calc_inmux_connection(config->rx_dma.channel,
					config->rx_dma.base), true);
	INPUTMUX_EnableSignal(INPUTMUX,
		fc_uart_calc_inmux_connection(config->tx_dma.channel,
					config->tx_dma.base), true);
	INPUTMUX_Deinit(INPUTMUX);
#endif /* RT5xx and RT6xx */

	/* Init work objects for RX and TX timeouts */
	k_work_init_delayable(&data->tx_data.timeout_work,
			config->tx_timeout_func);
	k_work_init_delayable(&data->rx_data.timeout_work,
			config->rx_timeout_func);

	return 0;
}

#endif /* CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_MCUX_FLEXCOMM_ISR_SUPPORT
static void mcux_flexcomm_isr(const struct device *dev)
{
	struct mcux_flexcomm_data *data = dev->data;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->irq_callback) {
		data->irq_callback(dev, data->irq_cb_data);
	}
#endif

#ifdef CONFIG_UART_ASYNC_API
	const struct mcux_flexcomm_config *config = dev->config;

	/* If there is an async callback then we are using async api */
	if (data->async_callback) {

		/* Handle RX interrupt (START bit detected)
		 * RX interrupt defeats the purpose of UART ASYNC API
		 * because core is involved for every byte but
		 * it is included for compatibility of applications.
		 * There is no other way with flexcomm UART to handle
		 * Zephyr's RX ASYNC API. However, if not using the RX
		 * timeout (timeout is forever), then the performance is
		 * still as might be expected.
		 */
		if (config->base->INTSTAT & USART_INTSTAT_START_MASK) {

			/* Receiving some data so reschedule timeout,
			 * unless timeout is 0 in which case just handle
			 * rx data now. If timeout is forever, don't do anything.
			 */
			if (data->rx_data.timeout == 0) {
				flexcomm_uart_rx_update(dev);
			} else if (data->rx_data.timeout != SYS_FOREVER_US) {
				k_work_reschedule(&data->rx_data.timeout_work,
						K_USEC(data->rx_data.timeout));
			}

			/* Write 1 to clear start bit status bit */
			config->base->STAT |= USART_STAT_START_MASK;
		}

		/* Handle TX interrupt (TXLVL = 0)
		 * Default TXLVL interrupt happens when TXLVL = 0, which
		 * has not been changed by this driver, so in this case the
		 * TX interrupt should happen when transfer is complete
		 * because DMA filling TX fifo is faster than transmitter rate
		 */
		if (config->base->FIFOINTSTAT & USART_FIFOINTSTAT_TXLVL_MASK) {

			/* Disable interrupt */
			config->base->FIFOINTENCLR = USART_FIFOINTENCLR_TXLVL_MASK;

			/* Set up TX done event to notify the user of completion */
			struct uart_event tx_done_event = {
				.type = UART_TX_DONE,
				.data.tx.buf = data->tx_data.xfer_buf,
				.data.tx.len = data->tx_data.xfer_len,
			};

			/* Reset TX data */
			data->tx_data.xfer_len = 0;
			data->tx_data.xfer_buf = NULL;

			async_user_callback(dev, &tx_done_event);

#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
			mcux_flexcomm_pm_policy_state_lock_put(dev);
#endif
		}

	}
#endif /* CONFIG_UART_ASYNC_API */
}
#endif /* CONFIG_UART_MCUX_FLEXCOMM_ISR_SUPPORT */

static int mcux_flexcomm_init_common(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct mcux_flexcomm_data *data = dev->data;
	struct uart_config *cfg = &data->uart_config;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
	usart_config_t usart_config;
	usart_parity_mode_t parity_mode;
	uint32_t clock_freq;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	/* Get the clock frequency */
	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	if (config->parity == UART_CFG_PARITY_ODD) {
		parity_mode = kUSART_ParityOdd;
	} else if (config->parity == UART_CFG_PARITY_EVEN) {
		parity_mode = kUSART_ParityEven;
	} else {
		parity_mode = kUSART_ParityDisabled;
	}

	USART_GetDefaultConfig(&usart_config);
	usart_config.enableTx = true;
	usart_config.enableRx = true;
	usart_config.parityMode = parity_mode;
	usart_config.baudRate_Bps = config->baud_rate;

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	cfg->baudrate = config->baud_rate;
	cfg->parity = config->parity;
	 /* From USART_GetDefaultConfig */
	cfg->stop_bits = UART_CFG_STOP_BITS_1;
	cfg->data_bits = UART_CFG_DATA_BITS_8;
	cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

	USART_Init(config->base, &usart_config, clock_freq);

#ifdef CONFIG_UART_MCUX_FLEXCOMM_ISR_SUPPORT
	config->irq_config_func(dev);
#endif

#ifdef CONFIG_UART_ASYNC_API
	err = flexcomm_uart_async_init(dev);
	if (err) {
		return err;
	}
#endif

	return 0;
}

#if FC_UART_IS_WAKEUP
static void mcux_flexcomm_pm_prepare_wake(const struct device *dev, enum pm_state state)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	USART_Type *base = config->base;

	/* Switch to the lowest possible baud rate, in order to
	 * both minimize power consumption and also be able to
	 * potentially wake up the chip from this mode.
	 */
	if (pm_policy_device_is_disabling_state(dev, state, 0)) {
		clock_control_configure(config->clock_dev, config->lp_clock_subsys, NULL);
		data->old_brg = base->BRG;
		data->old_osr = base->OSR;
		base->OSR = 8;
		base->BRG = 0;
	}
}

static void mcux_flexcomm_pm_restore_wake(const struct device *dev, enum pm_state state)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	USART_Type *base = config->base;

	if (pm_policy_device_is_disabling_state(dev, state, 0)) {
		clock_control_configure(config->clock_dev, config->clock_subsys, NULL);
		base->OSR = data->old_osr;
		base->BRG = data->old_brg;
	}
}
#endif /* FC_UART_IS_WAKEUP */

static uint32_t usart_intenset;
static int mcux_flexcomm_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct mcux_flexcomm_config *config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		usart_intenset = USART_GetEnabledInterrupts(config->base);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		ret = mcux_flexcomm_init_common(dev);
		if (ret) {
			return ret;
		}
		USART_EnableInterrupts(config->base, usart_intenset);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int mcux_flexcomm_init(const struct device *dev)
{
#if FC_UART_IS_WAKEUP || defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
#endif

#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	k_work_init(&data->pm_lock_work, config->pm_unlock_work_fn);
#endif

#if FC_UART_IS_WAKEUP
	config->wakeup_cfg();
	pm_notifier_register(&data->pm_handles);
#endif

	/* Rest of the init is done from the PM_DEVICE_TURN_ON action
	 * which is invoked by pm_device_driver_init().
	 */
	return pm_device_driver_init(dev, mcux_flexcomm_pm_action);
}

static DEVICE_API(uart, mcux_flexcomm_driver_api) = {
	.poll_in = mcux_flexcomm_poll_in,
	.poll_out = mcux_flexcomm_poll_out,
	.err_check = mcux_flexcomm_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = mcux_flexcomm_uart_configure,
	.config_get = mcux_flexcomm_uart_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = mcux_flexcomm_fifo_fill,
	.fifo_read = mcux_flexcomm_fifo_read,
	.irq_tx_enable = mcux_flexcomm_irq_tx_enable,
	.irq_tx_disable = mcux_flexcomm_irq_tx_disable,
	.irq_tx_complete = mcux_flexcomm_irq_tx_complete,
	.irq_tx_ready = mcux_flexcomm_irq_tx_ready,
	.irq_rx_enable = mcux_flexcomm_irq_rx_enable,
	.irq_rx_disable = mcux_flexcomm_irq_rx_disable,
	.irq_rx_ready = mcux_flexcomm_irq_rx_full,
	.irq_err_enable = mcux_flexcomm_irq_err_enable,
	.irq_err_disable = mcux_flexcomm_irq_err_disable,
	.irq_is_pending = mcux_flexcomm_irq_is_pending,
	.irq_update = mcux_flexcomm_irq_update,
	.irq_callback_set = mcux_flexcomm_irq_callback_set,
#endif
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = mcux_flexcomm_uart_callback_set,
	.tx = mcux_flexcomm_uart_tx,
	.tx_abort = mcux_flexcomm_uart_tx_abort,
	.rx_enable = mcux_flexcomm_uart_rx_enable,
	.rx_disable = mcux_flexcomm_uart_rx_disable,
	.rx_buf_rsp = mcux_flexcomm_uart_rx_buf_rsp,
#endif
};


#ifdef CONFIG_UART_MCUX_FLEXCOMM_ISR_SUPPORT
#define UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC(n)					\
	static void mcux_flexcomm_irq_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    mcux_flexcomm_isr, DEVICE_DT_INST_GET(n), 0);	\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}
#define UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC_INIT(n)					\
	.irq_config_func = mcux_flexcomm_irq_config_func_##n,
#else
#define UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC(n)
#define UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC_INIT(n)
#endif /* CONFIG_UART_MCUX_FLEXCOMM_ISR_SUPPORT */

#ifdef CONFIG_PM_POLICY_DEVICE_CONSTRAINTS
#define UART_MCUX_FLEXCOMM_PM_UNLOCK_FUNC_DEFINE(n)				\
static void mcux_flexcomm_##n##_pm_unlock(struct k_work *work)			\
{										\
	const struct device *dev = DEVICE_DT_INST_GET(n);			\
										\
	mcux_flexcomm_pm_unlock_if_idle(dev);					\
}
#define UART_MCUX_FLEXCOMM_PM_UNLOCK_FUNC_BIND(n)				\
	.pm_unlock_work_fn = mcux_flexcomm_##n##_pm_unlock,
#else
#define UART_MCUX_FLEXCOMM_PM_UNLOCK_FUNC_DEFINE(n)
#define UART_MCUX_FLEXCOMM_PM_UNLOCK_FUNC_BIND(n)
#endif /* CONFIG_PM_POLICY_DEVICE_CONSTRAINTS */

#ifdef CONFIG_UART_ASYNC_API
#define UART_MCUX_FLEXCOMM_TX_TIMEOUT_FUNC(n)					\
	static void mcux_flexcomm_uart_##n##_tx_timeout(struct k_work *work)	\
	{									\
		mcux_flexcomm_uart_tx_abort(DEVICE_DT_INST_GET(n));		\
	}
#define UART_MCUX_FLEXCOMM_RX_TIMEOUT_FUNC(n)					\
	static void mcux_flexcomm_uart_##n##_rx_timeout(struct k_work *work)	\
	{									\
		flexcomm_uart_rx_update(DEVICE_DT_INST_GET(n));			\
	}

DT_INST_FOREACH_STATUS_OKAY(UART_MCUX_FLEXCOMM_TX_TIMEOUT_FUNC);
DT_INST_FOREACH_STATUS_OKAY(UART_MCUX_FLEXCOMM_RX_TIMEOUT_FUNC);

#define UART_MCUX_FLEXCOMM_ASYNC_CFG(n)						\
	.tx_dma = {								\
		.dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),		\
		.channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),		\
		.cfg = {							\
			.source_burst_length = 1,				\
			.dest_burst_length = 1,					\
			.source_data_size = 1,					\
			.dest_data_size = 1,					\
			.complete_callback_en = 1,				\
			.error_callback_dis = 1,				\
			.block_count = 1,					\
			.head_block =						\
				&mcux_flexcomm_##n##_data.tx_data.active_block,	\
			.channel_direction = MEMORY_TO_PERIPHERAL,		\
			.dma_callback = mcux_flexcomm_uart_dma_tx_callback,	\
			.user_data = (void *)DEVICE_DT_INST_GET(n),		\
		},								\
		.base = (DMA_Type *)						\
				DT_REG_ADDR(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),	\
	},									\
	.rx_dma = {								\
		.dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),		\
		.channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),		\
		.cfg = {							\
			.source_burst_length = 1,				\
			.dest_burst_length = 1,					\
			.source_data_size = 1,					\
			.dest_data_size = 1,					\
			.complete_callback_en = 1,				\
			.error_callback_dis = 1,				\
			.block_count = 1,					\
			.head_block =						\
				&mcux_flexcomm_##n##_data.rx_data.active_block,	\
			.channel_direction = PERIPHERAL_TO_MEMORY,		\
			.dma_callback = mcux_flexcomm_uart_dma_rx_callback,	\
			.user_data = (void *)DEVICE_DT_INST_GET(n)		\
		},								\
		.base = (DMA_Type *)						\
				DT_REG_ADDR(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),	\
	},									\
	.rx_timeout_func = mcux_flexcomm_uart_##n##_rx_timeout,			\
	.tx_timeout_func = mcux_flexcomm_uart_##n##_tx_timeout,
#else
#define UART_MCUX_FLEXCOMM_ASYNC_CFG(n)
#endif /* CONFIG_UART_ASYNC_API */

#if FC_UART_IS_WAKEUP

#define UART_MCUX_FLEXCOMM_WAKEUP_CFG_DEFINE(n)					\
static void serial_mcux_flexcomm_##n##_wakeup_cfg(void)				\
{										\
	IF_ENABLED(DT_INST_PROP(n, wakeup_source), (				\
		NXP_ENABLE_WAKEUP_SIGNAL(DT_INST_IRQN(n));			\
	))									\
}
#define UART_MCUX_FLEXCOMM_WAKEUP_CFG_BIND(n)					\
		.wakeup_cfg = serial_mcux_flexcomm_##n##_wakeup_cfg,

#define UART_MCUX_FLEXCOMM_PM_HANDLES_DEFINE(n)					\
static void serial_mcux_flexcomm_##n##_pm_entry(enum pm_state state)		\
{										\
	IF_ENABLED(DT_INST_PROP(n, wakeup_source), (				\
		mcux_flexcomm_pm_prepare_wake(DEVICE_DT_INST_GET(n), state);	\
	))									\
}										\
										\
static void serial_mcux_flexcomm_##n##_pm_exit(enum pm_state state)		\
{										\
	IF_ENABLED(DT_INST_PROP(n, wakeup_source), (				\
		mcux_flexcomm_pm_restore_wake(DEVICE_DT_INST_GET(n), state);	\
	))									\
}
#define UART_MCUX_FLEXCOMM_PM_HANDLES_BIND(n)					\
	.pm_handles = {								\
			.state_entry = serial_mcux_flexcomm_##n##_pm_entry,	\
			.state_exit = serial_mcux_flexcomm_##n##_pm_exit,	\
	},
#define UART_MCUX_FLEXCOMM_LP_CLK_SUBSYS(n)					\
	.lp_clock_subsys = (clock_control_subsys_t)				\
				DT_INST_CLOCKS_CELL_BY_NAME(n, sleep, name),
#else
#define UART_MCUX_FLEXCOMM_WAKEUP_CFG_DEFINE(n)
#define UART_MCUX_FLEXCOMM_WAKEUP_CFG_BIND(n)
#define UART_MCUX_FLEXCOMM_PM_HANDLES_DEFINE(n)
#define UART_MCUX_FLEXCOMM_PM_HANDLES_BIND(n)
#define UART_MCUX_FLEXCOMM_LP_CLK_SUBSYS(n)
#endif /* FC_UART_IS_WAKEUP */

#define UART_MCUX_FLEXCOMM_INIT_CFG(n)						\
static const struct mcux_flexcomm_config mcux_flexcomm_##n##_config = {		\
	.base = (USART_Type *)DT_INST_REG_ADDR(n),				\
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
	.clock_subsys =								\
	(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),			\
	.baud_rate = DT_INST_PROP(n, current_speed),				\
	.parity = DT_INST_ENUM_IDX(n, parity),					\
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
	UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC_INIT(n)					\
	UART_MCUX_FLEXCOMM_ASYNC_CFG(n)						\
	UART_MCUX_FLEXCOMM_PM_UNLOCK_FUNC_BIND(n)				\
	UART_MCUX_FLEXCOMM_WAKEUP_CFG_BIND(n)					\
	UART_MCUX_FLEXCOMM_LP_CLK_SUBSYS(n)					\
};

#define UART_MCUX_FLEXCOMM_INIT_DATA(n)						\
static struct mcux_flexcomm_data mcux_flexcomm_##n##_data = {			\
	UART_MCUX_FLEXCOMM_PM_HANDLES_BIND(n)					\
};

#define UART_MCUX_FLEXCOMM_INIT(n)						\
										\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	UART_MCUX_FLEXCOMM_PM_UNLOCK_FUNC_DEFINE(n)				\
	UART_MCUX_FLEXCOMM_WAKEUP_CFG_DEFINE(n)					\
	UART_MCUX_FLEXCOMM_PM_HANDLES_DEFINE(n)					\
	PM_DEVICE_DT_INST_DEFINE(n, mcux_flexcomm_pm_action);			\
										\
	UART_MCUX_FLEXCOMM_INIT_DATA(n)						\
										\
	UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC(n)					\
										\
	UART_MCUX_FLEXCOMM_INIT_CFG(n)						\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			    &mcux_flexcomm_init,				\
			    PM_DEVICE_DT_INST_GET(n),				\
			    &mcux_flexcomm_##n##_data,				\
			    &mcux_flexcomm_##n##_config,			\
			    PRE_KERNEL_1,					\
			    CONFIG_SERIAL_INIT_PRIORITY,			\
			    &mcux_flexcomm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_MCUX_FLEXCOMM_INIT)
