/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_uart

#include <device.h>
#include <errno.h>
#include <init.h>
#include <sys/__assert.h>
#include <soc.h>
#include <drivers/uart.h>
#include <drivers/dma.h>
#include <string.h>

#ifndef SERCOM_USART_CTRLA_MODE_USART_INT_CLK
#define SERCOM_USART_CTRLA_MODE_USART_INT_CLK SERCOM_USART_CTRLA_MODE(0x1)
#endif

/* Device constant configuration parameters */
struct uart_sam0_dev_cfg {
	SercomUsart *regs;
	uint32_t baudrate;
	uint32_t pads;
#ifdef MCLK
	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint16_t gclk_core_id;
#else
	uint32_t pm_apbcmask;
	uint16_t gclk_clkctrl_id;
#endif
#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
	void (*irq_config_func)(const struct device *dev);
#endif
#if CONFIG_UART_ASYNC_API
	char *dma_dev;
	uint8_t tx_dma_request;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_request;
	uint8_t rx_dma_channel;
#endif
};

/* Device run time data */
struct uart_sam0_dev_data {
	struct uart_config config_cache;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
#endif
#if CONFIG_UART_ASYNC_API
	const struct device *dev;
	const struct uart_sam0_dev_cfg *cfg;
	const struct device *dma;

	uart_callback_t async_cb;
	void *async_cb_data;

	struct k_delayed_work tx_timeout_work;
	const uint8_t *tx_buf;
	size_t tx_len;

	struct k_delayed_work rx_timeout_work;
	size_t rx_timeout_time;
	size_t rx_timeout_chunk;
	uint32_t rx_timeout_start;
	uint8_t *rx_buf;
	size_t rx_len;
	size_t rx_processed_len;
	uint8_t *rx_next_buf;
	size_t rx_next_len;
	bool rx_waiting_for_irq;
	bool rx_timeout_from_isr;
#endif
};

#define DEV_CFG(dev) \
	((const struct uart_sam0_dev_cfg *const)(dev)->config)
#define DEV_DATA(dev) ((struct uart_sam0_dev_data * const)(dev)->data)

static void wait_synchronization(SercomUsart *const usart)
{
#if defined(SERCOM_USART_SYNCBUSY_MASK)
	/* SYNCBUSY is a register */
	while ((usart->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_MASK) != 0) {
	}
#elif defined(SERCOM_USART_STATUS_SYNCBUSY)
	/* SYNCBUSY is a bit */
	while ((usart->STATUS.reg & SERCOM_USART_STATUS_SYNCBUSY) != 0) {
	}
#else
#error Unsupported device
#endif
}

static int uart_sam0_set_baudrate(SercomUsart *const usart, uint32_t baudrate,
				  uint32_t clk_freq_hz)
{
	uint64_t tmp;
	uint16_t baud;

	tmp = (uint64_t)baudrate << 20;
	tmp = (tmp + (clk_freq_hz >> 1)) / clk_freq_hz;

	/* Verify that the calculated result is within range */
	if (tmp < 1 || tmp > UINT16_MAX) {
		return -ERANGE;
	}

	baud = 65536 - (uint16_t)tmp;
	usart->BAUD.reg = baud;
	wait_synchronization(usart);

	return 0;
}


#if CONFIG_UART_ASYNC_API

static void uart_sam0_dma_tx_done(const struct device *dma_dev, void *arg,
				  uint32_t id, int error_code)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	struct uart_sam0_dev_data *const dev_data =
		(struct uart_sam0_dev_data *const) arg;
	const struct device *dev = dev_data->dev;

	k_delayed_work_cancel(&dev_data->tx_timeout_work);

	int key = irq_lock();

	struct uart_event evt = {
		.type = UART_TX_DONE,
		.data.tx = {
			.buf = dev_data->tx_buf,
			.len = dev_data->tx_len,
		},
	};

	dev_data->tx_buf = NULL;
	dev_data->tx_len = 0U;

	if (evt.data.tx.len != 0U && dev_data->async_cb) {
		dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
	}

	irq_unlock(key);
}

