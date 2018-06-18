/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF5X UART
 */

#include <uart.h>
#include <gpio.h>
#include <hal/nrf_uart.h>


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static uart_irq_callback_t m_irq_callback; /**< Callback function pointer */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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
	case 38400:
		nrf_baudrate = NRF_UART_BAUDRATE_38400;
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

	nrf_uart_baudrate_set(NRF_UART0, nrf_baudrate);

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
	if (!nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY)) {
		return -1;
	}

	/* Clear the interrupt */
	nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);

	/* got a character */
	*c = nrf_uart_rxd_get(NRF_UART0);

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UART device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static unsigned char uart_nrfx_poll_out(struct device *dev,
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

	/* reset transmitter ready state */
	nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_TXDRDY);

	/* send a character */
	nrf_uart_txd_set(NRF_UART0, (u8_t)c);

	/* Wait for transmitter to be ready */
	while (!nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_TXDRDY)) {
	}

	return c;
}

/** Console I/O function */
static int uart_nrfx_err_check(struct device *dev)
{
	u32_t error = 0;

	if (nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_ERROR)) {
		/* register bitfields maps to the defines in uart.h */
		error = nrf_uart_errorsrc_get_and_clear(NRF_UART0);
	}

	return error;
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/** Interrupt driven FIFO fill function */
static int uart_nrfx_fifo_fill(struct device *dev,
			       const u8_t *tx_data,
			       int len)
{
	u8_t num_tx = 0;

	while ((len - num_tx > 0) &&
	       nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_TXDRDY)) {
		/* Clear the interrupt */
		nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_TXDRDY);

		/* Send a character */
		nrf_uart_txd_set(NRF_UART0, (u8_t)tx_data[num_tx++]);
	}

	return (int)num_tx;
}

/** Interrupt driven FIFO read function */
static int uart_nrfx_fifo_read(struct device *dev,
			       u8_t *rx_data,
			       const int size)
{
	u8_t num_rx = 0;

	while ((size - num_rx > 0) &&
	       nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY)) {
		/* Clear the interrupt */
		nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);

		/* Receive a character */
		rx_data[num_rx++] = (u8_t)nrf_uart_rxd_get(NRF_UART0);
	}

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uart_nrfx_irq_tx_enable(struct device *dev)
{
	nrf_uart_int_enable(NRF_UART0, NRF_UART_INT_MASK_TXDRDY);
}

/** Interrupt driven transfer disabling function */
static void uart_nrfx_irq_tx_disable(struct device *dev)
{
	nrf_uart_int_disable(NRF_UART0, NRF_UART_INT_MASK_TXDRDY);
}

/** Interrupt driven transfer ready function */
static int uart_nrfx_irq_tx_ready(struct device *dev)
{
	return nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_TXDRDY);
}

/** Interrupt driven receiver enabling function */
static void uart_nrfx_irq_rx_enable(struct device *dev)
{
	nrf_uart_int_enable(NRF_UART0, NRF_UART_INT_MASK_RXDRDY);
}

/** Interrupt driven receiver disabling function */
static void uart_nrfx_irq_rx_disable(struct device *dev)
{
	nrf_uart_int_disable(NRF_UART0, NRF_UART_INT_MASK_RXDRDY);
}

/** Interrupt driven transfer empty function */
static int uart_nrfx_irq_tx_complete(struct device *dev)
{
	return !nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_TXDRDY);
}

/** Interrupt driven receiver ready function */
static int uart_nrfx_irq_rx_ready(struct device *dev)
{
	return nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY);
}

/** Interrupt driven error enabling function */
static void uart_nrfx_irq_err_enable(struct device *dev)
{
	nrf_uart_int_enable(NRF_UART0, NRF_UART_INT_MASK_ERROR);
}

/** Interrupt driven error disabling function */
static void uart_nrfx_irq_err_disable(struct device *dev)
{
	nrf_uart_int_disable(NRF_UART0, NRF_UART_INT_MASK_ERROR);
}



