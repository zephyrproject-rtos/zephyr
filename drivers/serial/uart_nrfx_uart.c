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

/*
 * Extract information from devicetree.
 *
 * This driver only supports one instance of this IP block, so the
 * instance number is always 0.
 */
#define DT_DRV_COMPAT	nordic_nrf_uart

#define PROP(prop)	DT_INST_PROP(0, prop)
#define HAS_PROP(prop)	DT_INST_NODE_HAS_PROP(0, prop)

#define BAUDRATE	PROP(current_speed)
#define TX_PIN		PROP(tx_pin)

#define RX_PIN_USED	HAS_PROP(rx_pin)
#if RX_PIN_USED
#define RX_PIN		PROP(rx_pin)
#else
#define RX_PIN		NRF_UART_PSEL_DISCONNECTED
#endif

#define HW_FLOW_CONTROL_AVAILABLE	(HAS_PROP(rts_pin) || HAS_PROP(cts_pin))

/* Protect against enabling flow control without pins set. */
BUILD_ASSERT((PROP(hw_flow_control) && HW_FLOW_CONTROL_AVAILABLE) ||
		!PROP(hw_flow_control));

#define RTS_PIN \
	COND_CODE_1(HAS_PROP(rts_pin), \
		(PROP(rts_pin)), (NRF_UART_PSEL_DISCONNECTED))

#define CTS_PIN \
	COND_CODE_1(HAS_PROP(cts_pin), \
		(PROP(cts_pin)), (NRF_UART_PSEL_DISCONNECTED))

#define IRQN		DT_INST_IRQN(0)
#define IRQ_PRIO	DT_INST_IRQ(0, priority)

static NRF_UART_Type *const uart0_addr = (NRF_UART_Type *)DT_INST_REG_ADDR(0);

/* Device data structure */
struct uart_nrfx_data {
	struct uart_config uart_config;
};

static inline struct uart_nrfx_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

#ifdef CONFIG_UART_0_ASYNC
static struct {
	uart_callback_t callback;
	void *user_data;

	uint8_t *rx_buffer;
	uint8_t *rx_secondary_buffer;
	size_t rx_buffer_length;
	size_t rx_secondary_buffer_length;
	volatile size_t rx_counter;
	volatile size_t rx_offset;
	int32_t rx_timeout;
	struct k_timer rx_timeout_timer;
	bool rx_enabled;

	bool tx_abort;
	const uint8_t *volatile tx_buffer;
	size_t tx_buffer_length;
	volatile size_t tx_counter;
#if HW_FLOW_CONTROL_AVAILABLE
	int32_t tx_timeout;
	struct k_timer tx_timeout_timer;
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
static volatile uint8_t uart_sw_event_txdrdy;
static volatile bool disable_tx_irq;

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

static int baudrate_set(const struct device *dev, uint32_t baudrate)
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

static int uart_nrfx_poll_in(const struct device *dev, unsigned char *c)
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

#ifdef CONFIG_UART_0_ASYNC
static void uart_nrfx_isr(const struct device *dev);
#endif

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_nrfx_poll_out(const struct device *dev, unsigned char c)
{
	atomic_t *lock;
#ifdef CONFIG_UART_0_ASYNC
	while (uart0_cb.tx_buffer) {
		/* If there is ongoing asynchronous transmission, and we are in
		 * ISR, then call uart interrupt routine, otherwise
		 * busy wait until transmission is finished.
		 */
		if (k_is_in_isr()) {
			uart_nrfx_isr(dev);
		}
	}
	/* Use tx_buffer_length as lock, this way uart_nrfx_tx will
	 * return -EBUSY during poll_out.
	 */
	lock = &uart0_cb.tx_buffer_length;
#else
	static atomic_val_t poll_out_lock;

	lock = &poll_out_lock;
#endif

	if (!k_is_in_isr()) {
		uint8_t safety_cnt = 100;

		while (atomic_cas((atomic_t *) lock,
				  (atomic_val_t) 0,
				  (atomic_val_t) 1) == false) {
			if (IS_ENABLED(CONFIG_MULTITHREADING)) {
				/* k_sleep allows other threads to execute and finish
				 * their transactions.
				 */
				k_msleep(1);
			} else {
				k_busy_wait(1000);
			}
			if (--safety_cnt == 0) {
				break;
			}
		}
	} else {
		*lock = 1;
	}
	/* Reset the transmitter ready state. */
	event_txdrdy_clear();

	/* Activate the transmitter. */
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTTX);

