/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <init.h>
#include <sys/__assert.h>
#include <soc.h>
#include <drivers/uart.h>
#include <drivers/dma.h>

/* Device constant configuration parameters */
struct uart_sam0_dev_cfg {
	SercomUsart *regs;
	u32_t baudrate;
	u32_t pads;
	u32_t pm_apbcmask;
	u16_t gclk_clkctrl_id;
#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
	void (*irq_config_func)(struct device *dev);
#endif
#if CONFIG_UART_ASYNC_API
	u8_t tx_dma_request;
	u8_t tx_dma_channel;
	u8_t rx_dma_request;
	u8_t rx_dma_channel;
#endif
};

/* Device run time data */
struct uart_sam0_dev_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
#endif
#if CONFIG_UART_ASYNC_API
	const struct uart_sam0_dev_cfg *cfg;
	struct device *dma;

	uart_callback_t async_cb;
	void *async_cb_data;

	struct k_delayed_work tx_timeout_work;
	const u8_t *tx_buf;
	size_t tx_len;

	struct k_delayed_work rx_timeout_work;
	size_t rx_timeout_time;
	size_t rx_timeout_chunk;
	u32_t rx_timeout_start;
	u8_t *rx_buf;
	size_t rx_len;
	size_t rx_processed_len;
	u8_t *rx_next_buf;
	size_t rx_next_len;
	bool rx_waiting_for_irq;
	bool rx_timeout_from_isr;
#endif
};

#define DEV_CFG(dev) \
	((const struct uart_sam0_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) ((struct uart_sam0_dev_data * const)(dev)->driver_data)

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

static int uart_sam0_set_baudrate(SercomUsart *const usart, u32_t baudrate,
				  u32_t clk_freq_hz)
{
	u64_t tmp;
	u16_t baud;

	tmp = (u64_t)baudrate << 20;
	tmp = (tmp + (clk_freq_hz >> 1)) / clk_freq_hz;

	/* Verify that the calculated result is within range */
	if (tmp < 1 || tmp > UINT16_MAX) {
		return -ERANGE;
	}

	baud = 65536 - (u16_t)tmp;
	usart->BAUD.reg = baud;
	wait_synchronization(usart);

	return 0;
}

#if CONFIG_UART_ASYNC_API

static void uart_sam0_dma_tx_done(void *arg, u32_t id, int error_code)
{
	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	struct device *dev = arg;
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

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
		dev_data->async_cb(&evt, dev_data->async_cb_data);
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
			dev_data->async_cb(&evt, dev_data->async_cb_data);
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

	dev_data->async_cb(&evt, dev_data->async_cb_data);
}

static void uart_sam0_dma_rx_done(void *arg, u32_t id, int error_code)
{
	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	struct device *dev = arg;
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
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

		dev_data->async_cb(&evt, dev_data->async_cb_data);
	}

	/* No next buffer, so end the transfer */
	if (!dev_data->rx_next_len) {
		dev_data->rx_buf = NULL;
		dev_data->rx_len = 0U;

		if (dev_data->async_cb) {
			struct uart_event evt = {
				.type = UART_RX_DISABLED,
			};

			dev_data->async_cb(&evt, dev_data->async_cb_data);
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
		   (u32_t)(&(regs->DATA.reg)),
		   (u32_t)dev_data->rx_buf, dev_data->rx_len);

	/*
	 * If there should be a timeout, handle starting the DMA in the
	 * ISR, since reception resets it and DMA completion implies
	 * reception.  This also catches the case of DMA completion during
	 * timeout handling.
	 */
	if (dev_data->rx_timeout_time != K_FOREVER) {
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

	dev_data->async_cb(&evt, dev_data->async_cb_data);

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

	u8_t *rx_dma_start = dev_data->rx_buf + dev_data->rx_len -
			     st.pending_length;
	size_t rx_processed = rx_dma_start - dev_data->rx_buf;

	/*
	 * We know we still have space, since the above will catch the
	 * empty buffer, so always restart the transfer.
	 */
	dma_reload(dev_data->dma, cfg->rx_dma_channel,
		   (u32_t)(&(regs->DATA.reg)),
		   (u32_t)rx_dma_start,
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
				      dev_data->rx_timeout_chunk);
		irq_unlock(key);
		return;
	}

	u32_t now = k_uptime_get_32();
	u32_t elapsed = now - dev_data->rx_timeout_start;

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
		u32_t remaining = MIN(dev_data->rx_timeout_time - elapsed,
				      dev_data->rx_timeout_chunk);

		k_delayed_work_submit(&dev_data->rx_timeout_work, remaining);
	}

	irq_unlock(key);
}