/** Interrupt driven pending status function */
static int uart_nrfx_irq_is_pending(struct device *dev)
{
	return ((nrf_uart_int_enable_check(NRF_UART0,
					   NRF_UART_INT_MASK_TXDRDY) &&
		uart_nrfx_irq_tx_ready(dev))
		||
		(nrf_uart_int_enable_check(NRF_UART0,
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
				       uart_irq_callback_t cb)
{
	(void)dev;
	m_irq_callback = cb;
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
	struct device *dev = arg;

	if (m_irq_callback) {
		m_irq_callback(dev);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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
	struct device *gpio_dev;
	int err;

	gpio_dev = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	__ASSERT(gpio_dev,
		 "UART init failed. Cannot find %s",
		 CONFIG_GPIO_NRF5_P0_DEV_NAME);

	/* Setting default height state of the TX PIN to avoid glitches
	 * on the line during peripheral activation/deactivation.
	 */
	gpio_pin_write(gpio_dev, CONFIG_UART_0_NRF_TX_PIN, 1);
	gpio_pin_configure(gpio_dev,
			   CONFIG_UART_0_NRF_TX_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_configure(gpio_dev,
			   CONFIG_UART_0_NRF_RX_PIN,
			   GPIO_DIR_IN);

	nrf_uart_txrx_pins_set(NRF_UART0,
			       CONFIG_UART_0_NRF_TX_PIN,
			       CONFIG_UART_0_NRF_RX_PIN);

#ifdef CONFIG_UART_0_NRF_FLOW_CONTROL
	/* Setting default height state of the RTS PIN to avoid glitches
	 * on the line during peripheral activation/deactivation.
	 */
	gpio_pin_write(gpio_dev, CONFIG_UART_0_NRF_RTS_PIN, 1);
	gpio_pin_configure(gpio_dev,
			   CONFIG_UART_0_NRF_RTS_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_configure(gpio_dev,
			   CONFIG_UART_0_NRF_CTS_PIN,
			   GPIO_DIR_IN);
	nrf_uart_hwfc_pins_set(NRF_UART0,
			       CONFIG_UART_0_NRF_RTS_PIN,
			       CONFIG_UART_0_NRF_CTS_PIN);
#endif /* CONFIG_UART_0_NRF_FLOW_CONTROL */

	nrf_uart_configure(NRF_UART0,
#ifdef CONFIG_UART_0_NRF_PARITY_BIT
			   NRF_UART_PARITY_INCLUDED,
#else
			   NRF_UART_PARITY_EXCLUDED,
#endif /* CONFIG_UART_0_NRF_PARITY_BIT */
#ifdef CONFIG_UART_0_NRF_FLOW_CONTROL
			   NRF_UART_HWFC_ENABLED);
#else
			   NRF_UART_HWFC_DISABLED);
#endif /* CONFIG_UART_0_NRF_PARITY_BIT */

	/* Set baud rate */
	err = baudrate_set(dev, CONFIG_UART_0_BAUD_RATE);
	if (err) {
		return err;
	}

	/* Enable receiver and transmitter */
	nrf_uart_enable(NRF_UART0);

	nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_TXDRDY);
	nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);

	nrf_uart_task_trigger(NRF_UART0, NRF_UART_TASK_STARTTX);
	nrf_uart_task_trigger(NRF_UART0, NRF_UART_TASK_STARTRX);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_UART0),
		    CONFIG_UART_0_IRQ_PRI,
		    uart_nrfx_isr,
		    DEVICE_GET(uart_nrfx_uart0),
		    0);
	irq_enable(NRFX_IRQ_NUMBER_GET(NRF_UART0));
#endif

	return 0;
}

static const struct uart_driver_api uart_nrfx_uart_driver_api = {
	.poll_in          = uart_nrfx_poll_in,          /** Console I/O function */
	.poll_out         = uart_nrfx_poll_out,         /** Console I/O function */
	.err_check        = uart_nrfx_err_check,        /** Console I/O function */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = uart_nrfx_fifo_fill,        /** IRQ FIFO fill function */
	.fifo_read        = uart_nrfx_fifo_read,        /** IRQ FIFO read function */
	.irq_tx_enable    = uart_nrfx_irq_tx_enable,    /** IRQ transfer enabling function */
	.irq_tx_disable   = uart_nrfx_irq_tx_disable,   /** IRQ transfer disabling function */
	.irq_tx_ready     = uart_nrfx_irq_tx_ready,     /** IRQ transfer ready function */
	.irq_rx_enable    = uart_nrfx_irq_rx_enable,    /** IRQ receiver enabling function */
	.irq_rx_disable   = uart_nrfx_irq_rx_disable,   /** IRQ receiver disabling function */
	.irq_tx_complete  = uart_nrfx_irq_tx_complete,  /** IRQ transfer complete function */
	.irq_rx_ready     = uart_nrfx_irq_rx_ready,     /** IRQ receiver ready function */
	.irq_err_enable   = uart_nrfx_irq_err_enable,   /** IRQ error enabling function */
	.irq_err_disable  = uart_nrfx_irq_err_disable,  /** IRQ error disabling function */
	.irq_is_pending   = uart_nrfx_irq_is_pending,   /** IRQ pending status function */
	.irq_update       = uart_nrfx_irq_update,       /** IRQ interrupt update function */
	.irq_callback_set = uart_nrfx_irq_callback_set, /** Set the callback function */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

DEVICE_AND_API_INIT(uart_nrfx_uart0,
		    CONFIG_UART_0_NAME,
		    uart_nrfx_init,
		    NULL,
		    NULL,
		    /* Initialize UART device before UART console. */
		    PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_nrfx_uart_driver_api);

