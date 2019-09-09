/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF5X UART
 */

#include <drivers/uart.h>
#include <hal/nrf_uart.h>
#include <hal/nrf_gpio.h>


static NRF_UART_Type *const uart0_addr =
		(NRF_UART_Type *)DT_NORDIC_NRF_UART_UART_0_BASE_ADDRESS;

/* Device data structure */
struct uart_nrfx_data {
	struct uart_config uart_config;
};

/**
 * @brief Structure for UART configuration.
 */
struct uart_nrfx_config {
	bool rts_cts_pins_set;
};

static inline struct uart_nrfx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct uart_nrfx_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

#ifdef CONFIG_UART_0_ASYNC
static struct {
	uart_callback_t callback;
	void *user_data;

	u8_t *rx_buffer;
	u8_t *rx_secondary_buffer;
	size_t rx_buffer_length;
	size_t rx_secondary_buffer_length;
	volatile size_t rx_counter;
	volatile size_t rx_offset;
	size_t rx_timeout;
	struct k_delayed_work rx_timeout_work;
	bool rx_enabled;

	bool tx_abort;
	const u8_t *tx_buffer;
	size_t tx_buffer_length;
	volatile size_t tx_counter;
#if defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
		defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
	size_t tx_timeout;
	struct k_delayed_work tx_timeout_work;
#endif
} uart0_cb;
#endif /* CONFIG_UART_0_ASYNC */

#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN

static uart_irq_callback_user_data_t irq_callback; /**< Callback function pointer */
static void *irq_cb_data; /**< Callback function arg */

/* Variable used to override the state of the TXDRDY event in the initial state
 * of the driver. This event is not set by the hardware until a first byte is
 * sent, and we want to use it as an indication if the transmitter is ready
 * to accept a new byte.
 */
static volatile u8_t uart_sw_event_txdrdy;

#endif /* CONFIG_UART_0_INTERRUPT_DRIVEN */


static bool event_txdrdy_check(void)
{
	return (nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_TXDRDY)
#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN
		|| uart_sw_event_txdrdy
#endif
	       );
}

static void event_txdrdy_clear(void)
{
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_TXDRDY);
#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN
	uart_sw_event_txdrdy = 0U;
#endif
}


/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UART.
 *
 * @param dev UART device struct
 * @param baudrate Baud rate
 *
 * @return N/A
 */

static int baudrate_set(struct device *dev, u32_t baudrate)
{
	nrf_uart_baudrate_t nrf_baudrate; /* calculated baudrate divisor */

	switch (baudrate) {
	case 300:
		/* value not supported by Nordic HAL */
		nrf_baudrate = 0x00014000;
		break;
	case 600:
		/* value not supported by Nordic HAL */
		nrf_baudrate = 0x00027000;
		break;
	case 1200:
		nrf_baudrate = NRF_UART_BAUDRATE_1200;
		break;
	case 2400:
		nrf_baudrate = NRF_UART_BAUDRATE_2400;
		break;
	case 4800:
		nrf_baudrate = NRF_UART_BAUDRATE_4800;
		break;
	case 9600:
		nrf_baudrate = NRF_UART_BAUDRATE_9600;
		break;
	case 14400:
		nrf_baudrate = NRF_UART_BAUDRATE_14400;
		break;
	case 19200:
		nrf_baudrate = NRF_UART_BAUDRATE_19200;
		break;
	case 28800:
		nrf_baudrate = NRF_UART_BAUDRATE_28800;
		break;
	case 31250:
		nrf_baudrate = NRF_UART_BAUDRATE_31250;
		break;
	case 38400:
		nrf_baudrate = NRF_UART_BAUDRATE_38400;
		break;
	case 56000:
		nrf_baudrate = NRF_UART_BAUDRATE_56000;
		break;
	case 57600:
		nrf_baudrate = NRF_UART_BAUDRATE_57600;
		break;
	case 76800:
		nrf_baudrate = NRF_UART_BAUDRATE_76800;
		break;
	case 115200:
		nrf_baudrate = NRF_UART_BAUDRATE_115200;
		break;
	case 230400:
		nrf_baudrate = NRF_UART_BAUDRATE_230400;
		break;
	case 250000:
		nrf_baudrate = NRF_UART_BAUDRATE_250000;
		break;
	case 460800:
		nrf_baudrate = NRF_UART_BAUDRATE_460800;
		break;
	case 921600:
		nrf_baudrate = NRF_UART_BAUDRATE_921600;
		break;
	case 1000000:
		nrf_baudrate = NRF_UART_BAUDRATE_1000000;
		break;
	default:
		return -EINVAL;
	}

	nrf_uart_baudrate_set(uart0_addr, nrf_baudrate);

	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

static int uart_nrfx_poll_in(struct device *dev, unsigned char *c)
{
	if (!nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXDRDY)) {
		return -1;
	}

	/* Clear the interrupt */
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXDRDY);

	/* got a character */
	*c = nrf_uart_rxd_get(uart0_addr);

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_nrfx_poll_out(struct device *dev,
					unsigned char c)
{
	/* The UART API dictates that poll_out should wait for the transmitter
	 * to be empty before sending a character. However, without locking,
	 * this introduces a rare yet possible race condition if the thread is
	 * preempted between sending the byte and checking for completion.

	 * Because of this race condition, the while loop has to be placed
	 * after the write to TXD, and we can't wait for an empty transmitter
	 * before writing. This is a trade-off between losing a byte once in a
	 * blue moon against hanging up the whole thread permanently
	 */

	/* Reset the transmitter ready state. */
	event_txdrdy_clear();

	/* Activate the transmitter. */
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTTX);

	/* Send the provided character. */
	nrf_uart_txd_set(uart0_addr, (u8_t)c);

	/* Wait until the transmitter is ready, i.e. the character is sent. */
	while (!event_txdrdy_check()) {
	}

	/* Deactivate the transmitter so that it does not needlessly consume
	 * power.
	 */
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);
}