static int uart_sam0_tx_halt(struct uart_sam0_dev_data *dev_data)
{
	const struct uart_sam0_dev_cfg *const cfg = dev_data->cfg;
	int key = irq_lock();
	size_t tx_active = dev_data->tx_len;
	struct dma_status st;

	struct uart_event evt = {
		.type = UART_TX_ABORTED,
		.data.tx = {
			.buf = dev_data->tx_buf,
			.len = 0U,
		},
	};

	dev_data->tx_buf = NULL;
	dev_data->tx_len = 0U;

	dma_stop(dev_data->dma, cfg->tx_dma_channel);

	irq_unlock(key);

	if (dma_get_status(dev_data->dma, cfg->tx_dma_channel, &st) == 0) {
		evt.data.tx.len = tx_active - st.pending_length;
	}

	if (tx_active) {
		if (dev_data->async_cb) {
			dev_data->async_cb(dev_data->dev,
					   &evt, dev_data->async_cb_data);
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static void uart_sam0_tx_timeout(struct k_work *work)
{
	struct uart_sam0_dev_data *dev_data = CONTAINER_OF(work,
							   struct uart_sam0_dev_data, tx_timeout_work);

	uart_sam0_tx_halt(dev_data);
}

static void uart_sam0_notify_rx_processed(struct uart_sam0_dev_data *dev_data,
					  size_t processed)
{
	if (!dev_data->async_cb) {
		return;
	}

	if (dev_data->rx_processed_len == processed) {
		return;
	}

	struct uart_event evt = {
		.type = UART_RX_RDY,
		.data.rx = {
			.buf = dev_data->rx_buf,
			.offset = dev_data->rx_processed_len,
			.len = processed - dev_data->rx_processed_len,
		},
	};

	dev_data->rx_processed_len = processed;

	dev_data->async_cb(dev_data->dev,
			   &evt, dev_data->async_cb_data);
}

static void uart_sam0_dma_rx_done(const struct device *dma_dev, void *arg,
				  uint32_t id, int error_code)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	struct uart_sam0_dev_data *const dev_data =
		(struct uart_sam0_dev_data *const)arg;
	const struct device *dev = dev_data->dev;
	const struct uart_sam0_dev_cfg *const cfg = dev_data->cfg;
	SercomUsart * const regs = cfg->regs;
	int key = irq_lock();

	if (dev_data->rx_len == 0U) {
		irq_unlock(key);
		return;
	}

	uart_sam0_notify_rx_processed(dev_data, dev_data->rx_len);

	if (dev_data->async_cb) {
		struct uart_event evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf = {
				.buf = dev_data->rx_buf,
			},
		};

		dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
	}

	/* No next buffer, so end the transfer */
	if (!dev_data->rx_next_len) {
		dev_data->rx_buf = NULL;
		dev_data->rx_len = 0U;

		if (dev_data->async_cb) {
			struct uart_event evt = {
				.type = UART_RX_DISABLED,
			};

			dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
		}

		irq_unlock(key);
		return;
	}

	dev_data->rx_buf = dev_data->rx_next_buf;
	dev_data->rx_len = dev_data->rx_next_len;
	dev_data->rx_next_buf = NULL;
	dev_data->rx_next_len = 0U;
	dev_data->rx_processed_len = 0U;

	dma_reload(dev_data->dma, cfg->rx_dma_channel,
		   (uint32_t)(&(regs->DATA.reg)),
		   (uint32_t)dev_data->rx_buf, dev_data->rx_len);

	/*
	 * If there should be a timeout, handle starting the DMA in the
	 * ISR, since reception resets it and DMA completion implies
	 * reception.  This also catches the case of DMA completion during
	 * timeout handling.
	 */
	if (dev_data->rx_timeout_time != SYS_FOREVER_MS) {
		dev_data->rx_waiting_for_irq = true;
		regs->INTENSET.reg = SERCOM_USART_INTENSET_RXC;
		irq_unlock(key);
		return;
	}

	/* Otherwise, start the transfer immediately. */
	dma_start(dev_data->dma, cfg->rx_dma_channel);

	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	dev_data->async_cb(dev, &evt, dev_data->async_cb_data);

	irq_unlock(key);
}

static void uart_sam0_rx_timeout(struct k_work *work)
{
	struct uart_sam0_dev_data *dev_data = CONTAINER_OF(work,
							   struct uart_sam0_dev_data, rx_timeout_work);
	const struct uart_sam0_dev_cfg *const cfg = dev_data->cfg;
	SercomUsart * const regs = cfg->regs;
	struct dma_status st;
	int key = irq_lock();

	if (dev_data->rx_len == 0U) {
		irq_unlock(key);
		return;
	}

	/*
	 * Stop the DMA transfer and restart the interrupt read
	 * component (so the timeout restarts if there's still data).
	 * However, just ignore it if the transfer has completed (nothing
	 * pending) that means the DMA ISR is already pending, so just let
	 * it handle things instead when we re-enable IRQs.
	 */
	dma_stop(dev_data->dma, cfg->rx_dma_channel);
	if (dma_get_status(dev_data->dma, cfg->rx_dma_channel,
			   &st) == 0 && st.pending_length == 0U) {
		irq_unlock(key);
		return;
	}

	uint8_t *rx_dma_start = dev_data->rx_buf + dev_data->rx_len -
			     st.pending_length;
	size_t rx_processed = rx_dma_start - dev_data->rx_buf;

	/*
	 * We know we still have space, since the above will catch the
	 * empty buffer, so always restart the transfer.
	 */
	dma_reload(dev_data->dma, cfg->rx_dma_channel,
		   (uint32_t)(&(regs->DATA.reg)),
		   (uint32_t)rx_dma_start,
		   dev_data->rx_len - rx_processed);

	dev_data->rx_waiting_for_irq = true;
	regs->INTENSET.reg = SERCOM_USART_INTENSET_RXC;

	/*
	 * Never do a notify on a timeout started from the ISR: timing
	 * granularity means the first timeout can be in the middle
	 * of reception but still have the total elapsed time exhausted.
	 * So we require a timeout chunk with no data seen at all
	 * (i.e. no ISR entry).
	 */
	if (dev_data->rx_timeout_from_isr) {
		dev_data->rx_timeout_from_isr = false;
		k_delayed_work_submit(&dev_data->rx_timeout_work,
				      K_MSEC(dev_data->rx_timeout_chunk));
		irq_unlock(key);
		return;
	}

	uint32_t now = k_uptime_get_32();
	uint32_t elapsed = now - dev_data->rx_timeout_start;

	if (elapsed >= dev_data->rx_timeout_time) {
		/*
		 * No time left, so call the handler, and let the ISR
		 * restart the timeout when it sees data.
		 */
		uart_sam0_notify_rx_processed(dev_data, rx_processed);
	} else {
		/*
		 * Still have time left, so start another timeout.
		 */
		uint32_t remaining = MIN(dev_data->rx_timeout_time - elapsed,
				      dev_data->rx_timeout_chunk);

		k_delayed_work_submit(&dev_data->rx_timeout_work,
				      K_MSEC(remaining));
	}

	irq_unlock(key);
}

#endif

static int uart_sam0_configure(const struct device *dev,
			       const struct uart_config *new_cfg)
{
	int retval;

	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
	SercomUsart * const usart = cfg->regs;

	wait_synchronization(usart);

	usart->CTRLA.bit.ENABLE = 0;
	wait_synchronization(usart);

	if (new_cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		/* Flow control not yet supported though in principle possible
		 * on this soc family.
		 */
		return -ENOTSUP;
	}

	dev_data->config_cache.flow_ctrl = new_cfg->flow_ctrl;

	SERCOM_USART_CTRLA_Type CTRLA_temp = usart->CTRLA;
	SERCOM_USART_CTRLB_Type CTRLB_temp = usart->CTRLB;

	switch (new_cfg->parity) {
	case UART_CFG_PARITY_NONE:
		CTRLA_temp.bit.FORM = 0x0;
		break;
	case UART_CFG_PARITY_ODD:
		CTRLA_temp.bit.FORM = 0x1;
		CTRLB_temp.bit.PMODE = 1;
		break;
	case UART_CFG_PARITY_EVEN:
		CTRLA_temp.bit.FORM = 0x1;
		CTRLB_temp.bit.PMODE = 0;
		break;
	default:
		return -ENOTSUP;
	}

	dev_data->config_cache.parity = new_cfg->parity;

	switch (new_cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		CTRLB_temp.bit.SBMODE = 0;
		break;
	case UART_CFG_STOP_BITS_2:
		CTRLB_temp.bit.SBMODE = 1;
		break;
	default:
		return -ENOTSUP;
	}

	dev_data->config_cache.stop_bits = new_cfg->stop_bits;

	switch (new_cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		CTRLB_temp.bit.CHSIZE = 0x5;
		break;
	case UART_CFG_DATA_BITS_6:
		CTRLB_temp.bit.CHSIZE = 0x6;
		break;
	case UART_CFG_DATA_BITS_7:
		CTRLB_temp.bit.CHSIZE = 0x7;
		break;
	case UART_CFG_DATA_BITS_8:
		CTRLB_temp.bit.CHSIZE = 0x0;
		break;
	case UART_CFG_DATA_BITS_9:
		CTRLB_temp.bit.CHSIZE = 0x1;
		break;
	default:
		return -ENOTSUP;
	}

	dev_data->config_cache.data_bits = new_cfg->data_bits;

	usart->CTRLA = CTRLA_temp;
	wait_synchronization(usart);

	usart->CTRLB = CTRLB_temp;
	wait_synchronization(usart);

	retval = uart_sam0_set_baudrate(usart, new_cfg->baudrate,
					SOC_ATMEL_SAM0_GCLK0_FREQ_HZ);
	if (retval != 0) {
		return retval;
	}

	dev_data->config_cache.baudrate = new_cfg->baudrate;

	usart->CTRLA.bit.ENABLE = 1;
	wait_synchronization(usart);

	return 0;
}

static int uart_sam0_config_get(const struct device *dev,
				struct uart_config *out_cfg)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
	memcpy(out_cfg, &(dev_data->config_cache),
				sizeof(dev_data->config_cache));

	return 0;
}

