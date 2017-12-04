/*
 * Copyright (c) 2017 Byteflies, Inc. (Bertold Van den Bergh)
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF52 UARTE
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

/* UART structure for nRF5X.
 * More detailed description of each register can be found in nrf5X.h
 */
struct _uart {
	__O u32_t  TASKS_STARTRX;
	__O u32_t  TASKS_STOPRX;
	__O u32_t  TASKS_STARTTX;
	__O u32_t  TASKS_STOPTX;

	__I u32_t  RESERVED0[7];
	__O u32_t  TASKS_FLUSHRX;

	__I u32_t  RESERVED1[52];
	__IO u32_t EVENTS_CTS;
	__IO u32_t EVENTS_NCTS;
	__IO u32_t EVENTS_RXDRDY;

	__I u32_t  RESERVED2[1];
	__IO u32_t EVENTS_ENDRX;

	__I u32_t  RESERVED3[2];
	__IO u32_t EVENTS_TXDRDY;
	__IO u32_t EVENTS_ENDTX;
	__IO u32_t EVENTS_ERROR;

	__I u32_t  RESERVED4[7];
	__IO u32_t EVENTS_RXTO;

	__I u32_t  RESERVED5[1];
	__IO u32_t EVENTS_RXSTARTED;
	__IO u32_t EVENTS_TXSTARTED;

	__I u32_t  RESERVED6[1];
	__IO u32_t EVENTS_TXSTOPPED;

	__I u32_t  RESERVED7[41];
	__IO u32_t SHORTS;

	__I u32_t  RESERVED8[63];
	__IO u32_t INTEN;
	__IO u32_t INTENSET;
	__IO u32_t INTENCLR;

	__I u32_t  RESERVED9[93];
	__IO u32_t ERRORSRC;

	__I u32_t  RESERVED10[31];
	__IO u32_t ENABLE;

	__I u32_t  RESERVED11[1];
	__IO u32_t PSELRTS;
	__IO u32_t PSELTXD;
	__IO u32_t PSELCTS;
	__IO u32_t PSELRXD;

	__I u32_t  RESERVED12[3];
	__IO u32_t BAUDRATE;

	__I u32_t  RESERVED13[3];
	__IO u32_t RXD;
	__IO u32_t RXDMAXCNT;
	__IO u32_t RXDAMOUNT;

	__I u32_t  RESERVED14[1];
	__IO u32_t TXD;
	__IO u32_t TXDMAXCNT;
	__IO u32_t TXDAMOUNT;

	__I u32_t  RESERVED15[7];
	__IO u32_t CONFIG;
};

/* Device data structure */
struct uart_nrf5e_dev_data_t {
	u32_t baud_rate;		/**< Baud rate */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t  cb;  /**< Callback function pointer */
	u8_t fifo_tx_data[CONFIG_UART_NRF5E_TX_DMA_BUFSIZE];
	u8_t fifo_tx_read_index;
	u8_t fifo_tx_write_index;
	volatile u8_t fifo_tx_operating;

	u8_t fifo_rx_data[CONFIG_UART_NRF5E_RX_DMA_BUFSIZE];
	u8_t fifo_rx_read_index;
	u8_t fifo_rx_write_index;
	volatile u8_t fifo_rx_operating;

	u8_t rx_irq_enable;
	u8_t tx_irq_enable;

	volatile u8_t in_interrupt;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_nrf5e_dev_data_t * const)(dev)->driver_data)
#define UART_STRUCT(dev) \
	((volatile struct _uart *)(DEV_CFG(dev))->base)

#define UART_IRQ_MASK_ENDRX	(1 << 4)
#define UART_IRQ_MASK_ENDTX	(1 << 8)
#define UART_IRQ_MASK_ERROR	(1 << 9)

static const struct uart_driver_api uart_nrf5e_driver_api;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_nrf5e_irq_tx_ready(struct device *dev);
static int uart_nrf5e_fifo_fill(struct device *dev,
			 const u8_t *tx_data, int len);
static int uart_nrf5e_fifo_read(struct device *dev,
			 u8_t *rx_data, const int size);
