/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#include <wrap_max32_dma.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>

#include <wrap_max32_uart.h>

#define DT_DRV_COMPAT adi_max32_uart

LOG_MODULE_REGISTER(uart_max32, CONFIG_UART_LOG_LEVEL);

#ifdef CONFIG_UART_ASYNC_API
struct max32_uart_dma_config {
	const struct device *dev;
	const uint32_t channel;
	const uint32_t slot;
};
#endif /* CONFIG_UART_ASYNC_API */

struct max32_uart_config {
	mxc_uart_regs_t *regs;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
	struct uart_config uart_conf;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */
#ifdef CONFIG_UART_ASYNC_API
	const struct max32_uart_dma_config tx_dma;
	const struct max32_uart_dma_config rx_dma;
#endif /* CONFIG_UART_ASYNC_API */
};

#ifdef CONFIG_UART_ASYNC_API
#define MAX32_UART_TX_CACHE_NUM 2
struct max32_uart_async_tx {
	const uint8_t *buf;
	const uint8_t *src;
	size_t len;
	uint8_t cache[MAX32_UART_TX_CACHE_NUM][CONFIG_UART_TX_CACHE_LEN];
	uint8_t cache_id;
	struct dma_block_config dma_blk;
	int32_t timeout;
	struct k_work_delayable timeout_work;
};

struct max32_uart_async_rx {
	uint8_t *buf;
	size_t len;
	size_t offset;
	size_t counter;
	uint8_t *next_buf;
	size_t next_len;
	int32_t timeout;
	struct k_work_delayable timeout_work;
};

struct max32_uart_async_data {
	const struct device *uart_dev;
	struct max32_uart_async_tx tx;
	struct max32_uart_async_rx rx;
	uart_callback_t cb;
	void *user_data;
};
#endif

struct max32_uart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /* Interrupt callback */
	void *cb_data;                    /* Interrupt callback arg */
	uint32_t flags;                   /* Cached interrupt flags */
	uint32_t status;                  /* Cached status flags */
#endif
#ifdef CONFIG_UART_ASYNC_API
	struct max32_uart_async_data async;
#endif
	struct uart_config conf; /* baudrate, stopbits, ... */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_max32_isr(const struct device *dev);
#endif

#ifdef CONFIG_UART_ASYNC_API
static int uart_max32_tx_dma_load(const struct device *dev, uint8_t *buf, size_t len);
#endif

static void api_poll_out(const struct device *dev, unsigned char c)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_WriteCharacter(cfg->regs, c);
}

static int api_poll_in(const struct device *dev, unsigned char *c)
{
	int val;
	const struct max32_uart_config *cfg = dev->config;

	val = MXC_UART_ReadCharacterRaw(cfg->regs);
	if (val >= 0) {
		*c = (unsigned char)val;
	} else {
		return -1;
	}

	return 0;
}

static int api_err_check(const struct device *dev)
{
	int err = 0;
	uint32_t flags;
	const struct max32_uart_config *cfg = dev->config;

	flags = MXC_UART_GetFlags(cfg->regs);

	if (flags & ADI_MAX32_UART_ERROR_FRAMING) {
		err |= UART_ERROR_FRAMING;
	}

	if (flags & ADI_MAX32_UART_ERROR_PARITY) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & ADI_MAX32_UART_ERROR_OVERRUN) {
		err |= UART_ERROR_OVERRUN;
	}

	return err;
}