static int uart_sam0_init(const struct device *dev)
{
	int retval;
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

	SercomUsart *const usart = cfg->regs;

#ifdef MCLK
	/* Enable the GCLK */
	GCLK->PCHCTRL[cfg->gclk_core_id].reg = GCLK_PCHCTRL_GEN_GCLK0 |
					       GCLK_PCHCTRL_CHEN;

	/* Enable SERCOM clock in MCLK */
	*cfg->mclk |= cfg->mclk_mask;
#else
	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = cfg->gclk_clkctrl_id | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	/* Enable SERCOM clock in PM */
	PM->APBCMASK.reg |= cfg->pm_apbcmask;
#endif

	/* Disable all USART interrupts */
	usart->INTENCLR.reg = SERCOM_USART_INTENCLR_MASK;
	wait_synchronization(usart);

	/* 8 bits of data, no parity, 1 stop bit in normal mode */
	usart->CTRLA.reg =
	    cfg->pads
	    /* Internal clock */
	    | SERCOM_USART_CTRLA_MODE_USART_INT_CLK
#if defined(SERCOM_USART_CTRLA_SAMPR)
	    /* 16x oversampling with arithmetic baud rate generation */
	    | SERCOM_USART_CTRLA_SAMPR(0)
#endif
	    | SERCOM_USART_CTRLA_FORM(0) |
	    SERCOM_USART_CTRLA_CPOL | SERCOM_USART_CTRLA_DORD;
	wait_synchronization(usart);

	dev_data->config_cache.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	dev_data->config_cache.parity = UART_CFG_PARITY_NONE;
	dev_data->config_cache.stop_bits = UART_CFG_STOP_BITS_1;
	dev_data->config_cache.data_bits = UART_CFG_DATA_BITS_8;

	/* Enable receiver and transmitter */
	usart->CTRLB.reg = SERCOM_USART_CTRLB_CHSIZE(0) |
			   SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
	wait_synchronization(usart);

	retval = uart_sam0_set_baudrate(usart, cfg->baudrate,
					SOC_ATMEL_SAM0_GCLK0_FREQ_HZ);
	if (retval != 0) {
		return retval;
	}
	dev_data->config_cache.data_bits = cfg->baudrate;

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
	cfg->irq_config_func(dev);
#endif

#ifdef CONFIG_UART_ASYNC_API
	dev_data->dev = dev;
	dev_data->cfg = cfg;
	dev_data->dma = device_get_binding(cfg->dma_dev);

	k_delayed_work_init(&dev_data->tx_timeout_work, uart_sam0_tx_timeout);
	k_delayed_work_init(&dev_data->rx_timeout_work, uart_sam0_rx_timeout);

	if (cfg->tx_dma_channel != 0xFFU) {
		struct dma_config dma_cfg = { 0 };
		struct dma_block_config dma_blk = { 0 };

		if (!dev_data->dma) {
			return -ENOTSUP;
		}

		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_cfg.source_data_size = 1;
		dma_cfg.dest_data_size = 1;
		dma_cfg.user_data = dev_data;
		dma_cfg.dma_callback = uart_sam0_dma_tx_done;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_blk;
		dma_cfg.dma_slot = cfg->tx_dma_request;

		dma_blk.block_size = 1;
		dma_blk.dest_address = (uint32_t)(&(usart->DATA.reg));
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		retval = dma_config(dev_data->dma, cfg->tx_dma_channel,
				    &dma_cfg);
		if (retval != 0) {
			return retval;
		}
	}

	if (cfg->rx_dma_channel != 0xFFU) {
		struct dma_config dma_cfg = { 0 };
		struct dma_block_config dma_blk = { 0 };

		if (!dev_data->dma) {
			return -ENOTSUP;
		}

		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_cfg.source_data_size = 1;
		dma_cfg.dest_data_size = 1;
		dma_cfg.user_data = dev_data;
		dma_cfg.dma_callback = uart_sam0_dma_rx_done;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_blk;
		dma_cfg.dma_slot = cfg->rx_dma_request;

		dma_blk.block_size = 1;
		dma_blk.source_address = (uint32_t)(&(usart->DATA.reg));
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		retval = dma_config(dev_data->dma, cfg->rx_dma_channel,
				    &dma_cfg);
		if (retval != 0) {
			return retval;
		}
	}

#endif

	usart->CTRLA.bit.ENABLE = 1;
	wait_synchronization(usart);

	return 0;
}

