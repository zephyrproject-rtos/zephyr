/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright (c) 2018 Justin Watson
 * Copyright (c) 2023 Gerson Fernando Budke
 * Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_usart

/** @file
 * @brief USART driver for Atmel SAM MCU family.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_UART_ASYNC_API) && defined(CONFIG_USART_SAM_HAS_PDC)
#define USART_SAM_ASYNC 1
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(USART_SAM_ASYNC)
#define USART_SAM_HAS_IRQ 1
#endif

#ifdef USART_SAM_ASYNC
/* The PDC transfer counters are 16 bits wide */
#define USART_SAM_PDC_MAX_LEN UINT16_MAX

#define USART_SAM_RX_ERRORS (US_CSR_OVRE | US_CSR_FRAME | US_CSR_PARE)

#define USART_SAM_SRAM_START DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define USART_SAM_SRAM_END   (USART_SAM_SRAM_START + DT_REG_SIZE(DT_CHOSEN(zephyr_sram)))

struct usart_sam_async_tx {
	const uint8_t *buf;
	size_t len;
	/* Bytes sent in completed chunks when transmitting through the cache */
	size_t sent;
	/* Length of the current chunk when transmitting through the cache, 0 otherwise */
	size_t chunk;
	struct k_timer timer;
	uint8_t cache[CONFIG_USART_SAM_ASYNC_TX_CACHE_SIZE];
};

struct usart_sam_async_rx {
	uint8_t *buf;
	size_t len;
	/* Bytes of buf already reported in a UART_RX_RDY event */
	size_t offset;
	uint8_t *next_buf;
	size_t next_len;
	bool enabled;
};
#endif /* USART_SAM_ASYNC */

/* Device constant configuration parameters */
struct usart_sam_dev_cfg {
	Usart *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	bool hw_flow_control;

#ifdef USART_SAM_HAS_IRQ
	uart_irq_config_func_t	irq_config_func;
#endif
};

/* Device run time data */
struct usart_sam_dev_data {
	uint32_t baud_rate;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;	/* Interrupt Callback */
	void *cb_data;	/* Interrupt Callback Arg */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef USART_SAM_ASYNC
	const struct device *dev;
	uart_callback_t async_cb;
	void *async_cb_data;
	struct usart_sam_async_tx tx;
	struct usart_sam_async_rx rx;
#endif /* USART_SAM_ASYNC */
};

static int usart_sam_poll_in(const struct device *dev, unsigned char *c)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	Usart * const usart = config->regs;

#ifdef USART_SAM_ASYNC
	struct usart_sam_dev_data *const dev_data = dev->data;

	if (dev_data->rx.enabled) {
		return -EBUSY;
	}
#endif /* USART_SAM_ASYNC */

	if (!(usart->US_CSR & US_CSR_RXRDY)) {
		return -1;
	}

	/* got a character */
	*c = (unsigned char)usart->US_RHR;

	return 0;
}

static void usart_sam_poll_out(const struct device *dev, unsigned char c)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	Usart * const usart = config->regs;

	/* Wait for transmitter to be ready */
	while (!(usart->US_CSR & US_CSR_TXRDY)) {
	}

	/* send a character */
	usart->US_THR = (uint32_t)c;
}

static int usart_sam_err_check(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;
	int errors = 0;

	if (usart->US_CSR & US_CSR_OVRE) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (usart->US_CSR & US_CSR_PARE) {
		errors |= UART_ERROR_PARITY;
	}

	if (usart->US_CSR & US_CSR_FRAME) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int usart_sam_baudrate_set(const struct device *dev, uint32_t baudrate)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	uint32_t divisor;

#ifdef SOC_ATMEL_SAM_MCK_FREQ_HZ
	uint32_t rate = SOC_ATMEL_SAM_MCK_FREQ_HZ;
#else
	uint32_t rate;
	int ret;

	ret = clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				     (clock_control_subsys_t)&config->clock_cfg,
				     &rate);
	if (ret) {
		return ret;
	}
#endif

	__ASSERT(baudrate,
		 "baud rate has to be bigger than 0");
	__ASSERT(rate/16U >= baudrate,
		 "MCK frequency is too small to set required baud rate");

	divisor = rate / 16U / baudrate;

	if (divisor > 0xFFFF) {
		return -EINVAL;
	}

	usart->US_BRGR = US_BRGR_CD(divisor);
	dev_data->baud_rate = baudrate;

	return 0;
}

