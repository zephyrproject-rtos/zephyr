/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file uart_mchp_sercom_g1.c
 * @brief UART driver implementation for Microchip devices.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#ifdef CONFIG_UART_MCHP_ASYNC
#include <zephyr/drivers/dma.h>
#include <mchp_dt_helper.h>
#endif /* CONFIG_UART_MCHP_ASYNC */
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <string.h>
#include <zephyr/logging/log.h>

/******************************************************************************
 * @brief Devicetree definitions
 *****************************************************************************/
#define DT_DRV_COMPAT microchip_sercom_g1_uart

/******************************************************************************
 * @brief Macro definitions
 *****************************************************************************/
LOG_MODULE_REGISTER(uart_mchp_sercom_g1, CONFIG_UART_LOG_LEVEL);
#define TIMEOUT_VALUE_US       1000
#define DELAY_US               2
#define UART_SUCCESS           0
#define BITSHIFT_FOR_BAUD_CALC 20

/* Do the peripheral interrupt related configuration */
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_MCHP_ASYNC)

#if DT_INST_IRQ_HAS_IDX(0, 3)
/**
 * @brief Configure UART IRQ handler for multiple interrupts.
 *
 * This macro sets up the IRQ handler for the UART peripheral when
 * multiple interrupts are available.
 *
 * @param n Instance number.
 */
#define UART_MCHP_IRQ_HANDLER(n)                                                                   \
	static void uart_mchp_irq_config_##n(const struct device *dev)                             \
	{                                                                                          \
		MCHP_UART_IRQ_CONNECT(n, 0);                                                       \
		MCHP_UART_IRQ_CONNECT(n, 1);                                                       \
		MCHP_UART_IRQ_CONNECT(n, 2);                                                       \
		MCHP_UART_IRQ_CONNECT(n, 3);                                                       \
	}
#else /* DT_INST_IRQ_HAS_IDX(0, 3) */
/**
 * @brief Configure UART IRQ handler for a single interrupt.
 *
 * This macro sets up the IRQ handler for the UART peripheral when
 * only a single interrupt is available.
 *
 * @param n Instance number.
 */
#define UART_MCHP_IRQ_HANDLER(n)                                                                   \
	static void uart_mchp_irq_config_##n(const struct device *dev)                             \
	{                                                                                          \
		MCHP_UART_IRQ_CONNECT(n, 0);                                                       \
	}
#endif /* DT_INST_IRQ_HAS_IDX(0, 3) */

#else /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_MCHP_ASYNC */
#define UART_MCHP_IRQ_HANDLER(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_MCHP_ASYNC */

/******************************************************************************
 * @brief Data type definitions
 *****************************************************************************/
/**
 * @brief Clock configuration structure for the UART.
 *
 * This structure contains the clock configuration parameters for the UART
 * peripheral.
 */
typedef struct mchp_uart_clock {
	/* Clock driver */
	const struct device *clock_dev;

	/* Main clock subsystem. */
	clock_control_subsys_t mclk_sys;

	/* Generic clock subsystem. */
	clock_control_subsys_t gclk_sys;
} mchp_uart_clock_t;

#define UART_MCHP_CLOCK_DEFN(n)                                                                    \
	.uart_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                \
	.uart_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),          \
	.uart_clock.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),

#ifdef CONFIG_UART_MCHP_ASYNC
/**
 * @brief DMA configuration structure for the UART.
 *
 * This structure contains the DMA configuration parameters for the UART
 * peripheral.
 */
typedef struct mchp_uart_dma {
	/* DMA driver for asynchronous operations */
	const struct device *dma_dev;

	/* TX DMA request line. */
	uint8_t tx_dma_request;

	/* TX DMA channel. */
	uint8_t tx_dma_channel;

	/* RX DMA request line. */
	uint8_t rx_dma_request;

	/* RX DMA channel. */
	uint8_t rx_dma_channel;
} mchp_uart_dma_t;
#endif /* CONFIG_UART_MCHP_ASYNC */

/**
 * @brief UART device constant configuration structure.
 */
typedef struct uart_mchp_dev_cfg {
	/* Baud rate for UART communication */
	uint32_t baudrate;
	uint8_t data_bits;
	uint8_t parity;
	uint8_t stop_bits;

	/* Pointer to the SERCOM registers. */
	sercom_registers_t *regs;

	/* Flag indicating if the clock is external. */
	bool is_clock_external;

	/* defines the functionality in standby sleep mode */
	uint8_t run_in_standby_en;

	/* RX pinout configuration. */
	uint32_t rxpo;

	/* TX pinout configuration. */
	uint32_t txpo;

#ifdef CONFIG_UART_MCHP_ASYNC
	mchp_uart_dma_t uart_dma;
#endif /* CONFIG_UART_MCHP_ASYNC */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_MCHP_ASYNC)
	/* IRQ configuration function */
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_MCHP_ASYNC*/

	/* Clock configuration */
	mchp_uart_clock_t uart_clock;

	/* Pin control configuration */
	const struct pinctrl_dev_config *pcfg;

} uart_mchp_dev_cfg_t;

/**
 * @brief UART device runtime data structure.
 */
typedef struct uart_mchp_dev_data {
	/* Cached UART configuration */
	struct uart_config config_cache;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* IRQ callback function */
	uart_irq_callback_user_data_t cb;

	/* IRQ callback user data */
	void *cb_data;

	/* Cached status of TX completion */
	bool is_tx_completed_cache;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_MCHP_ASYNC
	/* Device structure */
	const struct device *dev;

	/* Device configuration */
	const uart_mchp_dev_cfg_t *cfg;

	/* Asynchronous callback function */
	uart_callback_t async_cb;

	/* Asynchronous callback user data */
	void *async_cb_data;

	/* TX timeout work structure */
	struct k_work_delayable tx_timeout_work;

	/* TX buffer */
	const uint8_t *tx_buf;

	/* TX buffer length */
	size_t tx_len;

	/* RX timeout work structure */
	struct k_work_delayable rx_timeout_work;

	/* RX timeout time */
	size_t rx_timeout_time;

	/* RX timeout chunk */
	size_t rx_timeout_chunk;

	/* RX timeout start time */
	uint32_t rx_timeout_start;

	/* RX buffer */
	uint8_t *rx_buf;

	/* RX buffer length */
	size_t rx_len;

	/* RX processed length */
	size_t rx_processed_len;

	/* Next RX buffer */
	uint8_t *rx_next_buf;

	/* Next RX buffer length */
	size_t rx_next_len;

	/* RX waiting for IRQ flag */
	bool rx_waiting_for_irq;

	/* RX timeout from ISR flag */
	bool rx_timeout_from_isr;
#endif /* CONFIG_UART_MCHP_ASYNC */
} uart_mchp_dev_data_t;

/******************************************************************************
 * @brief Function forward declarations
 *****************************************************************************/
#ifdef CONFIG_UART_MCHP_ASYNC
static void uart_mchp_tx_timeout(struct k_work *work);

static void uart_mchp_rx_timeout(struct k_work *work);

static void uart_mchp_dma_tx_done(const struct device *dma_dev, void *arg, uint32_t id,
				  int error_code);

static void uart_mchp_dma_rx_done(const struct device *dma_dev, void *arg, uint32_t id,
				  int error_code);

#endif /* CONFIG_UART_MCHP_ASYNC */

/******************************************************************************
 * @brief Helper functions
 *****************************************************************************/
/**
 * @brief Wait for synchronization of the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 */