static int uart_sam0_poll_in(const struct device *dev, unsigned char *c)
{
	SercomUsart *const usart = DEV_CFG(dev)->regs;

	if (!usart->INTFLAG.bit.RXC) {
		return -EBUSY;
	}

	*c = (unsigned char)usart->DATA.reg;
	return 0;
}

static void uart_sam0_poll_out(const struct device *dev, unsigned char c)
{
	SercomUsart *const usart = DEV_CFG(dev)->regs;

	while (!usart->INTFLAG.bit.DRE) {
	}

	/* send a character */
	usart->DATA.reg = c;
}

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API

static void uart_sam0_isr(const struct device *dev)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

#if CONFIG_UART_INTERRUPT_DRIVEN
	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}
#endif

#if CONFIG_UART_ASYNC_API
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);
	SercomUsart * const regs = cfg->regs;

	if (dev_data->rx_len && regs->INTFLAG.bit.RXC &&
	    dev_data->rx_waiting_for_irq) {
		dev_data->rx_waiting_for_irq = false;
		regs->INTENCLR.reg = SERCOM_USART_INTENCLR_RXC;

		/* Receive started, so request the next buffer */
		if (dev_data->rx_next_len == 0U && dev_data->async_cb) {
			struct uart_event evt = {
				.type = UART_RX_BUF_REQUEST,
			};

			dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
		}

		/*
		 * If we have a timeout, restart the time remaining whenever
		 * we see data.
		 */
		if (dev_data->rx_timeout_time != SYS_FOREVER_MS) {
			dev_data->rx_timeout_from_isr = true;
			dev_data->rx_timeout_start = k_uptime_get_32();
			k_delayed_work_submit(&dev_data->rx_timeout_work,
					      K_MSEC(dev_data->rx_timeout_chunk));
		}

		/* DMA will read the currently ready byte out */
		dma_start(dev_data->dma, cfg->rx_dma_channel);
	}