static uint32_t usart_sam_cfg2sam_parity(uint8_t parity)
{
	switch (parity) {
	case UART_CFG_PARITY_EVEN:
		return US_MR_PAR_EVEN;
	case UART_CFG_PARITY_ODD:
		return US_MR_PAR_ODD;
	case UART_CFG_PARITY_SPACE:
		return US_MR_PAR_SPACE;
	case UART_CFG_PARITY_MARK:
		return US_MR_PAR_MARK;
	case UART_CFG_PARITY_NONE:
	default:
		return US_MR_PAR_NO;
	}
}

static uint8_t usart_sam_get_parity(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	switch (usart->US_MR & US_MR_PAR_Msk) {
	case US_MR_PAR_EVEN:
		return UART_CFG_PARITY_EVEN;
	case US_MR_PAR_ODD:
		return UART_CFG_PARITY_ODD;
	case US_MR_PAR_SPACE:
		return UART_CFG_PARITY_SPACE;
	case US_MR_PAR_MARK:
		return UART_CFG_PARITY_MARK;
	case US_MR_PAR_NO:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static uint32_t usart_sam_cfg2sam_stop_bits(uint8_t stop_bits)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1_5:
		return US_MR_NBSTOP_1_5_BIT;
	case UART_CFG_STOP_BITS_2:
		return US_MR_NBSTOP_2_BIT;
	case UART_CFG_STOP_BITS_1:
	default:
		return US_MR_NBSTOP_1_BIT;
	}
}

static uint8_t usart_sam_get_stop_bits(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	switch (usart->US_MR & US_MR_NBSTOP_Msk) {
	case US_MR_NBSTOP_1_5_BIT:
		return UART_CFG_STOP_BITS_1_5;
	case US_MR_NBSTOP_2_BIT:
		return UART_CFG_STOP_BITS_2;
	case US_MR_NBSTOP_1_BIT:
	default:
		return UART_CFG_STOP_BITS_1;
	}
}

static uint32_t usart_sam_cfg2sam_data_bits(uint8_t data_bits)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		return US_MR_CHRL_5_BIT;
	case UART_CFG_DATA_BITS_6:
		return US_MR_CHRL_6_BIT;
	case UART_CFG_DATA_BITS_7:
		return US_MR_CHRL_7_BIT;
	case UART_CFG_DATA_BITS_8:
	default:
		return US_MR_CHRL_8_BIT;
	}
}

static uint8_t usart_sam_get_data_bits(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	switch (usart->US_MR & US_MR_CHRL_Msk) {
	case US_MR_CHRL_5_BIT:
		return UART_CFG_DATA_BITS_5;
	case US_MR_CHRL_6_BIT:
		return UART_CFG_DATA_BITS_6;
	case US_MR_CHRL_7_BIT:
		return UART_CFG_DATA_BITS_7;
	case US_MR_CHRL_8_BIT:
	default:
		return UART_CFG_DATA_BITS_8;
	}
}

static uint32_t usart_sam_cfg2sam_flow_ctrl(uint8_t flow_ctrl)
{
	switch (flow_ctrl) {
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		return US_MR_USART_MODE_HW_HANDSHAKING;
	case UART_CFG_FLOW_CTRL_NONE:
	default:
		return US_MR_USART_MODE_NORMAL;
	}
}

static uint8_t usart_sam_get_flow_ctrl(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	switch (usart->US_MR & US_MR_USART_MODE_Msk) {
	case US_MR_USART_MODE_HW_HANDSHAKING:
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	case US_MR_USART_MODE_NORMAL:
	default:
		return UART_CFG_FLOW_CTRL_NONE;
	}
}

static int usart_sam_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	int retval;

	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

#ifdef USART_SAM_ASYNC
	struct usart_sam_dev_data *const dev_data = dev->data;

	/* Reconfiguring resets the receiver and transmitter, which would break a PDC transfer */
	if (dev_data->rx.enabled || dev_data->rx.buf != NULL || dev_data->tx.buf != NULL) {
		return -EBUSY;
	}
