/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF UARTE
 */

#include <uart.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_uarte.h>

#if (defined(CONFIG_UART_0_NRF_UARTE) &&         \
     defined(CONFIG_UART_0_INTERRUPT_DRIVEN)) || \
    (defined(CONFIG_UART_1_NRF_UARTE) &&         \
     defined(CONFIG_UART_1_INTERRUPT_DRIVEN))
	#define UARTE_INTERRUPT_DRIVEN	(1u)
#endif


/* Device data structure */
struct uarte_nrfx_data {
	u8_t rx_data;

#ifdef UARTE_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
	void *cb_data; /**< Callback function arg */
	u8_t *tx_buffer;
#endif /* UARTE_INTERRUPT_DRIVEN */
};

/**
 * @brief Structure for UARTE configuration.
 */
struct uarte_nrfx_config {
	NRF_UARTE_Type *uarte_regs; /* Instance address */
#ifdef UARTE_INTERRUPT_DRIVEN
	u16_t tx_buff_size;
#endif /* UARTE_INTERRUPT_DRIVEN */
};

struct uarte_init_config {
	u32_t  pseltxd; /* TXD pin number */
	u32_t  pselrxd; /* RXD pin number */
	u32_t  pselcts; /* CTS pin number */
	u32_t  pselrts; /* RTS pin number */

	nrf_uarte_hwfc_t      hwfc;     /* Flow control configuration */
	nrf_uarte_parity_t    parity;   /* Parity configuration */
	nrf_uarte_baudrate_t  baudrate;
};

static inline struct uarte_nrfx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct uarte_nrfx_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static inline NRF_UARTE_Type *get_uarte_instance(struct device *dev)
{
	const struct uarte_nrfx_config *config = get_dev_config(dev);

	return config->uarte_regs;
}

#ifdef UARTE_INTERRUPT_DRIVEN
/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void uarte_nrfx_isr(void *arg)
{
	struct device *dev = arg;
	const struct uarte_nrfx_data *data = get_dev_data(dev);

	if (data->cb) {
		data->cb(data->cb_data);
	}
}
#endif /* UARTE_INTERRUPT_DRIVEN */

/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UARTE.
 *
 * @param dev UARTE device struct
 * @param baudrate Baud rate
 *
 * @return 0 on success or error code
 */
static int baudrate_set(struct device *dev, u32_t baudrate)
{
	nrf_uarte_baudrate_t nrf_baudrate; /* calculated baudrate divisor */
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

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
		nrf_baudrate = NRF_UARTE_BAUDRATE_1200;
		break;
	case 2400:
		nrf_baudrate = NRF_UARTE_BAUDRATE_2400;
		break;
	case 4800:
		nrf_baudrate = NRF_UARTE_BAUDRATE_4800;
		break;
	case 9600:
		nrf_baudrate = NRF_UARTE_BAUDRATE_9600;
		break;
	case 14400:
		nrf_baudrate = NRF_UARTE_BAUDRATE_14400;
		break;
	case 19200:
		nrf_baudrate = NRF_UARTE_BAUDRATE_19200;
		break;
	case 28800:
		nrf_baudrate = NRF_UARTE_BAUDRATE_28800;
		break;
	case 31250:
		nrf_baudrate = NRF_UARTE_BAUDRATE_31250;
		break;
	case 38400:
		nrf_baudrate = NRF_UARTE_BAUDRATE_38400;
		break;
	case 56000:
		nrf_baudrate = NRF_UARTE_BAUDRATE_56000;
		break;
	case 57600:
		nrf_baudrate = NRF_UARTE_BAUDRATE_57600;
		break;
	case 76800:
		nrf_baudrate = NRF_UARTE_BAUDRATE_76800;
		break;
	case 115200:
		nrf_baudrate = NRF_UARTE_BAUDRATE_115200;
		break;
	case 230400:
		nrf_baudrate = NRF_UARTE_BAUDRATE_230400;
		break;
	case 250000:
		nrf_baudrate = NRF_UARTE_BAUDRATE_250000;
		break;
	case 460800:
		nrf_baudrate = NRF_UARTE_BAUDRATE_460800;
		break;
	case 921600:
		nrf_baudrate = NRF_UARTE_BAUDRATE_921600;
		break;
	case 1000000:
		nrf_baudrate = NRF_UARTE_BAUDRATE_1000000;
		break;
	default:
		return -EINVAL;
	}

	nrf_uarte_baudrate_set(uarte, nrf_baudrate);

	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UARTE device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer is empty.
 */