	/* Send the provided character. */
	nrf_uart_txd_set(uart0_addr, (uint8_t)c);

	/* Wait until the transmitter is ready, i.e. the character is sent. */
	int res;

	NRFX_WAIT_FOR(event_txdrdy_check(), 1000, 1, res);

	/* Deactivate the transmitter so that it does not needlessly
	 * consume power.
	 */
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);

	/* Release the lock. */
	*lock = 0;
}

/** Console I/O function */
static int uart_nrfx_err_check(const struct device *dev)
{
	/* register bitfields maps to the defines in uart.h */
	return nrf_uart_errorsrc_get_and_clear(uart0_addr);
}

static int uart_nrfx_configure(const struct device *dev,
			       const struct uart_config *cfg)
{
	nrf_uart_config_t uart_cfg;

#if defined(UART_CONFIG_STOP_Msk)
	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_cfg.stop = NRF_UART_STOP_ONE;
		break;
	case UART_CFG_STOP_BITS_2:
		uart_cfg.stop = NRF_UART_STOP_TWO;
		break;
	default:
		return -ENOTSUP;
	}
#else
	if (cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		return -ENOTSUP;
	}
#endif

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uart_cfg.hwfc = NRF_UART_HWFC_DISABLED;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		if (HW_FLOW_CONTROL_AVAILABLE) {
			uart_cfg.hwfc = NRF_UART_HWFC_ENABLED;
		} else {
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

#if defined(UART_CONFIG_PARITYTYPE_Msk)
	uart_cfg.paritytype = NRF_UART_PARITYTYPE_EVEN;
#endif
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_cfg.parity = NRF_UART_PARITY_EXCLUDED;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_cfg.parity = NRF_UART_PARITY_INCLUDED;
		break;
#if defined(UART_CONFIG_PARITYTYPE_Msk)
	case UART_CFG_PARITY_ODD:
		uart_cfg.parity = NRF_UART_PARITY_INCLUDED;
		uart_cfg.paritytype = NRF_UART_PARITYTYPE_ODD;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (baudrate_set(dev, cfg->baudrate) != 0) {
		return -ENOTSUP;
	}

	nrf_uart_configure(uart0_addr, &uart_cfg);

	get_dev_data(dev)->uart_config = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_nrfx_config_get(const struct device *dev,
				struct uart_config *cfg)
{
	*cfg = get_dev_data(dev)->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_0_ASYNC

static void user_callback(const struct device *dev, struct uart_event *event)
{
	if (uart0_cb.callback) {
		uart0_cb.callback(dev, event, uart0_cb.user_data);
	}
}

static int uart_nrfx_callback_set(const struct device *dev,
				  uart_callback_t callback,
				  void *user_data)
{
	uart0_cb.callback = callback;
	uart0_cb.user_data = user_data;

	return 0;
}

static int uart_nrfx_tx(const struct device *dev, const uint8_t *buf,
			size_t len,
			int32_t timeout)
{
	if (atomic_cas((atomic_t *) &uart0_cb.tx_buffer_length,
		       (atomic_val_t) 0,
		       (atomic_val_t) len) == false) {
		return -EBUSY;
	}

	uart0_cb.tx_buffer = buf;
#if	HW_FLOW_CONTROL_AVAILABLE
	uart0_cb.tx_timeout = timeout;
#endif
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_TXDRDY);
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTTX);
	nrf_uart_int_enable(uart0_addr, NRF_UART_INT_MASK_TXDRDY);

	uint8_t txd = uart0_cb.tx_buffer[uart0_cb.tx_counter];

	nrf_uart_txd_set(uart0_addr, txd);

	return 0;
}