#endif /* USART_SAM_ASYNC */

	/* Driver doesn't support 9 data bits, 0.5 stop bits, or DTR DSR flow control */
	if (cfg->data_bits == UART_CFG_DATA_BITS_9 ||
		cfg->stop_bits == UART_CFG_STOP_BITS_0_5 ||
		cfg->flow_ctrl == UART_CFG_FLOW_CTRL_DTR_DSR) {
		return -ENOTSUP;
	}

	/* Reset and disable USART */
	usart->US_CR = US_CR_RSTRX | US_CR_RSTTX
		     | US_CR_RXDIS | US_CR_TXDIS
		     | US_CR_RSTSTA;

	/* normal UART mode, baud rate driven by peripheral clock, all
	 * other values chosen by config
	 */
	usart->US_MR = US_MR_CHMODE_NORMAL
		     | US_MR_USCLKS_MCK
		     | usart_sam_cfg2sam_parity(cfg->parity)
		     | usart_sam_cfg2sam_stop_bits(cfg->stop_bits)
		     | usart_sam_cfg2sam_data_bits(cfg->data_bits)
		     | usart_sam_cfg2sam_flow_ctrl(cfg->flow_ctrl);

	/* Set baud rate */
	retval = usart_sam_baudrate_set(dev, cfg->baudrate);
	if (retval != 0) {
		return retval;
	}

	/* Enable receiver and transmitter */
	usart->US_CR = US_CR_RXEN | US_CR_TXEN;

	return 0;
}

static int usart_sam_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	cfg->baudrate = dev_data->baud_rate;
	cfg->parity = usart_sam_get_parity(dev);
	cfg->stop_bits = usart_sam_get_stop_bits(dev);
	cfg->data_bits = usart_sam_get_data_bits(dev);
	cfg->flow_ctrl = usart_sam_get_flow_ctrl(dev);

	return 0;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

static int usart_sam_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data,
			       int size)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	/* Wait for transmitter to be ready. */
	while ((usart->US_CSR & US_CSR_TXRDY) == 0) {
	}

	usart->US_THR = *tx_data;

	return 1;
}

static int usart_sam_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int size)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;
	int bytes_read;

	bytes_read = 0;

	while (bytes_read < size) {
		if (usart->US_CSR & US_CSR_RXRDY) {
			rx_data[bytes_read] = usart->US_RHR;
			bytes_read++;
		} else {
			break;
		}
	}

	return bytes_read;
}

static void usart_sam_irq_tx_enable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IER = US_IER_TXRDY;
}

static void usart_sam_irq_tx_disable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IDR = US_IDR_TXRDY;
}

static int usart_sam_irq_tx_ready(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	/* Check that the transmitter is ready but only
	 * return true if the interrupt is also enabled
	 */
	return (usart->US_CSR & US_CSR_TXRDY &&
		usart->US_IMR & US_IMR_TXRDY);
}

static void usart_sam_irq_rx_enable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IER = US_IER_RXRDY;
}

static void usart_sam_irq_rx_disable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IDR = US_IDR_RXRDY;
}

static int usart_sam_irq_tx_complete(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	return (usart->US_CSR & US_CSR_TXRDY &&
		usart->US_CSR & US_CSR_TXEMPTY);
}

static int usart_sam_irq_rx_ready(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	return (usart->US_CSR & US_CSR_RXRDY);
}

static void usart_sam_irq_err_enable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IER = US_IER_OVRE | US_IER_FRAME | US_IER_PARE;
}

static void usart_sam_irq_err_disable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IDR = US_IDR_OVRE | US_IDR_FRAME | US_IDR_PARE;
}

static int usart_sam_irq_is_pending(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	return (usart->US_IMR & (US_IMR_TXRDY | US_IMR_RXRDY)) &
		(usart->US_CSR & (US_CSR_TXRDY | US_CSR_RXRDY));
}

static void usart_sam_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	dev_data->irq_cb = cb;
	dev_data->cb_data = cb_data;

#if defined(USART_SAM_ASYNC) && defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	dev_data->async_cb = NULL;
	dev_data->async_cb_data = NULL;
#endif
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef USART_SAM_ASYNC