static int uarte_nrfx_poll_in(struct device *dev, unsigned char *c)
{
	const struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		return -1;
	}

	*c = data->rx_data;

	/* clear the interrupt */
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UARTE device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static unsigned char uarte_nrfx_poll_out(struct device *dev,
					 unsigned char c)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	/* The UART API dictates that poll_out should wait for the transmitter
	 * to be empty before sending a character. However, the only way of
	 * telling if the transmitter became empty in the UARTE peripheral is
	 * to check if the ENDTX event for the previous transmission was set.
	 * Since this event is not cleared automatically when a new transmission
	 * is started, it must be cleared in software, and this leads to a rare
	 * yet possible race condition if the thread is preempted right after
	 * clearing the event but before sending a new character. The preempting
	 * thread, if it also called poll_out, would then wait for the ENDTX
	 * event that had no chance to become set.

	 * Because of this race condition, the while loop has to be placed
	 * after the write to TXD, and we can't wait for an empty transmitter
	 * before writing. This is a trade-off between losing a byte once in a
	 * blue moon against hanging up the whole thread permanently
	 */

	/* reset transmitter ready state */
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);

	/* send a character */
	nrf_uarte_tx_buffer_set(uarte, &c, 1);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);

	/* Wait for transmitter to be ready */
	while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
	}

	/* If there is nothing to send, driver will save an energy
	 * when TX is stopped.
	 */
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);

	return c;
}

static int uarte_nrfx_err_check(struct device *dev)
{
	u32_t error = 0;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ERROR)) {
		/* register bitfields maps to the defines in uart.h */
		error = nrf_uarte_errorsrc_get_and_clear(uarte);
	}

	return error;
}

#ifdef UARTE_INTERRUPT_DRIVEN
/** Interrupt driven FIFO fill function */
static int uarte_nrfx_fifo_fill(struct device *dev,
				const u8_t *tx_data,
				int len)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);
	const struct uarte_nrfx_config *config = get_dev_config(dev);


	if (len > config->tx_buff_size) {
		len = config->tx_buff_size;
	}

	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);

	/* Copy data to RAM buffer for EasyDMA transfer */
	for (int i = 0; i < len; i++) {
		data->tx_buffer[i] = tx_data[i];
	}

	nrf_uarte_tx_buffer_set(uarte, data->tx_buffer, len);

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);

	return len;
}

/** Interrupt driven FIFO read function */
static int uarte_nrfx_fifo_read(struct device *dev,
				u8_t *rx_data,
				const int size)
{
	int num_rx = 0;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_data *data = get_dev_data(dev);

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		/* Clear the interrupt */
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

		/* Receive a character */
		rx_data[num_rx++] = (u8_t)data->rx_data;

		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
	}

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uarte_nrfx_irq_tx_enable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDTX_MASK);
}

/** Interrupt driven transfer disabling function */
static void uarte_nrfx_irq_tx_disable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDTX_MASK);

	/* If there is nothing to send, driver will save an energy
	 * when TX is stopped.
	 */
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
}

/** Interrupt driven transfer ready function */
static int uarte_nrfx_irq_tx_ready_complete(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX);
}

static int uarte_nrfx_irq_rx_ready(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX);
}

/** Interrupt driven receiver enabling function */
static void uarte_nrfx_irq_rx_enable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDRX_MASK);
}

/** Interrupt driven receiver disabling function */
static void uarte_nrfx_irq_rx_disable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDRX_MASK);
}

/** Interrupt driven error enabling function */
static void uarte_nrfx_irq_err_enable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ERROR_MASK);
}

