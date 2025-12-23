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

/******************************************************************************
 * @brief Devicetree definitions
 *****************************************************************************/
#define DT_DRV_COMPAT microchip_sercom_g1_uart

/******************************************************************************
 * @brief Macro definitions
 *****************************************************************************/
#define UART_SUCCESS           0
#define BITSHIFT_FOR_BAUD_CALC 20

/* Do the peripheral interrupt related configuration */
#if CONFIG_UART_INTERRUPT_DRIVEN

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

#else /* CONFIG_UART_INTERRUPT_DRIVEN */
#define UART_MCHP_IRQ_HANDLER(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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

	/* defines the functionality in standby sleep mode */
	uint8_t run_in_standby_en;

	/* RX pinout configuration. */
	uint32_t rxpo;

	/* TX pinout configuration. */
	uint32_t txpo;

#if CONFIG_UART_INTERRUPT_DRIVEN
	/* IRQ configuration function */
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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
} uart_mchp_dev_data_t;

/******************************************************************************
 * @brief Helper functions
 *****************************************************************************/
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
 * @retval -EINVAL for invalid argument.
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
 * @param run_in_standby Boolean to enable UART operation in standby mode.
 * @param enable Boolean to enable or disable the UART.
 */
static void uart_enable(sercom_registers_t *regs, bool clock_external, bool run_in_standby,
			bool enable)
{
	if (clock_external == false) {
		if (enable == true) {
			if (run_in_standby == true) {
				regs->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_RUNSTDBY_Msk;
			}
			regs->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE_Msk;
		} else {
			regs->USART_INT.SERCOM_CTRLA &= ~SERCOM_USART_INT_CTRLA_ENABLE_Msk;
		}
	} else {
		if (enable == true) {
			if (run_in_standby == true) {
				regs->USART_EXT.SERCOM_CTRLA |= SERCOM_USART_EXT_CTRLA_RUNSTDBY_Msk;
			}
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
static bool uart_is_rx_complete(sercom_registers_t *regs, bool clock_external)
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
 * @brief Check if the UART TX is ready
 *
 * This function checks if the TX operation is ready for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if TX is ready, false otherwise.
 */
static bool uart_is_tx_ready(sercom_registers_t *regs, bool clock_external)
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
 * @brief Check if there is a buffer overflow error.
 *
 * This function checks if there is a buffer overflow error for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if there is a buffer overflow error, false otherwise.
 */
static bool uart_is_err_buffer_overflow(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval =
			((regs->USART_INT.SERCOM_STATUS & SERCOM_USART_INT_STATUS_BUFOVF_Msk) != 0);
	} else {
		retval =
			((regs->USART_EXT.SERCOM_STATUS & SERCOM_USART_EXT_STATUS_BUFOVF_Msk) != 0);
	}

	return retval;
}

/**
 * @brief Check if there is a frame error.
 *
 * This function checks if there is a frame error for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if there is a frame error, false otherwise.
 */
static bool uart_is_err_frame(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_STATUS & SERCOM_USART_INT_STATUS_FERR_Msk) != 0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_STATUS & SERCOM_USART_EXT_STATUS_FERR_Msk) != 0);
	}

	return retval;
}

/**
 * @brief Check if there is a parity error.
 *
 * This function checks if there is a parity error for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if there is a parity error, false otherwise.
 */
static bool uart_is_err_parity(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_STATUS & SERCOM_USART_INT_STATUS_PERR_Msk) != 0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_STATUS & SERCOM_USART_EXT_STATUS_PERR_Msk) != 0);
	}

	return retval;
}

/**
 * @brief Check if there is an autobaud synchronization error.
 *
 * This function checks if there is an autobaud synchronization error for the specified UART
 * instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if there is an autobaud synchronization error, false otherwise.
 */
static bool uart_is_err_autobaud_sync(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_STATUS & SERCOM_USART_INT_STATUS_ISF_Msk) != 0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_STATUS & SERCOM_USART_EXT_STATUS_ISF_Msk) != 0);
	}

	return retval;
}

/**
 * @brief Check if there is a collision error.
 *
 * This function checks if there is a collision error for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if there is a collision error, false otherwise.
 */
static bool uart_is_err_collision(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_STATUS & SERCOM_USART_INT_STATUS_COLL_Msk) != 0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_STATUS & SERCOM_USART_EXT_STATUS_COLL_Msk) != 0);
	}

	return retval;
}