static void usart_sam_async_evt(const struct device *dev, struct uart_event *evt)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	if (dev_data->async_cb != NULL) {
		dev_data->async_cb(dev, evt, dev_data->async_cb_data);
	}
}

static int usart_sam_callback_set(const struct device *dev, uart_callback_t callback,
				  void *user_data)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	dev_data->async_cb = callback;
	dev_data->async_cb_data = user_data;

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	dev_data->irq_cb = NULL;
	dev_data->cb_data = NULL;
#endif

	return 0;
}

static void usart_sam_tx_complete(const struct device *dev, enum uart_event_type type)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;
	struct uart_event evt = {
		.type = type,
		.data.tx.buf = dev_data->tx.buf,
	};

	Usart * const usart = config->regs;

	usart->US_PTCR = US_PTCR_TXTDIS;
	usart->US_IDR = US_IDR_ENDTX | US_IDR_TXEMPTY;

	if (dev_data->tx.chunk != 0U) {
		evt.data.tx.len = dev_data->tx.sent + dev_data->tx.chunk - usart->US_TCR;
	} else {
		evt.data.tx.len = dev_data->tx.len - usart->US_TCR;
	}
	usart->US_TCR = 0;
	dev_data->tx.buf = NULL;

	k_timer_stop(&dev_data->tx.timer);

	usart_sam_async_evt(dev, &evt);
}

/* The PDC can't read the internal flash, send buffers outside the SRAM through the cache */
static inline bool usart_sam_tx_needs_cache(const uint8_t *buf, size_t len)
{
	return (uintptr_t)buf < USART_SAM_SRAM_START || (uintptr_t)buf + len > USART_SAM_SRAM_END;
}

/* Copy the next chunk into the cache and start it, a non-zero counter clears ENDTX */
static void usart_sam_tx_chunk(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;
	struct usart_sam_async_tx *tx = &dev_data->tx;

	Usart * const usart = config->regs;

	tx->chunk = MIN(tx->len - tx->sent, sizeof(tx->cache));
	memcpy(tx->cache, &tx->buf[tx->sent], tx->chunk);

	usart->US_TPR = (uint32_t)tx->cache;
	usart->US_TCR = tx->chunk;
}

static int usart_sam_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;
	unsigned int key;

	Usart * const usart = config->regs;

	if (buf == NULL || len == 0U || len > USART_SAM_PDC_MAX_LEN) {
		return -EINVAL;
	}

	key = irq_lock();

	if (dev_data->tx.buf != NULL) {
		irq_unlock(key);
		return -EBUSY;
	}

	dev_data->tx.buf = buf;
	dev_data->tx.len = len;
	dev_data->tx.sent = 0;
	dev_data->tx.chunk = 0;

	/* The timeout only applies with flow control, where the peer can stall the transfer */
	if (timeout != SYS_FOREVER_US &&
	    (usart->US_MR & US_MR_USART_MODE_Msk) == US_MR_USART_MODE_HW_HANDSHAKING) {
		k_timer_start(&dev_data->tx.timer, K_USEC(timeout), K_NO_WAIT);
	}

	/* A non-zero counter clears ENDTX, so only arm its interrupt afterwards */
	if (usart_sam_tx_needs_cache(buf, len)) {
		usart_sam_tx_chunk(dev);
	} else {
		usart->US_TPR = (uint32_t)buf;
		usart->US_TCR = len;
	}
	usart->US_PTCR = US_PTCR_TXTEN;
	usart->US_IER = US_IER_ENDTX;

	irq_unlock(key);

	return 0;
}

static int usart_sam_tx_abort(const struct device *dev)
{
	struct usart_sam_dev_data *const dev_data = dev->data;
	unsigned int key;

	key = irq_lock();

	if (dev_data->tx.buf == NULL) {
		irq_unlock(key);
		return -EFAULT;
	}

	usart_sam_tx_complete(dev, UART_TX_ABORTED);

	irq_unlock(key);

	return 0;
}

static void usart_sam_tx_timeout(struct k_timer *timer)
{
	struct usart_sam_async_tx *tx = CONTAINER_OF(timer, struct usart_sam_async_tx, timer);
	struct usart_sam_dev_data *dev_data = CONTAINER_OF(tx, struct usart_sam_dev_data, tx);
	unsigned int key;

	key = irq_lock();

	if (tx->buf != NULL) {
		usart_sam_tx_complete(dev_data->dev, UART_TX_ABORTED);
	}

	irq_unlock(key);
}