#endif
}

#endif

#if CONFIG_UART_INTERRUPT_DRIVEN

static int uart_sam0_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data, int len)
{
	SercomUsart *regs = DEV_CFG(dev)->regs;

	if (regs->INTFLAG.bit.DRE && len >= 1) {
		regs->DATA.reg = tx_data[0];
		return 1;
	} else {
		return 0;
	}
}

static void uart_sam0_irq_tx_enable(const struct device *dev)
{
	SercomUsart *regs = DEV_CFG(dev)->regs;

	regs->INTENSET.reg = SERCOM_USART_INTENCLR_DRE;
}

static void uart_sam0_irq_tx_disable(const struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
}

static int uart_sam0_irq_tx_ready(const struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return regs->INTFLAG.bit.DRE != 0;
}

static void uart_sam0_irq_rx_enable(const struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENSET.reg = SERCOM_USART_INTENSET_RXC;
}

static void uart_sam0_irq_rx_disable(const struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENCLR.reg = SERCOM_USART_INTENCLR_RXC;
}

static int uart_sam0_irq_rx_ready(const struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return regs->INTFLAG.bit.RXC != 0;
}

static int uart_sam0_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int size)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	if (regs->INTFLAG.bit.RXC) {
		uint8_t ch = regs->DATA.reg;

		if (size >= 1) {
			*rx_data = ch;
			return 1;
		} else {
			return -EINVAL;
		}
	}
	return 0;
}