static int api_configure(const struct device *dev, const struct uart_config *uart_cfg)
{
	int err;
	const struct max32_uart_config *const cfg = dev->config;
	mxc_uart_regs_t *regs = cfg->regs;
	struct max32_uart_data *data = dev->data;

	/*
	 *  Set parity
	 */
	if (data->conf.parity != uart_cfg->parity) {
		mxc_uart_parity_t mxc_parity;

		switch (uart_cfg->parity) {
		case UART_CFG_PARITY_NONE:
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_NONE;
			break;
		case UART_CFG_PARITY_ODD:
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_ODD;
			break;
		case UART_CFG_PARITY_EVEN:
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_EVEN;
			break;
		case UART_CFG_PARITY_MARK:
#if defined(ADI_MAX32_UART_CFG_PARITY_MARK)
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_MARK;
			break;
#else
			return -ENOTSUP;
#endif
		case UART_CFG_PARITY_SPACE:
#if defined(ADI_MAX32_UART_CFG_PARITY_SPACE)
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_SPACE;
			break;
#else
			return -ENOTSUP;
#endif
		default:
			return -EINVAL;
		}

		err = MXC_UART_SetParity(regs, mxc_parity);
		if (err < 0) {
			return -ENOTSUP;
		}
		/* incase of success keep configuration */
		data->conf.parity = uart_cfg->parity;
	}

	/*
	 *  Set stop bit
	 */
	if (data->conf.stop_bits != uart_cfg->stop_bits) {
		if (uart_cfg->stop_bits == UART_CFG_STOP_BITS_1) {
			err = MXC_UART_SetStopBits(regs, MXC_UART_STOP_1);
		} else if (uart_cfg->stop_bits == UART_CFG_STOP_BITS_2) {
			err = MXC_UART_SetStopBits(regs, MXC_UART_STOP_2);
		} else {
			return -ENOTSUP;
		}
		if (err < 0) {
			return -ENOTSUP;
		}
		/* incase of success keep configuration */
		data->conf.stop_bits = uart_cfg->stop_bits;
	}

	/*
	 *  Set data bit
	 *  Valid data for MAX32  is 5-6-7-8
	 *  Valid data for Zepyhr is 0-1-2-3
	 *  Added +5 to index match.
	 */
	if (data->conf.data_bits != uart_cfg->data_bits) {
		err = MXC_UART_SetDataSize(regs, (5 + uart_cfg->data_bits));
		if (err < 0) {
			return -ENOTSUP;
		}
		/* incase of success keep configuration */
		data->conf.data_bits = uart_cfg->data_bits;
	}

	/*
	 *  Set flow control
	 *  Flow control not implemented yet so that only support no flow mode
	 */
	if (data->conf.flow_ctrl != uart_cfg->flow_ctrl) {
		if (uart_cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
			return -ENOTSUP;
		}
		data->conf.flow_ctrl = uart_cfg->flow_ctrl;
	}

	/*
	 *  Set baudrate
	 */
	if (data->conf.baudrate != uart_cfg->baudrate) {
		err = Wrap_MXC_UART_SetFrequency(regs, uart_cfg->baudrate, cfg->perclk.clk_src);
		if (err < 0) {
			return -ENOTSUP;
		}
		/* In case of success keep configuration */
		data->conf.baudrate = uart_cfg->baudrate;
	}
	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int api_config_get(const struct device *dev, struct uart_config *uart_cfg)
{
	struct max32_uart_data *data = dev->data;

	/* copy configs from global setting */
	*uart_cfg = data->conf;

	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_ASYNC_API
static void uart_max32_async_tx_timeout(struct k_work *work);
static void uart_max32_async_rx_timeout(struct k_work *work);
#endif /* CONFIG_UART_ASYNC_API */

static int uart_max32_init(const struct device *dev)
{
	int ret;
	const struct max32_uart_config *const cfg = dev->config;
	mxc_uart_regs_t *regs = cfg->regs;
#ifdef CONFIG_UART_ASYNC_API
	struct max32_uart_data *data = dev->data;
#endif

	if (!device_is_ready(cfg->clock)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	ret = MXC_UART_Shutdown(regs);
	if (ret) {
		return ret;
	}

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret != 0) {
		LOG_ERR("Cannot enable UART clock");
		return ret;
	}

	ret = Wrap_MXC_UART_SetClockSource(regs, cfg->perclk.clk_src);
	if (ret != 0) {
		LOG_ERR("Cannot set UART clock source");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = api_configure(dev, &cfg->uart_conf);
	if (ret) {
		return ret;
	}

	ret = Wrap_MXC_UART_Init(regs);
	if (ret) {
		return ret;
	}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	/* Clear any pending UART RX/TX interrupts */
	MXC_UART_ClearFlags(regs, (ADI_MAX32_UART_INT_RX | ADI_MAX32_UART_INT_TX));
	cfg->irq_config_func(dev);
#endif

#ifdef CONFIG_UART_ASYNC_API
	data->async.uart_dev = dev;
	k_work_init_delayable(&data->async.tx.timeout_work, uart_max32_async_tx_timeout);
	k_work_init_delayable(&data->async.rx.timeout_work, uart_max32_async_rx_timeout);
	data->async.rx.len = 0;
	data->async.rx.offset = 0;
#endif

	return ret;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int api_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	unsigned int num_tx = 0;
	const struct max32_uart_config *cfg = dev->config;

	num_tx = MXC_UART_WriteTXFIFO(cfg->regs, (unsigned char *)tx_data, size);

	return (int)num_tx;
}

static int api_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	unsigned int num_rx = 0;
	const struct max32_uart_config *cfg = dev->config;

	num_rx = MXC_UART_ReadRXFIFO(cfg->regs, (unsigned char *)rx_data, size);
	if (num_rx == 0) {
		MXC_UART_ClearFlags(cfg->regs, ADI_MAX32_UART_INT_RX);
	}

	return num_rx;
}

static void api_irq_tx_enable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;
	unsigned int key;

	MXC_UART_EnableInt(cfg->regs, ADI_MAX32_UART_INT_TX | ADI_MAX32_UART_INT_TX_OEM);

	key = irq_lock();
	uart_max32_isr(dev);
	irq_unlock(key);
}

static void api_irq_tx_disable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_DisableInt(cfg->regs, ADI_MAX32_UART_INT_TX | ADI_MAX32_UART_INT_TX_OEM);
}