static void uart_nrf5e_fifo_rx_setup(struct device *dev);
static void uart_nrf5e_pump_tx_fifo(struct device *dev);

static void uart_nrf5e_mask(struct device *dev, int mask)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->in_interrupt) {
		return;
	}

	if (mask) {
		uart->INTENCLR = UART_IRQ_MASK_ENDTX;
		uart->INTENCLR = UART_IRQ_MASK_ENDRX;
	} else {
		uart->INTENSET = UART_IRQ_MASK_ENDTX;
		uart->INTENSET = UART_IRQ_MASK_ENDRX;
	}
}

#endif

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
static int uart_nrf5e_init(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	struct device *gpio_dev;
	int err;

	gpio_dev = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	(void) gpio_pin_configure(gpio_dev,
				  CONFIG_UART_NRF5E_GPIO_TX_PIN,
				  (GPIO_DIR_OUT | GPIO_PUD_PULL_UP));
	(void) gpio_pin_configure(gpio_dev,
				  CONFIG_UART_NRF5E_GPIO_RX_PIN,
				  (GPIO_DIR_IN));

	uart->PSELTXD = CONFIG_UART_NRF5E_GPIO_TX_PIN;
	uart->PSELRXD = CONFIG_UART_NRF5E_GPIO_RX_PIN;

#ifdef CONFIG_UART_NRF5E_FLOW_CONTROL

	(void) gpio_pin_configure(gpio_dev,
				  CONFIG_UART_NRF5E_GPIO_RTS_PIN,
				  (GPIO_DIR_OUT | GPIO_PUD_PULL_UP));
	(void) gpio_pin_configure(gpio_dev,
				  CONFIG_UART_NRF5E_GPIO_CTS_PIN,
				  (GPIO_DIR_IN));

	uart->PSELRTS = CONFIG_UART_NRF5E_GPIO_RTS_PIN;
	uart->PSELCTS = CONFIG_UART_NRF5E_GPIO_CTS_PIN;
	uart->CONFIG = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);

#endif /* CONFIG_UART_NRF5E_FLOW_CONTROL */

	DEV_DATA(dev)->baud_rate = CONFIG_UART_NRF5_BAUD_RATE;

	/* Set baud rate */
	err = baudrate_set(dev, DEV_DATA(dev)->baud_rate,
		  DEV_CFG(dev)->sys_clk_freq);
	if (err) {
		return err;
	}

	/* Enable receiver and transmitter */
	uart->ENABLE = (UARTE_ENABLE_ENABLE_Enabled << UARTE_ENABLE_ENABLE_Pos);

	uart->EVENTS_ENDTX = 0;
	uart->EVENTS_ENDRX = 0;
	uart->EVENTS_RXDRDY = 0;

	dev->driver_api = &uart_nrf5e_driver_api;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	DEV_CFG(dev)->irq_config_func(dev);
	uart_nrf5e_mask(dev, 0);
	uart_nrf5e_fifo_rx_setup(dev);
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