static int uart_sam0_irq_is_pending(const struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return (regs->INTENSET.reg & regs->INTFLAG.reg) != 0;
}

static int uart_sam0_irq_update(const struct device *dev) { return 1; }

static void uart_sam0_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}
#endif

#ifdef CONFIG_UART_ASYNC_API

static int uart_sam0_callback_set(const struct device *dev,
				  uart_callback_t callback,
				  void *user_data)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

	dev_data->async_cb = callback;
	dev_data->async_cb_data = user_data;

	return 0;
}

static int uart_sam0_tx(const struct device *dev, const uint8_t *buf,
			size_t len,
			int32_t timeout)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);
	SercomUsart *regs = DEV_CFG(dev)->regs;
	int retval;

	if (!dev_data->dma || cfg->tx_dma_channel == 0xFFU) {
		return -ENOTSUP;
	}

	if (len > 0xFFFFU) {
		return -EINVAL;
	}

	int key = irq_lock();

	if (dev_data->tx_len != 0U) {
		retval = -EBUSY;
		goto err;
	}

	dev_data->tx_buf = buf;
	dev_data->tx_len = len;

	irq_unlock(key);

	retval = dma_reload(dev_data->dma, cfg->tx_dma_channel, (uint32_t)buf,
			    (uint32_t)(&(regs->DATA.reg)), len);
	if (retval != 0U) {
		return retval;
	}

	if (timeout != SYS_FOREVER_MS) {
		k_delayed_work_submit(&dev_data->tx_timeout_work,
				      K_MSEC(timeout));
	}

	return dma_start(dev_data->dma, cfg->tx_dma_channel);
err:
	irq_unlock(key);
	return retval;
}

static int uart_sam0_tx_abort(const struct device *dev)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);

	if (!dev_data->dma || cfg->tx_dma_channel == 0xFFU) {
		return -ENOTSUP;
	}

	k_delayed_work_cancel(&dev_data->tx_timeout_work);

	return uart_sam0_tx_halt(dev_data);
}

static int uart_sam0_rx_enable(const struct device *dev, uint8_t *buf,
			       size_t len,
			       int32_t timeout)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);
	SercomUsart *regs = DEV_CFG(dev)->regs;
	int retval;

	if (!dev_data->dma || cfg->rx_dma_channel == 0xFFU) {
		return -ENOTSUP;
	}

	if (len > 0xFFFFU) {
		return -EINVAL;
	}

	int key = irq_lock();

	if (dev_data->rx_len != 0U) {
		retval = -EBUSY;
		goto err;
	}

	/* Read off anything that was already there */
	while (regs->INTFLAG.bit.RXC) {
		char discard = regs->DATA.reg;

		(void)discard;
	}

	retval = dma_reload(dev_data->dma, cfg->rx_dma_channel,
			    (uint32_t)(&(regs->DATA.reg)),
			    (uint32_t)buf, len);
	if (retval != 0) {
		return retval;
	}

	dev_data->rx_buf = buf;
	dev_data->rx_len = len;
	dev_data->rx_processed_len = 0U;
	dev_data->rx_waiting_for_irq = true;
	dev_data->rx_timeout_from_isr = true;
	dev_data->rx_timeout_time = timeout;
	dev_data->rx_timeout_chunk = MAX(timeout / 4U, 1);

	regs->INTENSET.reg = SERCOM_USART_INTENSET_RXC;

	irq_unlock(key);
	return 0;