static int api_irq_tx_ready(const struct device *dev)
{
	struct max32_uart_data *const data = dev->data;
	const struct max32_uart_config *cfg = dev->config;
	uint32_t inten = Wrap_MXC_UART_GetRegINTEN(cfg->regs);

	return ((inten & (ADI_MAX32_UART_INT_TX | ADI_MAX32_UART_INT_TX_OEM)) &&
		!(data->status & ADI_MAX32_UART_STATUS_TX_FULL));
}

static int api_irq_tx_complete(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	if (MXC_UART_GetActive(cfg->regs) == E_BUSY) {
		return 0;
	} else {
		return 1; /* transmission completed */
	}
}

static int api_irq_rx_ready(const struct device *dev)
{
	struct max32_uart_data *const data = dev->data;
	const struct max32_uart_config *cfg = dev->config;
	uint32_t inten = Wrap_MXC_UART_GetRegINTEN(cfg->regs);

	return ((inten & ADI_MAX32_UART_INT_RX) && !(data->status & ADI_MAX32_UART_RX_EMPTY));
}

static void api_irq_err_enable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_EnableInt(cfg->regs, ADI_MAX32_UART_ERROR_INTERRUPTS);
}

static void api_irq_err_disable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_DisableInt(cfg->regs, ADI_MAX32_UART_ERROR_INTERRUPTS);
}

static int api_irq_is_pending(const struct device *dev)
{
	struct max32_uart_data *const data = dev->data;

	return (data->flags & (ADI_MAX32_UART_INT_RX | ADI_MAX32_UART_INT_TX));
}

static int api_irq_update(const struct device *dev)
{
	struct max32_uart_data *const data = dev->data;
	const struct max32_uart_config *const cfg = dev->config;

	data->flags = MXC_UART_GetFlags(cfg->regs);
	data->status = MXC_UART_GetStatus(cfg->regs);

	MXC_UART_ClearFlags(cfg->regs, data->flags);

	return 1;
}