static void uart_wait_sync(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (WAIT_FOR(((usart->SERCOM_SYNCBUSY & SERCOM_USART_SYNCBUSY_Msk) == 0), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for UART SYNCBUSY clear");
	}
}

/**
 * @brief Disable UART interrupts.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 */
static inline void uart_disable_interrupts(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	usart->SERCOM_INTENCLR = SERCOM_USART_INTENCLR_Msk;
}

/**
 * @brief Configure the number of data bits for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param count Number of data bits (5 to 9).
 * @return 0 on success, -1 on invalid count.
 */
static int uart_config_data_bits(sercom_registers_t *regs, bool is_clock_external,
				 unsigned int count)
{
	uint32_t value;

	switch (count) {
	case UART_CFG_DATA_BITS_5:
		value = SERCOM_USART_CTRLB_CHSIZE_5_BIT;
		break;
	case UART_CFG_DATA_BITS_6:
		value = SERCOM_USART_CTRLB_CHSIZE_6_BIT;
		break;
	case UART_CFG_DATA_BITS_7:
		value = SERCOM_USART_CTRLB_CHSIZE_7_BIT;
		break;
	case UART_CFG_DATA_BITS_8:
		value = SERCOM_USART_CTRLB_CHSIZE_8_BIT;
		break;
	case UART_CFG_DATA_BITS_9:
		value = SERCOM_USART_CTRLB_CHSIZE_9_BIT;
		break;
	default:
		return -ENOTSUP;
	}

	/* Writing to the CTRLB register requires synchronization */
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	usart->SERCOM_CTRLB &= ~SERCOM_USART_CTRLB_CHSIZE_Msk;
	usart->SERCOM_CTRLB |= value;
	uart_wait_sync(regs, is_clock_external);

	return UART_SUCCESS;
}

/**
 * @brief Configure the parity mode for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param parity Parity mode to configure.
 */
static void uart_config_parity(sercom_registers_t *regs, bool is_clock_external,
			       enum uart_config_parity parity)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	usart->SERCOM_CTRLA &= ~SERCOM_USART_CTRLA_FORM_Msk;

	switch (parity) {
	case UART_CFG_PARITY_ODD:
		usart->SERCOM_CTRLA |= SERCOM_USART_CTRLA_FORM_USART_FRAME_WITH_PARITY;
		usart->SERCOM_CTRLB |= SERCOM_USART_CTRLB_PMODE_Msk;
		uart_wait_sync(regs, is_clock_external);
		break;
	case UART_CFG_PARITY_EVEN:
		usart->SERCOM_CTRLA |= SERCOM_USART_CTRLA_FORM_USART_FRAME_WITH_PARITY;
		usart->SERCOM_CTRLB &= ~SERCOM_USART_CTRLB_PMODE_Msk;
		uart_wait_sync(regs, is_clock_external);
		break;
	default:
		usart->SERCOM_CTRLA |= SERCOM_USART_CTRLA_FORM_USART_FRAME_NO_PARITY;
		break;
	}
}

/**
 * @brief Configure the number of stop bits for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param count Number of stop bits (1 or 2).
 * @return 0 on success, -1 on invalid count.
 */
static int uart_config_stop_bits(sercom_registers_t *regs, bool is_clock_external,
				 unsigned int count)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (count == UART_CFG_STOP_BITS_1) {
		usart->SERCOM_CTRLB &= ~SERCOM_USART_CTRLB_SBMODE_Msk;
	} else if (count == UART_CFG_STOP_BITS_2) {
		usart->SERCOM_CTRLB |= SERCOM_USART_CTRLB_SBMODE_Msk;
	} else {
		return -ENOTSUP;
	}

	uart_wait_sync(regs, is_clock_external);
	return UART_SUCCESS;
}
/**
 * @brief Configure the UART pinout.
 *
 * @param regs Pointer to the uart_mchp_dev_cfg_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 */
static void uart_config_pinout(const uart_mchp_dev_cfg_t *const cfg)
{
	sercom_registers_t *regs = cfg->regs;
	uint32_t rxpo = cfg->rxpo;
	uint32_t txpo = cfg->txpo;
	bool is_clock_external = cfg->is_clock_external;

	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);
	uint32_t reg_value = usart->SERCOM_CTRLA;

	reg_value &= ~(SERCOM_USART_CTRLA_RXPO_Msk | SERCOM_USART_CTRLA_TXPO_Msk);
	reg_value |= (SERCOM_USART_CTRLA_RXPO(rxpo) | SERCOM_USART_CTRLA_TXPO(txpo));
	usart->SERCOM_CTRLA = reg_value;
}

/**
 * @brief Set clock polarity for UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param tx_rising transmit on rising edge
 */
static void uart_set_clock_polarity(sercom_registers_t *regs, bool is_clock_external,
				    bool tx_rising)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (tx_rising == true) {
		usart->SERCOM_CTRLA &= ~SERCOM_USART_CTRLA_CPOL_Msk;
	} else {
		usart->SERCOM_CTRLA |= SERCOM_USART_CTRLA_CPOL_Msk;
	}
}

/**
 * @brief Set the clock source for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 */
static void uart_set_clock_source(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);
	uint32_t reg_value = usart->SERCOM_CTRLA;

	reg_value &= ~SERCOM_USART_CTRLA_MODE_Msk;

	if (is_clock_external == true) {
		usart->SERCOM_CTRLA = reg_value | SERCOM_USART_CTRLA_MODE_USART_EXT;
	} else {
		usart->SERCOM_CTRLA = reg_value | SERCOM_USART_CTRLA_MODE_USART_INT;
	}
}

/**
 * @brief Set the data order for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param lsb_first Boolean to set the data order.
 */
static void uart_set_lsb_first(sercom_registers_t *regs, bool is_clock_external, bool lsb_first)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (lsb_first == true) {
		usart->SERCOM_CTRLA |= SERCOM_USART_CTRLA_DORD_Msk;
	} else {
		usart->SERCOM_CTRLA &= ~SERCOM_USART_CTRLA_DORD_Msk;
	}
}

/**
 * @brief Enable or disable the UART receiver.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param enable Boolean to enable or disable the receiver.
 */
static void uart_rx_on_off(sercom_registers_t *regs, bool is_clock_external, bool enable)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (enable == true) {
		usart->SERCOM_CTRLB |= SERCOM_USART_CTRLB_RXEN_Msk;
	} else {
		usart->SERCOM_CTRLB &= ~SERCOM_USART_CTRLB_RXEN_Msk;
	}

	uart_wait_sync(regs, is_clock_external);
}

/**
 * @brief Enable or disable the UART transmitter.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param enable Boolean to enable or disable the transmitter.
 */
static void uart_tx_on_off(sercom_registers_t *regs, bool is_clock_external, bool enable)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (enable == true) {
		usart->SERCOM_CTRLB |= SERCOM_USART_CTRLB_TXEN_Msk;
	} else {
		usart->SERCOM_CTRLB &= ~SERCOM_USART_CTRLB_TXEN_Msk;
	}

	uart_wait_sync(regs, is_clock_external);
}

/**
 * @brief Set the UART baud rate.
 *
 * This function sets the baud rate for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param baudrate Desired baud rate.
 * @param clk_freq_hz Clock frequency in Hz.
 * @return 0 on success, -ERANGE if the calculated baud rate is out of range.
 * @retval -EINVAL for invalid argument.
 */
static int uart_set_baudrate(sercom_registers_t *regs, bool is_clock_external, uint32_t baudrate,
			     uint32_t clk_freq_hz)
{
	uint64_t tmp;
	uint16_t baud;

	if (clk_freq_hz == 0) {
		return -EINVAL;
	}

	tmp = (uint64_t)baudrate << BITSHIFT_FOR_BAUD_CALC;
	tmp = (tmp + (clk_freq_hz >> 1)) / clk_freq_hz;

	/* Verify that the calculated result is within range */
	if ((tmp < 1) || (tmp > UINT16_MAX)) {
		return -ERANGE;
	}

	baud = (UINT16_MAX + 1) - (uint16_t)tmp;

	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	usart->SERCOM_CTRLA &= ~SERCOM_USART_CTRLA_SAMPR_Msk;
	usart->SERCOM_BAUD = baud;

	return UART_SUCCESS;
}

/**
 * @brief Enable or disable the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param run_in_standby Boolean to enable UART operation in standby mode.
 * @param enable Boolean to enable or disable the UART.
 */