/**
 * @brief Clear all UART error flags.
 *
 * This function clears all error flags for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 */
static void uart_err_clear_all(sercom_registers_t *regs, bool clock_external)
{
	if (clock_external == false) {
		regs->USART_INT.SERCOM_STATUS |=
			SERCOM_USART_INT_STATUS_BUFOVF_Msk | SERCOM_USART_INT_STATUS_FERR_Msk |
			SERCOM_USART_INT_STATUS_PERR_Msk | SERCOM_USART_INT_STATUS_ISF_Msk |
			SERCOM_USART_INT_STATUS_COLL_Msk;
	} else {
		regs->USART_EXT.SERCOM_STATUS |=
			SERCOM_USART_EXT_STATUS_BUFOVF_Msk | SERCOM_USART_EXT_STATUS_FERR_Msk |
			SERCOM_USART_EXT_STATUS_PERR_Msk | SERCOM_USART_EXT_STATUS_ISF_Msk |
			SERCOM_USART_EXT_STATUS_COLL_Msk;
	}
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
	bool clock_external = cfg->clock_external;

	if (uart_is_err_buffer_overflow(regs, clock_external) == true) {
		err |= UART_ERROR_OVERRUN;
	}

	if (uart_is_err_frame(regs, clock_external) == true) {
		err |= UART_ERROR_FRAMING;
	}

	if (uart_is_err_parity(regs, clock_external) == true) {
		err |= UART_ERROR_PARITY;
	}

	if (uart_is_err_autobaud_sync(regs, clock_external) == true) {
		err |= UART_BREAK;
	}

	if (uart_is_err_collision(regs, clock_external) == true) {
		err |= UART_ERROR_COLLISION;
	}

	return err;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Check if the UART TX is complete.
 *
 * This function checks if the TX operation is complete for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if TX is complete, false otherwise.
 */
static bool uart_is_tx_complete(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk) != 0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_INTFLAG & SERCOM_USART_EXT_INTFLAG_TXC_Msk) != 0);
	}

	return retval;
}

/**
 * @brief Check if the UART transmit interrupt is enabled.
 *
 * This function checks if transmit interrupt is enabled for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if interrupt is enabled, false otherwise.
 */
static bool uart_is_tx_interrupt_enabled(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_INTENSET & SERCOM_USART_INT_INTENSET_DRE_Msk) !=
			  0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_INTENSET & SERCOM_USART_EXT_INTENSET_DRE_Msk) !=
			  0);
	}

	return retval;
}

/**
 * @brief Check if any UART interrupt is pending.
 *
 * This function checks if any interrupt is pending for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @return True if any interrupt is pending, false otherwise.
 */
static bool uart_is_interrupt_pending(sercom_registers_t *regs, bool clock_external)
{
	bool retval;

	if (clock_external == false) {
		retval = ((regs->USART_INT.SERCOM_INTENSET & regs->USART_INT.SERCOM_INTFLAG) != 0);
	} else {
		retval = ((regs->USART_EXT.SERCOM_INTENSET & regs->USART_EXT.SERCOM_INTFLAG) != 0);
	}

	return retval;
}

/**
 * @brief Enable or disable the UART RX interrupt.
 *
 * This function enables or disables the RX interrupt for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param enable True to enable the interrupt, false to disable.
 */
static void uart_enable_rx_interrupt(sercom_registers_t *regs, bool clock_external, bool enable)
{
	if (clock_external == false) {
		if (enable == true) {
			regs->USART_INT.SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_RXC_Msk;
		} else {
			regs->USART_INT.SERCOM_INTENCLR = SERCOM_USART_INT_INTENCLR_RXC_Msk;
		}
	} else {
		if (enable == true) {
			regs->USART_EXT.SERCOM_INTENSET = SERCOM_USART_EXT_INTENSET_RXC_Msk;
		} else {
			regs->USART_EXT.SERCOM_INTENCLR = SERCOM_USART_EXT_INTENCLR_RXC_Msk;
		}
	}
}

/**
 * @brief Enable or disable the UART TX complete interrupt.
 *
 * This function enables or disables the TX complete interrupt for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param enable True to enable the interrupt, false to disable.
 */