static void api_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				 void *cb_data)
{
	struct max32_uart_data *const dev_data = dev->data;

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
static void api_irq_rx_enable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_EnableInt(cfg->regs, ADI_MAX32_UART_INT_RX);
}

static void api_irq_rx_disable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_DisableInt(cfg->regs, ADI_MAX32_UART_INT_RX);
}

static void uart_max32_isr(const struct device *dev)
{
	struct max32_uart_data *data = dev->data;
	const struct max32_uart_config *cfg = dev->config;
	uint32_t intfl;

	intfl = MXC_UART_GetFlags(cfg->regs);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->cb) {
		data->cb(dev, data->cb_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	if (data->async.rx.timeout != SYS_FOREVER_US && data->async.rx.timeout != 0 &&
	    (intfl & ADI_MAX32_UART_INT_RX)) {
		k_work_reschedule(&data->async.rx.timeout_work, K_USEC(data->async.rx.timeout));
	}
#endif /* CONFIG_UART_ASYNC_API */

	/* Clear RX/TX interrupts flag after cb is called */
	MXC_UART_ClearFlags(cfg->regs, intfl);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

#if defined(CONFIG_UART_ASYNC_API)

static inline void async_timer_start(struct k_work_delayable *work, int32_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void async_user_callback(const struct device *dev, struct uart_event *evt)
{
	const struct max32_uart_data *data = dev->data;

	if (data->async.cb) {
		data->async.cb(dev, evt, data->async.user_data);
	}
}

static uint32_t load_tx_cache(const uint8_t *src, size_t len, uint8_t *dest)
{
	memcpy(dest, src, MIN(len, CONFIG_UART_TX_CACHE_LEN));

	return MIN(len, CONFIG_UART_TX_CACHE_LEN);
}

static void uart_max32_async_tx_callback(const struct device *dma_dev, void *user_data,
					 uint32_t channel, int status)
{
	const struct device *dev = user_data;
	const struct max32_uart_config *config = dev->config;
	struct max32_uart_data *data = dev->data;
	struct max32_uart_async_tx *tx = &data->async.tx;
	struct dma_status dma_stat;
	int ret;

	unsigned int key = irq_lock();

	dma_get_status(config->tx_dma.dev, config->tx_dma.channel, &dma_stat);
	/* Skip callback if channel is still busy */
	if (dma_stat.busy) {
		irq_unlock(key);
		return;
	}

	k_work_cancel_delayable(&tx->timeout_work);
	Wrap_MXC_UART_DisableTxDMA(config->regs);

	irq_unlock(key);

	tx->len -= tx->dma_blk.block_size;
	if (tx->len > 0) {
		tx->cache_id = !(tx->cache_id);
		ret = uart_max32_tx_dma_load(dev, tx->cache[tx->cache_id],
					     MIN(tx->len, CONFIG_UART_TX_CACHE_LEN));
		if (ret < 0) {
			LOG_ERR("Error configuring Tx DMA (%d)", ret);
			return;
		}

		ret = dma_start(config->tx_dma.dev, config->tx_dma.channel);
		if (ret < 0) {
			LOG_ERR("Error starting Tx DMA (%d)", ret);
			return;
		}

		async_timer_start(&tx->timeout_work, tx->timeout);

		Wrap_MXC_UART_SetTxDMALevel(config->regs, 2);
		Wrap_MXC_UART_EnableTxDMA(config->regs);

		/* Load next chunk as well */
		if (tx->len > CONFIG_UART_TX_CACHE_LEN) {
			tx->src += load_tx_cache(tx->src, tx->len - CONFIG_UART_TX_CACHE_LEN,
						 tx->cache[!(tx->cache_id)]);
		}
	} else {
		struct uart_event tx_done = {
			.type = status == 0 ? UART_TX_DONE : UART_TX_ABORTED,
			.data.tx.buf = tx->buf,
			.data.tx.len = tx->len,
		};
		async_user_callback(dev, &tx_done);
	}
}

static int uart_max32_tx_dma_load(const struct device *dev, uint8_t *buf, size_t len)
{
	int ret;
	const struct max32_uart_config *config = dev->config;
	struct max32_uart_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config *dma_blk = &data->async.tx.dma_blk;

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_callback = uart_max32_async_tx_callback;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_slot = config->tx_dma.slot;
	dma_cfg.block_count = 1;
	dma_cfg.source_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.head_block = dma_blk;
	dma_blk->block_size = len;
	dma_blk->source_address = (uint32_t)buf;

	ret = dma_config(config->tx_dma.dev, config->tx_dma.channel, &dma_cfg);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int api_callback_set(const struct device *dev, uart_callback_t callback, void *user_data)
{
	struct max32_uart_data *data = dev->data;

	data->async.cb = callback;
	data->async.user_data = user_data;

	return 0;
}

static int api_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	struct max32_uart_data *data = dev->data;
	const struct max32_uart_config *config = dev->config;
	struct dma_status dma_stat;
	int ret;
	bool use_cache = false;
	unsigned int key = irq_lock();

	if (config->tx_dma.channel == 0xFF) {
		LOG_ERR("Tx DMA channel is not configured");
		ret = -ENOTSUP;
		goto unlock;
	}

	ret = dma_get_status(config->tx_dma.dev, config->tx_dma.channel, &dma_stat);
	if (ret < 0 || dma_stat.busy) {
		LOG_ERR("DMA Tx %s", ret < 0 ? "error" : "busy");
		irq_unlock(key);
		return ret < 0 ? ret : -EBUSY;
	}

	data->async.tx.buf = buf;
	data->async.tx.len = len;
	data->async.tx.src = data->async.tx.buf;

	if (((uint32_t)buf < MXC_SRAM_MEM_BASE) ||
	    (((uint32_t)buf + len) > (MXC_SRAM_MEM_BASE + MXC_SRAM_MEM_SIZE))) {
		use_cache = true;
		len = load_tx_cache(data->async.tx.src, MIN(len, CONFIG_UART_TX_CACHE_LEN),
				    data->async.tx.cache[0]);
		data->async.tx.src += len;
		data->async.tx.cache_id = 0;
	}

	ret = uart_max32_tx_dma_load(dev, use_cache ? data->async.tx.cache[0] : ((uint8_t *)buf),
				     len);
	if (ret < 0) {
		LOG_ERR("Error configuring Tx DMA (%d)", ret);
		goto unlock;
	}

	ret = dma_start(config->tx_dma.dev, config->tx_dma.channel);
	if (ret < 0) {
		LOG_ERR("Error starting Tx DMA (%d)", ret);
		goto unlock;
	}

	data->async.tx.timeout = timeout;
	async_timer_start(&data->async.tx.timeout_work, timeout);

	Wrap_MXC_UART_SetTxDMALevel(config->regs, 2);
	Wrap_MXC_UART_EnableTxDMA(config->regs);

unlock:
	irq_unlock(key);

	return ret;
}

static int api_tx_abort(const struct device *dev)
{
	int ret;
	struct max32_uart_data *data = dev->data;
	const struct max32_uart_config *config = dev->config;
	struct dma_status dma_stat;
	size_t bytes_sent;

	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async.tx.timeout_work);

	Wrap_MXC_UART_DisableTxDMA(config->regs);

	ret = dma_get_status(config->tx_dma.dev, config->tx_dma.channel, &dma_stat);
	if (!dma_stat.busy) {
		irq_unlock(key);
		return 0;
	}

	bytes_sent = (ret == 0) ? (data->async.tx.len - dma_stat.pending_length) : 0;

	ret = dma_stop(config->tx_dma.dev, config->tx_dma.channel);

	irq_unlock(key);

	if (ret == 0) {
		struct uart_event tx_aborted = {
			.type = UART_TX_ABORTED,
			.data.tx.buf = data->async.tx.buf,
			.data.tx.len = bytes_sent,
		};
		async_user_callback(dev, &tx_aborted);
	}

	return 0;
}

static void uart_max32_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct max32_uart_async_tx *tx =
		CONTAINER_OF(dwork, struct max32_uart_async_tx, timeout_work);
	struct max32_uart_async_data *async = CONTAINER_OF(tx, struct max32_uart_async_data, tx);
	struct max32_uart_data *data = CONTAINER_OF(async, struct max32_uart_data, async);

	api_tx_abort(data->async.uart_dev);
}

static int api_rx_disable(const struct device *dev)
{
	struct max32_uart_data *data = dev->data;
	const struct max32_uart_config *config = dev->config;
	int ret;
	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async.rx.timeout_work);

	Wrap_MXC_UART_DisableRxDMA(config->regs);

	ret = dma_stop(config->rx_dma.dev, config->rx_dma.channel);
	if (ret) {
		LOG_ERR("Error stopping Rx DMA (%d)", ret);
		irq_unlock(key);
		return ret;
	}

	api_irq_rx_disable(dev);

	irq_unlock(key);

	/* Release current buffer event */
	struct uart_event rel_event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->async.rx.buf,
	};
	async_user_callback(dev, &rel_event);

	/* Disable RX event */
	struct uart_event rx_disabled = {.type = UART_RX_DISABLED};

	async_user_callback(dev, &rx_disabled);

	data->async.rx.buf = NULL;
	data->async.rx.len = 0;
	data->async.rx.counter = 0;
	data->async.rx.offset = 0;

	if (data->async.rx.next_buf) {
		/* Release next buffer event */
		struct uart_event next_rel_event = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->async.rx.next_buf,
		};
		async_user_callback(dev, &next_rel_event);
		data->async.rx.next_buf = NULL;
		data->async.rx.next_len = 0;
	}

	return 0;
}

