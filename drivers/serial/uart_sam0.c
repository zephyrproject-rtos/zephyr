/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_uart

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <string.h>
#include <zephyr/irq.h>

#include "uart_sam0.h"

/* clang-format off */

/*
 * Interrupt error flag is only supported in devices with
 * SERCOM revision 0x500
 */
#if defined(CONFIG_SOC_SERIES_SAMD51) || defined(CONFIG_SOC_SERIES_SAME51) ||                      \
	defined(CONFIG_SOC_SERIES_SAME53) || defined(CONFIG_SOC_SERIES_SAME54)
#define SERCOM_REV500
#endif

/* Device constant configuration parameters */
struct uart_sam0_dev_cfg {
	uintptr_t regs;
	uint32_t baudrate;
	uint32_t pads;
	bool     collision_detect;
	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint32_t gclk_gen;
	uint16_t gclk_id;

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_SAM0_ASYNC
	void (*irq_config_func)(const struct device *dev);
#endif
#if CONFIG_UART_SAM0_ASYNC
	const struct device *dma_dev;
	uint8_t tx_dma_request;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_request;
	uint8_t rx_dma_channel;
#endif
	const struct pinctrl_dev_config *pcfg;
};

/* Device run time data */
struct uart_sam0_dev_data {
	struct uart_config config_cache;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
	uint8_t txc_cache;
#endif
#if CONFIG_UART_SAM0_ASYNC
	const struct device *dev;
	const struct uart_sam0_dev_cfg *cfg;

	uart_callback_t async_cb;
	void *async_cb_data;

	struct k_work_delayable tx_timeout_work;
	const uint8_t *tx_buf;
	size_t tx_len;

	struct k_work_delayable rx_timeout_work;
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

static void wait_synchronization(const uintptr_t usart)
{
#ifndef CONFIG_SOC_SERIES_SAMD20
	/* SYNCBUSY is a register */
	while ((sys_read32(usart + SYNCBUSY_OFFSET) & SYNCBUSY_MASK) != 0) {
	}
#else
	/* SYNCBUSY is a bit */
	while (sys_test_bit(usart + STATUS_OFFSET, STATUS_SYNCBUSY_BIT)) {
	}
#endif
}

static int uart_sam0_set_baudrate(const uintptr_t usart, uint32_t baudrate,
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
	sys_write16(baud, usart + BAUD_OFFSET);
	wait_synchronization(usart);

	return 0;
}


#if CONFIG_UART_SAM0_ASYNC

static void uart_sam0_dma_tx_done(const struct device *dma_dev, void *arg,
				  uint32_t id, int error_code)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	struct uart_sam0_dev_data *const dev_data =
		(struct uart_sam0_dev_data *const) arg;
	const struct uart_sam0_dev_cfg *const cfg = dev_data->cfg;

	sys_write8(INTENSET_TXC, cfg->regs + INTENSET_OFFSET);
}