static void uart_enable(sercom_registers_t *regs, bool is_clock_external, bool run_in_standby,
			bool enable)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (enable == true) {
		if (run_in_standby == true) {
			usart->SERCOM_CTRLA |= SERCOM_USART_CTRLA_RUNSTDBY_Msk;
		}
		usart->SERCOM_CTRLA |= SERCOM_USART_CTRLA_ENABLE_Msk;
	} else {
		usart->SERCOM_CTRLA &= ~SERCOM_USART_CTRLA_ENABLE_Msk;
	}

	uart_wait_sync(regs, is_clock_external);
}

/**
 * @brief Check if the UART receive is complete.
 *
 * This function checks if the receive operation is complete for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if receive is complete, false otherwise.
 */
static bool uart_is_rx_complete(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_INTFLAG & SERCOM_USART_INTFLAG_RXC_Msk) != 0);
}

/**
 * @brief Get the received character from the UART.
 *
 * This function retrieves the received character from the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return The received character.
 */
static inline unsigned char uart_get_received_char(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return (unsigned char)(usart->SERCOM_DATA);
}

/**
 * @brief Check if the UART TX is ready
 *
 * This function checks if the TX operation is ready for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if TX is ready, false otherwise.
 */
static bool uart_is_tx_ready(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_INTFLAG & SERCOM_USART_INTFLAG_DRE_Msk) != 0);
}

/**
 * @brief Transmit a character via UART.
 *
 * This function transmits a character via the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param data The character to transmit.
 */
static inline void uart_tx_char(sercom_registers_t *regs, bool is_clock_external,
				unsigned char data)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	usart->SERCOM_DATA = data;
}

/**
 * @brief Check if there is a buffer overflow error.
 *
 * This function checks if there is a buffer overflow error for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if there is a buffer overflow error, false otherwise.
 */
static bool uart_is_err_buffer_overflow(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_STATUS & SERCOM_USART_STATUS_BUFOVF_Msk) != 0);
}

/**
 * @brief Check if there is a frame error.
 *
 * This function checks if there is a frame error for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if there is a frame error, false otherwise.
 */
static bool uart_is_err_frame(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_STATUS & SERCOM_USART_STATUS_FERR_Msk) != 0);
}

/**
 * @brief Check if there is a parity error.
 *
 * This function checks if there is a parity error for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if there is a parity error, false otherwise.
 */
static bool uart_is_err_parity(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_STATUS & SERCOM_USART_STATUS_PERR_Msk) != 0);
}

/**
 * @brief Check if there is an autobaud synchronization error.
 *
 * This function checks if there is an autobaud synchronization error for the specified UART
 * instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if there is an autobaud synchronization error, false otherwise.
 */
static bool uart_is_err_autobaud_sync(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_STATUS & SERCOM_USART_STATUS_ISF_Msk) != 0);
}

/**
 * @brief Check if there is a collision error.
 *
 * This function checks if there is a collision error for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if there is a collision error, false otherwise.
 */
static bool uart_is_err_collision(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_STATUS & SERCOM_USART_STATUS_COLL_Msk) != 0);
}

/**
 * @brief Clear all UART error flags.
 *
 * This function clears all error flags for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 */
static void uart_err_clear_all(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	usart->SERCOM_STATUS |= (SERCOM_USART_STATUS_BUFOVF_Msk | SERCOM_USART_STATUS_FERR_Msk |
				 SERCOM_USART_STATUS_PERR_Msk | SERCOM_USART_STATUS_ISF_Msk |
				 SERCOM_USART_STATUS_COLL_Msk);
}

/**
 * @brief See if any error present.
 *
 * This function check for error flags for the specified UART instance.
 *
 * @param dev Pointer to the UART device.
 */
static uint32_t uart_get_err(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	uint32_t err = 0U;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;

	if (uart_is_err_buffer_overflow(regs, is_clock_external) == true) {
		err |= UART_ERROR_OVERRUN;
	}

	if (uart_is_err_frame(regs, is_clock_external) == true) {
		err |= UART_ERROR_FRAMING;
	}

	if (uart_is_err_parity(regs, is_clock_external) == true) {
		err |= UART_ERROR_PARITY;
	}

	if (uart_is_err_autobaud_sync(regs, is_clock_external) == true) {
		err |= UART_BREAK;
	}

	if (uart_is_err_collision(regs, is_clock_external) == true) {
		err |= UART_ERROR_COLLISION;
	}

	return err;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_MCHP_ASYNC)
/**
 * @brief Check if the UART TX is complete.
 *
 * This function checks if the TX operation is complete for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if TX is complete, false otherwise.
 */
static bool uart_is_tx_complete(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_INTFLAG & SERCOM_USART_INTFLAG_TXC_Msk) != 0);
}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * @brief Check if the UART transmit interrupt is enabled.
 *
 * This function checks if transmit interrupt is enabled for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if interrupt is enabled, false otherwise.
 */
static bool uart_is_tx_interrupt_enabled(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_INTENSET & SERCOM_USART_INTENSET_DRE_Msk) != 0);
}

/**
 * @brief Check if any UART interrupt is pending.
 *
 * This function checks if any interrupt is pending for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return True if any interrupt is pending, false otherwise.
 */
static bool uart_is_interrupt_pending(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return ((usart->SERCOM_INTENSET & usart->SERCOM_INTFLAG) != 0);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_MCHP_ASYNC
/**
 * @brief Get the DMA destination address for UART.
 *
 * This function retrieves the DMA destination address for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return The DMA destination address.
 */
static inline void *uart_get_dma_dest_addr(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return (void *)&usart->SERCOM_DATA;
}

/**
 * @brief Get the DMA source address for UART.
 *
 * This function retrieves the DMA source address for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @return The DMA source address.
 */
static inline void *uart_get_dma_source_addr(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	return (void *)&usart->SERCOM_DATA;
}
#endif /* CONFIG_UART_MCHP_ASYNC */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_MCHP_ASYNC)
/**
 * @brief Enable or disable the UART RX interrupt.
 *
 * This function enables or disables the RX interrupt for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param enable True to enable the interrupt, false to disable.
 */
static void uart_enable_rx_interrupt(sercom_registers_t *regs, bool is_clock_external, bool enable)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (enable == true) {
		usart->SERCOM_INTENSET = SERCOM_USART_INTENSET_RXC_Msk;
	} else {
		usart->SERCOM_INTENCLR = SERCOM_USART_INTENCLR_RXC_Msk;
	}
}

/**
 * @brief Enable or disable the UART TX complete interrupt.
 *
 * This function enables or disables the TX complete interrupt for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param enable True to enable the interrupt, false to disable.
 */
static void uart_enable_tx_complete_interrupt(sercom_registers_t *regs, bool is_clock_external,
					      bool enable)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (enable == true) {
		usart->SERCOM_INTENSET = SERCOM_USART_INTENSET_TXC_Msk;
	} else {
		usart->SERCOM_INTENCLR = SERCOM_USART_INTENCLR_TXC_Msk;
	}
}

/**
 * @brief Enable or disable the UART error interrupt.
 *
 * This function enables or disables the error interrupt for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param enable True to enable the interrupt, false to disable.
 */
static void uart_enable_err_interrupt(sercom_registers_t *regs, bool is_clock_external, bool enable)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (enable == true) {
		usart->SERCOM_INTENSET = SERCOM_USART_INTENSET_ERROR_Msk;
	} else {
		usart->SERCOM_INTENCLR = SERCOM_USART_INTENCLR_ERROR_Msk;
	}
}

/**
 * @brief Clear all UART interrupts.
 *
 * This function clears all interrupts for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 */
static void uart_clear_interrupts(sercom_registers_t *regs, bool is_clock_external)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	usart->SERCOM_INTFLAG = (SERCOM_USART_INTFLAG_ERROR_Msk | SERCOM_USART_INTFLAG_RXBRK_Msk |
				 SERCOM_USART_INTFLAG_CTSIC_Msk | SERCOM_USART_INTFLAG_RXS_Msk |
				 SERCOM_USART_INTFLAG_TXC_Msk);
}

/**
 * @brief UART ISR handler.
 *
 * @param dev Device structure.
 */
static void uart_mchp_isr(const struct device *dev)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (dev_data->cb != NULL) {
		dev_data->cb(dev, dev_data->cb_data);
	}
