/*
 * Copyright (c) 2025 Microchip Technology Inc.
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
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <string.h>

/*******************************************
 * Const and Macro Defines
 ******************************************
 */

/* Define compatible string */
#define DT_DRV_COMPAT microchip_sercom_g1_uart

#define UART_SUCCESS           0
#define BITSHIFT_FOR_BAUD_CALC 20

/* Do the peripheral clock related configuration */

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
	bool clock_external;

	/* RX pinout configuration. */
	uint32_t rxpo;

	/* TX pinout configuration. */
	uint32_t txpo;

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
} uart_mchp_dev_data_t;

/**
 * @brief Wait for synchronization of the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 */
static void uart_wait_sync(sercom_registers_t *regs, bool clock_external)
{
	if (clock_external == false) {
		while ((regs->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_Msk) != 0) {
		}
	} else {
		while ((regs->USART_EXT.SERCOM_SYNCBUSY & SERCOM_USART_EXT_SYNCBUSY_Msk) != 0) {
		}
	}
}

/**
 * @brief Disable UART interrupts.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 */
static inline void uart_disable_interrupts(sercom_registers_t *regs, bool clock_external)
{
	if (clock_external == false) {
		regs->USART_INT.SERCOM_INTENCLR = SERCOM_USART_INT_INTENCLR_Msk;
	} else {
		regs->USART_EXT.SERCOM_INTENCLR = SERCOM_USART_EXT_INTENCLR_Msk;
	}
}

/**
 * @brief Configure the number of data bits for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param count Number of data bits (5 to 9).
 * @return 0 on success, -1 on invalid count.
 */
static int uart_config_data_bits(sercom_registers_t *regs, bool clock_external, unsigned int count)
{
	uint32_t value;
	int retval = UART_SUCCESS;

	do {
		if (clock_external == false) {
			switch (count) {
			case UART_CFG_DATA_BITS_5:
				value = SERCOM_USART_INT_CTRLB_CHSIZE_5_BIT;
				break;
			case UART_CFG_DATA_BITS_6:
				value = SERCOM_USART_INT_CTRLB_CHSIZE_6_BIT;
				break;
			case UART_CFG_DATA_BITS_7:
				value = SERCOM_USART_INT_CTRLB_CHSIZE_7_BIT;
				break;
			case UART_CFG_DATA_BITS_8:
				value = SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT;
				break;
			case UART_CFG_DATA_BITS_9:
				value = SERCOM_USART_INT_CTRLB_CHSIZE_9_BIT;
				break;
			default:
				retval = -ENOTSUP;
			}
		} else {
			switch (count) {
			case UART_CFG_DATA_BITS_5:
				value = SERCOM_USART_EXT_CTRLB_CHSIZE_5_BIT;
				break;
			case UART_CFG_DATA_BITS_6:
				value = SERCOM_USART_EXT_CTRLB_CHSIZE_6_BIT;
				break;
			case UART_CFG_DATA_BITS_7:
				value = SERCOM_USART_EXT_CTRLB_CHSIZE_7_BIT;
				break;
			case UART_CFG_DATA_BITS_8:
				value = SERCOM_USART_EXT_CTRLB_CHSIZE_8_BIT;
				break;
			case UART_CFG_DATA_BITS_9:
				value = SERCOM_USART_EXT_CTRLB_CHSIZE_9_BIT;
				break;
			default:
				retval = -ENOTSUP;
			}
		}
		if (retval != UART_SUCCESS) {
			break;
		}

		/* Writing to the CTRLB register requires synchronization */
		if (clock_external == false) {
			regs->USART_INT.SERCOM_CTRLB &= ~SERCOM_USART_INT_CTRLB_CHSIZE_Msk;
			regs->USART_INT.SERCOM_CTRLB |= value;
		} else {
			regs->USART_EXT.SERCOM_CTRLB &= ~SERCOM_USART_EXT_CTRLB_CHSIZE_Msk;
			regs->USART_EXT.SERCOM_CTRLB |= value;
		}
		uart_wait_sync(regs, clock_external);
	} while (0);

	return retval;
}

/**
 * @brief Configure the parity mode for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param parity Parity mode to configure.
 */