static int uart_sam0_tx_halt(struct uart_sam0_dev_data *dev_data)
{
	const struct uart_sam0_dev_cfg *const cfg = dev_data->cfg;
	unsigned int key = irq_lock();
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

	dma_stop(cfg->dma_dev, cfg->tx_dma_channel);

	irq_unlock(key);

	if (dma_get_status(cfg->dma_dev, cfg->tx_dma_channel, &st) == 0) {
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
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_sam0_dev_data *dev_data = CONTAINER_OF(dwork,
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
	unsigned int key = irq_lock();

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

	dma_reload(cfg->dma_dev, cfg->rx_dma_channel,
		   (uint32_t)(cfg->regs + DATA_OFFSET),
		   (uint32_t)dev_data->rx_buf, dev_data->rx_len);

	/*
	 * If there should be a timeout, handle starting the DMA in the
	 * ISR, since reception resets it and DMA completion implies
	 * reception.  This also catches the case of DMA completion during
	 * timeout handling.
	 */
	if (dev_data->rx_timeout_time != SYS_FOREVER_US) {
		dev_data->rx_waiting_for_irq = true;
		sys_write8(INTENSET_RXC, cfg->regs + INTENSET_OFFSET);
		irq_unlock(key);
		return;
	}

	/* Otherwise, start the transfer immediately. */
	dma_start(cfg->dma_dev, cfg->rx_dma_channel);

	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	dev_data->async_cb(dev, &evt, dev_data->async_cb_data);

	irq_unlock(key);
}

static void uart_sam0_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_sam0_dev_data *dev_data = CONTAINER_OF(dwork,
							   struct uart_sam0_dev_data, rx_timeout_work);
	const struct uart_sam0_dev_cfg *const cfg = dev_data->cfg;
	struct dma_status st;
	unsigned int key = irq_lock();

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
	dma_stop(cfg->dma_dev, cfg->rx_dma_channel);
	if (dma_get_status(cfg->dma_dev, cfg->rx_dma_channel,
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
	dma_reload(cfg->dma_dev, cfg->rx_dma_channel,
		   (uint32_t)(cfg->regs + DATA_OFFSET),
		   (uint32_t)rx_dma_start,
		   dev_data->rx_len - rx_processed);

	dev_data->rx_waiting_for_irq = true;
	sys_write8(INTENSET_RXC, cfg->regs + INTENSET_OFFSET);

	/*
	 * Never do a notify on a timeout started from the ISR: timing
	 * granularity means the first timeout can be in the middle
	 * of reception but still have the total elapsed time exhausted.
	 * So we require a timeout chunk with no data seen at all
	 * (i.e. no ISR entry).
	 */
	if (dev_data->rx_timeout_from_isr) {
		dev_data->rx_timeout_from_isr = false;
		k_work_reschedule(&dev_data->rx_timeout_work,
				      K_USEC(dev_data->rx_timeout_chunk));
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

		k_work_reschedule(&dev_data->rx_timeout_work,
				      K_USEC(remaining));
	}

	irq_unlock(key);
}

#endif

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_sam0_configure(const struct device *dev,
			       const struct uart_config *new_cfg)
{
	int retval;

	const struct uart_sam0_dev_cfg *const cfg = dev->config;
	struct uart_sam0_dev_data *const dev_data = dev->data;
	const uintptr_t usart = cfg->regs;

	wait_synchronization(usart);

	sys_clear_bit(usart + CTRLA_OFFSET, CTRLA_ENABLE_BIT);
	wait_synchronization(usart);

	if (new_cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		/* Flow control not yet supported though in principle possible
		 * on this soc family.
		 */
		return -ENOTSUP;
	}

	dev_data->config_cache.flow_ctrl = new_cfg->flow_ctrl;

	uint32_t CTRLA_temp = sys_read32(usart + CTRLA_OFFSET);
	uint32_t CTRLB_temp = sys_read32(usart + CTRLB_OFFSET);

	switch (new_cfg->parity) {
	case UART_CFG_PARITY_NONE:
		WRITE_BIT(CTRLA_temp, CTRLA_FORM_BIT, 0);
		break;
	case UART_CFG_PARITY_ODD:
		WRITE_BIT(CTRLA_temp, CTRLA_FORM_BIT, 1);
		WRITE_BIT(CTRLB_temp, CTRLB_PMODE_BIT, 1);
		break;
	case UART_CFG_PARITY_EVEN:
		WRITE_BIT(CTRLA_temp, CTRLA_FORM_BIT, 1);
		WRITE_BIT(CTRLB_temp, CTRLB_PMODE_BIT, 0);
		break;
	default:
		return -ENOTSUP;
	}

	dev_data->config_cache.parity = new_cfg->parity;

	switch (new_cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		WRITE_BIT(CTRLB_temp, CTRLB_SBMODE_BIT, 0);
		break;
	case UART_CFG_STOP_BITS_2:
		WRITE_BIT(CTRLB_temp, CTRLB_SBMODE_BIT, 1);
		break;
	default:
		return -ENOTSUP;
	}

	dev_data->config_cache.stop_bits = new_cfg->stop_bits;

	CTRLB_temp &= ~CTRLB_CHSIZE_MASK;
	switch (new_cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		CTRLB_temp |= CTRLB_CHSIZE(0x5);
		break;
	case UART_CFG_DATA_BITS_6:
		CTRLB_temp |= CTRLB_CHSIZE(0x6);
		break;
	case UART_CFG_DATA_BITS_7:
		CTRLB_temp |= CTRLB_CHSIZE(0x7);
		break;
	case UART_CFG_DATA_BITS_8:
		CTRLB_temp |= CTRLB_CHSIZE(0x0);
		break;
	case UART_CFG_DATA_BITS_9:
		CTRLB_temp |= CTRLB_CHSIZE(0x1);
		break;
	default:
		return -ENOTSUP;
	}

	dev_data->config_cache.data_bits = new_cfg->data_bits;

#if defined(SERCOM_REV500)
	WRITE_BIT(CTRLB_temp, CTRLB_COLDEN_BIT, cfg->pads);
#endif

	sys_write32(CTRLA_temp, usart + CTRLA_OFFSET);
	wait_synchronization(usart);

	sys_write32(CTRLB_temp, usart + CTRLB_OFFSET);
	wait_synchronization(usart);

	retval = uart_sam0_set_baudrate(usart, new_cfg->baudrate,
					CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	if (retval != 0) {
		return retval;
	}

	dev_data->config_cache.baudrate = new_cfg->baudrate;

	sys_set_bit(usart + CTRLA_OFFSET, CTRLA_ENABLE_BIT);
	wait_synchronization(usart);

	return 0;
}

static int uart_sam0_config_get(const struct device *dev,
				struct uart_config *out_cfg)
{
	struct uart_sam0_dev_data *const dev_data = dev->data;

	memcpy(out_cfg, &(dev_data->config_cache),
				sizeof(dev_data->config_cache));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_sam0_init(const struct device *dev)
{
	int retval;
	const struct uart_sam0_dev_cfg *const cfg = dev->config;
	struct uart_sam0_dev_data *const dev_data = dev->data;
	const uintptr_t gclk = DT_REG_ADDR(DT_INST(0, atmel_sam0_gclk));

	const uintptr_t usart = cfg->regs;

	*cfg->mclk |= cfg->mclk_mask;

#if !defined(CONFIG_SOC_SERIES_SAMD20) && !defined(CONFIG_SOC_SERIES_SAMD21) &&                    \
	!defined(CONFIG_SOC_SERIES_SAMR21)

	sys_write32(PCHCTRL_CHEN | PCHCTRL_GEN(cfg->gclk_gen),
		    gclk + PCHCTRL_OFFSET + (4 * cfg->gclk_id));
#else
	sys_write16(CLKCTRL_CLKEN | CLKCTRL_GEN(cfg->gclk_gen) | CLKCTRL_ID(cfg->gclk_id),
		    gclk + CLKCTRL_OFFSET);
#endif

	/* Disable all USART interrupts */
	sys_write8(INTENCLR_MASK, usart + INTENCLR_OFFSET);
	wait_synchronization(usart);

	/* 8 bits of data, no parity, 1 stop bit in normal mode */
	uint32_t reg =
	    cfg->pads |
	    /* Internal clock */
	    CTRLA_MODE(0x1) |
#ifndef CONFIG_SOC_SERIES_SAMD20
	    /* 16x oversampling with arithmetic baud rate generation */
	    CTRLA_SAMPR(0) |
#endif
	    CTRLA_FORM(0) | CTRLA_CPOL | CTRLA_DORD;
	sys_write32(reg, usart + CTRLA_OFFSET);
	wait_synchronization(usart);

	/* Enable PINMUX based on PINCTRL */
	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	dev_data->config_cache.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	dev_data->config_cache.parity = UART_CFG_PARITY_NONE;
	dev_data->config_cache.stop_bits = UART_CFG_STOP_BITS_1;
	dev_data->config_cache.data_bits = UART_CFG_DATA_BITS_8;

	/* Enable receiver and transmitter */
	sys_write32(CTRLB_CHSIZE(0) | CTRLB_RXEN | CTRLB_TXEN, usart + CTRLB_OFFSET);
	wait_synchronization(usart);

	retval = uart_sam0_set_baudrate(usart, cfg->baudrate,
					CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	if (retval != 0) {
		return retval;
	}
	dev_data->config_cache.baudrate = cfg->baudrate;

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_SAM0_ASYNC
	cfg->irq_config_func(dev);
#endif

#ifdef CONFIG_UART_SAM0_ASYNC
	dev_data->dev = dev;
	dev_data->cfg = cfg;
	if (!device_is_ready(cfg->dma_dev)) {
		return -ENODEV;
	}

	k_work_init_delayable(&dev_data->tx_timeout_work, uart_sam0_tx_timeout);
	k_work_init_delayable(&dev_data->rx_timeout_work, uart_sam0_rx_timeout);

	if (cfg->tx_dma_channel != 0xFFU) {
		struct dma_config dma_cfg = { 0 };
		struct dma_block_config dma_blk = { 0 };

		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_cfg.source_data_size = 1;
		dma_cfg.dest_data_size = 1;
		dma_cfg.user_data = dev_data;
		dma_cfg.dma_callback = uart_sam0_dma_tx_done;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_blk;
		dma_cfg.dma_slot = cfg->tx_dma_request;

		dma_blk.block_size = 1;
		dma_blk.dest_address = (uint32_t)(usart + DATA_OFFSET);
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		retval = dma_config(cfg->dma_dev, cfg->tx_dma_channel,
				    &dma_cfg);
		if (retval != 0) {
			return retval;
		}
	}

	if (cfg->rx_dma_channel != 0xFFU) {
		struct dma_config dma_cfg = { 0 };
		struct dma_block_config dma_blk = { 0 };

		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_cfg.source_data_size = 1;
		dma_cfg.dest_data_size = 1;
		dma_cfg.user_data = dev_data;
		dma_cfg.dma_callback = uart_sam0_dma_rx_done;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_blk;
		dma_cfg.dma_slot = cfg->rx_dma_request;

		dma_blk.block_size = 1;
		dma_blk.source_address = (uint32_t)(usart + DATA_OFFSET);
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		retval = dma_config(cfg->dma_dev, cfg->rx_dma_channel,
				    &dma_cfg);
		if (retval != 0) {
			return retval;
		}
	}

#endif

	sys_set_bit(usart + CTRLA_OFFSET, CTRLA_ENABLE_BIT);
	wait_synchronization(usart);

	return 0;
}

static int uart_sam0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_sam0_dev_cfg *config = dev->config;

	if (!(sys_read8(config->regs + INTFLAG_OFFSET) & INTFLAG_RXC)) {
		return -EBUSY;
	}

	*c = (unsigned char)sys_read8(config->regs + DATA_OFFSET);
	return 0;
}

static void uart_sam0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_sam0_dev_cfg *config = dev->config;

	while (!(sys_read8(config->regs + INTFLAG_OFFSET) & INTFLAG_DRE)) {
	}

	/* send a character */
	sys_write16(c, config->regs + DATA_OFFSET);
}

static int uart_sam0_err_check(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;

	uint32_t err = 0U;
	uint16_t status = sys_read16(config->regs + STATUS_OFFSET);

	if (IS_BIT_SET(status, STATUS_BUFOVF_BIT)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (IS_BIT_SET(status, STATUS_FERR_BIT)) {
		err |= UART_ERROR_PARITY;
	}

	if (IS_BIT_SET(status, STATUS_PERR_BIT)) {
		err |= UART_ERROR_FRAMING;
	}

#if defined(SERCOM_REV500)
	if (IS_BIT_SET(status, STATUS_ISF_BIT)) {
		err |= UART_BREAK;
	}

	if (IS_BIT_SET(status, STATUS_COLL_BIT)) {
		err |= UART_ERROR_COLLISION;
	}

	status |= BIT(STATUS_BUFOVF_BIT) | BIT(STATUS_FERR_BIT) |
		  BIT(STATUS_PERR_BIT) | BIT(STATUS_COLL_BIT) |
		  BIT(STATUS_ISF_BIT);
#else
	status |= BIT(STATUS_BUFOVF_BIT) | BIT(STATUS_FERR_BIT) |
		  BIT(STATUS_PERR_BIT);
#endif
	sys_write16(status, config->regs + STATUS_OFFSET);
	wait_synchronization(config->regs);
	return err;
}

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_SAM0_ASYNC

static void uart_sam0_isr(const struct device *dev)
{
	struct uart_sam0_dev_data *const dev_data = dev->data;

#if CONFIG_UART_INTERRUPT_DRIVEN
	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}
#endif

#if CONFIG_UART_SAM0_ASYNC
	const struct uart_sam0_dev_cfg *const cfg = dev->config;
	const uintptr_t regs = cfg->regs;

	if (dev_data->tx_len && (sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_TXC)) {
		sys_write8(INTENCLR_TXC, regs + INTENCLR_OFFSET);

		k_work_cancel_delayable(&dev_data->tx_timeout_work);

		unsigned int key = irq_lock();

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

	if (dev_data->rx_len && (sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_RXC) &&
	    dev_data->rx_waiting_for_irq) {
		dev_data->rx_waiting_for_irq = false;
		sys_write8(INTENCLR_RXC, regs + INTENCLR_OFFSET);

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
		if (dev_data->rx_timeout_time != SYS_FOREVER_US) {
			dev_data->rx_timeout_from_isr = true;
			dev_data->rx_timeout_start = k_uptime_get_32();
			k_work_reschedule(&dev_data->rx_timeout_work,
					      K_USEC(dev_data->rx_timeout_chunk));
		}

		/* DMA will read the currently ready byte out */
		dma_start(cfg->dma_dev, cfg->rx_dma_channel);
	}
#endif
}

#endif

#if CONFIG_UART_INTERRUPT_DRIVEN

static int uart_sam0_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data, int len)
{
	const struct uart_sam0_dev_cfg *config = dev->config;
	uintptr_t regs = config->regs;

	if ((sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_DRE) && len >= 1) {
		sys_write16(tx_data[0], regs + DATA_OFFSET);
		return 1;
	} else {
		return 0;
	}
}

static void uart_sam0_irq_tx_enable(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;

	sys_write8(INTENSET_DRE | INTENSET_TXC, config->regs + INTENSET_OFFSET);
}

static void uart_sam0_irq_tx_disable(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;

	sys_write8(INTENCLR_DRE | INTENCLR_TXC, config->regs + INTENCLR_OFFSET);
}

static int uart_sam0_irq_tx_ready(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;
	const uintptr_t regs = config->regs;

	return ((sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_DRE) &&
	       (sys_read8(regs + INTENSET_OFFSET) & INTENSET_DRE));
}

static int uart_sam0_irq_tx_complete(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;
	struct uart_sam0_dev_data *const dev_data = dev->data;
	const uintptr_t regs = config->regs;

	return (dev_data->txc_cache != 0) &&
		((sys_read8(regs + INTENSET_OFFSET) & INTENSET_DRE) != 0);
}

static void uart_sam0_irq_rx_enable(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;

	sys_write8(INTENSET_RXC, config->regs + INTENSET_OFFSET);
}

static void uart_sam0_irq_rx_disable(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;

	sys_write8(INTENCLR_RXC, config->regs + INTENCLR_OFFSET);
}

static int uart_sam0_irq_rx_ready(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;
	const uintptr_t regs = config->regs;

	return (sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_RXC) != 0;
}

static int uart_sam0_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int size)
{
	const struct uart_sam0_dev_cfg *config = dev->config;
	const uintptr_t regs = config->regs;

	if (sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_RXC) {
		uint8_t ch = sys_read8(regs + DATA_OFFSET);

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
	const struct uart_sam0_dev_cfg *config = dev->config;
	const uintptr_t regs = config->regs;

	return (sys_read8(regs + INTENSET_OFFSET) & sys_read8(regs + INTFLAG_OFFSET)) !=
	       0;
}

#if defined(SERCOM_REV500)
static void uart_sam0_irq_err_enable(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;
	uint8_t flag = sys_read8(config->regs + INTENSET_OFFSET);

	flag |= INTENSET_ERROR;
	sys_write8(flag, config->regs + INTENSET_OFFSET);
	wait_synchronization(config->regs);
}

static void uart_sam0_irq_err_disable(const struct device *dev)
{
	const struct uart_sam0_dev_cfg *config = dev->config;
	uint8_t flag = sys_read8(config->regs + INTENCLR_OFFSET);

	flag |= INTENCLR_ERROR;
	sys_write8(flag, config->regs + INTENCLR_OFFSET);
	wait_synchronization(config->regs);
}
#endif

static int uart_sam0_irq_update(const struct device *dev)
{
	/* Clear sticky interrupts */
	const struct uart_sam0_dev_cfg *config = dev->config;
	const uintptr_t regs = config->regs;

#if defined(SERCOM_REV500)
	/*
	 * Cache the TXC flag, and use this cached value to clear the interrupt
	 * if we do not used the cached value, there is a chance TXC will set
	 * after caching...this will cause TXC to never cached.
	 */
	struct uart_sam0_dev_data *const dev_data = dev->data;

	dev_data->txc_cache = sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_TXC;
	sys_write8(INTFLAG_ERROR | INTFLAG_RXBRK | INTFLAG_CTSIC | INTFLAG_RXS |
			dev_data->txc_cache, regs + INTFLAG_OFFSET);
#else
	sys_write8(INTFLAG_RXS, regs + INTFLAG_OFFSET);
#endif
	return 1;
}

static void uart_sam0_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_sam0_dev_data *const dev_data = dev->data;

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;

#if defined(CONFIG_UART_SAM0_ASYNC) && defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	dev_data->async_cb = NULL;
	dev_data->async_cb_data = NULL;
#endif
}
#endif

#ifdef CONFIG_UART_SAM0_ASYNC

static int uart_sam0_callback_set(const struct device *dev,
				  uart_callback_t callback,
				  void *user_data)
{
	struct uart_sam0_dev_data *const dev_data = dev->data;

	dev_data->async_cb = callback;
	dev_data->async_cb_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	dev_data->cb = NULL;
	dev_data->cb_data = NULL;
#endif

	return 0;
}

static int uart_sam0_tx(const struct device *dev, const uint8_t *buf,
			size_t len,
			int32_t timeout)
{
	struct uart_sam0_dev_data *const dev_data = dev->data;
	const struct uart_sam0_dev_cfg *const cfg = dev->config;
	uintptr_t regs = cfg->regs;
	int retval;

	if (cfg->tx_dma_channel == 0xFFU) {
		return -ENOTSUP;
	}

	if (len > 0xFFFFU) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	if (dev_data->tx_len != 0U) {
		retval = -EBUSY;
		goto err;
	}

	dev_data->tx_buf = buf;
	dev_data->tx_len = len;

	irq_unlock(key);

	retval = dma_reload(cfg->dma_dev, cfg->tx_dma_channel, (uint32_t)buf,
			    (uint32_t)(regs + DATA_OFFSET), len);
	if (retval != 0U) {
		return retval;
	}

	if (timeout != SYS_FOREVER_US) {
		k_work_reschedule(&dev_data->tx_timeout_work,
				      K_USEC(timeout));
	}

	return dma_start(cfg->dma_dev, cfg->tx_dma_channel);
err:
	irq_unlock(key);
	return retval;
}

static int uart_sam0_tx_abort(const struct device *dev)
{
	struct uart_sam0_dev_data *const dev_data = dev->data;
	const struct uart_sam0_dev_cfg *const cfg = dev->config;

	if (cfg->tx_dma_channel == 0xFFU) {
		return -ENOTSUP;
	}

	k_work_cancel_delayable(&dev_data->tx_timeout_work);

	return uart_sam0_tx_halt(dev_data);
}

static int uart_sam0_rx_enable(const struct device *dev, uint8_t *buf,
			       size_t len,
			       int32_t timeout)
{
	struct uart_sam0_dev_data *const dev_data = dev->data;
	const struct uart_sam0_dev_cfg *const cfg = dev->config;
	uintptr_t regs = cfg->regs;
	int retval;

	if (cfg->rx_dma_channel == 0xFFU) {
		return -ENOTSUP;
	}

	if (len > 0xFFFFU) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	if (dev_data->rx_len != 0U) {
		retval = -EBUSY;
		goto err;
	}

	/* Read off anything that was already there */
	while (sys_read8(regs + INTFLAG_OFFSET) & INTFLAG_RXC) {
		char discard = sys_read8(regs + DATA_OFFSET);

		(void)discard;
	}

	retval = dma_reload(cfg->dma_dev, cfg->rx_dma_channel,
			    (uint32_t)(regs + DATA_OFFSET),
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

	sys_write8(INTENSET_RXC, regs + INTENSET_OFFSET);

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

	struct uart_sam0_dev_data *const dev_data = dev->data;
	unsigned int key = irq_lock();
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
	struct uart_sam0_dev_data *const dev_data = dev->data;
	const struct uart_sam0_dev_cfg *const cfg = dev->config;
	const uintptr_t regs = cfg->regs;
	struct dma_status st;

	k_work_cancel_delayable(&dev_data->rx_timeout_work);

	unsigned int key = irq_lock();

	if (dev_data->rx_len == 0U) {
		irq_unlock(key);
		return -EINVAL;
	}

	sys_write8(INTENCLR_RXC, regs + INTENCLR_OFFSET);
	dma_stop(cfg->dma_dev, cfg->rx_dma_channel);


	if (dma_get_status(cfg->dma_dev, cfg->rx_dma_channel,
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
		struct uart_event next_evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf = {
				.buf = dev_data->rx_next_buf,
			},
		};

		dev_data->rx_next_buf = NULL;
		dev_data->rx_next_len = 0U;

		if (dev_data->async_cb) {
			dev_data->async_cb(dev, &next_evt, dev_data->async_cb_data);
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

static DEVICE_API(uart, uart_sam0_driver_api) = {
	.poll_in = uart_sam0_poll_in,
	.poll_out = uart_sam0_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_sam0_configure,
	.config_get = uart_sam0_config_get,
#endif
	.err_check = uart_sam0_err_check,
#if CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_sam0_fifo_fill,
	.fifo_read = uart_sam0_fifo_read,
	.irq_tx_enable = uart_sam0_irq_tx_enable,
	.irq_tx_disable = uart_sam0_irq_tx_disable,
	.irq_tx_ready = uart_sam0_irq_tx_ready,
	.irq_tx_complete = uart_sam0_irq_tx_complete,
	.irq_rx_enable = uart_sam0_irq_rx_enable,
	.irq_rx_disable = uart_sam0_irq_rx_disable,
	.irq_rx_ready = uart_sam0_irq_rx_ready,
	.irq_is_pending = uart_sam0_irq_is_pending,
#if defined(SERCOM_REV500)
	.irq_err_enable = uart_sam0_irq_err_enable,
	.irq_err_disable = uart_sam0_irq_err_disable,
#endif
	.irq_update = uart_sam0_irq_update,
	.irq_callback_set = uart_sam0_irq_callback_set,
#endif
#if CONFIG_UART_SAM0_ASYNC
	.callback_set = uart_sam0_callback_set,
	.tx = uart_sam0_tx,
	.tx_abort = uart_sam0_tx_abort,
	.rx_enable = uart_sam0_rx_enable,
	.rx_buf_rsp = uart_sam0_rx_buf_rsp,
	.rx_disable = uart_sam0_rx_disable,
#endif
};

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_SAM0_ASYNC

#define SAM0_UART_IRQ_CONNECT(n, m)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq),		\
			    DT_INST_IRQ_BY_IDX(n, m, priority),		\
			    uart_sam0_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));		\
	} while (false)

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

#if CONFIG_UART_SAM0_ASYNC
#define UART_SAM0_DMA_CHANNELS(n)					\
	.dma_dev = DEVICE_DT_GET(ATMEL_SAM0_DT_INST_DMA_CTLR(n, tx)),	\
	.tx_dma_request = ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, tx),	\
	.tx_dma_channel = ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, tx),	\
	.rx_dma_request = ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, rx),	\
	.rx_dma_channel = ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, rx),