static void uart_enable_tx_complete_interrupt(sercom_registers_t *regs, bool clock_external,
					      bool enable)
{
	if (clock_external == false) {
		if (enable == true) {
			regs->USART_INT.SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_TXC_Msk;
		} else {
			regs->USART_INT.SERCOM_INTENCLR = SERCOM_USART_INT_INTENCLR_TXC_Msk;
		}
	} else {
		if (enable == true) {
			regs->USART_EXT.SERCOM_INTENSET = SERCOM_USART_EXT_INTENSET_TXC_Msk;
		} else {
			regs->USART_EXT.SERCOM_INTENCLR = SERCOM_USART_EXT_INTENCLR_TXC_Msk;
		}
	}
}

/**
 * @brief Enable or disable the UART error interrupt.
 *
 * This function enables or disables the error interrupt for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param enable True to enable the interrupt, false to disable.
 */
static void uart_enable_err_interrupt(sercom_registers_t *regs, bool clock_external, bool enable)
{
	if (clock_external == false) {
		if (enable == true) {
			regs->USART_INT.SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_ERROR_Msk;
		} else {
			regs->USART_INT.SERCOM_INTENCLR = SERCOM_USART_INT_INTENCLR_ERROR_Msk;
		}
	} else {
		if (enable == true) {
			regs->USART_EXT.SERCOM_INTENSET = SERCOM_USART_EXT_INTENSET_ERROR_Msk;
		} else {
			regs->USART_EXT.SERCOM_INTENCLR = SERCOM_USART_EXT_INTENCLR_ERROR_Msk;
		}
	}
}

/**
 * @brief Clear all UART interrupts.
 *
 * This function clears all interrupts for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 */
static void uart_clear_interrupts(sercom_registers_t *regs, bool clock_external)
{
	if (clock_external == false) {
		regs->USART_INT.SERCOM_INTFLAG =
			SERCOM_USART_INT_INTFLAG_ERROR_Msk | SERCOM_USART_INT_INTFLAG_RXBRK_Msk |
			SERCOM_USART_INT_INTFLAG_CTSIC_Msk | SERCOM_USART_INT_INTFLAG_RXS_Msk |
			SERCOM_USART_INT_INTFLAG_TXC_Msk;
	} else {
		regs->USART_EXT.SERCOM_INTFLAG =
			SERCOM_USART_EXT_INTFLAG_ERROR_Msk | SERCOM_USART_EXT_INTFLAG_RXBRK_Msk |
			SERCOM_USART_EXT_INTFLAG_CTSIC_Msk | SERCOM_USART_EXT_INTFLAG_RXS_Msk |
			SERCOM_USART_EXT_INTFLAG_TXC_Msk;
	}
}

/**
 * @brief UART ISR handler.
 *
 * @param dev Device structure.
 */