/* Bytes received in the current buffer */
static size_t usart_sam_rx_count(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;
	uint32_t rcr;

	Usart * const usart = config->regs;

	/*
	 * Read RCR first: if the PDC switches to the next buffer after this, RNCR reads zero below.
	 * The switch moves the next counter to the current one and clears the next counter, so the
	 * current buffer is full.
	 */
	rcr = usart->US_RCR;
	if (dev_data->rx.next_buf != NULL && usart->US_RNCR == 0U) {
		return dev_data->rx.len;
	}

	return dev_data->rx.len - rcr;
}

/* Report the bytes received in the current buffer since the last report */
static void usart_sam_rx_rdy(const struct device *dev, size_t received)
{
	struct usart_sam_dev_data *const dev_data = dev->data;
	struct uart_event evt = {
		.type = UART_RX_RDY,
		.data.rx.buf = dev_data->rx.buf,
		.data.rx.offset = dev_data->rx.offset,
	};

	if (received <= dev_data->rx.offset) {
		return;
	}

	evt.data.rx.len = received - dev_data->rx.offset;
	dev_data->rx.offset = received;

	usart_sam_async_evt(dev, &evt);
}

static void usart_sam_rx_buf_release(const struct device *dev, uint8_t *buf)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = buf,
	};

	if (buf != NULL) {
		usart_sam_async_evt(dev, &evt);
	}
}

static void usart_sam_rx_buf_request(const struct device *dev)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	usart_sam_async_evt(dev, &evt);
}

/*
 * ENDRX stays set until a non-zero value is written to RCR or RNCR, so it only signals the switch
 * to a next buffer. Without a next buffer, RXBUFF signals the current one is full.
 */
static void usart_sam_rx_arm(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;

	Usart * const usart = config->regs;

	if (dev_data->rx.next_buf != NULL) {
		usart->US_IDR = US_IDR_RXBUFF;
		usart->US_IER = US_IER_ENDRX;
	} else {
		usart->US_IDR = US_IDR_ENDRX;
		usart->US_IER = US_IER_RXBUFF;
	}
}

/* Move on to the next buffer once the PDC switched to it */
static void usart_sam_rx_next(const struct device *dev)
{
	struct usart_sam_dev_data *const dev_data = dev->data;
	uint8_t *buf = dev_data->rx.buf;
	size_t len = dev_data->rx.len;

	/* Report the rest while the buffer is still current, so a nested stop releases it */
	usart_sam_rx_rdy(dev, len);

	/* The callback may have stopped reception, and started a new one */
	if (dev_data->rx.buf != buf || dev_data->rx.offset != len) {
		return;
	}

	dev_data->rx.buf = dev_data->rx.next_buf;
	dev_data->rx.len = dev_data->rx.next_len;
	dev_data->rx.offset = 0;
	dev_data->rx.next_buf = NULL;

	usart_sam_rx_buf_release(dev, buf);
}

/* Stop reception, reporting UART_RX_STOPPED first for a non-zero reason */
static void usart_sam_rx_stop(const struct device *dev, enum uart_rx_stop_reason reason)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;
	struct uart_event evt = {
		.type = UART_RX_DISABLED,
	};
	uint8_t *buf;
	uint8_t *next_buf;

	Usart * const usart = config->regs;

	usart->US_PTCR = US_PTCR_RXTDIS;
	usart->US_IDR = US_IDR_ENDRX | US_IDR_RXBUFF | US_IDR_TIMEOUT | USART_SAM_RX_ERRORS;
	usart->US_RTOR = 0;

	/*
	 * Reception is stopped from here on, so callbacks can't disable it again. It can't be
	 * enabled again either while a buffer is held, see usart_sam_rx_enable().
	 */
	dev_data->rx.enabled = false;

	if (reason != 0) {
		struct uart_event stop_evt = {
			.type = UART_RX_STOPPED,
			.data.rx_stop.reason = reason,
			.data.rx_stop.data.buf = dev_data->rx.buf,
			.data.rx_stop.data.offset = dev_data->rx.offset,
			.data.rx_stop.data.len = usart_sam_rx_count(dev) - dev_data->rx.offset,
		};

		usart_sam_async_evt(dev, &stop_evt);
	}

	if (dev_data->rx.next_buf != NULL && (usart->US_CSR & US_CSR_ENDRX) != 0U) {
		usart_sam_rx_next(dev);
	}

	usart_sam_rx_rdy(dev, usart_sam_rx_count(dev));

	usart->US_RCR = 0;
	usart->US_RNCR = 0;

	buf = dev_data->rx.buf;
	next_buf = dev_data->rx.next_buf;

	usart_sam_rx_buf_release(dev, buf);
	usart_sam_rx_buf_release(dev, next_buf);

	/* Clear the state last, so reception can be enabled again from UART_RX_DISABLED */
	dev_data->rx.buf = NULL;
	dev_data->rx.next_buf = NULL;
	usart_sam_async_evt(dev, &evt);
}