/** Console I/O function */
static int uart_nrfx_err_check(struct device *dev)
{
	u32_t error = 0U;

	if (nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_ERROR)) {
		/* register bitfields maps to the defines in uart.h */
		error = nrf_uart_errorsrc_get_and_clear(uart0_addr);
	}

	return error;
}

static int uart_nrfx_configure(struct device *dev,
			       const struct uart_config *cfg)
{
	nrf_uart_parity_t parity;
	nrf_uart_hwfc_t hwfc;
#ifdef UART_CONFIG_STOP_Two
	bool two_stop_bits = false;
#endif

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		break;
#ifdef UART_CONFIG_STOP_Two
	case UART_CFG_STOP_BITS_2:
		two_stop_bits = true;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		hwfc = NRF_UART_HWFC_DISABLED;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		if (get_dev_config(dev)->rts_cts_pins_set) {
			hwfc = NRF_UART_HWFC_ENABLED;
		} else {
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		parity = NRF_UART_PARITY_EXCLUDED;
		break;
	case UART_CFG_PARITY_EVEN:
		parity = NRF_UART_PARITY_INCLUDED;
		break;
	default:
		return -ENOTSUP;
	}

	if (baudrate_set(dev, cfg->baudrate) != 0) {
		return -ENOTSUP;
	}

	nrf_uart_configure(uart0_addr, parity, hwfc);

#ifdef UART_CONFIG_STOP_Two
	if (two_stop_bits) {
		/* TODO Change this to nrfx HAL function when available */
		uart0_addr->CONFIG |=
			UART_CONFIG_STOP_Two << UART_CONFIG_STOP_Pos;
	}
#endif
	get_dev_data(dev)->uart_config = *cfg;

	return 0;
}

static int uart_nrfx_config_get(struct device *dev, struct uart_config *cfg)
{
	*cfg = get_dev_data(dev)->uart_config;
	return 0;
}


#ifdef CONFIG_UART_0_ASYNC

static void user_callback(struct uart_event *event)
{
	if (uart0_cb.callback) {
		uart0_cb.callback(event, uart0_cb.user_data);
	}
}

static int uart_nrfx_callback_set(struct device *dev, uart_callback_t callback,
				  void *user_data)
{
	uart0_cb.callback = callback;
	uart0_cb.user_data = user_data;

	return 0;
}