#endif /*CONFIG_UART_INTERRUPT_DRIVEN*/

#ifdef CONFIG_UART_MCHP_ASYNC
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;

	if ((dev_data->tx_len != 0) && (0 != uart_is_tx_complete(regs, is_clock_external))) {
		uart_enable_tx_complete_interrupt(regs, is_clock_external, false);
		k_work_cancel_delayable(&dev_data->tx_timeout_work);

		unsigned int key = irq_lock();

		/* clang-format off */
		struct uart_event evt = {
			.type = UART_TX_DONE,
			.data.tx = {
					.buf = dev_data->tx_buf,
					.len = dev_data->tx_len,
				},
		};
		/* clang-format on */

		dev_data->tx_buf = NULL;
		dev_data->tx_len = 0U;

		if (dev_data->async_cb != NULL) {
			dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
		}

		irq_unlock(key);
	}

	if (dev_data->rx_len != 0) {
		if (uart_get_err(dev) != 0) {

			if (dev_data->async_cb != NULL) {
				struct uart_event evt = {
					.type = UART_RX_STOPPED,
				};
				dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
			}

			uart_clear_interrupts(regs, is_clock_external);
			uart_err_clear_all(regs, is_clock_external);

			/* Once the error is processed, nothing more to do for RX */
			return;
		}

		if ((uart_is_rx_complete(regs, is_clock_external) == true) &&
		    (dev_data->rx_waiting_for_irq == true)) {
			dev_data->rx_waiting_for_irq = false;
			uart_enable_rx_interrupt(regs, is_clock_external, false);

			/* Receive started, so request the next buffer */
			if ((dev_data->rx_next_len == 0U) && (dev_data->async_cb != NULL)) {
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
			dma_start(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel);
		}
	}
#endif /* CONFIG_UART_MCHP_ASYNC */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_MCHP_ASYNC */

/******************************************************************************
 * @brief API functions
 *****************************************************************************/
/**
 * @brief Initialize the UART device.
 *
 * @param dev Device structure.
 * @return 0 on success, negative error code on failure.
 */
static int uart_mchp_init(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	uart_mchp_dev_data_t *const dev_data = dev->data;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;
	int retval = UART_SUCCESS;

	/* Enable the GCLK and MCLK*/
	retval = clock_control_on(cfg->uart_clock.clock_dev, cfg->uart_clock.gclk_sys);
	if ((retval != UART_SUCCESS) && (retval != -EALREADY)) {
		return retval;
	}

	retval = clock_control_on(cfg->uart_clock.clock_dev, cfg->uart_clock.mclk_sys);
	if ((retval != UART_SUCCESS) && (retval != -EALREADY)) {
		return retval;
	}

	uart_disable_interrupts(regs, is_clock_external);

	dev_data->config_cache.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	retval = uart_config_data_bits(regs, is_clock_external, cfg->data_bits);
	if (retval != UART_SUCCESS) {
		return retval;
	}
	dev_data->config_cache.data_bits = cfg->data_bits;

	uart_config_parity(regs, is_clock_external, cfg->parity);
	dev_data->config_cache.parity = cfg->parity;

	retval = uart_config_stop_bits(regs, is_clock_external, cfg->stop_bits);
	if (retval != UART_SUCCESS) {
		return retval;
	}
	dev_data->config_cache.stop_bits = cfg->stop_bits;

	uart_config_pinout(cfg);
	uart_set_clock_polarity(regs, is_clock_external, false);
	uart_set_clock_source(regs, is_clock_external);
	uart_set_lsb_first(regs, is_clock_external, true);

	/* Enable PINMUX based on PINCTRL */
	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval != UART_SUCCESS) {
		return retval;
	}

	/* Enable receiver and transmitter */
	uart_rx_on_off(regs, is_clock_external, true);
	uart_tx_on_off(regs, is_clock_external, true);

	uint32_t clock_rate;

	clock_control_get_rate(cfg->uart_clock.clock_dev, cfg->uart_clock.gclk_sys, &clock_rate);

	retval = uart_set_baudrate(regs, is_clock_external, cfg->baudrate, clock_rate);
	if (retval != UART_SUCCESS) {
		return retval;
	}
	dev_data->config_cache.baudrate = cfg->baudrate;

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_MCHP_ASYNC)
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_MCHP_ASYNC */

#ifdef CONFIG_UART_MCHP_ASYNC
	dev_data->dev = dev;
	dev_data->cfg = cfg;
	if (device_is_ready(cfg->uart_dma.dma_dev) == false) {
		return -ENODEV;
	}

	k_work_init_delayable(&dev_data->tx_timeout_work, uart_mchp_tx_timeout);
	k_work_init_delayable(&dev_data->rx_timeout_work, uart_mchp_rx_timeout);

	int dma_ch = cfg->uart_dma.tx_dma_channel;
	int dma_ch_request = dma_request_channel(cfg->uart_dma.dma_dev, (void *)&dma_ch);

	if ((cfg->uart_dma.tx_dma_channel != 0xFFU) &&
	    (dma_ch_request == cfg->uart_dma.tx_dma_channel)) {
		struct dma_config dma_cfg = {0};
		struct dma_block_config dma_blk = {0};

		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_cfg.source_data_size = 1;
		dma_cfg.dest_data_size = 1;
		dma_cfg.user_data = dev_data;
		dma_cfg.dma_callback = uart_mchp_dma_tx_done;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_blk;
		dma_cfg.dma_slot = cfg->uart_dma.tx_dma_request;

		dma_blk.block_size = 1;
		dma_blk.dest_address = (uint32_t)(uart_get_dma_dest_addr(regs, is_clock_external));
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		retval = dma_config(cfg->uart_dma.dma_dev, cfg->uart_dma.tx_dma_channel, &dma_cfg);
		if (retval != 0) {
			return retval;
		}
	}

	dma_ch = cfg->uart_dma.rx_dma_channel;
	dma_ch_request = dma_request_channel(cfg->uart_dma.dma_dev, (void *)&dma_ch);

	if ((cfg->uart_dma.rx_dma_channel != 0xFFU) &&
	    (dma_ch_request == cfg->uart_dma.rx_dma_channel)) {
		struct dma_config dma_cfg = {0};
		struct dma_block_config dma_blk = {0};

		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_cfg.source_data_size = 1;
		dma_cfg.dest_data_size = 1;
		dma_cfg.user_data = dev_data;
		dma_cfg.dma_callback = uart_mchp_dma_rx_done;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_blk;
		dma_cfg.dma_slot = cfg->uart_dma.rx_dma_request;

		dma_blk.block_size = 1;
		dma_blk.source_address =
			(uint32_t)(uart_get_dma_source_addr(regs, is_clock_external));
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		retval = dma_config(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel, &dma_cfg);
		if (retval != 0) {
			return retval;
		}
	}
#endif /* CONFIG_UART_MCHP_ASYNC */

	uart_enable(regs, is_clock_external, (cfg->run_in_standby_en == 1) ? true : false, true);

	return retval;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

/**
 * @brief Configure the UART device.
 *
 * @param dev Device structure.
 * @param new_cfg New UART configuration.
 * @return 0 on success, negative error code on failure.
 */