static void uart_config_parity(sercom_registers_t *regs, bool clock_external,
			       enum uart_config_parity parity)
{
	if (clock_external == false) {
		regs->USART_INT.SERCOM_CTRLA &= ~SERCOM_USART_INT_CTRLA_FORM_Msk;
		switch (parity) {
		case UART_CFG_PARITY_ODD: {
			regs->USART_INT.SERCOM_CTRLA |=
				SERCOM_USART_INT_CTRLA_FORM_USART_FRAME_WITH_PARITY;

			/* Writing to the CTRLB register requires synchronization */
			regs->USART_INT.SERCOM_CTRLB |= SERCOM_USART_INT_CTRLB_PMODE_Msk;
			uart_wait_sync(regs, clock_external);
			break;
		}
		case UART_CFG_PARITY_EVEN: {
			regs->USART_INT.SERCOM_CTRLA |=
				SERCOM_USART_INT_CTRLA_FORM_USART_FRAME_WITH_PARITY;

			/* Writing to the CTRLB register requires synchronization */
			regs->USART_INT.SERCOM_CTRLB &= ~SERCOM_USART_INT_CTRLB_PMODE_Msk;
			uart_wait_sync(regs, clock_external);
			break;
		}
		default: {
			regs->USART_INT.SERCOM_CTRLA |=
				SERCOM_USART_INT_CTRLA_FORM_USART_FRAME_NO_PARITY;
			break;
		}
		}
	} else {
		regs->USART_EXT.SERCOM_CTRLA &= ~SERCOM_USART_EXT_CTRLA_FORM_Msk;
		switch (parity) {
		case UART_CFG_PARITY_ODD: {
			regs->USART_EXT.SERCOM_CTRLA |=
				SERCOM_USART_EXT_CTRLA_FORM_USART_FRAME_WITH_PARITY;

			/* Writing to the CTRLB register requires synchronization */
			regs->USART_EXT.SERCOM_CTRLB |= SERCOM_USART_EXT_CTRLB_PMODE_Msk;
			uart_wait_sync(regs, clock_external);
			break;
		}
		case UART_CFG_PARITY_EVEN: {
			regs->USART_EXT.SERCOM_CTRLA |=
				SERCOM_USART_EXT_CTRLA_FORM_USART_FRAME_WITH_PARITY;

			/* Writing to the CTRLB register requires synchronization */
			regs->USART_EXT.SERCOM_CTRLB &= ~SERCOM_USART_EXT_CTRLB_PMODE_Msk;
			uart_wait_sync(regs, clock_external);
			break;
		}
		default: {
			regs->USART_EXT.SERCOM_CTRLA |=
				SERCOM_USART_EXT_CTRLA_FORM_USART_FRAME_NO_PARITY;
			break;
		}
		}
	}
}

/**
 * @brief Configure the number of stop bits for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param count Number of stop bits (1 or 2).
 * @return 0 on success, -1 on invalid count.
 */
static int uart_config_stop_bits(sercom_registers_t *regs, bool clock_external, unsigned int count)
{
	int retval = UART_SUCCESS;

	do {
		if (clock_external == false) {
			if (count == UART_CFG_STOP_BITS_1) {
				regs->USART_INT.SERCOM_CTRLB &= ~SERCOM_USART_INT_CTRLB_SBMODE_Msk;
			} else if (count == UART_CFG_STOP_BITS_2) {
				regs->USART_INT.SERCOM_CTRLB |= SERCOM_USART_INT_CTRLB_SBMODE_Msk;
			} else {
				retval = -ENOTSUP;
				break;
			}
		} else {
			if (count == UART_CFG_STOP_BITS_1) {
				regs->USART_EXT.SERCOM_CTRLB &= ~SERCOM_USART_EXT_CTRLB_SBMODE_Msk;
			} else if (count == UART_CFG_STOP_BITS_2) {
				regs->USART_EXT.SERCOM_CTRLB |= SERCOM_USART_EXT_CTRLB_SBMODE_Msk;
			} else {
				retval = -ENOTSUP;
				break;
			}
		}
		uart_wait_sync(regs, clock_external);
	} while (0);

	return retval;
}

/**
 * @brief Configure the UART pinout.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 */