static int uart_nrfx_tx_abort(const struct device *dev)
{
	if (uart0_cb.tx_buffer_length == 0) {
		return -EINVAL;
	}
#if	HW_FLOW_CONTROL_AVAILABLE
	if (uart0_cb.tx_timeout != SYS_FOREVER_MS) {
		k_timer_stop(&uart0_cb.tx_timeout_timer);
	}
#endif
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);

	struct uart_event evt = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = uart0_cb.tx_buffer,
		.data.tx.len = uart0_cb.tx_counter
	};

	uart0_cb.tx_buffer_length = 0;
	uart0_cb.tx_counter = 0;

	user_callback(dev, &evt);

	return 0;
}

static int uart_nrfx_rx_enable(const struct device *dev, uint8_t *buf,
			       size_t len,
			       int32_t timeout)
{
	if (!RX_PIN_USED) {
		__ASSERT(false, "TX only UART instance");
		return -ENOTSUP;
	}

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

static int uart_nrfx_rx_buf_rsp(const struct device *dev, uint8_t *buf,
				size_t len)
{
	int err;
	int key = irq_lock();

	if (!uart0_cb.rx_enabled) {
		err = -EACCES;
	} else if (uart0_cb.rx_secondary_buffer_length != 0) {
		err = -EBUSY;
	} else {
		uart0_cb.rx_secondary_buffer = buf;
		uart0_cb.rx_secondary_buffer_length = len;
		err = 0;
	}

	irq_unlock(key);

	return err;
}

static int uart_nrfx_rx_disable(const struct device *dev)
{
	if (uart0_cb.rx_buffer_length == 0) {
		return -EFAULT;
	}

	uart0_cb.rx_enabled = 0;
	if (uart0_cb.rx_timeout != SYS_FOREVER_MS) {
		k_timer_stop(&uart0_cb.rx_timeout_timer);
	}
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPRX);

	return 0;
}

static void rx_rdy_evt(const struct device *dev)
{
	struct uart_event event;
	size_t rx_cnt = uart0_cb.rx_counter;

	event.type = UART_RX_RDY;
	event.data.rx.buf = uart0_cb.rx_buffer;
	event.data.rx.len = rx_cnt - uart0_cb.rx_offset;
	event.data.rx.offset = uart0_cb.rx_offset;

	uart0_cb.rx_offset = rx_cnt;

	user_callback(dev, &event);
}

static void buf_released_evt(const struct device *dev)
{
	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = uart0_cb.rx_buffer
	};
	user_callback(dev, &event);
}

static void rx_disabled_evt(const struct device *dev)
{
	struct uart_event event = {
		.type = UART_RX_DISABLED
	};
	user_callback(dev, &event);
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

static void rx_isr(const struct device *dev)
{
	struct uart_event event;

	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXDRDY);

	if (!uart0_cb.rx_buffer_length || !uart0_cb.rx_enabled) {
		/* Byte received when receiving is disabled - data lost. */
		nrf_uart_rxd_get(uart0_addr);
	} else {
		if (uart0_cb.rx_counter == 0) {
			event.type = UART_RX_BUF_REQUEST;
			user_callback(dev, &event);
		}
		uart0_cb.rx_buffer[uart0_cb.rx_counter] =
			nrf_uart_rxd_get(uart0_addr);
		uart0_cb.rx_counter++;
		if (uart0_cb.rx_timeout == 0) {
			rx_rdy_evt(dev);
		} else if (uart0_cb.rx_timeout != SYS_FOREVER_MS) {
			k_timer_start(&uart0_cb.rx_timeout_timer,
				      K_MSEC(uart0_cb.rx_timeout),
				      K_NO_WAIT);
		}
	}

	if (uart0_cb.rx_buffer_length == uart0_cb.rx_counter) {
		if (uart0_cb.rx_timeout != SYS_FOREVER_MS) {
			k_timer_stop(&uart0_cb.rx_timeout_timer);
		}
		rx_rdy_evt(dev);

		int key = irq_lock();

		if (uart0_cb.rx_secondary_buffer_length == 0) {
			uart0_cb.rx_enabled = 0;
		}
		irq_unlock(key);

		if (uart0_cb.rx_secondary_buffer_length) {
			buf_released_evt(dev);
			/* Switch to secondary buffer. */
			uart0_cb.rx_buffer_length =
				uart0_cb.rx_secondary_buffer_length;
			uart0_cb.rx_buffer = uart0_cb.rx_secondary_buffer;
			uart0_cb.rx_secondary_buffer_length = 0;
			uart0_cb.rx_counter = 0;
			uart0_cb.rx_offset = 0;

			event.type = UART_RX_BUF_REQUEST;
			user_callback(dev, &event);
		} else {
			uart_nrfx_rx_disable(dev);
		}
	}
}