static int uart_mchp_configure(const struct device *dev, const struct uart_config *new_cfg)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	uart_mchp_dev_data_t *const dev_data = dev->data;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;
	int retval = UART_SUCCESS;

	do {
		/* Forcefully disable UART before configuring. run_in_standby is ignored here */
		uart_enable(regs, is_clock_external, false, false);

		if (new_cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
			/* Flow control not yet supported though in principle possible
			 * on this soc family.
			 */
			retval = -ENOTSUP;
			break;
		}
		dev_data->config_cache.flow_ctrl = new_cfg->flow_ctrl;

		switch (new_cfg->parity) {
		case UART_CFG_PARITY_NONE:
		case UART_CFG_PARITY_ODD:
		case UART_CFG_PARITY_EVEN:
			uart_config_parity(regs, is_clock_external, new_cfg->parity);
			break;
		default:
			retval = -ENOTSUP;
		}
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.parity = new_cfg->parity;

		retval = uart_config_stop_bits(regs, is_clock_external, new_cfg->stop_bits);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.stop_bits = new_cfg->stop_bits;

		retval = uart_config_data_bits(regs, is_clock_external, new_cfg->data_bits);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.data_bits = new_cfg->data_bits;

		uint32_t clock_rate = 0;

		clock_control_get_rate(cfg->uart_clock.clock_dev, cfg->uart_clock.gclk_sys,
				       &clock_rate);

		retval = uart_set_baudrate(regs, is_clock_external, new_cfg->baudrate, clock_rate);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.baudrate = new_cfg->baudrate;

		uart_enable(regs, is_clock_external, (cfg->run_in_standby_en == 1) ? true : false,
			    true);
	} while (0);

	return retval;
}

/**
 * @brief Get the current UART configuration.
 *
 * @param dev Device structure.
 * @param out_cfg Output configuration structure.
 * @return 0 on success.
 */
static int uart_mchp_config_get(const struct device *dev, struct uart_config *out_cfg)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;

	memcpy(out_cfg, &(dev_data->config_cache), sizeof(dev_data->config_cache));

	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

/**
 * @brief Poll the UART device for input.
 *
 * @param dev Device structure.
 * @param data Pointer to store the received data.
 * @return 0 on success, negative error code on failure.
 */
static int uart_mchp_poll_in(const struct device *dev, unsigned char *data)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;
	int retval = UART_SUCCESS;

	do {
#ifdef CONFIG_UART_MCHP_ASYNC
		uart_mchp_dev_data_t *const dev_data = dev->data;

		if (dev_data->rx_len != 0U) {
			retval = -EBUSY;
			break;
		}
#endif /* CONFIG_UART_MCHP_ASYNC */

		if (uart_is_rx_complete(regs, is_clock_external) == false) {
			retval = -EBUSY;
			break;
		}

		*data = uart_get_received_char(regs, is_clock_external);
	} while (0);

	return retval;
}

/**
 * @brief Output a character via UART.
 *
 * @param dev Device structure.
 * @param data Character to send.
 */
static void uart_mchp_poll_out(const struct device *dev, unsigned char data)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;

	while (uart_is_tx_ready(regs, is_clock_external) == false) {
	}

	/* send a character */
	uart_tx_char(regs, is_clock_external, data);
}

/**
 * @brief Check for UART errors.
 *
 * @param dev Device structure.
 * @return Error code.
 */
static int uart_mchp_err_check(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;

	uint32_t err = uart_get_err(dev);

	/* Clear all errors */
	uart_err_clear_all(regs, is_clock_external);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Enable or disable the UART TX ready interrupt.
 *
 * This function enables or disables the TX ready interrupt for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param is_clock_external Boolean to indicate external or internal clock
 * @param enable True to enable the interrupt, false to disable.
 */
static void uart_enable_tx_ready_interrupt(sercom_registers_t *regs, bool is_clock_external,
					   bool enable)
{
	sercom_usart_registers_t *usart = UART_GET_BASE_ADDR(regs, is_clock_external);

	if (enable == true) {
		usart->SERCOM_INTENSET = SERCOM_USART_INTENSET_DRE_Msk;
	} else {
		usart->SERCOM_INTENCLR = SERCOM_USART_INTENCLR_DRE_Msk;
	}
}
/**
 * @brief Enable UART TX interrupt.
 *
 * This function enables the UART TX ready and TX complete interrupts.
 *
 * @param dev Pointer to the device structure.
 */
static void uart_mchp_irq_tx_enable(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;
	unsigned int key = irq_lock();

	uart_enable_tx_ready_interrupt(regs, is_clock_external, true);
	uart_enable_tx_complete_interrupt(regs, is_clock_external, true);
	irq_unlock(key);
}

/**
 * @brief Fill the UART FIFO with data.
 *
 * This function fills the UART FIFO with data from the provided buffer.
 *
 * @param dev Pointer to the device structure.
 * @param tx_data Pointer to the data buffer to be transmitted.
 * @param len Length of the data buffer.
 * @return Number of bytes written to the FIFO.
 */
static int uart_mchp_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;
	int retval = 0;

	if ((uart_is_tx_ready(regs, is_clock_external) == true) && (len >= 1)) {
		uart_tx_char(regs, is_clock_external,
			     tx_data[0]); /* Transmit the first character */
		retval = 1;
	}

	return retval;
}

/**
 * @brief Disable UART TX interrupt.
 *
 * This function disables the UART TX ready and TX complete interrupts.
 *
 * @param dev Pointer to the device structure.
 */
static void uart_mchp_irq_tx_disable(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;

	uart_enable_tx_ready_interrupt(regs, is_clock_external, false);
	uart_enable_tx_complete_interrupt(regs, is_clock_external, false);
}

/**
 * @brief Check if UART TX is ready.
 *
 * This function checks if the UART TX is ready to transmit data.
 *
 * @param dev Pointer to the device structure.
 * @return 1 if TX is ready, 0 otherwise.
 */
static int uart_mchp_irq_tx_ready(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;

	return (uart_is_tx_ready(regs, is_clock_external) &&
		uart_is_tx_interrupt_enabled(regs, is_clock_external));
}

/**
 * @brief Check if UART TX is complete.
 *
 * This function checks if the UART TX has completed transmission.
 *
 * @param dev Pointer to the device structure.
 * @return 1 if TX is complete, 0 otherwise.
 */
static int uart_mchp_irq_tx_complete(const struct device *dev)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;
	int retval = 0;

	if (dev_data->is_tx_completed_cache == true) {
		retval = 1;
	}

	return retval;
}

/**
 * @brief Enable UART RX interrupt.
 *
 * This function enables the UART RX interrupt.
 *
 * @param dev Pointer to the device structure.
 */
static void uart_mchp_irq_rx_enable(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;

	uart_enable_rx_interrupt(cfg->regs, cfg->is_clock_external, true);
}

/**
 * @brief Disable UART RX interrupt.
 *
 * This function disables the UART RX interrupt.
 *
 * @param dev Pointer to the device structure.
 */
static void uart_mchp_irq_rx_disable(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;

	uart_enable_rx_interrupt(cfg->regs, cfg->is_clock_external, false);
}

/**
 * @brief Check if UART RX is ready.
 *
 * This function checks if the UART RX has received data.
 *
 * @param dev Pointer to the device structure.
 * @return 1 if RX is ready, 0 otherwise.
 */
static int uart_mchp_irq_rx_ready(const struct device *dev)
{
	int retval = 0;
	const uart_mchp_dev_cfg_t *const cfg = dev->config;

	if (uart_is_rx_complete(cfg->regs, cfg->is_clock_external) == true) {
		retval = 1;
	}

	return retval;
}

/**
 * @brief Read data from UART FIFO.
 *
 * This function reads data from the UART FIFO into the provided buffer.
 *
 * @param dev Pointer to the device structure.
 * @param rx_data Pointer to the buffer to store received data.
 * @param size Size of the buffer.
 * @return Number of bytes read from the FIFO.
 * @retval -EINVAL for invalid argument.
 */
static int uart_mchp_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;
	int retval = 0;

	if (uart_is_rx_complete(regs, is_clock_external) == true) {
		uint8_t ch = uart_get_received_char(
			/* Get the received character */
			regs, is_clock_external);

		if (size >= 1) {
			/* Store the received character in the buffer */
			*rx_data = ch;
			retval = 1;
		} else {
			retval = -EINVAL;
		}
	}

	return retval;
}

/**
 * @brief Check if UART interrupt is pending.
 *
 * This function checks if there is any pending UART interrupt.
 *
 * @param dev Pointer to the device structure.
 * @return 1 if an interrupt is pending, 0 otherwise.
 */
static int uart_mchp_irq_is_pending(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	int retval = 0;

	if (uart_is_interrupt_pending(cfg->regs, cfg->is_clock_external) == true) {
		retval = 1;
	}

	return retval;
}