static void uart_max32_async_rx_callback(const struct device *dma_dev, void *user_data,
					 uint32_t channel, int status)
{
	const struct device *dev = user_data;
	const struct max32_uart_config *config = dev->config;
	struct max32_uart_data *data = dev->data;
	struct max32_uart_async_data *async = &data->async;
	struct dma_status dma_stat;
	size_t total_rx;

	unsigned int key = irq_lock();

	dma_get_status(config->rx_dma.dev, config->rx_dma.channel, &dma_stat);

	if (dma_stat.pending_length > 0) {
		irq_unlock(key);
		return;
	}

	total_rx = async->rx.len - dma_stat.pending_length;

	api_irq_rx_disable(dev);

	irq_unlock(key);

	if (total_rx > async->rx.offset) {
		async->rx.counter = total_rx - async->rx.offset;
		struct uart_event rdy_event = {
			.type = UART_RX_RDY,
			.data.rx.buf = async->rx.buf,
			.data.rx.len = async->rx.counter,
			.data.rx.offset = async->rx.offset,
		};
		async_user_callback(dev, &rdy_event);
	}

	if (async->rx.next_buf) {
		async->rx.offset = 0;
		async->rx.counter = 0;

		struct uart_event rel_event = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = async->rx.buf,
		};
		async_user_callback(dev, &rel_event);

		async->rx.buf = async->rx.next_buf;
		async->rx.len = async->rx.next_len;

		async->rx.next_buf = NULL;
		async->rx.next_len = 0;
		struct uart_event req_event = {
			.type = UART_RX_BUF_REQUEST,
		};
		async_user_callback(dev, &req_event);

		dma_reload(config->rx_dma.dev, config->rx_dma.channel, config->rx_dma.slot,
			   (uint32_t)async->rx.buf, async->rx.len);
		dma_start(config->rx_dma.dev, config->rx_dma.channel);

		api_irq_rx_enable(dev);
		async_timer_start(&async->rx.timeout_work, async->rx.timeout);
	} else {
		api_rx_disable(dev);
	}
}