static void tx_isr(const struct device *dev)
{
	uart0_cb.tx_counter++;
	if (uart0_cb.tx_counter < uart0_cb.tx_buffer_length &&
	    !uart0_cb.tx_abort) {
#if	HW_FLOW_CONTROL_AVAILABLE
		if (uart0_cb.tx_timeout != SYS_FOREVER_MS) {
			k_timer_start(&uart0_cb.tx_timeout_timer,
				      K_MSEC(uart0_cb.tx_timeout),
				      K_NO_WAIT);
		}
#endif
		nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_TXDRDY);

		uint8_t txd = uart0_cb.tx_buffer[uart0_cb.tx_counter];

		nrf_uart_txd_set(uart0_addr, txd);
	} else {
#if	HW_FLOW_CONTROL_AVAILABLE

		if (uart0_cb.tx_timeout != SYS_FOREVER_MS) {
			k_timer_stop(&uart0_cb.tx_timeout_timer);
		}
#endif
		nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);
		struct uart_event event = {
			.type = UART_TX_DONE,
			.data.tx.buf = uart0_cb.tx_buffer,
			.data.tx.len = uart0_cb.tx_counter
		};
		nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_TXDRDY);
		uart0_cb.tx_buffer_length = 0;
		uart0_cb.tx_counter = 0;
		uart0_cb.tx_buffer = NULL;

		nrf_uart_int_disable(uart0_addr, NRF_UART_INT_MASK_TXDRDY);
		user_callback(dev, &event);
	}
}

#define UART_ERROR_FROM_MASK(mask) \
	(mask & NRF_UART_ERROR_OVERRUN_MASK ? UART_ERROR_OVERRUN	\
	 : mask & NRF_UART_ERROR_PARITY_MASK ? UART_ERROR_PARITY	\
	 : mask & NRF_UART_ERROR_FRAMING_MASK ? UART_ERROR_FRAMING	\
	 : mask & NRF_UART_ERROR_BREAK_MASK ? UART_BREAK		\
	 : 0)

static void error_isr(const struct device *dev)
{
	if (uart0_cb.rx_timeout != SYS_FOREVER_MS) {
		k_timer_stop(&uart0_cb.rx_timeout_timer);
	}
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

	user_callback(dev, &event);
	/* Abort transfer. */
	uart_nrfx_rx_disable(dev);
}

/*
 * In nRF hardware RX timeout can occur only after stopping the peripheral,
 * it is used as a sign that peripheral has finished its operation and is
 * disabled.
 */
static void rxto_isr(const struct device *dev)
{
	nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXTO);

	/* Send rxrdy if there is any data pending. */
	if (uart0_cb.rx_counter - uart0_cb.rx_offset) {
		rx_rdy_evt(dev);
	}

	buf_released_evt(dev);
	if (uart0_cb.rx_secondary_buffer_length) {
		uart0_cb.rx_buffer = uart0_cb.rx_secondary_buffer;
		buf_released_evt(dev);
	}

	rx_reset_state();
	rx_disabled_evt(dev);
}