#endif

static int uart_sam0_init(struct device *dev)
{
	int retval;
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);
	SercomUsart *const usart = cfg->regs;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg =
	    cfg->gclk_clkctrl_id | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;

	/* Enable SERCOM clock in PM */
	PM->APBCMASK.reg |= cfg->pm_apbcmask;

	/* Disable all USART interrupts */
	usart->INTENCLR.reg = SERCOM_USART_INTENCLR_MASK;
	wait_synchronization(usart);

	/* 8 bits of data, no parity, 1 stop bit in normal mode */
	usart->CTRLA.reg =
	    cfg->pads |
	    /* Internal clock */
	    SERCOM_USART_CTRLA_MODE_USART_INT_CLK
#if defined(SERCOM_USART_CTRLA_SAMPR)
	    /* 16x oversampling with arithmetic baud rate generation */
	    | SERCOM_USART_CTRLA_SAMPR(0)
#endif
	    | SERCOM_USART_CTRLA_FORM(0) |
	    SERCOM_USART_CTRLA_CPOL | SERCOM_USART_CTRLA_DORD;
	wait_synchronization(usart);

	/* Enable receiver and transmitter */
	usart->CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(0) |
			   SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
	wait_synchronization(usart);

	retval = uart_sam0_set_baudrate(usart, cfg->baudrate,
					SOC_ATMEL_SAM0_GCLK0_FREQ_HZ);
	if (retval != 0) {
		return retval;
	}

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
	cfg->irq_config_func(dev);
#endif

#ifdef CONFIG_UART_ASYNC_API

	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

	dev_data->cfg = cfg;
	dev_data->dma = device_get_binding(CONFIG_DMA_0_NAME);

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
		dma_cfg.callback_arg = dev;
		dma_cfg.dma_callback = uart_sam0_dma_tx_done;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_blk;
		dma_cfg.dma_slot = cfg->tx_dma_request;

		dma_blk.block_size = 1;
		dma_blk.dest_address = (u32_t)(&(usart->DATA.reg));
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
		dma_cfg.callback_arg = dev;
		dma_cfg.dma_callback = uart_sam0_dma_rx_done;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_blk;
		dma_cfg.dma_slot = cfg->rx_dma_request;

		dma_blk.block_size = 1;
		dma_blk.source_address = (u32_t)(&(usart->DATA.reg));
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

static int uart_sam0_poll_in(struct device *dev, unsigned char *c)
{
	SercomUsart *const usart = DEV_CFG(dev)->regs;

	if (!usart->INTFLAG.bit.RXC) {
		return -EBUSY;
	}

	*c = (unsigned char)usart->DATA.reg;
	return 0;
}

static void uart_sam0_poll_out(struct device *dev, unsigned char c)
{
	SercomUsart *const usart = DEV_CFG(dev)->regs;

	while (!usart->INTFLAG.bit.DRE) {
	}

	/* send a character */
	usart->DATA.reg = c;
}

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API

static void uart_sam0_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

#if CONFIG_UART_INTERRUPT_DRIVEN
	if (dev_data->cb) {
		dev_data->cb(dev_data->cb_data);
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

			dev_data->async_cb(&evt, dev_data->async_cb_data);
		}

		/*
		 * If we have a timeout, restart the time remaining whenever
		 * we see data.
		 */
		if (dev_data->rx_timeout_time != K_FOREVER) {
			dev_data->rx_timeout_from_isr = true;
			dev_data->rx_timeout_start = k_uptime_get_32();
			k_delayed_work_submit(&dev_data->rx_timeout_work,
					      dev_data->rx_timeout_chunk);
		}

		/* DMA will read the currently ready byte out */
		dma_start(dev_data->dma, cfg->rx_dma_channel);
	}
#endif
}

#endif

#if CONFIG_UART_INTERRUPT_DRIVEN