static void usart_sam_rx_end(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;

	Usart * const usart = config->regs;

	while (dev_data->rx.enabled) {
		if (dev_data->rx.next_buf == NULL) {
			if (usart->US_RCR == 0U) {
				/* Buffer full without a next one */
				usart_sam_rx_stop(dev, 0);
			} else {
				usart_sam_rx_arm(dev);
			}
			return;
		}

		if ((usart->US_CSR & US_CSR_ENDRX) == 0U) {
			return;
		}

		usart_sam_rx_next(dev);

		if (dev_data->rx.enabled) {
			usart_sam_rx_buf_request(dev);
		}
	}
}

static void usart_sam_rx_error(const struct device *dev, uint32_t csr)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	enum uart_rx_stop_reason reason;

	Usart * const usart = config->regs;

	usart->US_CR = US_CR_RSTSTA;

	if ((csr & US_CSR_OVRE) != 0U) {
		reason = UART_ERROR_OVERRUN;
	} else if ((csr & US_CSR_PARE) != 0U) {
		reason = UART_ERROR_PARITY;
	} else {
		reason = UART_ERROR_FRAMING;
	}

	usart_sam_rx_stop(dev, reason);
}

static int usart_sam_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;
	uint32_t ier = USART_SAM_RX_ERRORS;
	uint64_t timeout_bits = 0;
	unsigned int key;

	Usart * const usart = config->regs;

	if (buf == NULL || len == 0U || len > USART_SAM_PDC_MAX_LEN) {
		return -EINVAL;
	}

	if (timeout != SYS_FOREVER_US) {
		if (timeout < 0) {
			return -EINVAL;
		}

		/* The receiver time-out counts bit periods */
		timeout_bits = DIV_ROUND_UP((uint64_t)timeout * dev_data->baud_rate, USEC_PER_SEC);
		if (timeout_bits > (US_RTOR_TO_Msk >> US_RTOR_TO_Pos)) {
			return -EINVAL;
		}

		/* Zero disables the time-out, use the shortest one instead */
		timeout_bits = MAX(timeout_bits, 1U);
	}

	key = irq_lock();

	/* A held buffer means reception is still stopping */
	if (dev_data->rx.enabled || dev_data->rx.buf != NULL) {
		irq_unlock(key);
		return -EBUSY;
	}

	dev_data->rx.buf = buf;
	dev_data->rx.len = len;
	dev_data->rx.offset = 0;
	dev_data->rx.next_buf = NULL;
	dev_data->rx.enabled = true;

	/* Reception starts now, drop a character received before and its errors */
	while ((usart->US_CSR & US_CSR_RXRDY) != 0U) {
		(void)usart->US_RHR;
	}
	usart->US_CR = US_CR_RSTSTA;

	usart->US_RTOR = timeout_bits;
	if (timeout_bits != 0U) {
		usart->US_CR = US_CR_STTTO;
		ier |= US_IER_TIMEOUT;
	}

	usart->US_RPR = (uint32_t)buf;
	usart->US_RCR = len;
	usart->US_RNCR = 0;
	usart->US_PTCR = US_PTCR_RXTEN;
	usart->US_IER = ier;
	usart_sam_rx_arm(dev);

	/* Last, the callback may stop reception and enable it again */
	usart_sam_rx_buf_request(dev);

	irq_unlock(key);

	return 0;
}