static int uart_nrf5e_poll_in(struct device *dev, unsigned char *c)
{

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
	volatile struct _uart *uart = UART_STRUCT(dev);

	if (!uart->EVENTS_RXDRDY) {
		return -1;
	}

	uart->EVENTS_RXDRDY = 0;

	do {
		uart->RXD = (u32_t)&c;
		uart->RXDMAXCNT = 1;
		uart->TASKS_STARTRX = 1;

		while (!uart->EVENTS_ENDRX) {
		}

		uart->EVENTS_ENDRX = 0;
	} while (uart->RXDAMOUNT == 0);

	return 0;

#else
	if (uart_nrf5e_fifo_read(dev, c, 1)) {
		return 0;
	}
	return -1;
#endif
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

static unsigned char uart_nrf5e_poll_out(struct device *dev,
					unsigned char c)
{
#ifndef CONFIG_UART_INTERRUPT_DRIVEN
	volatile struct _uart *uart = UART_STRUCT(dev);

	do {
		uart->TXD = (u32_t)&c;
		uart->TXDMAXCNT = 1;
		uart->TASKS_STARTTX = 1;

		while (!uart->EVENTS_ENDTX) {
		}

		uart->EVENTS_ENDTX = 0;
	} while (uart->TXDAMOUNT == 0);

#else
	/* If the queue is full we need to pump it to make room */
	uart_nrf5e_mask(dev, 1);
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	while (!uart_nrf5e_fifo_fill(dev, &c, 1)) {
		if (dev_data->in_interrupt) {
			uart_nrf5e_pump_tx_fifo(dev);
		}
	}

#endif

	return c;
}

/** Console I/O function */
static int uart_nrf5e_err_check(struct device *dev)
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

static void uart_nrf5e_fifo_tx_start(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);
	volatile struct _uart *uart = UART_STRUCT(dev);

	if (dev_data->fifo_tx_operating) {
		return;
	}

	if (dev_data->fifo_tx_read_index == dev_data->fifo_tx_write_index) {
		return;
	}

	dev_data->fifo_tx_operating = 1;

	uart->TXD =
		(u32_t)&dev_data->fifo_tx_data[dev_data->fifo_tx_read_index];

	u8_t tx_size = 0;

	if (dev_data->fifo_tx_write_index < dev_data->fifo_tx_read_index) {
		/* We can go till the end of the buffer */
		tx_size = sizeof(dev_data->fifo_tx_data) -
			  dev_data->fifo_tx_read_index;
	} else {
		/* We can read as many bytes as needed to
		 * make read index equal to write index
		 */
		tx_size = dev_data->fifo_tx_write_index -
			  dev_data->fifo_tx_read_index;
	}

	/* Limit the blocklength */
	if (tx_size > CONFIG_UART_NRF5E_TX_DMA_MAXBLOCK) {
		tx_size = CONFIG_UART_NRF5E_TX_DMA_MAXBLOCK;
	}

	/* And go */
	uart->TXDMAXCNT = tx_size;
	uart->TASKS_STARTTX = 1;
}

/** Interrupt driven FIFO fill function */
static int uart_nrf5e_fifo_fill(struct device *dev,
			 const u8_t *tx_data, int len)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);
	u8_t num_tx = 0;

	if (!tx_data) {
		return 0;
	}

	uart_nrf5e_mask(dev, 1);

	while (len - num_tx) {
		u8_t new_index = dev_data->fifo_tx_write_index + 1;

		if (new_index >= sizeof(dev_data->fifo_tx_data)) {
			new_index = 0;
		}

		/* Is the fifo full */
		if (new_index == dev_data->fifo_tx_read_index) {
			break;
		}

		dev_data->fifo_tx_data[dev_data->fifo_tx_write_index] =
								*tx_data;
		dev_data->fifo_tx_write_index = new_index;

		tx_data++;
		num_tx++;
	}

	if (num_tx) {
		uart_nrf5e_fifo_tx_start(dev);
	}

	uart_nrf5e_mask(dev, 0);

	return (int)num_tx;
}

/** Interrupt driven FIFO read function */
static int uart_nrf5e_fifo_read(struct device *dev, u8_t *rx_data, int size)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);
	u8_t num_rx = 0;

	uart_nrf5e_mask(dev, 1);

	while (size - num_rx) {
		if (dev_data->fifo_rx_write_index ==
				dev_data->fifo_rx_read_index) {
			break;
		}

		if (rx_data) {
			*rx_data = dev_data->fifo_rx_data
				[dev_data->fifo_rx_read_index];
			rx_data++;
		}

		num_rx++;

		u8_t tmp = dev_data->fifo_rx_read_index + 1;

		if (tmp >= sizeof(dev_data->fifo_rx_data)) {
			tmp = 0;
		}

		dev_data->fifo_rx_read_index = tmp;
	}

	if (num_rx) {
		uart_nrf5e_fifo_rx_setup(dev);
	}

	uart_nrf5e_mask(dev, 0);

	return num_rx;
}