static int api_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	struct max32_uart_data *data = dev->data;
	const struct max32_uart_config *config = dev->config;
	struct dma_status dma_stat;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int ret;

	unsigned int key = irq_lock();

	if (config->rx_dma.channel == 0xFF) {
		LOG_ERR("Rx DMA channel is not configured");
		irq_unlock(key);
		return -ENOTSUP;
	}

	ret = dma_get_status(config->rx_dma.dev, config->rx_dma.channel, &dma_stat);
	if (ret < 0 || dma_stat.busy) {
		LOG_ERR("DMA Rx %s", ret < 0 ? "error" : "busy");
		irq_unlock(key);
		return ret < 0 ? ret : -EBUSY;
	}

	data->async.rx.buf = buf;
	data->async.rx.len = len;

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_callback = uart_max32_async_rx_callback;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_slot = config->rx_dma.slot;
	dma_cfg.block_count = 1;
	dma_cfg.source_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = len;
	dma_blk.dest_address = (uint32_t)buf;

	ret = dma_config(config->rx_dma.dev, config->rx_dma.channel, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("Error configuring Rx DMA (%d)", ret);
		irq_unlock(key);
		return ret;
	}

	ret = dma_start(config->rx_dma.dev, config->rx_dma.channel);
	if (ret < 0) {
		LOG_ERR("Error starting Rx DMA (%d)", ret);
		irq_unlock(key);
		return ret;
	}

	data->async.rx.timeout = timeout;

	Wrap_MXC_UART_SetRxDMALevel(config->regs, 1);
	Wrap_MXC_UART_EnableRxDMA(config->regs);

	struct uart_event buf_req = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(dev, &buf_req);

	api_irq_rx_enable(dev);
	async_timer_start(&data->async.rx.timeout_work, timeout);

	irq_unlock(key);
	return ret;
}