static int uart_nrfx_tx(struct device *dev, const u8_t *buf, size_t len,
			u32_t timeout)
{
	if (uart0_cb.tx_buffer_length != 0) {
		return -EBUSY;
	}

	uart0_cb.tx_buffer = buf;
	uart0_cb.tx_buffer_length = len;
#if	defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
	defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
	uart0_cb.tx_timeout = timeout;
#endif
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_TXDRDY);
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTTX);
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_TXDRDY);
	nrf_uart_int_enable(uart0_addr, NRF_UART_INT_MASK_TXDRDY);

	u8_t txd = uart0_cb.tx_buffer[uart0_cb.tx_counter];

	nrf_uart_txd_set(uart0_addr, txd);

	return 0;
}

static int uart_nrfx_tx_abort(struct device *dev)
{
	if (uart0_cb.tx_buffer_length == 0) {
		return -EINVAL;
	}
#if	defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
	defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
	k_delayed_work_cancel(&uart0_cb.tx_timeout_work);
#endif
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);

	struct uart_event evt = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = uart0_cb.tx_buffer,
		.data.tx.len = uart0_cb.tx_counter
	};

	uart0_cb.tx_buffer_length = 0;
	uart0_cb.tx_counter = 0;

	user_callback(&evt);

	return 0;
}

static int uart_nrfx_rx_enable(struct device *dev, u8_t *buf, size_t len,
			       u32_t timeout)
{
	if (uart0_cb.rx_buffer_length != 0) {
		return -EBUSY;
	}
	uart0_cb.rx_enabled = 1;
	uart0_cb.rx_buffer = buf;
	uart0_cb.rx_buffer_length = len;
	uart0_cb.rx_counter = 0;
	uart0_cb.rx_secondary_buffer_length = 0;
	uart0_cb.rx_timeout = timeout;

	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_ERROR);
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXDRDY);
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXTO);
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTRX);
	nrf_uart_int_enable(uart0_addr, NRF_UART_INT_MASK_RXDRDY |
					NRF_UART_INT_MASK_ERROR |
					NRF_UART_INT_MASK_RXTO);

	return 0;
}

static int uart_nrfx_rx_buf_rsp(struct device *dev, u8_t *buf, size_t len)
{
	if (!uart0_cb.rx_enabled) {
		return -EACCES;
	}
	if (uart0_cb.rx_secondary_buffer_length != 0) {
		return -EBUSY;
	}
	uart0_cb.rx_secondary_buffer = buf;
	uart0_cb.rx_secondary_buffer_length = len;

	return 0;
}

static int uart_nrfx_rx_disable(struct device *dev)
{
	if (uart0_cb.rx_buffer_length == 0) {
		return -EFAULT;
	}

	uart0_cb.rx_enabled = 0;
	k_delayed_work_cancel(&uart0_cb.rx_timeout_work);
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPRX);

	return 0;
}

static void rx_rdy_evt(void)
{
	struct uart_event event;
	size_t rx_cnt = uart0_cb.rx_counter;

	event.type = UART_RX_RDY;
	event.data.rx.buf = uart0_cb.rx_buffer;
	event.data.rx.len = rx_cnt - uart0_cb.rx_offset;
	event.data.rx.offset = uart0_cb.rx_offset;

	uart0_cb.rx_offset = rx_cnt;

	user_callback(&event);
}

static void buf_released_evt(void)
{
	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = uart0_cb.rx_buffer
	};
	user_callback(&event);
}

static void rx_disabled_evt(void)
{
	struct uart_event event = {
		.type = UART_RX_DISABLED
	};
	user_callback(&event);
}

static void rx_reset_state(void)
{
	nrf_uart_int_disable(uart0_addr,
			     NRF_UART_INT_MASK_RXDRDY |
			     NRF_UART_INT_MASK_ERROR |
			     NRF_UART_INT_MASK_RXTO);
	uart0_cb.rx_buffer_length = 0;
	uart0_cb.rx_enabled = 0;
	uart0_cb.rx_counter = 0;
	uart0_cb.rx_offset = 0;
	uart0_cb.rx_secondary_buffer_length = 0;
}