static void uart_nrf5e_fifo_rx_setup(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);
	volatile struct _uart *uart = UART_STRUCT(dev);
	u8_t len = 0;

	if (dev_data->fifo_rx_operating) {
		return;
	}

	/* How many bytes can we write? */
	if (dev_data->fifo_rx_write_index < dev_data->fifo_rx_read_index) {
		len = dev_data->fifo_rx_read_index -
			dev_data->fifo_rx_write_index - 1;
	} else {
		len = sizeof(dev_data->fifo_rx_data) -
			dev_data->fifo_rx_write_index;
		if (dev_data->fifo_rx_read_index == 0) {
			len--;
		}
	}

	if (!len) {
		return;
	}

	if (len > CONFIG_UART_NRF5E_RX_DMA_MAXBLOCK) {
		len = CONFIG_UART_NRF5E_RX_DMA_MAXBLOCK;
	}

	dev_data->fifo_rx_operating = 1;

	uart->RXD = (u32_t)&dev_data->fifo_rx_data
			[dev_data->fifo_rx_write_index];
	uart->RXDMAXCNT = len;
	uart->TASKS_STARTRX = 1;
}

/** Interrupt driven transfer ready function */
static int uart_nrf5e_irq_tx_ready(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);
	int ret = 1;

	uart_nrf5e_mask(dev, 1);

	/* Check if the TX FIFO is full */
	u8_t tmp = dev_data->fifo_tx_write_index + 1;

	if (tmp >= sizeof(dev_data->fifo_tx_data)) {
		tmp -= sizeof(dev_data->fifo_tx_data);
	}

	if (tmp == dev_data->fifo_tx_read_index) {
		ret = 0;
	}

	uart_nrf5e_mask(dev, 0);

	return ret;
}


/** Interrupt driven transfer enabling function */
static void uart_nrf5e_irq_tx_enable(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->tx_irq_enable = 1;
	if (uart_nrf5e_irq_tx_ready(dev)) {
		if (dev_data->cb) {
			dev_data->cb(dev);
		}
	}
}

/** Interrupt driven transfer disabling function */
static void uart_nrf5e_irq_tx_disable(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->tx_irq_enable = 0;
}


/** Interrupt driven transfer empty function */
static int uart_nrf5e_irq_tx_complete(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	return dev_data->fifo_tx_operating;
}

/** Interrupt driven receiver ready function */
static int uart_nrf5e_irq_rx_ready(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	int ret = 1;

	uart_nrf5e_mask(dev, 1);

	/* Check if the RX FIFO is empty */
	if (dev_data->fifo_rx_read_index == dev_data->fifo_rx_write_index) {
		ret = 0;
	}

	uart_nrf5e_mask(dev, 0);

	return ret;
}

/** Interrupt driven receiver enabling function */
static void uart_nrf5e_irq_rx_enable(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->rx_irq_enable = 1;
	if (uart_nrf5e_irq_rx_ready(dev)) {
		if (dev_data->cb) {
			dev_data->cb(dev);
		}
	}

}

/** Interrupt driven receiver disabling function */
static void uart_nrf5e_irq_rx_disable(struct device *dev)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->rx_irq_enable = 0;
}


/** Interrupt driven error enabling function */
static void uart_nrf5e_irq_err_enable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->INTENSET |= UART_IRQ_MASK_ERROR;
}

/** Interrupt driven error disabling function */
static void uart_nrf5e_irq_err_disable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->INTENCLR |= UART_IRQ_MASK_ERROR;
}

/** Interrupt driven pending status function */
static int uart_nrf5e_irq_is_pending(struct device *dev)
{
	return (uart_nrf5e_irq_tx_ready(dev) || uart_nrf5e_irq_rx_ready(dev));
}