static int api_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct max32_uart_data *data = dev->data;

	data->async.rx.next_buf = buf;
	data->async.rx.next_len = len;

	return 0;
}

static void uart_max32_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct max32_uart_async_rx *rx =
		CONTAINER_OF(dwork, struct max32_uart_async_rx, timeout_work);
	struct max32_uart_async_data *async = CONTAINER_OF(rx, struct max32_uart_async_data, rx);
	struct max32_uart_data *data = CONTAINER_OF(async, struct max32_uart_data, async);
	const struct max32_uart_config *config = data->async.uart_dev->config;
	struct dma_status dma_stat;
	uint32_t total_rx;

	unsigned int key = irq_lock();

	dma_get_status(config->rx_dma.dev, config->rx_dma.channel, &dma_stat);

	api_irq_rx_disable(data->async.uart_dev);
	k_work_cancel_delayable(&data->async.rx.timeout_work);

	irq_unlock(key);

	total_rx = async->rx.len - dma_stat.pending_length;

	if (total_rx > async->rx.offset) {
		async->rx.counter = total_rx - async->rx.offset;
		struct uart_event rdy_event = {
			.type = UART_RX_RDY,
			.data.rx.buf = async->rx.buf,
			.data.rx.len = async->rx.counter,
			.data.rx.offset = async->rx.offset,
		};
		async_user_callback(async->uart_dev, &rdy_event);
	}
	async->rx.offset += async->rx.counter;
	async->rx.counter = 0;

	api_irq_rx_enable(data->async.uart_dev);
}

#endif