static void rx_isr(struct device *dev)
{
	struct uart_event event;

	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXDRDY);

	if (!uart0_cb.rx_buffer_length || !uart0_cb.rx_enabled) {
		/* Byte received when receiving is disabled - data lost. */
		nrf_uart_rxd_get(uart0_addr);
	} else {
		if (uart0_cb.rx_counter == 0) {
			event.type = UART_RX_BUF_REQUEST;
			user_callback(&event);
		}
		uart0_cb.rx_buffer[uart0_cb.rx_counter] =
			nrf_uart_rxd_get(uart0_addr);
		uart0_cb.rx_counter++;
		if (uart0_cb.rx_timeout == K_NO_WAIT) {
			rx_rdy_evt();
		} else if (uart0_cb.rx_timeout != K_FOREVER) {
			k_delayed_work_submit(&uart0_cb.rx_timeout_work,
					      uart0_cb.rx_timeout);
		}
	}

	if (uart0_cb.rx_buffer_length == uart0_cb.rx_counter) {
		k_delayed_work_cancel(&uart0_cb.rx_timeout_work);
		rx_rdy_evt();

		if (uart0_cb.rx_secondary_buffer_length) {
			buf_released_evt();
			/* Switch to secondary buffer. */
			uart0_cb.rx_buffer_length =
				uart0_cb.rx_secondary_buffer_length;
			uart0_cb.rx_buffer = uart0_cb.rx_secondary_buffer;
			uart0_cb.rx_secondary_buffer_length = 0;
			uart0_cb.rx_counter = 0;
			uart0_cb.rx_offset = 0;

			event.type = UART_RX_BUF_REQUEST;
			user_callback(&event);
		} else {
			uart_nrfx_rx_disable(dev);
		}
	}
}

static void tx_isr(void)
{
	uart0_cb.tx_counter++;
	if (uart0_cb.tx_counter < uart0_cb.tx_buffer_length &&
	    !uart0_cb.tx_abort) {
#if	defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
		defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
		k_delayed_work_submit(&uart0_cb.tx_timeout_work,
				uart0_cb.tx_timeout);
#endif
		nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_TXDRDY);

		u8_t txd = uart0_cb.tx_buffer[uart0_cb.tx_counter];

		nrf_uart_txd_set(uart0_addr, txd);
	} else {
#if	defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
		defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
		k_delayed_work_cancel(&uart0_cb.tx_timeout_work);
#endif
		nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_TXDRDY);
		uart0_cb.tx_buffer_length = 0;
		uart0_cb.tx_counter = 0;
		struct uart_event event = {
			.type = UART_TX_DONE,
			.data.tx.buf = uart0_cb.tx_buffer,
			.data.tx.len = uart0_cb.tx_counter
		};
		user_callback(&event);
	}
}

#define UART_ERROR_FROM_MASK(mask) \
	(mask & NRF_UART_ERROR_OVERRUN_MASK ? UART_ERROR_OVERRUN	\
	 : mask & NRF_UART_ERROR_PARITY_MASK ? UART_ERROR_PARITY	\
	 : mask & NRF_UART_ERROR_FRAMING_MASK ? UART_ERROR_FRAMING	\
	 : mask & NRF_UART_ERROR_BREAK_MASK ? UART_BREAK		\
	 : 0)

static void error_isr(struct device *dev)
{
	k_delayed_work_cancel(&uart0_cb.rx_timeout_work);
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_ERROR);

	if (!uart0_cb.rx_enabled) {
		nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPRX);
	}
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason =
			UART_ERROR_FROM_MASK(
				nrf_uart_errorsrc_get_and_clear(uart0_addr)),
		.data.rx_stop.data.len = uart0_cb.rx_counter
					 - uart0_cb.rx_offset,
		.data.rx_stop.data.offset = uart0_cb.rx_offset,
		.data.rx_stop.data.buf = uart0_cb.rx_buffer
	};

	user_callback(&event);
	/* Abort transfer. */
	uart_nrfx_rx_disable(dev);
}

/*
 * In nRF hardware RX timeout can occur only after stopping the peripheral,
 * it is used as a sign that peripheral has finished its operation and is
 * disabled.
 */
static void rxto_isr(void)
{
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXTO);

	buf_released_evt();
	if (uart0_cb.rx_secondary_buffer_length) {
		uart0_cb.rx_buffer = uart0_cb.rx_secondary_buffer;
		buf_released_evt();
	}

	rx_reset_state();
	rx_disabled_evt();
}

