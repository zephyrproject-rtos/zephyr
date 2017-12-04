/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF5X UART
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <board.h>
#include <init.h>
#include <uart.h>
#include <linker/sections.h>
#include <gpio.h>

#ifdef CONFIG_SOC_NRF52840
/* Undefine MDK-defined macros */
#ifdef PSELRTS
#undef PSELRTS
#endif
#ifdef PSELCTS
#undef PSELCTS
#endif
#ifdef PSELTXD
#undef PSELTXD
#endif
#ifdef PSELRXD
#undef PSELRXD
#endif
#endif

/* UART structure for nRF5X. More detailed description of each register can be found in nrf5X.h */
struct _uart {
	__O u32_t  TASKS_STARTRX;
	__O u32_t  TASKS_STOPRX;
	__O u32_t  TASKS_STARTTX;
	__O u32_t  TASKS_STOPTX;

	__I u32_t  RESERVED0[3];
	__O u32_t  TASKS_SUSPEND;

	__I u32_t  RESERVED1[56];
	__IO u32_t EVENTS_CTS;
	__IO u32_t EVENTS_NCTS;
	__IO u32_t EVENTS_RXDRDY;

	__I u32_t  RESERVED2[4];
	__IO u32_t EVENTS_TXDRDY;

	__I u32_t  RESERVED3;
	__IO u32_t EVENTS_ERROR;
	__I u32_t  RESERVED4[7];
	__IO u32_t EVENTS_RXTO;
	__I u32_t  RESERVED5[46];
	__IO u32_t SHORTS;
	__I u32_t  RESERVED6[64];
	__IO u32_t INTENSET;
	__IO u32_t INTENCLR;
	__I u32_t  RESERVED7[93];
	__IO u32_t ERRORSRC;
	__I u32_t  RESERVED8[31];
	__IO u32_t ENABLE;
	__I u32_t  RESERVED9;
	__IO u32_t PSELRTS;
	__IO u32_t PSELTXD;
	__IO u32_t PSELCTS;
	__IO u32_t PSELRXD;
	__I u32_t  RXD;
	__O u32_t  TXD;
	__I u32_t  RESERVED10;
	__IO u32_t BAUDRATE;
	__I u32_t  RESERVED11[17];
	__IO u32_t CONFIG;
};

/* Device data structure */
struct uart_nrf5_dev_data_t {
	u32_t baud_rate;	        /**< Baud rate */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t     cb;     /**< Callback function pointer */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_nrf5_dev_data_t * const)(dev)->driver_data)
#define UART_STRUCT(dev) \
	((volatile struct _uart *)(DEV_CFG(dev))->base)

#define UART_IRQ_MASK_RX	(1 << 2)
#define UART_IRQ_MASK_TX	(1 << 7)
#define UART_IRQ_MASK_ERROR	(1 << 9)

static const struct uart_driver_api uart_nrf5_driver_api;

/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UART.
 *
 * @param dev UART device struct
 * @param baudrate Baud rate
 * @param sys_clk_freq_hz System clock frequency in Hz
 *
 * @return N/A
 */