err:
	irq_unlock(key);
	return retval;
}

static int uart_sam0_rx_buf_rsp(const struct device *dev, uint8_t *buf,
				size_t len)
{
	if (len > 0xFFFFU) {
		return -EINVAL;
	}

	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
	int key = irq_lock();
	int retval = 0;

	if (dev_data->rx_len == 0U) {
		retval = -EACCES;
		goto err;
	}

	if (dev_data->rx_next_len != 0U) {
		retval = -EBUSY;
		goto err;
	}

	dev_data->rx_next_buf = buf;
	dev_data->rx_next_len = len;

	irq_unlock(key);
	return 0;

err:
	irq_unlock(key);
	return retval;
}

static int uart_sam0_rx_disable(const struct device *dev)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);
	SercomUsart *const regs = cfg->regs;
	struct dma_status st;

	k_delayed_work_cancel(&dev_data->rx_timeout_work);

	int key = irq_lock();

	if (dev_data->rx_len == 0U) {
		irq_unlock(key);
		return -EINVAL;
	}

	regs->INTENCLR.reg = SERCOM_USART_INTENCLR_RXC;
	dma_stop(dev_data->dma, cfg->rx_dma_channel);


	if (dma_get_status(dev_data->dma, cfg->rx_dma_channel,
			   &st) == 0 && st.pending_length != 0U) {
		size_t rx_processed = dev_data->rx_len - st.pending_length;

		uart_sam0_notify_rx_processed(dev_data, rx_processed);
	}

	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf = {
			.buf = dev_data->rx_buf,
		},
	};

	dev_data->rx_buf = NULL;
	dev_data->rx_len = 0U;

	if (dev_data->async_cb) {
		dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
	}

	if (dev_data->rx_next_len) {
		struct uart_event evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf = {
				.buf = dev_data->rx_next_buf,
			},
		};

		dev_data->rx_next_buf = NULL;
		dev_data->rx_next_len = 0U;

		if (dev_data->async_cb) {
			dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
		}
	}

	evt.type = UART_RX_DISABLED;
	if (dev_data->async_cb) {
		dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
	}

	irq_unlock(key);

	return 0;
}

#endif

static const struct uart_driver_api uart_sam0_driver_api = {
	.poll_in = uart_sam0_poll_in,
	.poll_out = uart_sam0_poll_out,
	.configure = uart_sam0_configure,
	.config_get = uart_sam0_config_get,
#if CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_sam0_fifo_fill,
	.fifo_read = uart_sam0_fifo_read,
	.irq_tx_enable = uart_sam0_irq_tx_enable,
	.irq_tx_disable = uart_sam0_irq_tx_disable,
	.irq_tx_ready = uart_sam0_irq_tx_ready,
	.irq_rx_enable = uart_sam0_irq_rx_enable,
	.irq_rx_disable = uart_sam0_irq_rx_disable,
	.irq_rx_ready = uart_sam0_irq_rx_ready,
	.irq_is_pending = uart_sam0_irq_is_pending,
	.irq_update = uart_sam0_irq_update,
	.irq_callback_set = uart_sam0_irq_callback_set,
#endif
#if CONFIG_UART_ASYNC_API
	.callback_set = uart_sam0_callback_set,
	.tx = uart_sam0_tx,
	.tx_abort = uart_sam0_tx_abort,
	.rx_enable = uart_sam0_rx_enable,
	.rx_buf_rsp = uart_sam0_rx_buf_rsp,
	.rx_disable = uart_sam0_rx_disable,
#endif
};

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API

#define SAM0_UART_IRQ_CONNECT(n, m)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq),		\
			    DT_INST_IRQ_BY_IDX(n, m, priority),		\
			    uart_sam0_isr,				\
			    DEVICE_GET(uart_sam0_##n), 0);		\
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));		\
	} while (0)