static void uart_mchp_isr(const struct device *dev)
{
	uart_mchp_dev_data_t *const dev_data = dev->data;

	if (dev_data->cb != NULL) {
		dev_data->cb(dev, dev_data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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

#if CONFIG_UART_INTERRUPT_DRIVEN
		cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

		uart_enable(regs, clock_external, (cfg->run_in_standby_en == 1) ? true : false,
			    true);
	} while (0);

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
	bool clock_external = cfg->clock_external;
	int retval = UART_SUCCESS;

	do {
		/* Forcefully disable UART before configuring. run_in_standby is ignored here */
		uart_enable(regs, clock_external, false, false);

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
			uart_config_parity(regs, clock_external, new_cfg->parity);
			break;
		default:
			retval = -ENOTSUP;
		}
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.parity = new_cfg->parity;

		retval = uart_config_stop_bits(regs, clock_external, new_cfg->stop_bits);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.stop_bits = new_cfg->stop_bits;

		retval = uart_config_data_bits(regs, clock_external, new_cfg->data_bits);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.data_bits = new_cfg->data_bits;

		uint32_t clock_rate = 0;

		clock_control_get_rate(cfg->uart_clock.clock_dev, cfg->uart_clock.gclk_sys,
				       &clock_rate);

		retval = uart_set_baudrate(regs, clock_external, new_cfg->baudrate, clock_rate);
		if (retval != UART_SUCCESS) {
			break;
		}
		dev_data->config_cache.baudrate = new_cfg->baudrate;

		uart_enable(regs, clock_external, (cfg->run_in_standby_en == 1) ? true : false,
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
	bool clock_external = cfg->clock_external;

	uint32_t err = uart_get_err(dev);

	/* Clear all errors */
	uart_err_clear_all(regs, clock_external);

	return err;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Enable or disable the UART TX ready interrupt.
 *
 * This function enables or disables the TX ready interrupt for the specified UART instance.
 *
 * @param regs Pointer to the sercom_registers_t structure.
 * @param clock_external Boolean to check external or internal clock
 * @param enable True to enable the interrupt, false to disable.
 */
static void uart_enable_tx_ready_interrupt(sercom_registers_t *regs, bool clock_external,
					   bool enable)
{
	if (clock_external == false) {
		if (enable == true) {
			regs->USART_INT.SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_DRE_Msk;
		} else {
			regs->USART_INT.SERCOM_INTENCLR = SERCOM_USART_INT_INTENCLR_DRE_Msk;
		}
	} else {
		if (enable == true) {
			regs->USART_EXT.SERCOM_INTENSET = SERCOM_USART_EXT_INTENSET_DRE_Msk;
		} else {
			regs->USART_EXT.SERCOM_INTENCLR = SERCOM_USART_EXT_INTENCLR_DRE_Msk;
		}
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
	bool clock_external = cfg->clock_external;
	unsigned int key = irq_lock();

	uart_enable_tx_ready_interrupt(regs, clock_external, true);
	uart_enable_tx_complete_interrupt(regs, clock_external, true);
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
	bool clock_external = cfg->clock_external;
	int retval = 0;

	if ((uart_is_tx_ready(regs, clock_external) == true) && (len >= 1)) {
		uart_tx_char(regs, clock_external, tx_data[0]); /* Transmit the first character */
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
	bool clock_external = cfg->clock_external;

	uart_enable_tx_ready_interrupt(regs, clock_external, false);
	uart_enable_tx_complete_interrupt(regs, clock_external, false);
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
	bool clock_external = cfg->clock_external;

	return (uart_is_tx_ready(regs, clock_external) &&
		uart_is_tx_interrupt_enabled(regs, clock_external));
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

	uart_enable_rx_interrupt(cfg->regs, cfg->clock_external, true);
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

	uart_enable_rx_interrupt(cfg->regs, cfg->clock_external, false);
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

	if (uart_is_rx_complete(cfg->regs, cfg->clock_external) == true) {
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
	bool clock_external = cfg->clock_external;
	int retval = 0;

	if (uart_is_rx_complete(regs, clock_external) == true) {
		uint8_t ch = uart_get_received_char(
			/* Get the received character */
			regs, clock_external);

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

	if (uart_is_interrupt_pending(cfg->regs, cfg->clock_external) == true) {
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

	uart_enable_err_interrupt(cfg->regs, cfg->clock_external, true);
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

	uart_enable_err_interrupt(cfg->regs, cfg->clock_external, false);
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
	bool clock_external = cfg->clock_external;

	/*
	 * Cache the TXC flag, and use this cached value to clear the interrupt
	 * if we do not use the cached value, there is a chance TXC will set
	 * after caching...this will cause TXC to never be cached.
	 */
	dev_data->is_tx_completed_cache = uart_is_tx_complete(regs, clock_external);
	uart_clear_interrupts(regs, clock_external);

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
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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
};

#if CONFIG_UART_INTERRUPT_DRIVEN

#define MCHP_UART_IRQ_CONNECT(n, m)                                                                \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    uart_mchp_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#define UART_MCHP_IRQ_HANDLER_DECL(n) static void uart_mchp_irq_config_##n(const struct device *dev)
#define UART_MCHP_IRQ_HANDLER_FUNC(n) .irq_config_func = uart_mchp_irq_config_##n,

#else /* CONFIG_UART_INTERRUPT_DRIVEN */
#define UART_MCHP_IRQ_HANDLER_DECL(n)
#define UART_MCHP_IRQ_HANDLER_FUNC(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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
		.run_in_standby_en = DT_INST_PROP(n, run_in_standby_en),                           \
		UART_MCHP_IRQ_HANDLER_FUNC(n) UART_MCHP_CLOCK_DEFN(n)}

#define UART_MCHP_DEVICE_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	UART_MCHP_IRQ_HANDLER_DECL(n);                                                             \
	UART_MCHP_CONFIG_DEFN(n);                                                                  \
	static uart_mchp_dev_data_t uart_mchp_data_##n;                                            \
	DEVICE_DT_INST_DEFINE(n, uart_mchp_init, NULL, &uart_mchp_data_##n, &uart_mchp_config_##n, \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_mchp_driver_api);   \
	UART_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(UART_MCHP_DEVICE_INIT)