static int usart_sam_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;
	unsigned int key;
	int ret = 0;

	Usart * const usart = config->regs;

	if (buf == NULL || len == 0U || len > USART_SAM_PDC_MAX_LEN) {
		return -EINVAL;
	}

	key = irq_lock();

	if (!dev_data->rx.enabled) {
		ret = -EACCES;
	} else if (dev_data->rx.next_buf != NULL) {
		ret = -EBUSY;
	} else {
		/* Pause the PDC, so the buffer can't fill between the check and the update */
		bool pdc_enabled = (usart->US_PTSR & US_PTSR_RXTEN) != 0U;

		usart->US_PTCR = US_PTCR_RXTDIS;

		if (usart->US_RCR == 0U) {
			/* The current buffer is already full and reception is about to stop */
			ret = -EACCES;
		} else {
			dev_data->rx.next_buf = buf;
			dev_data->rx.next_len = len;
			usart->US_RNPR = (uint32_t)buf;
			/* Clears ENDRX left over from the previous switch */
			usart->US_RNCR = len;
			usart_sam_rx_arm(dev);
		}

		if (pdc_enabled) {
			usart->US_PTCR = US_PTCR_RXTEN;
		}
	}

	irq_unlock(key);

	return ret;
}

static int usart_sam_rx_disable(const struct device *dev)
{
	struct usart_sam_dev_data *const dev_data = dev->data;
	unsigned int key;

	key = irq_lock();

	if (!dev_data->rx.enabled) {
		irq_unlock(key);
		return -EFAULT;
	}

	usart_sam_rx_stop(dev, 0);

	irq_unlock(key);

	return 0;
}

static void usart_sam_async_isr(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;
	struct usart_sam_dev_data *const dev_data = dev->data;
	uint32_t csr;

	Usart * const usart = config->regs;

	/* Callbacks may restart reception or transmission, read the status again for each step */
	csr = usart->US_CSR & usart->US_IMR;

	if (dev_data->rx.enabled && (csr & USART_SAM_RX_ERRORS) != 0U) {
		usart_sam_rx_error(dev, csr);
	}

	csr = usart->US_CSR & usart->US_IMR;

	if (dev_data->rx.enabled && (csr & (US_CSR_ENDRX | US_CSR_RXBUFF)) != 0U) {
		usart_sam_rx_end(dev);
	}

	csr = usart->US_CSR & usart->US_IMR;

	if (dev_data->rx.enabled && (csr & US_CSR_TIMEOUT) != 0U) {
		/* Restart first, STTTO cancels the time-out of characters received before it */
		usart->US_CR = US_CR_STTTO;
		usart_sam_rx_rdy(dev, usart_sam_rx_count(dev));
	}

	csr = usart->US_CSR & usart->US_IMR;

	/* ENDTX means the PDC is done, TXEMPTY that the last character left the shift register */
	if (dev_data->tx.buf != NULL && (csr & US_CSR_ENDTX) != 0U) {
		if (dev_data->tx.chunk != 0U &&
		    dev_data->tx.sent + dev_data->tx.chunk < dev_data->tx.len) {
			dev_data->tx.sent += dev_data->tx.chunk;
			usart_sam_tx_chunk(dev);
		} else {
			usart->US_IDR = US_IDR_ENDTX;
			usart->US_IER = US_IER_TXEMPTY;
		}
	}

	if (dev_data->tx.buf != NULL && (csr & US_CSR_TXEMPTY) != 0U) {
		usart_sam_tx_complete(dev, UART_TX_DONE);
	}
}

#endif /* USART_SAM_ASYNC */

#ifdef USART_SAM_HAS_IRQ