#define UART_SAM0_IRQ_HANDLER_DECL(n)					\
	static void uart_sam0_irq_config_##n(const struct device *dev)
#define UART_SAM0_IRQ_HANDLER_FUNC(n)					\
	.irq_config_func = uart_sam0_irq_config_##n,

#if DT_INST_IRQ_HAS_IDX(0, 3)
#define UART_SAM0_IRQ_HANDLER(n)					\
static void uart_sam0_irq_config_##n(const struct device *dev)		\
{									\
	SAM0_UART_IRQ_CONNECT(n, 0);					\
	SAM0_UART_IRQ_CONNECT(n, 1);					\
	SAM0_UART_IRQ_CONNECT(n, 2);					\
	SAM0_UART_IRQ_CONNECT(n, 3);					\
}
#else
#define UART_SAM0_IRQ_HANDLER(n)					\
static void uart_sam0_irq_config_##n(const struct device *dev)		\
{									\
	SAM0_UART_IRQ_CONNECT(n, 0);					\
}
#endif
#else
#define UART_SAM0_IRQ_HANDLER_DECL(n)
#define UART_SAM0_IRQ_HANDLER_FUNC(n)
#define UART_SAM0_IRQ_HANDLER(n)
#endif

#if CONFIG_UART_ASYNC_API
#define UART_SAM0_DMA_CHANNELS(n)					\
	.dma_dev = ATMEL_SAM0_DT_INST_DMA_NAME(n, tx),			\
	.tx_dma_request = ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, tx),	\
	.tx_dma_channel = ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, tx),	\
	.rx_dma_request = ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, rx),	\
	.rx_dma_channel = ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, rx),
#else
#define UART_SAM0_DMA_CHANNELS(n)
#endif

#define UART_SAM0_SERCOM_PADS(n) \
	(DT_INST_PROP(n, rxpo) << SERCOM_USART_CTRLA_RXPO_Pos) |	\
	(DT_INST_PROP(n, txpo) << SERCOM_USART_CTRLA_TXPO_Pos)

#ifdef MCLK
#define UART_SAM0_CONFIG_DEFN(n)					\
static const struct uart_sam0_dev_cfg uart_sam0_config_##n = {		\
	.regs = (SercomUsart *)DT_INST_REG_ADDR(n),			\
	.baudrate = DT_INST_PROP(n, current_speed),			\
	.mclk = (volatile uint32_t *)MCLK_MASK_DT_INT_REG_ADDR(n),	\
	.mclk_mask = BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, bit)),	\
	.gclk_core_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, periph_ch),\
	.pads = UART_SAM0_SERCOM_PADS(n),				\
	UART_SAM0_IRQ_HANDLER_FUNC(n)					\
	UART_SAM0_DMA_CHANNELS(n)					\
}
#else
#define UART_SAM0_CONFIG_DEFN(n)					\
static const struct uart_sam0_dev_cfg uart_sam0_config_##n = {		\
	.regs = (SercomUsart *)DT_INST_REG_ADDR(n),			\
	.baudrate = DT_INST_PROP(n, current_speed),			\
	.pm_apbcmask = BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, pm, bit)),	\
	.gclk_clkctrl_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, clkctrl_id),\
	.pads = UART_SAM0_SERCOM_PADS(n),				\
	UART_SAM0_IRQ_HANDLER_FUNC(n)					\
	UART_SAM0_DMA_CHANNELS(n)					\
}
#endif

#define UART_SAM0_DEVICE_INIT(n)					\
static struct uart_sam0_dev_data uart_sam0_data_##n;			\
UART_SAM0_IRQ_HANDLER_DECL(n);						\
UART_SAM0_CONFIG_DEFN(n);						\
DEVICE_AND_API_INIT(uart_sam0_##n, DT_INST_LABEL(n),			\
		    uart_sam0_init, &uart_sam0_data_##n,		\
		    &uart_sam0_config_##n, PRE_KERNEL_1,		\
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		    &uart_sam0_driver_api);				\
UART_SAM0_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(UART_SAM0_DEVICE_INIT)