static DEVICE_API(uart, uart_max32_driver_api) = {
	.poll_in = api_poll_in,
	.poll_out = api_poll_out,
	.err_check = api_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = api_configure,
	.config_get = api_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = api_fifo_fill,
	.fifo_read = api_fifo_read,
	.irq_tx_enable = api_irq_tx_enable,
	.irq_tx_disable = api_irq_tx_disable,
	.irq_tx_ready = api_irq_tx_ready,
	.irq_rx_enable = api_irq_rx_enable,
	.irq_rx_disable = api_irq_rx_disable,
	.irq_tx_complete = api_irq_tx_complete,
	.irq_rx_ready = api_irq_rx_ready,
	.irq_err_enable = api_irq_err_enable,
	.irq_err_disable = api_irq_err_disable,
	.irq_is_pending = api_irq_is_pending,
	.irq_update = api_irq_update,
	.irq_callback_set = api_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = api_callback_set,
	.tx = api_tx,
	.tx_abort = api_tx_abort,
	.rx_enable = api_rx_enable,
	.rx_buf_rsp = api_rx_buf_rsp,
	.rx_disable = api_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

#ifdef CONFIG_UART_ASYNC_API
#define MAX32_DT_INST_DMA_CTLR(n, name)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),                                                \
		    (DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, name))), (NULL))

#define MAX32_DT_INST_DMA_CELL(n, name, cell)                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas), (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),    \
		    (0xff))

#define MAX32_UART_DMA_INIT(n)                                                                     \
	.tx_dma.dev = MAX32_DT_INST_DMA_CTLR(n, tx),                                               \
	.tx_dma.channel = MAX32_DT_INST_DMA_CELL(n, tx, channel),                                  \
	.tx_dma.slot = MAX32_DT_INST_DMA_CELL(n, tx, slot),                                        \
	.rx_dma.dev = MAX32_DT_INST_DMA_CTLR(n, rx),                                               \
	.rx_dma.channel = MAX32_DT_INST_DMA_CELL(n, rx, channel),                                  \
	.rx_dma.slot = MAX32_DT_INST_DMA_CELL(n, rx, slot),
#else
#define MAX32_UART_DMA_INIT(n)
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define MAX32_UART_USE_IRQ 1
#else
#define MAX32_UART_USE_IRQ 0
#endif

#define MAX32_UART_INIT(_num)                                                                      \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	IF_ENABLED(MAX32_UART_USE_IRQ,                                                             \
		   (static void uart_max32_irq_init_##_num(const struct device *dev)               \
		   {             \
			   IRQ_CONNECT(DT_INST_IRQN(_num), DT_INST_IRQ(_num, priority),            \
				       uart_max32_isr, DEVICE_DT_INST_GET(_num), 0);               \
			   irq_enable(DT_INST_IRQN(_num));                                         \
		   }));                                                                            \
	static const struct max32_uart_config max32_uart_config_##_num = {                         \
		.regs = (mxc_uart_regs_t *)DT_INST_REG_ADDR(_num),                                 \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		.perclk.clk_src =                                                                  \
			DT_INST_PROP_OR(_num, clock_source, ADI_MAX32_PRPH_CLK_SRC_PCLK),          \
		.uart_conf.baudrate = DT_INST_PROP(_num, current_speed),                           \
		.uart_conf.parity = DT_INST_ENUM_IDX(_num, parity),                                \
		.uart_conf.data_bits = DT_INST_ENUM_IDX(_num, data_bits),                          \
		.uart_conf.stop_bits = DT_INST_ENUM_IDX(_num, stop_bits),                          \
		.uart_conf.flow_ctrl =                                                             \
			DT_INST_PROP_OR(index, hw_flow_control, UART_CFG_FLOW_CTRL_NONE),          \
		MAX32_UART_DMA_INIT(_num) IF_ENABLED(                                              \
			MAX32_UART_USE_IRQ, (.irq_config_func = uart_max32_irq_init_##_num,))};    \
	static struct max32_uart_data max32_uart_data##_num = {                                    \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (.cb = NULL,))};                          \
	DEVICE_DT_INST_DEFINE(_num, uart_max32_init, NULL, &max32_uart_data##_num,                 \
			      &max32_uart_config_##_num, PRE_KERNEL_1,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, (void *)&uart_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX32_UART_INIT)