/** Interrupt driven interrupt update function */
static int uart_nrf5e_irq_update(struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uart_nrf5e_irq_callback_set(struct device *dev,
				       uart_irq_callback_t cb)
{
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
}

static void uart_nrf5e_pump_tx_fifo(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	if (uart->EVENTS_ENDTX) {
		uart->EVENTS_ENDTX = 0;
		dev_data->fifo_tx_operating = 0;

		dev_data->fifo_tx_read_index += uart->TXDAMOUNT;
		if (dev_data->fifo_tx_read_index >=
				sizeof(dev_data->fifo_tx_data)) {
			dev_data->fifo_tx_read_index -=
				sizeof(dev_data->fifo_tx_data);
		}

		uart_nrf5e_fifo_tx_start(dev);
	}
}

static void uart_nrf5e_pump_rx_fifo(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	if (uart->EVENTS_ENDRX) {
		uart->EVENTS_ENDRX = 0;
		dev_data->fifo_rx_operating = 0;

		dev_data->fifo_rx_write_index += uart->RXDAMOUNT;
		if (dev_data->fifo_rx_write_index >=
				sizeof(dev_data->fifo_rx_data)) {
			dev_data->fifo_rx_write_index -=
				sizeof(dev_data->fifo_rx_data);
		}

		uart_nrf5e_fifo_rx_setup(dev);
	}
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
void uart_nrf5e_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_nrf5e_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->in_interrupt = 1;

	uart_nrf5e_pump_tx_fifo(dev);
	uart_nrf5e_pump_rx_fifo(dev);

	if (dev_data->cb &&
		((dev_data->rx_irq_enable && uart_nrf5e_irq_rx_ready(dev)) ||
		 (dev_data->tx_irq_enable && uart_nrf5e_irq_tx_ready(dev)))) {
		dev_data->cb(dev);
	}

	dev_data->in_interrupt = 0;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_nrf5e_driver_api = {
	/** Console I/O function */
	.poll_in		= uart_nrf5e_poll_in,
	/** Console I/O function */
	.poll_out		= uart_nrf5e_poll_out,
	/** Console I/O function */
	.err_check		= uart_nrf5e_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/** IRQ FIFO fill function */
	.fifo_fill		= uart_nrf5e_fifo_fill,
	/** IRQ FIFO read function */
	.fifo_read		= uart_nrf5e_fifo_read,
	 /** IRQ transfer enabling function */
	.irq_tx_enable		= uart_nrf5e_irq_tx_enable,
	/** IRQ transfer disabling function */
	.irq_tx_disable		= uart_nrf5e_irq_tx_disable,
	/** IRQ transfer ready function */
	.irq_tx_ready		= uart_nrf5e_irq_tx_ready,
	/** IRQ receiver enabling function */
	.irq_rx_enable		= uart_nrf5e_irq_rx_enable,
	/** IRQ receiver disabling function */
	.irq_rx_disable		= uart_nrf5e_irq_rx_disable,
	/** IRQ transfer complete function */
	.irq_tx_complete	= uart_nrf5e_irq_tx_complete,
	/** IRQ receiver ready function */
	.irq_rx_ready		= uart_nrf5e_irq_rx_ready,
	/** IRQ error enabling function */
	.irq_err_enable		= uart_nrf5e_irq_err_enable,
	/** IRQ error disabling function */
	.irq_err_disable	= uart_nrf5e_irq_err_disable,
	/** IRQ pending status function */
	.irq_is_pending		= uart_nrf5e_irq_is_pending,
	/** IRQ interrupt update function */
	.irq_update		= uart_nrf5e_irq_update,
	/** Set the callback function */
	.irq_callback_set	= uart_nrf5e_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* Forward declare function */
static void uart_nrf5e_irq_config(struct device *port);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_device_config uart_nrf5e_dev_cfg_0 = {
	.base = (u8_t *)NRF_UART0_BASE,
	.sys_clk_freq = CONFIG_UART_NRF5E_CLK_FREQ,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_nrf5e_irq_config,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static struct uart_nrf5e_dev_data_t uart_nrf5e_dev_data_0 = {
	.baud_rate = CONFIG_UART_NRF5_BAUD_RATE,
};

DEVICE_INIT(uart_nrf5e_0, CONFIG_UART_NRF5_NAME, &uart_nrf5e_init,
	 &uart_nrf5e_dev_data_0, &uart_nrf5e_dev_cfg_0,
	 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_nrf5e_irq_config(struct device *port)
{
	IRQ_CONNECT(NRF5_IRQ_UART0_IRQn,
		 CONFIG_UART_NRF5_IRQ_PRI,
		 uart_nrf5e_isr, DEVICE_GET(uart_nrf5e_0),
		 0);
	irq_enable(NRF5_IRQ_UART0_IRQn);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