/**
 * @brief Enable UART error interrupt.
 *
 * This function enables the UART error interrupt.
 *
 * @param dev Pointer to the device structure.
 */
static void uart_mchp_irq_err_enable(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;

	uart_enable_err_interrupt(cfg->regs, cfg->is_clock_external, true);
}

/**
 * @brief Disable UART error interrupt.
 *
 * This function disables the UART error interrupt.
 *
 * @param dev Pointer to the device structure.
 */
static void uart_mchp_irq_err_disable(const struct device *dev)
{
	const uart_mchp_dev_cfg_t *const cfg = dev->config;

	uart_enable_err_interrupt(cfg->regs, cfg->is_clock_external, false);
}

/**
 * @brief Update UART interrupt status.
 *
 * This function clears sticky interrupts and updates the TX complete cache.
 *
 * @param dev Pointer to the device structure.
 * @return Always returns 1.
 */
static int uart_mchp_irq_update(const struct device *dev)
{
	/* Clear sticky interrupts */
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	uart_mchp_dev_data_t *const dev_data = dev->data;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;

	/*
	 * Cache the TXC flag, and use this cached value to clear the interrupt
	 * if we do not use the cached value, there is a chance TXC will set
	 * after caching...this will cause TXC to never be cached.
	 */
	dev_data->is_tx_completed_cache = uart_is_tx_complete(regs, is_clock_external);
	uart_clear_interrupts(regs, is_clock_external);

	return 1;
}

/**
 * @brief Set UART interrupt callback.
 *
 * This function sets the callback function for UART interrupts.
 *
 * @param dev Pointer to the device structure.
 * @param cb Callback function.
 * @param cb_data User data to be passed to the callback function.
 */
static void uart_mchp_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;

#if defined(CONFIG_UART_MCHP_ASYNC) && defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	dev_data->async_cb = NULL;
	dev_data->async_cb_data = NULL;
#endif /*CONFIG_UART_MCHP_ASYNC && CONFIG_UART_EXCLUSIVE_API_CALLBACKS*/
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_MCHP_ASYNC
/**
 * @brief Halt UART transmission.
 *
 * This function halts the UART transmission and stops the DMA transfer.
 *
 * @param dev_data Pointer to the UART device data structure.
 * @return 0 on success, negative error code on failure.
 * @retval -EINVAL for invalid option.
 */
static int uart_mchp_tx_halt(uart_mchp_dev_data_t *dev_data)
{
	size_t tx_active;
	struct dma_status dma_stat;
	const uart_mchp_dev_cfg_t *const cfg = dev_data->cfg;
	int retval = UART_SUCCESS;

	unsigned int key = irq_lock();

	/* clang-format off */
	struct uart_event evt = {
		.type = UART_TX_ABORTED,
		.data.tx = {
				.buf = dev_data->tx_buf,
				.len = 0U,
			},
	};
	/* clang-format on */

	tx_active = dev_data->tx_len;
	dev_data->tx_buf = NULL;
	dev_data->tx_len = 0U;

	dma_stop(cfg->uart_dma.dma_dev, cfg->uart_dma.tx_dma_channel);

	irq_unlock(key);

	if (dma_get_status(cfg->uart_dma.dma_dev, cfg->uart_dma.tx_dma_channel, &dma_stat) == 0) {
		evt.data.tx.len = tx_active - dma_stat.pending_length;
	}

	if (tx_active != 0) {
		if (dev_data->async_cb != NULL) {
			dev_data->async_cb(dev_data->dev, &evt, dev_data->async_cb_data);
		}
	} else {
		retval = -EINVAL;
	}

	return retval;
}

/**
 * @brief Notify UART RX processed.
 *
 * This function notifies that UART RX data has been processed.
 *
 * @param dev_data Pointer to the UART device data structure.
 * @param processed Number of bytes processed.
 */
static void uart_mchp_notify_rx_processed(uart_mchp_dev_data_t *dev_data, size_t processed)
{
	do {
		if (dev_data->async_cb == NULL) {
			break;
		}

		if (dev_data->rx_processed_len == processed) {
			break;
		}

		/* clang-format off */
		struct uart_event evt = {
			.type = UART_RX_RDY,
			.data.rx = {
					.buf = dev_data->rx_buf,
					.offset = dev_data->rx_processed_len,
					.len = processed - dev_data->rx_processed_len,
				},
		};
		/* clang-format on */

		dev_data->rx_processed_len = processed;
		dev_data->async_cb(dev_data->dev, &evt, dev_data->async_cb_data);
	} while (0);
}

/**
 * @brief UART TX timeout handler.
 *
 * This function handles UART TX timeout events.
 *
 * @param work Pointer to the work structure.
 */
static void uart_mchp_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	uart_mchp_dev_data_t *dev_data = CONTAINER_OF(dwork, uart_mchp_dev_data_t, tx_timeout_work);

	uart_mchp_tx_halt(dev_data);
}

/**
 * @brief UART RX timeout handler.
 *
 * This function handles UART RX timeout events.
 *
 * @param work Pointer to the work structure.
 */
static void uart_mchp_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	uart_mchp_dev_data_t *dev_data = CONTAINER_OF(dwork, uart_mchp_dev_data_t, rx_timeout_work);
	const uart_mchp_dev_cfg_t *const cfg = dev_data->cfg;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;
	struct dma_status dma_stat;
	unsigned int key = irq_lock();

	do {
		if (dev_data->rx_len == 0U) {
			irq_unlock(key);
			break;
		}

		/*
		 * Stop the DMA transfer and restart the interrupt read
		 * component (so the timeout restarts if there's still data).
		 * However, just ignore it if the transfer has completed (nothing
		 * pending) that means the DMA ISR is already pending, so just let
		 * it handle things instead when we re-enable IRQs.
		 */
		dma_stop(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel);
		if ((dma_get_status(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel,
				    &dma_stat) == 0) &&
		    (dma_stat.pending_length == 0U)) {
			irq_unlock(key);
			break;
		}

		uint8_t *rx_dma_start =
			dev_data->rx_buf + dev_data->rx_len - dma_stat.pending_length;
		size_t rx_processed = rx_dma_start - dev_data->rx_buf;

		/*
		 * We know we still have space, since the above will catch the
		 * empty buffer, so always restart the transfer.
		 */
		dma_reload(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel,
			   (uint32_t)(uart_get_dma_source_addr(regs, is_clock_external)),
			   (uint32_t)rx_dma_start, dev_data->rx_len - rx_processed);

		dev_data->rx_waiting_for_irq = true;
		uart_enable_rx_interrupt(regs, is_clock_external, true);

		/*
		 * Never do a notify on a timeout started from the ISR: timing
		 * granularity means the first timeout can be in the middle
		 * of reception but still have the total elapsed time exhausted.
		 * So we require a timeout chunk with no data seen at all
		 * (i.e. no ISR entry).
		 */
		if (dev_data->rx_timeout_from_isr == true) {
			dev_data->rx_timeout_from_isr = false;
			k_work_reschedule(&dev_data->rx_timeout_work,
					  K_USEC(dev_data->rx_timeout_chunk));
			irq_unlock(key);
			break;
		}

		uint32_t now = k_uptime_get_32();

		/* Convert the difference to microseconds */
		uint32_t elapsed = (now - dev_data->rx_timeout_start) * USEC_PER_MSEC;

		if (elapsed >= dev_data->rx_timeout_time) {
			/*
			 * No time left, so call the handler, and let the ISR
			 * restart the timeout when it sees data.
			 */
			uart_mchp_notify_rx_processed(dev_data, rx_processed);
		} else {
			/*
			 * Still have time left, so start another timeout.
			 */
			uint32_t remaining = MIN(dev_data->rx_timeout_time - elapsed,
						 dev_data->rx_timeout_chunk);

			k_work_reschedule(&dev_data->rx_timeout_work, K_USEC(remaining));
		}

		irq_unlock(key);
	} while (0);
}