static int baudrate_set(struct device *dev,
			 u32_t baudrate, u32_t sys_clk_freq_hz)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	u32_t divisor; /* baud rate divisor */

	/* Use the common nRF5 macros */
	switch (baudrate) {
	case 300:
		divisor = NRF5_UART_BAUDRATE_300;
		break;
	case 600:
		divisor = NRF5_UART_BAUDRATE_600;
		break;
	case 1200:
		divisor = NRF5_UART_BAUDRATE_1200;
		break;
	case 2400:
		divisor = NRF5_UART_BAUDRATE_2400;
		break;
	case 4800:
		divisor = NRF5_UART_BAUDRATE_4800;
		break;
	case 9600:
		divisor = NRF5_UART_BAUDRATE_9600;
		break;
	case 14400:
		divisor = NRF5_UART_BAUDRATE_14400;
		break;
	case 19200:
		divisor = NRF5_UART_BAUDRATE_19200;
		break;
	case 28800:
		divisor = NRF5_UART_BAUDRATE_28800;
		break;
	case 38400:
		divisor = NRF5_UART_BAUDRATE_38400;
		break;
	case 57600:
		divisor = NRF5_UART_BAUDRATE_57600;
		break;
	case 76800:
		divisor = NRF5_UART_BAUDRATE_76800;
		break;
	case 115200:
		divisor = NRF5_UART_BAUDRATE_115200;
		break;
	case 230400:
		divisor = NRF5_UART_BAUDRATE_230400;
		break;
	case 250000:
		divisor = NRF5_UART_BAUDRATE_250000;
		break;
	case 460800:
		divisor = NRF5_UART_BAUDRATE_460800;
		break;
	case 921600:
		divisor = NRF5_UART_BAUDRATE_921600;
		break;
	case 1000000:
		divisor = NRF5_UART_BAUDRATE_1000000;
		break;
	default:
		return -EINVAL;
	}

	uart->BAUDRATE = divisor << UART_BAUDRATE_BAUDRATE_Pos;

	return 0;
}

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
static int uart_nrf5_init(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	struct device *gpio_dev;
	int err;

	gpio_dev = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	(void) gpio_pin_configure(gpio_dev,
				  CONFIG_UART_NRF5_GPIO_TX_PIN,
				  (GPIO_DIR_OUT | GPIO_PUD_PULL_UP));
	(void) gpio_pin_configure(gpio_dev,
				  CONFIG_UART_NRF5_GPIO_RX_PIN,
				  (GPIO_DIR_IN));

	uart->PSELTXD = CONFIG_UART_NRF5_GPIO_TX_PIN;
	uart->PSELRXD = CONFIG_UART_NRF5_GPIO_RX_PIN;

#ifdef CONFIG_UART_NRF5_FLOW_CONTROL

	(void) gpio_pin_configure(gpio_dev,
				  CONFIG_UART_NRF5_GPIO_RTS_PIN,
				  (GPIO_DIR_OUT | GPIO_PUD_PULL_UP));
	(void) gpio_pin_configure(gpio_dev,
				  CONFIG_UART_NRF5_GPIO_CTS_PIN,
				  (GPIO_DIR_IN));

	uart->PSELRTS = CONFIG_UART_NRF5_GPIO_RTS_PIN;
	uart->PSELCTS = CONFIG_UART_NRF5_GPIO_CTS_PIN;
	uart->CONFIG = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);

#endif /* CONFIG_UART_NRF5_FLOW_CONTROL */

	DEV_DATA(dev)->baud_rate = CONFIG_UART_NRF5_BAUD_RATE;

	/* Set baud rate */
	err = baudrate_set(dev, DEV_DATA(dev)->baud_rate,
		     DEV_CFG(dev)->sys_clk_freq);
	if (err) {
		return err;
	}

	/* Enable receiver and transmitter */
	uart->ENABLE = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);

	uart->EVENTS_TXDRDY = 0;
	uart->EVENTS_RXDRDY = 0;

	uart->TASKS_STARTTX = 1;
	uart->TASKS_STARTRX = 1;

	dev->driver_api = &uart_nrf5_driver_api;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	DEV_CFG(dev)->irq_config_func(dev);
#endif

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

static int uart_nrf5_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	if (!uart->EVENTS_RXDRDY) {
		return -1;
	}

	/* Clear the interrupt */
	uart->EVENTS_RXDRDY = 0;

	/* got a character */
	*c = (unsigned char)uart->RXD;

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * @param dev UART device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static unsigned char uart_nrf5_poll_out(struct device *dev,
					unsigned char c)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	/* send a character */
	uart->TXD = (u8_t)c;

	/* Wait for transmitter to be ready */
	while (!uart->EVENTS_TXDRDY) {
	}

	uart->EVENTS_TXDRDY = 0;

	return c;
}

/** Console I/O function */
static int uart_nrf5_err_check(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	u32_t error = 0;

	if (uart->EVENTS_ERROR) {
		/* register bitfields maps to the defines in uart.h */
		error = uart->ERRORSRC;

		/* Clear the register */
		uart->ERRORSRC = error;
	}

	error = error & 0x0F;

	return error;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/** Interrupt driven FIFO fill function */
static int uart_nrf5_fifo_fill(struct device *dev, const u8_t *tx_data, int len)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	u8_t num_tx = 0;

	while ((len - num_tx > 0) && uart->EVENTS_TXDRDY) {
		/* Clear the interrupt */
		uart->EVENTS_TXDRDY = 0;

		/* Send a character */
		uart->TXD = (u8_t)tx_data[num_tx++];
	}

	return (int)num_tx;
}

/** Interrupt driven FIFO read function */
static int uart_nrf5_fifo_read(struct device *dev, u8_t *rx_data, const int size)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	u8_t num_rx = 0;

	while ((size - num_rx > 0) && uart->EVENTS_RXDRDY) {
		/* Clear the interrupt */
		uart->EVENTS_RXDRDY = 0;

		/* Receive a character */
		rx_data[num_rx++] = (u8_t)uart->RXD;
	}

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uart_nrf5_irq_tx_enable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->INTENSET |= UART_IRQ_MASK_TX;
}