static void uart_config_pinout(const uart_mchp_dev_cfg_t *const cfg)
{
	uint32_t reg_value;

	sercom_registers_t *regs = cfg->regs;
	uint32_t rxpo = cfg->rxpo;
	uint32_t txpo = cfg->txpo;

	if (cfg->clock_external == false) {
		reg_value = regs->USART_INT.SERCOM_CTRLA;
		reg_value &= ~(SERCOM_USART_INT_CTRLA_RXPO_Msk | SERCOM_USART_INT_CTRLA_TXPO_Msk);
		reg_value |=
			(SERCOM_USART_INT_CTRLA_RXPO(rxpo) | SERCOM_USART_INT_CTRLA_TXPO(txpo));
		cfg->regs->USART_INT.SERCOM_CTRLA = reg_value;
	} else {
		reg_value = regs->USART_EXT.SERCOM_CTRLA;
		reg_value &= ~(SERCOM_USART_EXT_CTRLA_RXPO_Msk | SERCOM_USART_EXT_CTRLA_TXPO_Msk);
		reg_value |=
			(SERCOM_USART_EXT_CTRLA_RXPO(rxpo) | SERCOM_USART_EXT_CTRLA_TXPO(txpo));
		regs->USART_EXT.SERCOM_CTRLA = reg_value;
	}
}

/**
 * @brief Set clock polarity for UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param tx_rising transmit on rising edge
 */
static void uart_set_clock_polarity(sercom_registers_t *regs, bool clock_external, bool tx_rising)
{
	if (clock_external == false) {
		if (tx_rising == true) {
			regs->USART_INT.SERCOM_CTRLA &= ~SERCOM_USART_INT_CTRLA_CPOL_Msk;
		} else {
			regs->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_CPOL_Msk;
		}
	} else {
		if (tx_rising == true) {
			regs->USART_EXT.SERCOM_CTRLA &= ~SERCOM_USART_EXT_CTRLA_CPOL_Msk;
		} else {
			regs->USART_EXT.SERCOM_CTRLA |= SERCOM_USART_EXT_CTRLA_CPOL_Msk;
		}
	}
}

/**
 * @brief Set the clock source for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 */
static void uart_set_clock_source(sercom_registers_t *regs, bool clock_external)
{
	uint32_t reg_value;

	reg_value = regs->USART_INT.SERCOM_CTRLA;
	reg_value &= ~SERCOM_USART_INT_CTRLA_MODE_Msk;

	if (clock_external == true) {
		regs->USART_INT.SERCOM_CTRLA =
			reg_value | SERCOM_USART_INT_CTRLA_MODE_USART_EXT_CLK;
	} else {
		regs->USART_INT.SERCOM_CTRLA =
			reg_value | SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK;
	}
}

/**
 * @brief Set the data order for the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param lsb_first Boolean to set the data order.
 */
static void uart_set_lsb_first(sercom_registers_t *regs, bool clock_external, bool lsb_first)
{
	if (clock_external == false) {
		if (lsb_first == true) {
			regs->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_DORD_Msk;
		} else {
			regs->USART_INT.SERCOM_CTRLA &= ~SERCOM_USART_INT_CTRLA_DORD_Msk;
		}
	} else {
		if (lsb_first == true) {
			regs->USART_EXT.SERCOM_CTRLA |= SERCOM_USART_EXT_CTRLA_DORD_Msk;
		} else {
			regs->USART_EXT.SERCOM_CTRLA &= ~SERCOM_USART_EXT_CTRLA_DORD_Msk;
		}
	}
}

/**
 * @brief Enable or disable the UART receiver.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param enable Boolean to enable or disable the receiver.
 */
static void uart_rx_on_off(sercom_registers_t *regs, bool clock_external, bool enable)
{
	if (clock_external == false) {
		if (enable == true) {
			regs->USART_INT.SERCOM_CTRLB |= SERCOM_USART_INT_CTRLB_RXEN_Msk;
		} else {
			regs->USART_INT.SERCOM_CTRLB &= ~SERCOM_USART_INT_CTRLB_RXEN_Msk;
		}
	} else {
		if (enable == true) {
			regs->USART_EXT.SERCOM_CTRLB |= SERCOM_USART_EXT_CTRLB_RXEN_Msk;
		} else {
			regs->USART_EXT.SERCOM_CTRLB &= ~SERCOM_USART_EXT_CTRLB_RXEN_Msk;
		}
	}

	/* Writing to the CTRLB register requires synchronization */
	uart_wait_sync(regs, clock_external);
}