/**
 * @brief UART DMA TX done handler.
 *
 * This function handles the completion of UART DMA TX transfer.
 *
 * @param dma_dev Pointer to the DMA device structure.
 * @param arg User argument.
 * @param id DMA channel ID.
 * @param error_code Error code.
 */
static void uart_mchp_dma_tx_done(const struct device *dma_dev, void *arg, uint32_t id,
				  int error_code)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	uart_mchp_dev_data_t *const dev_data = (uart_mchp_dev_data_t *const)arg;
	const uart_mchp_dev_cfg_t *const cfg = dev_data->cfg;

	uart_enable_tx_complete_interrupt(cfg->regs, cfg->is_clock_external, true);
}

/**
 * @brief DMA RX done callback for UART.
 *
 * This function is called when the DMA transfer for UART RX is completed.
 *
 * @param dma_dev Pointer to the DMA device.
 * @param arg Pointer to the UART device data.
 * @param id DMA channel ID.
 * @param error_code Error code from the DMA transfer.
 */
static void uart_mchp_dma_rx_done(const struct device *dma_dev, void *arg, uint32_t id,
				  int error_code)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	uart_mchp_dev_data_t *const dev_data = (uart_mchp_dev_data_t *const)arg;
	const struct device *dev = dev_data->dev;
	const uart_mchp_dev_cfg_t *const cfg = dev_data->cfg;
	unsigned int key = irq_lock();

	do {
		if (dev_data->rx_len == 0U) {
			break;
		}

		uart_mchp_notify_rx_processed(dev_data, dev_data->rx_len);

		if (dev_data->async_cb != NULL) {
			/* clang-format off */
			struct uart_event evt = {
				.type = UART_RX_BUF_RELEASED,
				.data.rx_buf = {
						.buf = dev_data->rx_buf,
					},
			};
			/* clang-format on */

			dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
		}

		/* No next buffer, so end the transfer */
		if (dev_data->rx_next_len == 0) {
			dev_data->rx_buf = NULL;
			dev_data->rx_len = 0U;

			if (dev_data->async_cb != NULL) {
				struct uart_event evt = {
					.type = UART_RX_DISABLED,
				};

				dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
			}
			break;
		}

		dev_data->rx_buf = dev_data->rx_next_buf;
		dev_data->rx_len = dev_data->rx_next_len;
		dev_data->rx_next_buf = NULL;
		dev_data->rx_next_len = 0U;
		dev_data->rx_processed_len = 0U;

		dma_reload(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel,
			   (uint32_t)(uart_get_dma_source_addr(cfg->regs, cfg->is_clock_external)),
			   (uint32_t)dev_data->rx_buf, dev_data->rx_len);

		/*
		 * If there should be a timeout, handle starting the DMA in the
		 * ISR, since reception resets it and DMA completion implies
		 * reception.  This also catches the case of DMA completion during
		 * timeout handling.
		 */
		if (dev_data->rx_timeout_time != SYS_FOREVER_US) {
			dev_data->rx_waiting_for_irq = true;
			uart_enable_rx_interrupt(cfg->regs, cfg->is_clock_external, true);
			break;
		}

		/* Otherwise, start the transfer immediately. */
		dma_start(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel);

		if (dev_data->async_cb != NULL) {
			struct uart_event evt = {
				.type = UART_RX_BUF_REQUEST,
			};

			dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
		}
	} while (0);

	irq_unlock(key);
}

/**
 * @brief Set UART callback function.
 *
 * This function sets the callback function for UART events.
 *
 * @param dev Pointer to the UART device.
 * @param callback Callback function to be set.
 * @param user_data User data to be passed to the callback function.
 *
 * @return 0 on success, negative error code on failure.
 */
static int uart_mchp_callback_set(const struct device *dev, uart_callback_t callback,
				  void *user_data)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;

	dev_data->async_cb = callback;
	dev_data->async_cb_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	dev_data->cb = NULL;
	dev_data->cb_data = NULL;
#endif /* CONFIG_UART_EXCLUSIVE_API_CALLBACKS */

	return 0;
}

/**
 * @brief Transmit data over UART.
 *
 * This function transmits data over UART using DMA.
 *
 * @param dev Pointer to the UART device.
 * @param buf Pointer to the data buffer to be transmitted.
 * @param len Length of the data buffer.
 * @param timeout Timeout for the transmission.
 *
 * @return 0 on success, negative error code on failure.
 * @retval -EINVAL for invalid argument.
 */
static int uart_mchp_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	int retval = UART_SUCCESS;

	do {
		if (cfg->uart_dma.tx_dma_channel == 0xFFU) {
			retval = -ENOTSUP;
			break;
		}

		if (len > 0xFFFFU) {
			retval = -EINVAL;
			break;
		}

		unsigned int key = irq_lock();

		if (dev_data->tx_len != 0U) {
			retval = -EBUSY;
			irq_unlock(key);
			break;
		}

		dev_data->tx_buf = buf;
		dev_data->tx_len = len;

		irq_unlock(key);

		retval = dma_reload(
			cfg->uart_dma.dma_dev, cfg->uart_dma.tx_dma_channel, (uint32_t)buf,
			(uint32_t)(uart_get_dma_dest_addr(cfg->regs, cfg->is_clock_external)), len);
		if (retval != 0U) {
			break;
		}

		if (timeout != SYS_FOREVER_US) {
			k_work_reschedule(&dev_data->tx_timeout_work, K_USEC(timeout));
		}

		retval = dma_start(cfg->uart_dma.dma_dev, cfg->uart_dma.tx_dma_channel);
		if (retval != 0U) {
			break;
		}
	} while (0);

	return retval;
}

/**
 * @brief Abort UART transmission.
 *
 * This function aborts the ongoing UART transmission.
 *
 * @param dev Pointer to the UART device.
 *
 * @return 0 on success, negative error code on failure.
 */
static int uart_mchp_tx_abort(const struct device *dev)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	int retval = UART_SUCCESS;

	do {
		if (cfg->uart_dma.tx_dma_channel == 0xFFU) {
			retval = -ENOTSUP;
			break;
		}

		k_work_cancel_delayable(&dev_data->tx_timeout_work);

		retval = uart_mchp_tx_halt(dev_data);
	} while (0);

	return retval;
}

/**
 * @brief Provide a new RX buffer for UART.
 *
 * This function provides a new buffer for UART RX.
 *
 * @param dev Pointer to the UART device.
 * @param buf Pointer to the new RX buffer.
 * @param len Length of the new RX buffer.
 *
 * @return 0 on success, negative error code on failure.
 * @retval -EINVAL for invalid argument.
 */
static int uart_mchp_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;
	int retval = UART_SUCCESS;

	do {
		if (len > 0xFFFFU) {
			retval = -EINVAL;
			break;
		}

		unsigned int key = irq_lock();

		do {
			if (dev_data->rx_len == 0U) {
				retval = -EACCES;
				break;
			}
			if (dev_data->rx_next_len != 0U) {
				retval = -EBUSY;
				break;
			}
			dev_data->rx_next_buf = buf;
			dev_data->rx_next_len = len;
		} while (0);
		irq_unlock(key);

	} while (0);

	return retval;
}

/**
 * @brief Enable UART RX.
 *
 * This function enables UART RX and sets up the RX buffer and timeout.
 *
 * @param dev Pointer to the UART device.
 * @param buf Pointer to the RX buffer.
 * @param len Length of the RX buffer.
 * @param timeout Timeout for the RX operation.
 *
 * @return 0 on success, negative error code on failure.
 * @retval -EINVAL for invalid argument.
 */