static int uart_sam0_fifo_fill(struct device *dev, const u8_t *tx_data, int len)
{
	SercomUsart *regs = DEV_CFG(dev)->regs;

	if (regs->INTFLAG.bit.DRE && len >= 1) {
		regs->DATA.reg = tx_data[0];
		return 1;
	} else {
		return 0;
	}
}

static void uart_sam0_irq_tx_enable(struct device *dev)
{
	SercomUsart *regs = DEV_CFG(dev)->regs;

	regs->INTENSET.reg = SERCOM_USART_INTENCLR_DRE;
}

static void uart_sam0_irq_tx_disable(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
}

static int uart_sam0_irq_tx_ready(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return regs->INTFLAG.bit.DRE != 0;
}

static void uart_sam0_irq_rx_enable(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENSET.reg = SERCOM_USART_INTENSET_RXC;
}

static void uart_sam0_irq_rx_disable(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENCLR.reg = SERCOM_USART_INTENCLR_RXC;
}

static int uart_sam0_irq_rx_ready(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return regs->INTFLAG.bit.RXC != 0;
}

static int uart_sam0_fifo_read(struct device *dev, u8_t *rx_data,
			       const int size)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	if (regs->INTFLAG.bit.RXC) {
		u8_t ch = regs->DATA.reg;

		if (size >= 1) {
			*rx_data = ch;
			return 1;
		} else {
			return -EINVAL;
		}
	}
	return 0;
}

static int uart_sam0_irq_is_pending(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return (regs->INTENSET.reg & regs->INTFLAG.reg) != 0;
}

static int uart_sam0_irq_update(struct device *dev) { return 1; }

static void uart_sam0_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}
#endif

#ifdef CONFIG_UART_ASYNC_API

static int uart_sam0_callback_set(struct device *dev, uart_callback_t callback,
				  void *user_data)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

	dev_data->async_cb = callback;
	dev_data->async_cb_data = user_data;

	return 0;
}

static int uart_sam0_tx(struct device *dev, const u8_t *buf, size_t len,
			u32_t timeout)
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

	retval = dma_reload(dev_data->dma, cfg->tx_dma_channel, (u32_t)buf,
			    (u32_t)(&(regs->DATA.reg)), len);
	if (retval != 0U) {
		return retval;
	}

	if (timeout != K_FOREVER) {
		k_delayed_work_submit(&dev_data->tx_timeout_work, timeout);
	}

	return dma_start(dev_data->dma, cfg->tx_dma_channel);
err:
	irq_unlock(key);
	return retval;
}

static int uart_sam0_tx_abort(struct device *dev)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);

	if (!dev_data->dma || cfg->tx_dma_channel == 0xFFU) {
		return -ENOTSUP;
	}

	k_delayed_work_cancel(&dev_data->tx_timeout_work);

	return uart_sam0_tx_halt(dev_data);
}

static int uart_sam0_rx_enable(struct device *dev, u8_t *buf, size_t len,
			       u32_t timeout)
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
			    (u32_t)(&(regs->DATA.reg)),
			    (u32_t)buf, len);
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

static int uart_sam0_rx_buf_rsp(struct device *dev, u8_t *buf, size_t len)
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

static int uart_sam0_rx_disable(struct device *dev)
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
			dev_data->async_cb(&evt, dev_data->async_cb_data);
		}
	}

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
		dev_data->async_cb(&evt, dev_data->async_cb_data);
	}

	evt.type = UART_RX_DISABLED;
	if (dev_data->async_cb) {
		dev_data->async_cb(&evt, dev_data->async_cb_data);
	}

	irq_unlock(key);

	return 0;
}

#endif