#else
#define UART_SAM0_DMA_CHANNELS(n)
#endif

#define UART_SAM0_SERCOM_PADS(n) \
	(DT_INST_PROP(n, rxpo) << CTRLA_RXPO_BIT) |			\
	(DT_INST_PROP(n, txpo) << CTRLA_TXPO_BIT)

#define UART_SAM0_SERCOM_COLLISION_DETECT(n) \
	(DT_INST_PROP(n, collision_detection))

#ifndef ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET
#define ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, cell)		\
	(volatile uint32_t *)						\
	(DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, clocks, cell)) +	\
	 DT_INST_CLOCKS_CELL_BY_NAME(n, cell, offset))
#endif

#ifndef ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET
#define ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, pm)))
#endif

#ifndef ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK
#define ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, cell)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, cell))),	\
		(BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, pm, cell))))
#endif

#define UART_SAM0_CONFIG_DEFN(n)					\
static const struct uart_sam0_dev_cfg uart_sam0_config_##n = {		\
	.regs = DT_INST_REG_ADDR(n),					\
	.baudrate = DT_INST_PROP(n, current_speed),			\
	.gclk_gen = DT_PHA_BY_NAME(DT_DRV_INST(n), atmel_assigned_clocks, gclk, gen),	\
	.gclk_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, id),		\
	.mclk = ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n),		\
	.mclk_mask = ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, bit),	\
	.pads = UART_SAM0_SERCOM_PADS(n),				\
	.collision_detect = UART_SAM0_SERCOM_COLLISION_DETECT(n),	\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	UART_SAM0_IRQ_HANDLER_FUNC(n)					\
	UART_SAM0_DMA_CHANNELS(n)					\
}

#define UART_SAM0_DEVICE_INIT(n)					\
PINCTRL_DT_INST_DEFINE(n);						\
static struct uart_sam0_dev_data uart_sam0_data_##n;			\
UART_SAM0_IRQ_HANDLER_DECL(n);						\
UART_SAM0_CONFIG_DEFN(n);						\
DEVICE_DT_INST_DEFINE(n, uart_sam0_init, NULL,				\
		    &uart_sam0_data_##n,				\
		    &uart_sam0_config_##n, PRE_KERNEL_1,		\
		    CONFIG_SERIAL_INIT_PRIORITY,			\
		    &uart_sam0_driver_api);				\
UART_SAM0_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(UART_SAM0_DEVICE_INIT)

/* clang-format on */