static void usart_sam_isr(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct usart_sam_dev_data *const dev_data = dev->data;

	if (dev_data->irq_cb) {
		dev_data->irq_cb(dev, dev_data->cb_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef USART_SAM_ASYNC
	usart_sam_async_isr(dev);
#endif /* USART_SAM_ASYNC */
}

#endif /* USART_SAM_HAS_IRQ */

static int usart_sam_init(const struct device *dev)
{
	int retval;

	const struct usart_sam_dev_cfg *const cfg = dev->config;

	struct usart_sam_dev_data *const dev_data = dev->data;

	Usart * const usart = cfg->regs;

	/* Enable USART clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);

	/* Connect pins to the peripheral */
	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* Disable Interrupts */
	usart->US_IDR = 0xFFFFFFFF;

#ifdef USART_SAM_ASYNC
	dev_data->dev = dev;
	k_timer_init(&dev_data->tx.timer, usart_sam_tx_timeout, NULL);

	/* Stop the PDC channels, a bootloader may have left them running or a next transfer set */
	usart->US_PTCR = US_PTCR_RXTDIS | US_PTCR_TXTDIS;
	usart->US_TNCR = 0;
#endif /* USART_SAM_ASYNC */

#ifdef USART_SAM_HAS_IRQ
	cfg->irq_config_func(dev);
#endif /* USART_SAM_HAS_IRQ */

	struct uart_config uart_config = {
		.baudrate = dev_data->baud_rate,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};
	if (cfg->hw_flow_control) {
		uart_config.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	}
	return usart_sam_configure(dev, &uart_config);
}

static DEVICE_API(uart, usart_sam_driver_api) = {
	.poll_in = usart_sam_poll_in,
	.poll_out = usart_sam_poll_out,
	.err_check = usart_sam_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = usart_sam_configure,
	.config_get = usart_sam_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_sam_fifo_fill,
	.fifo_read = usart_sam_fifo_read,
	.irq_tx_enable = usart_sam_irq_tx_enable,
	.irq_tx_disable = usart_sam_irq_tx_disable,
	.irq_tx_ready = usart_sam_irq_tx_ready,
	.irq_rx_enable = usart_sam_irq_rx_enable,
	.irq_rx_disable = usart_sam_irq_rx_disable,
	.irq_tx_complete = usart_sam_irq_tx_complete,
	.irq_rx_ready = usart_sam_irq_rx_ready,
	.irq_err_enable = usart_sam_irq_err_enable,
	.irq_err_disable = usart_sam_irq_err_disable,
	.irq_is_pending = usart_sam_irq_is_pending,
	.irq_callback_set = usart_sam_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef USART_SAM_ASYNC
	.callback_set = usart_sam_callback_set,
	.tx = usart_sam_tx,
	.tx_abort = usart_sam_tx_abort,
	.rx_enable = usart_sam_rx_enable,
	.rx_buf_rsp = usart_sam_rx_buf_rsp,
	.rx_disable = usart_sam_rx_disable,
#endif /* USART_SAM_ASYNC */
};

#define USART_SAM_DECLARE_CFG(n, IRQ_FUNC_INIT)				\
	static const struct usart_sam_dev_cfg usart##n##_sam_config = {	\
		.regs = (Usart *)DT_INST_REG_ADDR(n),			\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control),	\
									\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
									\
		IRQ_FUNC_INIT						\
	}

#ifdef USART_SAM_HAS_IRQ
#define USART_SAM_CONFIG_FUNC(n)					\
	static void usart##n##_sam_irq_config_func(const struct device *port)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    usart_sam_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define USART_SAM_IRQ_CFG_FUNC_INIT(n)					\
	.irq_config_func = usart##n##_sam_irq_config_func
#define USART_SAM_INIT_CFG(n)						\
	USART_SAM_DECLARE_CFG(n, USART_SAM_IRQ_CFG_FUNC_INIT(n))
#else
#define USART_SAM_CONFIG_FUNC(n)
#define USART_SAM_IRQ_CFG_FUNC_INIT
#define USART_SAM_INIT_CFG(n)						\
	USART_SAM_DECLARE_CFG(n, USART_SAM_IRQ_CFG_FUNC_INIT)
#endif

#define USART_SAM_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static struct usart_sam_dev_data usart##n##_sam_data = {	\
		.baud_rate = DT_INST_PROP(n, current_speed),		\
	};								\
									\
	static const struct usart_sam_dev_cfg usart##n##_sam_config;	\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    usart_sam_init, NULL,			\
			    &usart##n##_sam_data,			\
			    &usart##n##_sam_config, PRE_KERNEL_1,	\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &usart_sam_driver_api);			\
									\
	USART_SAM_CONFIG_FUNC(n)					\
									\
	USART_SAM_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(USART_SAM_INIT)