static const struct uart_driver_api uart_sam0_driver_api = {
	.poll_in = uart_sam0_poll_in,
	.poll_out = uart_sam0_poll_out,
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
#define UART_SAM0_IRQ_HANDLER_DECL(n)					\
static void uart_sam0_irq_config_##n(struct device *dev)
#define UART_SAM0_IRQ_HANDLER_FUNC(n)					\
	.irq_config_func = uart_sam0_irq_config_##n,
#define UART_SAM0_IRQ_HANDLER(n)					\
static void uart_sam0_irq_config_##n(struct device *dev)		\
{									\
	IRQ_CONNECT(DT_ATMEL_SAM0_UART_SERCOM_##n##_IRQ_0,		\
		    DT_ATMEL_SAM0_UART_SERCOM_##n##_IRQ_0_PRIORITY,	\
		    uart_sam0_isr, DEVICE_GET(uart_sam0_##n),		\
		    0);							\
	irq_enable(DT_ATMEL_SAM0_UART_SERCOM_##n##_IRQ_0);		\
}
#else
#define UART_SAM0_IRQ_HANDLER_DECL(n)
#define UART_SAM0_IRQ_HANDLER_FUNC(n)
#define UART_SAM0_IRQ_HANDLER(n)
#endif

#if CONFIG_UART_ASYNC_API
#ifndef DT_ATMEL_SAM0_UART_SERCOM_0_TXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_0_TXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_0_RXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_0_RXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_1_TXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_1_TXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_1_RXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_1_RXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_2_TXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_2_TXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_2_RXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_2_RXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_3_TXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_3_TXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_3_RXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_3_RXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_4_TXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_4_TXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_4_RXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_4_RXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_5_TXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_5_TXDMA 0xFFU
#endif
#ifndef DT_ATMEL_SAM0_UART_SERCOM_5_RXDMA
#define DT_ATMEL_SAM0_UART_SERCOM_5_RXDMA 0xFFU
#endif

#define UART_SAM0_DMA_CHANNELS(n)				 \
	.tx_dma_request = SERCOM##n##_DMAC_ID_TX,		 \
	.tx_dma_channel = DT_ATMEL_SAM0_UART_SERCOM_##n##_TXDMA, \
	.rx_dma_request = SERCOM##n##_DMAC_ID_RX,		 \
	.rx_dma_channel = DT_ATMEL_SAM0_UART_SERCOM_##n##_RXDMA
#else
#define UART_SAM0_DMA_CHANNELS(n)
#endif

#define UART_SAM0_SERCOM_PADS(n) \
	(DT_ATMEL_SAM0_UART_SERCOM_##n##_RXPO << SERCOM_USART_CTRLA_RXPO_Pos) |\
	(DT_ATMEL_SAM0_UART_SERCOM_##n##_TXPO << SERCOM_USART_CTRLA_TXPO_Pos)

#define UART_SAM0_CONFIG_DEFN(n)					       \
static const struct uart_sam0_dev_cfg uart_sam0_config_##n = {		       \
	.regs = (SercomUsart *)DT_ATMEL_SAM0_UART_SERCOM_##n##_BASE_ADDRESS,   \
	.baudrate = DT_ATMEL_SAM0_UART_SERCOM_##n##_CURRENT_SPEED,	       \
	.pm_apbcmask = PM_APBCMASK_SERCOM##n,				       \
	.gclk_clkctrl_id = GCLK_CLKCTRL_ID_SERCOM##n##_CORE,		       \
	.pads = UART_SAM0_SERCOM_PADS(n),				       \
	UART_SAM0_IRQ_HANDLER_FUNC(n)					       \
	UART_SAM0_DMA_CHANNELS(n)					       \
}

#define UART_SAM0_DEVICE_INIT(n)					       \
static struct uart_sam0_dev_data uart_sam0_data_##n;			       \
UART_SAM0_IRQ_HANDLER_DECL(n);						       \
UART_SAM0_CONFIG_DEFN(n);						       \
DEVICE_AND_API_INIT(uart_sam0_##n, DT_ATMEL_SAM0_UART_SERCOM_##n##_LABEL,      \
		    uart_sam0_init, &uart_sam0_data_##n,		       \
		    &uart_sam0_config_##n, PRE_KERNEL_1,		       \
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			       \
		    &uart_sam0_driver_api);				       \
UART_SAM0_IRQ_HANDLER(n)

#if DT_ATMEL_SAM0_UART_SERCOM_0_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(0)
#endif

#if DT_ATMEL_SAM0_UART_SERCOM_1_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(1)
#endif

#if DT_ATMEL_SAM0_UART_SERCOM_2_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(2)
#endif

#if DT_ATMEL_SAM0_UART_SERCOM_3_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(3)
#endif

#if DT_ATMEL_SAM0_UART_SERCOM_4_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(4)
#endif

#if DT_ATMEL_SAM0_UART_SERCOM_5_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(5)
#endif