/** Interrupt driven error disabling function */
static void uarte_nrfx_irq_err_disable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ERROR_MASK);
}

/** Interrupt driven pending status function */
static int uarte_nrfx_irq_is_pending(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return ((nrf_uarte_int_enable_check(uarte,
					    NRF_UARTE_INT_ENDTX_MASK) &&
		 uarte_nrfx_irq_tx_ready_complete(dev))
		||
		(nrf_uarte_int_enable_check(uarte,
					    NRF_UARTE_INT_ENDRX_MASK) &&
		 uarte_nrfx_irq_rx_ready(dev)));
}

/** Interrupt driven interrupt update function */
static int uarte_nrfx_irq_update(struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uarte_nrfx_irq_callback_set(struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	data->cb = cb;
	data->cb_data = cb_data;
}
#endif /* UARTE_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_nrfx_uarte_driver_api = {
	.poll_in		= uarte_nrfx_poll_in,
	.poll_out		= uarte_nrfx_poll_out,
	.err_check		= uarte_nrfx_err_check,
#ifdef UARTE_INTERRUPT_DRIVEN
	.fifo_fill		= uarte_nrfx_fifo_fill,
	.fifo_read		= uarte_nrfx_fifo_read,
	.irq_tx_enable		= uarte_nrfx_irq_tx_enable,
	.irq_tx_disable		= uarte_nrfx_irq_tx_disable,
	.irq_tx_ready		= uarte_nrfx_irq_tx_ready_complete,
	.irq_rx_enable		= uarte_nrfx_irq_rx_enable,
	.irq_rx_disable		= uarte_nrfx_irq_rx_disable,
	.irq_tx_complete	= uarte_nrfx_irq_tx_ready_complete,
	.irq_rx_ready		= uarte_nrfx_irq_rx_ready,
	.irq_err_enable		= uarte_nrfx_irq_err_enable,
	.irq_err_disable	= uarte_nrfx_irq_err_disable,
	.irq_is_pending		= uarte_nrfx_irq_is_pending,
	.irq_update		= uarte_nrfx_irq_update,
	.irq_callback_set	= uarte_nrfx_irq_callback_set,
#endif /* UARTE_INTERRUPT_DRIVEN */
};

static int uarte_instance_init(struct device *dev,
			       const struct uarte_init_config *config,
			       u8_t interrupts_active)
{
	int err;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);

	nrf_gpio_pin_write(config->pseltxd, 1);
	nrf_gpio_cfg_output(config->pseltxd);

	nrf_gpio_cfg_input(config->pselrxd, NRF_GPIO_PIN_NOPULL);

	nrf_uarte_txrx_pins_set(uarte,
				config->pseltxd,
				config->pselrxd);

	if (config->hwfc == NRF_UARTE_HWFC_ENABLED) {
		nrf_gpio_pin_write(config->pselrts, 1);
		nrf_gpio_cfg_output(config->pselrts);

		nrf_gpio_cfg_input(config->pselcts, NRF_GPIO_PIN_NOPULL);

		nrf_uarte_hwfc_pins_set(uarte,
					config->pselrts,
					config->pselcts);
	}

	/* Configure flow control and parity checking */
	nrf_uarte_configure(uarte,
			    config->parity,
			    config->hwfc);

	err = baudrate_set(dev, config->baudrate);
	if (err) {
		return err;
	}

	/* Enable receiver and transmitter */
	nrf_uarte_enable(uarte);

	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

	nrf_uarte_rx_buffer_set(uarte, &data->rx_data, 1);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);

#if UARTE_INTERRUPT_DRIVEN
	if (interrupts_active) {
		/* Set ENDTX event by requesting fake (zero-length) transfer.
		 * Pointer to RAM variable (data->tx_buffer) is set because
		 * otherwise such operation may result in HardFault or RAM
		 * corruption.
		 */
		nrf_uarte_tx_buffer_set(uarte, data->tx_buffer, 0);
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);

		/* switch off transmitter to save an energy */
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
	}
#endif /* UARTE_INTERRUPT_DRIVEN */

	return 0;
}