/** Interrupt driven transfer disabling function */
static void uart_nrf5_irq_tx_disable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->INTENCLR |= UART_IRQ_MASK_TX;
}

/** Interrupt driven transfer ready function */
static int uart_nrf5_irq_tx_ready(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	return uart->EVENTS_TXDRDY;
}

/** Interrupt driven receiver enabling function */
static void uart_nrf5_irq_rx_enable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->INTENSET |= UART_IRQ_MASK_RX;
}

/** Interrupt driven receiver disabling function */
static void uart_nrf5_irq_rx_disable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->INTENCLR |= UART_IRQ_MASK_RX;
}

/** Interrupt driven transfer empty function */
static int uart_nrf5_irq_tx_complete(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	return !(uart->EVENTS_TXDRDY);
}

/** Interrupt driven receiver ready function */
static int uart_nrf5_irq_rx_ready(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	return uart->EVENTS_RXDRDY;
}

/** Interrupt driven error enabling function */
static void uart_nrf5_irq_err_enable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->INTENSET |= UART_IRQ_MASK_ERROR;
}

/** Interrupt driven error disabling function */
static void uart_nrf5_irq_err_disable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->INTENCLR |= UART_IRQ_MASK_ERROR;
}

/** Interrupt driven pending status function */
static int uart_nrf5_irq_is_pending(struct device *dev)
{
	return (uart_nrf5_irq_tx_ready(dev) || uart_nrf5_irq_rx_ready(dev));
}

/** Interrupt driven interrupt update function */
static int uart_nrf5_irq_update(struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uart_nrf5_irq_callback_set(struct device *dev, uart_irq_callback_t cb)
{
	struct uart_nrf5_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
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
void uart_nrf5_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_nrf5_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->cb) {
		dev_data->cb(dev);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_nrf5_driver_api = {
	.poll_in          = uart_nrf5_poll_in,          /** Console I/O function */
	.poll_out         = uart_nrf5_poll_out,         /** Console I/O function */
	.err_check        = uart_nrf5_err_check,        /** Console I/O function */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = uart_nrf5_fifo_fill,        /** IRQ FIFO fill function */
	.fifo_read        = uart_nrf5_fifo_read,        /** IRQ FIFO read function */
	.irq_tx_enable    = uart_nrf5_irq_tx_enable,    /** IRQ transfer enabling function */
	.irq_tx_disable   = uart_nrf5_irq_tx_disable,   /** IRQ transfer disabling function */
	.irq_tx_ready     = uart_nrf5_irq_tx_ready,     /** IRQ transfer ready function */
	.irq_rx_enable    = uart_nrf5_irq_rx_enable,    /** IRQ receiver enabling function */
	.irq_rx_disable   = uart_nrf5_irq_rx_disable,   /** IRQ receiver disabling function */
	.irq_tx_complete  = uart_nrf5_irq_tx_complete,  /** IRQ transfer complete function */
	.irq_rx_ready     = uart_nrf5_irq_rx_ready,     /** IRQ receiver ready function */
	.irq_err_enable   = uart_nrf5_irq_err_enable,   /** IRQ error enabling function */
	.irq_err_disable  = uart_nrf5_irq_err_disable,  /** IRQ error disabling function */
	.irq_is_pending   = uart_nrf5_irq_is_pending,   /** IRQ pending status function */
	.irq_update       = uart_nrf5_irq_update,       /** IRQ interrupt update function */
	.irq_callback_set = uart_nrf5_irq_callback_set, /** Set the callback function */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* Forward declare function */
static void uart_nrf5_irq_config(struct device *port);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_device_config uart_nrf5_dev_cfg_0 = {
	.base = (u8_t *)NRF_UART0_BASE,
	.sys_clk_freq = CONFIG_UART_NRF5_CLK_FREQ,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_nrf5_irq_config,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static struct uart_nrf5_dev_data_t uart_nrf5_dev_data_0 = {
	.baud_rate = CONFIG_UART_NRF5_BAUD_RATE,
};

DEVICE_INIT(uart_nrf5_0, CONFIG_UART_NRF5_NAME, &uart_nrf5_init,
	    &uart_nrf5_dev_data_0, &uart_nrf5_dev_cfg_0,
	    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_nrf5_irq_config(struct device *port)
{
	IRQ_CONNECT(NRF5_IRQ_UART0_IRQn,
		    CONFIG_UART_NRF5_IRQ_PRI,
		    uart_nrf5_isr, DEVICE_GET(uart_nrf5_0),
		    0);
	irq_enable(NRF5_IRQ_UART0_IRQn);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