void uart_nrfx_isr(const struct device *uart)
{
	if (nrf_uart_int_enable_check(uart0_addr, NRF_UART_INT_MASK_ERROR) &&
	    nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_ERROR)) {
		error_isr(uart);
	} else if (nrf_uart_int_enable_check(uart0_addr,
					     NRF_UART_INT_MASK_RXDRDY) &&
		   nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXDRDY)) {
		rx_isr(uart);
	}

	if (nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_TXDRDY)
	    && nrf_uart_int_enable_check(uart0_addr,
					 NRF_UART_INT_MASK_TXDRDY)) {
		tx_isr(uart);
	}

	if (nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXTO)) {
		rxto_isr(uart);
	}
}

static void rx_timeout(struct k_timer *timer)
{
	rx_rdy_evt(DEVICE_DT_GET(DT_DRV_INST(0)));
}

#if HW_FLOW_CONTROL_AVAILABLE
static void tx_timeout(struct k_timer *timer)
{
	struct uart_event evt;

	if (uart0_cb.tx_timeout != SYS_FOREVER_MS) {
		k_timer_stop(&uart0_cb.tx_timeout_timer);
	}
	nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);
	evt.type = UART_TX_ABORTED;
	evt.data.tx.buf = uart0_cb.tx_buffer;
	evt.data.tx.len = uart0_cb.tx_buffer_length;
	uart0_cb.tx_buffer_length = 0;
	uart0_cb.tx_counter = 0;
	user_callback(DEVICE_DT_GET(DT_DRV_INST(0)), &evt);
}
#endif

#endif /* CONFIG_UART_0_ASYNC */


#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN

/** Interrupt driven FIFO fill function */
static int uart_nrfx_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data,
			       int len)
{
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       event_txdrdy_check()) {

		/* Clear the interrupt */
		event_txdrdy_clear();

		/* Send a character */
		nrf_uart_txd_set(uart0_addr, (uint8_t)tx_data[num_tx++]);
	}

	return (int)num_tx;
}

/** Interrupt driven FIFO read function */
static int uart_nrfx_fifo_read(const struct device *dev,
			       uint8_t *rx_data,
			       const int size)
{
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) &&
	       nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXDRDY)) {
		/* Clear the interrupt */
		nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXDRDY);

		/* Receive a character */
		rx_data[num_rx++] = (uint8_t)nrf_uart_rxd_get(uart0_addr);
	}

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uart_nrfx_irq_tx_enable(const struct device *dev)
{
	uint32_t key;

	disable_tx_irq = false;

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
		NVIC_SetPendingIRQ(IRQN);
	}
	irq_unlock(key);
}

/** Interrupt driven transfer disabling function */
static void uart_nrfx_irq_tx_disable(const struct device *dev)
{
	/* Disable TX interrupt in uart_nrfx_isr() when transmission is done. */
	disable_tx_irq = true;
}

/** Interrupt driven receiver enabling function */
static void uart_nrfx_irq_rx_enable(const struct device *dev)
{
	nrf_uart_int_enable(uart0_addr, NRF_UART_INT_MASK_RXDRDY);
}

/** Interrupt driven receiver disabling function */
static void uart_nrfx_irq_rx_disable(const struct device *dev)
{
	nrf_uart_int_disable(uart0_addr, NRF_UART_INT_MASK_RXDRDY);
}

/** Interrupt driven transfer empty function */
static int uart_nrfx_irq_tx_ready_complete(const struct device *dev)
{
	/* Signal TX readiness only when the TX interrupt is enabled and there
	 * is no pending request to disable it. Note that this function may get
	 * called after the TX interrupt is requested to be disabled but before
	 * the disabling is actually performed (in the IRQ handler).
	 */
	return nrf_uart_int_enable_check(uart0_addr,
					 NRF_UART_INT_MASK_TXDRDY) &&
	       !disable_tx_irq &&
	       event_txdrdy_check();
}

/** Interrupt driven receiver ready function */
static int uart_nrfx_irq_rx_ready(const struct device *dev)
{
	return nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_RXDRDY);
}