#define UART_NRF_UARTE_DEVICE(idx)					  \
	DEVICE_DECLARE(uart_nrfx_uarte##idx);				  \
	UARTE_##idx##_CREATE_TX_BUFF;					  \
	static struct uarte_nrfx_data uarte_##idx##_data = {		  \
		UARTE_##idx##_DATA_INIT					  \
	};								  \
	static const struct uarte_nrfx_config uarte_##idx##_config = {	  \
		.uarte_regs = (NRF_UARTE_Type *)CONFIG_UART_##idx##_BASE, \
		UARTE_##idx##_CONFIG_INIT				  \
	};								  \
	static int uarte_##idx##_init(struct device *dev)		  \
	{								  \
		const struct uarte_init_config init_config = {		  \
			.pseltxd = CONFIG_UART_##idx##_TX_PIN,	  \
			.pselrxd = CONFIG_UART_##idx##_RX_PIN,	  \
			UARTE_##idx##_NRF_HWFC_CONFIG			  \
			.parity = UARTE_##idx##_NRF_PARITY_BIT,		  \
			.baudrate = CONFIG_UART_##idx##_BAUD_RATE	  \
		};							  \
		UARTE_##idx##_INTERRUPTS_INIT();			  \
		return uarte_instance_init(dev,				  \
				&init_config,				  \
				UARTE_##idx##_INTERRUPT_DRIVEN);	  \
	}								  \
	DEVICE_AND_API_INIT(uart_nrfx_uarte##idx,			  \
			CONFIG_UART_##idx##_NAME,			  \
			uarte_##idx##_init,				  \
			&uarte_##idx##_data,				  \
			&uarte_##idx##_config,				  \
			PRE_KERNEL_1,					  \
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		  \
			&uart_nrfx_uarte_driver_api)

#define UARTE_NRF_HWFC_ENABLED(idx)				\
	.pselcts = CONFIG_UART_##idx##_CTS_PIN,		\
	.pselrts = CONFIG_UART_##idx##_RTS_PIN,		\
	.hwfc = NRF_UARTE_HWFC_ENABLED,
#define UARTE_NRF_HWFC_DISABLED					\
	.pselcts = NRF_UARTE_PSEL_DISCONNECTED,			\
	.pselrts = NRF_UARTE_PSEL_DISCONNECTED,			\
	.hwfc = NRF_UARTE_HWFC_DISABLED,

#define UARTE_NRF_IRQ_ENABLED(idx)				\
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_UARTE##idx),	\
		CONFIG_UART_##idx##_IRQ_PRI,			\
		uarte_nrfx_isr,					\
		DEVICE_GET(uart_nrfx_uarte##idx),		\
		0);						\
	irq_enable(CONFIG_UART_##idx##_IRQ_NUM)

#define UARTE_TX_BUFFER_SIZE(idx)				\
	CONFIG_UART_##idx##_NRF_TX_BUFFER_SIZE <		\
		BIT_MASK(UARTE##idx##_EASYDMA_MAXCNT_SIZE) ?	\
	CONFIG_UART_##idx##_NRF_TX_BUFFER_SIZE :		\
		BIT_MASK(UARTE##idx##_EASYDMA_MAXCNT_SIZE)

#define UARTE_DATA_INT(idx)				\
	.tx_buffer = uarte##idx##_tx_buffer

#define UARTE_CONFIG_INT(idx)				\
	.tx_buff_size = UARTE_TX_BUFFER_SIZE(idx)

#define UARTE_TX_BUFFER_INIT(idx)			\
	static u8_t uarte##idx##_tx_buffer[UARTE_TX_BUFFER_SIZE(idx)]

#ifdef CONFIG_UART_0_NRF_UARTE
	#ifdef CONFIG_UART_0_INTERRUPT_DRIVEN
		#define UARTE_0_INTERRUPT_DRIVEN       (1u)
		#define UARTE_0_INTERRUPTS_INIT()      UARTE_NRF_IRQ_ENABLED(0)
		#define UARTE_0_CREATE_TX_BUFF	       UARTE_TX_BUFFER_INIT(0)
		#define UARTE_0_DATA_INIT	       UARTE_DATA_INT(0)
		#define UARTE_0_CONFIG_INIT	       UARTE_CONFIG_INT(0)
	#else
		#define UARTE_0_INTERRUPT_DRIVEN       (0u)
		#define UARTE_0_INTERRUPTS_INIT()
		#define UARTE_0_CREATE_TX_BUFF
		#define UARTE_0_DATA_INIT
		#define UARTE_0_CONFIG_INIT
	#endif /* CONFIG_UART_0_INTERRUPT_DRIVEN */

	#ifdef CONFIG_UART_0_NRF_FLOW_CONTROL
		#define UARTE_0_NRF_HWFC_CONFIG	       UARTE_NRF_HWFC_ENABLED(0)
		#ifndef CONFIG_UART_0_RTS_PIN
		#error Flow control for UARTE0 is enabled, but RTS pin is not defined.
		#endif
		#ifndef CONFIG_UART_0_CTS_PIN
		#error Flow control for UARTE0 is enabled, but CTS pin is not defined.
		#endif
	#else
		#define UARTE_0_NRF_HWFC_CONFIG	       UARTE_NRF_HWFC_DISABLED
	#endif /* CONFIG_UART_0_NRF_FLOW_CONTROL */

	#ifdef CONFIG_UART_0_NRF_PARITY_BIT
		#define UARTE_0_NRF_PARITY_BIT	       NRF_UARTE_PARITY_INCLUDED
	#else
		#define UARTE_0_NRF_PARITY_BIT	       NRF_UARTE_PARITY_EXCLUDED
	#endif /* CONFIG_UART_0_NRF_PARITY_BIT */

	UART_NRF_UARTE_DEVICE(0);
#endif /* CONFIG_UART_0_NRF_UARTE */

#ifdef CONFIG_UART_1_NRF_UARTE
	#ifdef CONFIG_UART_1_INTERRUPT_DRIVEN
		#define UARTE_1_INTERRUPT_DRIVEN       (1u)
		#define UARTE_1_INTERRUPTS_INIT()      UARTE_NRF_IRQ_ENABLED(1)
		#define UARTE_1_CREATE_TX_BUFF	       UARTE_TX_BUFFER_INIT(1)
		#define UARTE_1_DATA_INIT	       UARTE_DATA_INT(1)
		#define UARTE_1_CONFIG_INIT	       UARTE_CONFIG_INT(1)
	#else
		#define UARTE_1_INTERRUPT_DRIVEN       (0u)
		#define UARTE_1_INTERRUPTS_INIT()
		#define UARTE_1_CREATE_TX_BUFF
		#define UARTE_1_DATA_INIT
		#define UARTE_1_CONFIG_INIT
	#endif /* CONFIG_UART_1_INTERRUPT_DRIVEN */

	#ifdef CONFIG_UART_1_NRF_FLOW_CONTROL
		#define UARTE_1_NRF_HWFC_CONFIG	       UARTE_NRF_HWFC_ENABLED(1)
		#ifndef CONFIG_UART_1_RTS_PIN
		#error Flow control for UARTE0 is enabled, but RTS pin is not defined.
		#endif
		#ifndef CONFIG_UART_1_CTS_PIN
		#error Flow control for UARTE0 is enabled, but CTS pin is not defined.
		#endif
	#else
		#define UARTE_1_NRF_HWFC_CONFIG	       UARTE_NRF_HWFC_DISABLED
	#endif /* CONFIG_UART_1_NRF_FLOW_CONTROL */

	#ifdef CONFIG_UART_1_NRF_PARITY_BIT
		#define UARTE_1_NRF_PARITY_BIT	       NRF_UARTE_PARITY_INCLUDED

	#else
		#define UARTE_1_NRF_PARITY_BIT	       NRF_UARTE_PARITY_EXCLUDED
	#endif /* CONFIG_UART_1_NRF_PARITY_BIT */

	UART_NRF_UARTE_DEVICE(1);
#endif /* CONFIG_UART_1_NRF_UARTE */