void uart_nrfx_isr(void *arg)
{
	struct device *uart = (struct device *) arg;

	if (nrf_uart_int_enable_check(uart0_addr, NRF_UART_INT_MASK_ERROR) &&
	    nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_ERROR)) {
		error_isr(uart);
	} else if (nrf_uart_int_enable_check(uart0_addr,
					     NRF_UART_INT_MASK_RXDRDY) &&
		   nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXDRDY)) {
		rx_isr(uart);
	}

	if (nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_TXDRDY)) {
		tx_isr();
	}

	if (nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXTO)) {
		rxto_isr();
	}
}

static void rx_timeout(struct k_work *work)
{
	rx_rdy_evt();
}
#if	defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
	defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
static void tx_timeout(struct k_work *work)
{
	struct uart_event evt;

	k_delayed_work_cancel(&uart0_cb.tx_timeout_work);
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);
	evt.type = UART_TX_ABORTED;
	evt.data.tx.buf = uart0_cb.tx_buffer;
	evt.data.tx.len = uart0_cb.tx_buffer_length;
	uart0_cb.tx_buffer_length = 0;
	uart0_cb.tx_counter = 0;
	user_callback(&evt);

}
#endif

#endif /* CONFIG_UART_0_ASYNC */


#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN

/** Interrupt driven FIFO fill function */
static int uart_nrfx_fifo_fill(struct device *dev,
			       const u8_t *tx_data,
			       int len)
{
	u8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       event_txdrdy_check()) {

		/* Clear the interrupt */
		event_txdrdy_clear();

		/* Send a character */
		nrf_uart_txd_set(uart0_addr, (u8_t)tx_data[num_tx++]);
	}

	return (int)num_tx;
}

/** Interrupt driven FIFO read function */
static int uart_nrfx_fifo_read(struct device *dev,
			       u8_t *rx_data,
			       const int size)
{
	u8_t num_rx = 0U;

	while ((size - num_rx > 0) &&
	       nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXDRDY)) {
		/* Clear the interrupt */
		nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXDRDY);

		/* Receive a character */
		rx_data[num_rx++] = (u8_t)nrf_uart_rxd_get(uart0_addr);
	}

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uart_nrfx_irq_tx_enable(struct device *dev)
{
	u32_t key;

	/* Indicate that this device started a transaction that should not be
	 * interrupted by putting the SoC into the deep sleep mode.
	 */
	device_busy_set(dev);

	/* Activate the transmitter. */
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTTX);

	nrf_uart_int_enable(uart0_addr, NRF_UART_INT_MASK_TXDRDY);

	/* Critical section is used to avoid any UART related interrupt which
	 * can occur after the if statement and before call of the function
	 * forcing an interrupt.
	 */
	key = irq_lock();
	if (uart_sw_event_txdrdy) {
		/* Due to HW limitation first TXDRDY interrupt shall be
		 * triggered by the software.
		 */
		NVIC_SetPendingIRQ(DT_NORDIC_NRF_UART_UART_0_IRQ_0);
	}
	irq_unlock(key);
}

/** Interrupt driven transfer disabling function */
static void uart_nrfx_irq_tx_disable(struct device *dev)
{
	nrf_uart_int_disable(uart0_addr, NRF_UART_INT_MASK_TXDRDY);

	/* Deactivate the transmitter so that it does not needlessly consume
	 * power.
	 */
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);

	/* The transaction is over. It is okay to enter the deep sleep mode
	 * if needed.
	 */
	device_busy_clear(dev);
}

/** Interrupt driven receiver enabling function */
static void uart_nrfx_irq_rx_enable(struct device *dev)
{
	nrf_uart_int_enable(uart0_addr, NRF_UART_INT_MASK_RXDRDY);
}

/** Interrupt driven receiver disabling function */
static void uart_nrfx_irq_rx_disable(struct device *dev)
{
	nrf_uart_int_disable(uart0_addr, NRF_UART_INT_MASK_RXDRDY);
}

/** Interrupt driven transfer empty function */
static int uart_nrfx_irq_tx_ready_complete(struct device *dev)
{
	return event_txdrdy_check();
}

/** Interrupt driven receiver ready function */
static int uart_nrfx_irq_rx_ready(struct device *dev)
{
	return nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXDRDY);
}

/** Interrupt driven error enabling function */
static void uart_nrfx_irq_err_enable(struct device *dev)
{
	nrf_uart_int_enable(uart0_addr, NRF_UART_INT_MASK_ERROR);
}

/** Interrupt driven error disabling function */
static void uart_nrfx_irq_err_disable(struct device *dev)
{
	nrf_uart_int_disable(uart0_addr, NRF_UART_INT_MASK_ERROR);
}