/** Interrupt driven error enabling function */
static void uart_nrfx_irq_err_enable(const struct device *dev)
{
	nrf_uart_int_enable(uart0_addr, NRF_UART_INT_MASK_ERROR);
}

/** Interrupt driven error disabling function */
static void uart_nrfx_irq_err_disable(const struct device *dev)
{
	nrf_uart_int_disable(uart0_addr, NRF_UART_INT_MASK_ERROR);
}

/** Interrupt driven pending status function */
static int uart_nrfx_irq_is_pending(const struct device *dev)
{
	return ((nrf_uart_int_enable_check(uart0_addr,
					   NRF_UART_INT_MASK_TXDRDY) &&
		 uart_nrfx_irq_tx_ready_complete(dev))
		||
		(nrf_uart_int_enable_check(uart0_addr,
					   NRF_UART_INT_MASK_RXDRDY) &&
		 uart_nrfx_irq_rx_ready(dev)));
}

/** Interrupt driven interrupt update function */
static int uart_nrfx_irq_update(const struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uart_nrfx_irq_callback_set(const struct device *dev,
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
static void uart_nrfx_isr(const struct device *dev)
{
	if (disable_tx_irq &&
	    nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_TXDRDY)) {
		nrf_uart_int_disable(uart0_addr, NRF_UART_INT_MASK_TXDRDY);

		/* Deactivate the transmitter so that it does not needlessly
		 * consume power.
		 */
		nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STOPTX);

		/* The transaction is over. It is okay to enter the deep sleep
		 * mode if needed.
		 */
		device_busy_clear(dev);

		disable_tx_irq = false;

		return;
	}

	if (nrf_uart_event_check(uart0_addr, NRF_UART_EVENT_ERROR)) {
		nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_ERROR);
	}

	if (irq_callback) {
		irq_callback(dev, irq_cb_data);
	}
}
#endif /* CONFIG_UART_0_INTERRUPT_DRIVEN */

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
static int uart_nrfx_init(const struct device *dev)
{
	int err;

	nrf_uart_disable(uart0_addr);

	/* Setting default height state of the TX PIN to avoid glitches
	 * on the line during peripheral activation/deactivation.
	 */
	nrf_gpio_pin_write(TX_PIN, 1);
	nrf_gpio_cfg_output(TX_PIN);

	if (RX_PIN_USED) {
		nrf_gpio_cfg_input(RX_PIN, NRF_GPIO_PIN_PULLUP);
	}

	nrf_uart_txrx_pins_set(uart0_addr, TX_PIN, RX_PIN);

	if (HAS_PROP(rts_pin)) {
		/* Setting default height state of the RTS PIN to avoid glitches
		 * on the line during peripheral activation/deactivation.
		 */
		nrf_gpio_pin_write(RTS_PIN, 1);
		nrf_gpio_cfg_output(RTS_PIN);
	}

	if (HAS_PROP(cts_pin)) {
		nrf_gpio_cfg_input(CTS_PIN, NRF_GPIO_PIN_PULLUP);
	}

	nrf_uart_hwfc_pins_set(uart0_addr, RTS_PIN, CTS_PIN);

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

	if (RX_PIN_USED) {
		nrf_uart_event_clear(uart0_addr, NRF_UART_EVENT_RXDRDY);

		nrf_uart_task_trigger(uart0_addr, NRF_UART_TASK_STARTRX);
	}

#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN
	/* Simulate that the TXDRDY event is set, so that the transmitter status
	 * is indicated correctly.
	 */
	uart_sw_event_txdrdy = 1U;
#endif

#if defined(CONFIG_UART_0_ASYNC) || defined(CONFIG_UART_0_INTERRUPT_DRIVEN)

	IRQ_CONNECT(IRQN,
		    IRQ_PRIO,
		    uart_nrfx_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)),
		    0);
	irq_enable(IRQN);
#endif

#ifdef CONFIG_UART_0_ASYNC
	k_timer_init(&uart0_cb.rx_timeout_timer, rx_timeout, NULL);