static int uart_mchp_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	sercom_registers_t *regs = cfg->regs;
	bool is_clock_external = cfg->is_clock_external;
	int retval = UART_SUCCESS;

	do {
		if (cfg->uart_dma.rx_dma_channel == 0xFFU) {
			retval = -ENOTSUP;
			break;
		}

		if (len > 0xFFFFU) {
			retval = -EINVAL;
			break;
		}

		unsigned int key = irq_lock();

		do {
			if (dev_data->rx_len != 0U) {
				retval = -EBUSY;
				break;
			}

			/* Read off anything that was already there */
			while (uart_is_rx_complete(regs, is_clock_external) == true) {
				char discard = uart_get_received_char(regs, is_clock_external);
				(void)discard;
			}

			/* Enable error interrupt */
			uart_enable_err_interrupt(cfg->regs, cfg->is_clock_external, true);

			retval = dma_reload(
				cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel,
				(uint32_t)(uart_get_dma_source_addr(regs, is_clock_external)),
				(uint32_t)buf, len);
			if (retval != 0) {
				break;
			}

			dev_data->rx_buf = buf;
			dev_data->rx_len = len;
			dev_data->rx_processed_len = 0U;
			dev_data->rx_waiting_for_irq = true;
			dev_data->rx_timeout_from_isr = true;
			dev_data->rx_timeout_time = timeout;
			dev_data->rx_timeout_chunk = MAX(timeout / 4U, 1);

			uart_enable_rx_interrupt(regs, is_clock_external, true);
		} while (0);
		irq_unlock(key);

	} while (0);

	return retval;
}

/**
 * @brief Disable UART RX.
 *
 * This function disables UART RX and stops the DMA transfer.
 *
 * @param dev Pointer to the UART device.
 *
 * @return 0 on success, negative error code on failure.
 */
static int uart_mchp_rx_disable(const struct device *dev)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;
	const uart_mchp_dev_cfg_t *const cfg = dev->config;
	struct dma_status dma_stat;
	int retval = UART_SUCCESS;

	k_work_cancel_delayable(&dev_data->rx_timeout_work);

	unsigned int key = irq_lock();

	do {
		if (dev_data->rx_len == 0U) {
			retval = -EFAULT;
			break;
		}

		uart_enable_rx_interrupt(cfg->regs, cfg->is_clock_external, false);
		uart_enable_err_interrupt(cfg->regs, cfg->is_clock_external, false);
		dma_stop(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel);

		if ((dma_get_status(cfg->uart_dma.dma_dev, cfg->uart_dma.rx_dma_channel,
				    &dma_stat) == 0) &&
		    (dma_stat.pending_length != 0U)) {
			size_t rx_processed = dev_data->rx_len - dma_stat.pending_length;

			uart_mchp_notify_rx_processed(dev_data, rx_processed);
		}

		/* clang-format off */
		struct uart_event evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf = {
					.buf = dev_data->rx_buf,
				},
		};
		/* clang-format on */

		dev_data->rx_buf = NULL;
		dev_data->rx_len = 0U;

		if (dev_data->async_cb != NULL) {
			dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
		}

		if (dev_data->rx_next_len) {
			/* clang-format off */
			struct uart_event next_evt = {
				.type = UART_RX_BUF_RELEASED,
				.data.rx_buf = {
						.buf = dev_data->rx_next_buf,
					},
			};
			/* clang-format on */

			dev_data->rx_next_buf = NULL;
			dev_data->rx_next_len = 0U;

			if (dev_data->async_cb != NULL) {
				dev_data->async_cb(dev, &next_evt, dev_data->async_cb_data);
			}
		}

		evt.type = UART_RX_DISABLED;
		if (dev_data->async_cb != NULL) {
			dev_data->async_cb(dev, &evt, dev_data->async_cb_data);
		}
	} while (0);
	irq_unlock(key);

	return retval;
}
#endif /* CONFIG_UART_MCHP_ASYNC */

/******************************************************************************
 * @brief Zephyr driver instance creation
 *****************************************************************************/
static DEVICE_API(uart, uart_mchp_driver_api) = {
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_mchp_configure,
	.config_get = uart_mchp_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

	.poll_in = uart_mchp_poll_in,
	.poll_out = uart_mchp_poll_out,
	.err_check = uart_mchp_err_check,

#if CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_mchp_fifo_fill,
	.fifo_read = uart_mchp_fifo_read,
	.irq_tx_enable = uart_mchp_irq_tx_enable,
	.irq_tx_disable = uart_mchp_irq_tx_disable,
	.irq_tx_ready = uart_mchp_irq_tx_ready,
	.irq_tx_complete = uart_mchp_irq_tx_complete,
	.irq_rx_enable = uart_mchp_irq_rx_enable,
	.irq_rx_disable = uart_mchp_irq_rx_disable,
	.irq_rx_ready = uart_mchp_irq_rx_ready,
	.irq_is_pending = uart_mchp_irq_is_pending,
	.irq_err_enable = uart_mchp_irq_err_enable,
	.irq_err_disable = uart_mchp_irq_err_disable,
	.irq_update = uart_mchp_irq_update,
	.irq_callback_set = uart_mchp_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_MCHP_ASYNC
	.callback_set = uart_mchp_callback_set,
	.tx = uart_mchp_tx,
	.tx_abort = uart_mchp_tx_abort,
	.rx_enable = uart_mchp_rx_enable,
	.rx_buf_rsp = uart_mchp_rx_buf_rsp,
	.rx_disable = uart_mchp_rx_disable,
#endif /* CONFIG_UART_MCHP_ASYNC */
};

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_MCHP_ASYNC)
#define MCHP_UART_IRQ_CONNECT(n, m)                                                                \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    uart_mchp_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#define UART_MCHP_IRQ_HANDLER_DECL(n) static void uart_mchp_irq_config_##n(const struct device *dev)
#define UART_MCHP_IRQ_HANDLER_FUNC(n) .irq_config_func = uart_mchp_irq_config_##n,

#else /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_MCHP_ASYNC */
#define UART_MCHP_IRQ_HANDLER_DECL(n)
#define UART_MCHP_IRQ_HANDLER_FUNC(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_MCHP_ASYNC */

#ifdef CONFIG_UART_MCHP_ASYNC
#define UART_MCHP_DMA_CHANNELS(n)                                                                  \
	.uart_dma.dma_dev = DEVICE_DT_GET(MCHP_DT_INST_DMA_CTLR(n, tx)),                           \
	.uart_dma.tx_dma_request = MCHP_DT_INST_DMA_TRIGSRC(n, tx),                                \
	.uart_dma.tx_dma_channel = MCHP_DT_INST_DMA_CHANNEL(n, tx),                                \
	.uart_dma.rx_dma_request = MCHP_DT_INST_DMA_TRIGSRC(n, rx),                                \
	.uart_dma.rx_dma_channel = MCHP_DT_INST_DMA_CHANNEL(n, rx),
#else /* CONFIG_UART_MCHP_ASYNC */
#define UART_MCHP_DMA_CHANNELS(n)
#endif /* CONFIG_UART_MCHP_ASYNC */

#define UART_MCHP_CONFIG_DEFN(n)                                                                   \
	static const uart_mchp_dev_cfg_t uart_mchp_config_##n = {                                  \
		.baudrate = DT_INST_PROP(n, current_speed),                                        \
		.data_bits = DT_INST_ENUM_IDX_OR(n, data_bits, UART_CFG_DATA_BITS_8),              \
		.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),                    \
		.stop_bits = DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.regs = (sercom_registers_t *)DT_INST_REG_ADDR(n),                                 \
		.rxpo = (DT_INST_PROP(n, rxpo)),                                                   \
		.txpo = (DT_INST_PROP(n, txpo)),                                                   \
		.is_clock_external = DT_INST_PROP(n, clock_external),                              \
		.run_in_standby_en = DT_INST_PROP(n, run_in_standby_en),                           \
		UART_MCHP_IRQ_HANDLER_FUNC(n) UART_MCHP_DMA_CHANNELS(n) UART_MCHP_CLOCK_DEFN(n)}

#define UART_MCHP_DEVICE_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	UART_MCHP_IRQ_HANDLER_DECL(n);                                                             \
	UART_MCHP_CONFIG_DEFN(n);                                                                  \
	static uart_mchp_dev_data_t uart_mchp_data_##n;                                            \
	DEVICE_DT_INST_DEFINE(n, uart_mchp_init, NULL, &uart_mchp_data_##n, &uart_mchp_config_##n, \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_mchp_driver_api);   \
	UART_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(UART_MCHP_DEVICE_INIT)