/**
 * @brief Enable or disable the UART transmitter.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param enable Boolean to enable or disable the transmitter.
 */
static void uart_tx_on_off(sercom_registers_t *regs, bool clock_external, bool enable)
{
	if (clock_external == false) {
		if (enable == true) {
			regs->USART_INT.SERCOM_CTRLB |= SERCOM_USART_INT_CTRLB_TXEN_Msk;
		} else {
			regs->USART_INT.SERCOM_CTRLB &= ~SERCOM_USART_INT_CTRLB_TXEN_Msk;
		}
	} else {
		if (enable == true) {
			regs->USART_EXT.SERCOM_CTRLB |= SERCOM_USART_EXT_CTRLB_TXEN_Msk;
		} else {
			regs->USART_EXT.SERCOM_CTRLB &= ~SERCOM_USART_EXT_CTRLB_TXEN_Msk;
		}
	}

	/* Writing to the CTRLB register requires synchronization */
	uart_wait_sync(regs, clock_external);
}

/**
 * @brief Set the UART baud rate.
 *
 * This function sets the baud rate for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param baudrate Desired baud rate.
 * @param clk_freq_hz Clock frequency in Hz.
 * @return 0 on success, -ERANGE if the calculated baud rate is out of range.
 */
static int uart_set_baudrate(sercom_registers_t *regs, bool clock_external, uint32_t baudrate,
			     uint32_t clk_freq_hz)
{
	uint64_t tmp;
	uint16_t baud;

	int retval = UART_SUCCESS;

	do {
		if (clk_freq_hz == 0) {
			retval = -EINVAL;
			break;
		}
		tmp = (uint64_t)baudrate << BITSHIFT_FOR_BAUD_CALC;
		tmp = (tmp + (clk_freq_hz >> 1)) / clk_freq_hz;

		/* Verify that the calculated result is within range */
		if ((tmp < 1) || (tmp > UINT16_MAX)) {
			retval = -ERANGE;
			break;
		}

		baud = (UINT16_MAX + 1) - (uint16_t)tmp;

		if (clock_external == false) {
			regs->USART_INT.SERCOM_CTRLA &= ~SERCOM_USART_INT_CTRLA_SAMPR_Msk;
			regs->USART_INT.SERCOM_BAUD = baud;
		} else {
			regs->USART_EXT.SERCOM_CTRLA &= ~SERCOM_USART_EXT_CTRLA_SAMPR_Msk;
			regs->USART_EXT.SERCOM_BAUD = baud;
		}
	} while (0);

	return retval;
}

/**
 * @brief Enable or disable the UART.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param enable Boolean to enable or disable the UART.
 */
static void uart_enable(sercom_registers_t *regs, bool clock_external, bool enable)
{
	if (clock_external == false) {
		if (enable == true) {
			regs->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_RUNSTDBY_Msk;
			regs->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE_Msk;
		} else {
			regs->USART_INT.SERCOM_CTRLA &= ~SERCOM_USART_INT_CTRLA_ENABLE_Msk;
		}
	} else {
		if (enable == true) {
			regs->USART_EXT.SERCOM_CTRLA |= SERCOM_USART_EXT_CTRLA_RUNSTDBY_Msk;
			regs->USART_EXT.SERCOM_CTRLA |= SERCOM_USART_EXT_CTRLA_ENABLE_Msk;
		} else {
			regs->USART_EXT.SERCOM_CTRLA &= ~SERCOM_USART_EXT_CTRLA_ENABLE_Msk;
		}
	}

	/* Enabling and disabling the SERCOM (CTRLA.ENABLE) requires synchronization */
	uart_wait_sync(regs, clock_external);
}

/**
 * @brief Check if the UART receive is complete.
 *
 * This function checks if the receive operation is complete for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if receive is complete, false otherwise.
 */
static inline bool uart_is_rx_complete(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_RXC_Msk) != 0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_INTFLAG & SERCOM_USART_EXT_INTFLAG_RXC_Msk) != 0);
	}

	return retval;
}

/**
 * @brief Get the received character from the UART.
 *
 * This function retrieves the received character from the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return The received character.
 */