/** Interrupt driven pending status function */
static int uart_nrfx_irq_is_pending(struct device *dev)
{
	return ((nrf_uart_int_enable_check(uart0_addr,
					   NRF_UART_INT_MASK_TXDRDY) &&
		 event_txdrdy_check())
		||
		(nrf_uart_int_enable_check(uart0_addr,
					   NRF_UART_INT_MASK_RXDRDY) &&
		 uart_nrfx_irq_rx_ready(dev)));
}

/** Interrupt driven interrupt update function */
static int uart_nrfx_irq_update(struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uart_nrfx_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	(void)dev;
	irq_callback = cb;
	irq_cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void uart_nrfx_isr(void *arg)
{
	ARG_UNUSED(arg);

	if (irq_callback) {
		irq_callback(irq_cb_data);
	}
}
#endif /* CONFIG_UART_0_INTERRUPT_DRIVEN */

DEVICE_DECLARE(uart_nrfx_uart0);

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0 on success
 */
static int uart_nrfx_init(struct device *dev)
{
	int err;

	/* Setting default height state of the TX PIN to avoid glitches
	 * on the line during peripheral activation/deactivation.
	 */
	nrf_gpio_pin_write(DT_NORDIC_NRF_UART_UART_0_TX_PIN, 1);
	nrf_gpio_cfg_output(DT_NORDIC_NRF_UART_UART_0_TX_PIN);

	nrf_gpio_cfg_input(DT_NORDIC_NRF_UART_UART_0_RX_PIN,
			   NRF_GPIO_PIN_NOPULL);

	nrf_uart_txrx_pins_set(uart0_addr,
			       DT_NORDIC_NRF_UART_UART_0_TX_PIN,
			       DT_NORDIC_NRF_UART_UART_0_RX_PIN);
#if	defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
	defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
	/* Setting default height state of the RTS PIN to avoid glitches
	 * on the line during peripheral activation/deactivation.
	 */
	nrf_gpio_pin_write(DT_NORDIC_NRF_UART_UART_0_RTS_PIN, 1);
	nrf_gpio_cfg_output(DT_NORDIC_NRF_UART_UART_0_RTS_PIN);

	nrf_gpio_cfg_input(DT_NORDIC_NRF_UART_UART_0_CTS_PIN,
			   NRF_GPIO_PIN_NOPULL);

	nrf_uart_hwfc_pins_set(uart0_addr,
			       DT_NORDIC_NRF_UART_UART_0_RTS_PIN,
			       DT_NORDIC_NRF_UART_UART_0_CTS_PIN);
#endif

	/* Set initial configuration */
	err = uart_nrfx_configure(dev, &get_dev_data(dev)->uart_config);
	if (err) {
		return err;
	}

	/* Enable the UART and activate its receiver. With the current API
	 * the receiver needs to be active all the time. The transmitter
	 * will be activated when there is something to send.
	 */
	nrf_uart_enable(uart0_addr);

	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXDRDY);

	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTRX);

#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN
	/* Simulate that the TXDRDY event is set, so that the transmitter status
	 * is indicated correctly.
	 */
	uart_sw_event_txdrdy = 1U;
#endif

#if defined(CONFIG_UART_0_ASYNC) || defined(CONFIG_UART_0_INTERRUPT_DRIVEN)

	IRQ_CONNECT(DT_NORDIC_NRF_UART_UART_0_IRQ_0,
		    DT_NORDIC_NRF_UART_UART_0_IRQ_0_PRIORITY,
		    uart_nrfx_isr,
		    DEVICE_GET(uart_nrfx_uart0),
		    0);
	irq_enable(DT_NORDIC_NRF_UART_UART_0_IRQ_0);
#endif

#ifdef CONFIG_UART_0_ASYNC
	k_delayed_work_init(&uart0_cb.rx_timeout_work, rx_timeout);
#if	defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
	defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
	k_delayed_work_init(&uart0_cb.tx_timeout_work, tx_timeout);
#endif
#endif
	return 0;
}

/* Common function: uart_nrfx_irq_tx_ready_complete is used for two API entries
 * because Nordic hardware does not distinguish between them.
 */