#if HW_FLOW_CONTROL_AVAILABLE
	k_timer_init(&uart0_cb.tx_timeout_timer, tx_timeout, NULL);
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
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure        = uart_nrfx_configure,
	.config_get       = uart_nrfx_config_get,
#endif
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

#ifdef CONFIG_PM_DEVICE

static void uart_nrfx_pins_enable(const struct device *dev, bool enable)
{
	if (!IS_ENABLED(CONFIG_UART_0_GPIO_MANAGEMENT)) {
		return;
	}

	uint32_t tx_pin = nrf_uart_tx_pin_get(uart0_addr);
	uint32_t rx_pin = nrf_uart_rx_pin_get(uart0_addr);
	uint32_t cts_pin = nrf_uart_cts_pin_get(uart0_addr);
	uint32_t rts_pin = nrf_uart_rts_pin_get(uart0_addr);

	if (enable) {
		nrf_gpio_pin_write(tx_pin, 1);
		nrf_gpio_cfg_output(tx_pin);
		if (RX_PIN_USED) {
			nrf_gpio_cfg_input(rx_pin, NRF_GPIO_PIN_PULLUP);
		}

		if (HAS_PROP(rts_pin)) {
			nrf_gpio_pin_write(rts_pin, 1);
			nrf_gpio_cfg_output(rts_pin);
		}
		if (HAS_PROP(cts_pin)) {
			nrf_gpio_cfg_input(cts_pin, NRF_GPIO_PIN_PULLUP);
		}
	} else {
		nrf_gpio_cfg_default(tx_pin);
		if (RX_PIN_USED) {
			nrf_gpio_cfg_default(rx_pin);
		}

		if (HAS_PROP(rts_pin)) {
			nrf_gpio_cfg_default(rts_pin);
		}

		if (HAS_PROP(cts_pin)) {
			nrf_gpio_cfg_default(cts_pin);
		}
	}
}

static void uart_nrfx_set_power_state(const struct device *dev,
				      enum pm_device_state new_state)
{
	if (new_state == PM_DEVICE_STATE_ACTIVE) {
		uart_nrfx_pins_enable(dev, true);
		nrf_uart_enable(uart0_addr);
		if (RX_PIN_USED) {
			nrf_uart_task_trigger(uart0_addr,
					      NRF_UART_TASK_STARTRX);
		}
	} else {
		__ASSERT_NO_MSG(new_state == PM_DEVICE_STATE_LOW_POWER ||
				new_state == PM_DEVICE_STATE_SUSPEND ||
				new_state == PM_DEVICE_STATE_OFF);
		nrf_uart_disable(uart0_addr);
		uart_nrfx_pins_enable(dev, false);
	}
}

static int uart_nrfx_pm_control(const struct device *dev,
				uint32_t ctrl_command,
				enum pm_device_state *state)
{
	static enum pm_device_state current_state = PM_DEVICE_STATE_ACTIVE;

	if (ctrl_command == PM_DEVICE_STATE_SET) {
		enum pm_device_state new_state = *state;

		if (new_state != current_state) {
			uart_nrfx_set_power_state(dev, new_state);
			current_state = new_state;
		}
	} else {
		__ASSERT_NO_MSG(ctrl_command == PM_DEVICE_STATE_GET);
		*state = current_state;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static struct uart_nrfx_data uart_nrfx_uart0_data = {
	.uart_config = {
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.baudrate  = BAUDRATE,
#ifdef CONFIG_UART_0_NRF_PARITY_BIT
		.parity    = UART_CFG_PARITY_EVEN,
#else
		.parity    = UART_CFG_PARITY_NONE,
#endif /* CONFIG_UART_0_NRF_PARITY_BIT */
		.flow_ctrl = PROP(hw_flow_control) ?
			UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE,
	}
};

DEVICE_DT_INST_DEFINE(0,
	      uart_nrfx_init,
	      uart_nrfx_pm_control,
	      &uart_nrfx_uart0_data,
	      NULL,
	      /* Initialize UART device before UART console. */
	      PRE_KERNEL_1,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &uart_nrfx_uart_driver_api);