static inline unsigned char uart_get_received_char(sercom_registers_t *regs, bool clock_external)
{
	unsigned char retval;

	if (clock_external == false) {
		retval = (unsigned char)regs->USART_INT.SERCOM_DATA;
	} else {
		retval = (unsigned char)regs->USART_EXT.SERCOM_DATA;
	}

	return retval;
}

/**
 * @brief Check if the UART TX is ready.
 *
 * This function checks if the TX operation is ready for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if TX is ready, false otherwise.
 */
static inline bool uart_is_tx_ready(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) != 0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_INTFLAG & SERCOM_USART_EXT_INTFLAG_DRE_Msk) != 0);
	}

	return retval;
}

/**
 * @brief Transmit a character via UART.
 *
 * This function transmits a character via the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param data The character to transmit.
 */
static inline void uart_tx_char(sercom_registers_t *regs, bool clock_external, unsigned char data)
{
	if (clock_external == false) {
		regs->USART_INT.SERCOM_DATA = data;
	} else {
		regs->USART_EXT.SERCOM_DATA = data;
	}
}

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
	bool clock_external = cfg->clock_external;
	int retval = UART_SUCCESS;

	do {
		/* Enable the GCLK and MCLK*/
		retval = clock_control_on(cfg->uart_clock.clock_dev, cfg->uart_clock.gclk_sys);
		if ((retval != UART_SUCCESS) && (retval != -EALREADY)) {
			break;
		}
		retval = clock_control_on(cfg->uart_clock.clock_dev, cfg->uart_clock.mclk_sys);
		if ((retval != UART_SUCCESS) && (retval != -EALREADY)) {
			break;
		}

		uart_disable_interrupts(regs, clock_external);

		dev_data->config_cache.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

		retval = uart_config_data_bits(regs, clock_external, cfg->data_bits);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.data_bits = cfg->data_bits;

		uart_config_parity(regs, clock_external, cfg->parity);
		dev_data->config_cache.parity = cfg->parity;

		retval = uart_config_stop_bits(regs, clock_external, cfg->stop_bits);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.stop_bits = cfg->stop_bits;

		uart_config_pinout(cfg);
		uart_set_clock_polarity(regs, clock_external, false);
		uart_set_clock_source(regs, clock_external);
		uart_set_lsb_first(regs, clock_external, true);

		/* Enable PINMUX based on PINCTRL */
		retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (retval != UART_SUCCESS) {
			break;
		}

		/* Enable receiver and transmitter */
		uart_rx_on_off(regs, clock_external, true);
		uart_tx_on_off(regs, clock_external, true);

		uint32_t clock_rate;

		clock_control_get_rate(cfg->uart_clock.clock_dev, cfg->uart_clock.gclk_sys,
				       &clock_rate);

		retval = uart_set_baudrate(regs, clock_external, cfg->baudrate, clock_rate);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.baudrate = cfg->baudrate;

		uart_enable(regs, clock_external, true);
	} while (0);

	return retval;
}

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
	bool clock_external = cfg->clock_external;
	int retval = UART_SUCCESS;

	if (uart_is_rx_complete(regs, clock_external) == false) {
		retval = -EBUSY;
	} else {
		*data = uart_get_received_char(regs, clock_external);
	}

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
	bool clock_external = cfg->clock_external;

	while (uart_is_tx_ready(regs, clock_external) == false) {
	}

	/* send a character */
	uart_tx_char(regs, clock_external, data);
}

static DEVICE_API(uart, uart_mchp_driver_api) = {
	.poll_in = uart_mchp_poll_in,
	.poll_out = uart_mchp_poll_out,
};

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
		.clock_external = DT_INST_PROP(n, clock_external),                                 \
		UART_MCHP_CLOCK_DEFN(n)}

#define UART_MCHP_DEVICE_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	UART_MCHP_CONFIG_DEFN(n);                                                                  \
	static uart_mchp_dev_data_t uart_mchp_data_##n;                                            \
	DEVICE_DT_INST_DEFINE(n, uart_mchp_init, NULL, &uart_mchp_data_##n, &uart_mchp_config_##n, \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_mchp_driver_api)

DT_INST_FOREACH_STATUS_OKAY(UART_MCHP_DEVICE_INIT)