static const struct uart_driver_api uart_nrfx_uart_driver_api = {
#ifdef CONFIG_UART_0_ASYNC
	.callback_set	  = uart_nrfx_callback_set,
	.tx		  = uart_nrfx_tx,
	.tx_abort	  = uart_nrfx_tx_abort,
	.rx_enable	  = uart_nrfx_rx_enable,
	.rx_buf_rsp	  = uart_nrfx_rx_buf_rsp,
	.rx_disable	  = uart_nrfx_rx_disable,
#endif /* CONFIG_UART_0_ASYNC */
	.poll_in          = uart_nrfx_poll_in,
	.poll_out         = uart_nrfx_poll_out,
	.err_check        = uart_nrfx_err_check,
	.configure        = uart_nrfx_configure,
	.config_get       = uart_nrfx_config_get,
#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN
	.fifo_fill        = uart_nrfx_fifo_fill,
	.fifo_read        = uart_nrfx_fifo_read,
	.irq_tx_enable    = uart_nrfx_irq_tx_enable,
	.irq_tx_disable   = uart_nrfx_irq_tx_disable,
	.irq_tx_ready     = uart_nrfx_irq_tx_ready_complete,
	.irq_rx_enable    = uart_nrfx_irq_rx_enable,
	.irq_rx_disable   = uart_nrfx_irq_rx_disable,
	.irq_tx_complete  = uart_nrfx_irq_tx_ready_complete,
	.irq_rx_ready     = uart_nrfx_irq_rx_ready,
	.irq_err_enable   = uart_nrfx_irq_err_enable,
	.irq_err_disable  = uart_nrfx_irq_err_disable,
	.irq_is_pending   = uart_nrfx_irq_is_pending,
	.irq_update       = uart_nrfx_irq_update,
	.irq_callback_set = uart_nrfx_irq_callback_set,
#endif /* CONFIG_UART_0_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void uart_nrfx_set_power_state(u32_t new_state)
{
	if (new_state == DEVICE_PM_ACTIVE_STATE) {
		nrf_uart_enable(uart0_addr);
		nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTRX);
	} else {
		assert(new_state == DEVICE_PM_LOW_POWER_STATE ||
		       new_state == DEVICE_PM_SUSPEND_STATE ||
		       new_state == DEVICE_PM_OFF_STATE);
		nrf_uart_disable(uart0_addr);
	}
}

static int uart_nrfx_pm_control(struct device *dev, u32_t ctrl_command,
				void *context, device_pm_cb cb, void *arg)
{
	static u32_t current_state = DEVICE_PM_ACTIVE_STATE;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		u32_t new_state = *((const u32_t *)context);

		if (new_state != current_state) {
			uart_nrfx_set_power_state(new_state);
			current_state = new_state;
		}
	} else {
		assert(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((u32_t *)context) = current_state;
	}

	if (cb) {
		cb(dev, 0, context, arg);
	}

	return 0;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

static struct uart_nrfx_data uart_nrfx_uart0_data = {
	.uart_config = {
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.baudrate  = DT_NORDIC_NRF_UART_UART_0_CURRENT_SPEED,
#ifdef CONFIG_UART_0_NRF_PARITY_BIT
		.parity    = UART_CFG_PARITY_EVEN,
#else
		.parity    = UART_CFG_PARITY_NONE,
#endif /* CONFIG_UART_0_NRF_PARITY_BIT */
#ifdef CONFIG_UART_0_NRF_FLOW_CONTROL
		.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS,
#else
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
#endif /* CONFIG_UART_0_NRF_FLOW_CONTROL */
	}
};

static const struct uart_nrfx_config uart_nrfx_uart0_config = {
#if	defined(DT_NORDIC_NRF_UART_UART_0_RTS_PIN) && \
	defined(DT_NORDIC_NRF_UART_UART_0_CTS_PIN)
	.rts_cts_pins_set = true,
#else
	.rts_cts_pins_set = false,
#endif
};

DEVICE_DEFINE(uart_nrfx_uart0,
	      DT_NORDIC_NRF_UART_UART_0_LABEL,
	      uart_nrfx_init,
	      uart_nrfx_pm_control,
	      &uart_nrfx_uart0_data,
	      &uart_nrfx_uart0_config,
	      /* Initialize UART device before UART console. */
	      PRE_KERNEL_1,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &uart_nrfx_uart_driver_api);
