/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gigadevice_gd32_uart

/**
 * @brief Driver for UART port on GD32 family processor.
 * @note  LPUART and U(S)ART have the same base and
 *        majority of operations are performed the same way.
 *        Please validate for newly added series.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
#include <soc.h>
#include <init.h>
#include <drivers/uart.h>
#include <drivers/pinmux.h>
#include <drivers/clock_control/gd32_clock_control.h>
#include <drivers/gpio/gpio_gd32.h>
#include "gigadevice_gd32_dt.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_gd32);

/*
 * register definitions
 */

union gd32_usart_stat {
	struct {
		volatile uint32_t PERR               : 1;       /*!< parity error flag */
		volatile uint32_t FERR               : 1;       /*!< frame error flag */
		volatile uint32_t NERR               : 1;       /*!< noise error flag */
		volatile uint32_t ORERR              : 1;       /*!< overrun error */
		volatile uint32_t IDLEF              : 1;       /*!< IDLE frame detected flag */
		volatile uint32_t RBNE               : 1;       /*!< read data buffer not empty */
		volatile uint32_t TC                 : 1;       /*!< transmission complete */
		volatile uint32_t TBE                : 1;       /*!< transmit data buffer empty */
		volatile uint32_t LBDF               : 1;       /*!< LIN break detected flag */
		volatile uint32_t CTSF               : 1;       /*!< CTS change flag */
		volatile uint8_t _PAD1 : 6;
		volatile uint8_t _PAD2;
		volatile uint8_t _PAD3;
	};
	volatile uint32_t ALL;
};

union gd32_usart_data {
	struct {
		volatile uint8_t DATA;                          /*!< transmit or read data value */
		volatile uint8_t _PAD1;
		volatile uint8_t _PAD2;
		volatile uint8_t _PAD3;
	};
	volatile uint32_t ALL;
};

union gd32_usart_baud {
	struct {
		volatile uint32_t FRADIV : 4;                   /*!< fraction part of baud-rate divider */
		volatile uint32_t INTDIV : 12;                  /*!< integer part of baud-rate divider */
		volatile uint32_t _PAD : 16;
	};
	volatile uint32_t ALL;
};


union gd32_usart_ctl0 {
	struct {
		volatile uint32_t SBKCMD             : 1;       /*!< send break command */
		volatile uint32_t RWU                : 1;       /*!< receiver wakeup from mute mode */
		volatile uint32_t REN                : 1;       /*!< receiver enable */
		volatile uint32_t TEN                : 1;       /*!< transmitter enable */
		volatile uint32_t IDLEIE             : 1;       /*!< idle line detected interrupt enable */
		volatile uint32_t RBNEIE             : 1;       /*!< read data buffer not empty interrupt and overrun error interrupt enable */
		volatile uint32_t TCIE               : 1;       /*!< transmission complete interrupt enable */
		volatile uint32_t TBEIE              : 1;       /*!< transmitter buffer empty interrupt enable */
		volatile uint32_t PERRIE             : 1;       /*!< parity error interrupt enable */
		volatile uint32_t PARITY             : 2;
		volatile uint32_t WM                 : 1;       /*!< wakeup method in mute mode */
		volatile uint32_t WL                 : 1;       /*!< word length */
		volatile uint32_t UEN                : 1;       /*!< USART enable */
		volatile uint32_t _PAD1              : 2;
		volatile uint8_t _PAD2;
		volatile uint8_t _PAD3;
	};
	volatile uint32_t ALL;
};

union gd32_usart_ctl1 {
	struct {
		volatile uint32_t ADDR               : 4;       /*!< address of USART */
		volatile uint32_t LBLEN              : 1;       /*!< LIN break frame length */
		volatile uint32_t LBDIE              : 1;       /*!< LIN break detected interrupt eanble */
		volatile uint32_t CLEN               : 1;       /*!< CK length */
		volatile uint32_t CPH                : 1;       /*!< CK phase */
		volatile uint32_t CPL                : 1;       /*!< CK polarity */
		volatile uint32_t CKEN               : 1;       /*!< CK pin enable */
		volatile uint32_t STB                : 2;       /*!< STOP bits length */
		volatile uint32_t LMEN               : 1;       /*!< LIN mode enable */
		volatile uint8_t _PAD1               : 2;
		volatile uint8_t _PAD2;
		volatile uint8_t _PAD3;
	};
	volatile uint32_t ALL;
};

union gd32_usart_ctl2 {
	struct {
		volatile uint32_t ERRIE              : 1;       /*!< error interrupt enable */
		volatile uint32_t IREN               : 1;       /*!< IrDA mode enable */
		volatile uint32_t IRLP               : 1;       /*!< IrDA low-power */
		volatile uint32_t HDEN               : 1;       /*!< half-duplex enable */
		volatile uint32_t NKEN               : 1;       /*!< NACK enable in smartcard mode */
		volatile uint32_t SCEN               : 1;       /*!< smartcard mode enable */
		volatile uint32_t DENR               : 1;       /*!< DMA request enable for reception */
		volatile uint32_t DENT               : 1;       /*!< DMA request enable for transmission */
		volatile uint32_t HWFC               : 2;
		volatile uint32_t CTSIE              : 1;       /*!< CTS interrupt enable */
		volatile uint8_t _PAD1 : 5;
		volatile uint8_t _PAD2;
		volatile uint8_t _PAD3;
	};
	volatile uint32_t ALL;
};


union gd32_usart_gp {
	struct {
		volatile uint8_t PSC;   /*!< prescaler value for dividing the system clock */
		volatile uint8_t GUAT;  /*!< guard time value in smartcard mode */
		volatile uint8_t _PAD2;
		volatile uint8_t _PAD3;
	};
	volatile uint32_t ALL;
};


struct gd32_usart {
	volatile union gd32_usart_stat STAT;
	volatile union gd32_usart_data DATA;
	volatile union gd32_usart_baud BAUD;
	volatile union gd32_usart_ctl0 CTL0;
	volatile union gd32_usart_ctl1 CTL1;
	volatile union gd32_usart_ctl2 CTL2;
	volatile union gd32_usart_gp GP;
};


#if !defined(USART0) && DT_NODE_EXISTS(DT_NODELABEL(usart0))
#define USART0 DT_REG_ADDR(DT_NODELABEL(usart0))
#endif

#if !defined(USART1) && DT_NODE_EXISTS(DT_NODELABEL(usart1))
#define USART1 DT_REG_ADDR(DT_NODELABEL(usart1))
#endif

#if !defined(USART2) && DT_NODE_EXISTS(DT_NODELABEL(usart2))
#define USART2 DT_REG_ADDR(DT_NODELABEL(usart2))
#endif

#if !defined(UART3) && DT_NODE_EXISTS(DT_NODELABEL(uart3))
#define UART3 DT_REG_ADDR(DT_NODELABEL(uart3))
#endif

#if !defined(UART4) && DT_NODE_EXISTS(DT_NODELABEL(uart4))
#define UART4 DT_REG_ADDR(DT_NODELABEL(uart4))
#endif

#define IS_UART_HWFLOW_INSTANCE(base) (base == USART0 || base == USART1 || base == USART2)
#define IS_UART_LIN_INSTANCE(base) (base == USART0 || base == USART1 || base == USART2)

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct uart_gd32_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct uart_gd32_data *const)(dev)->data)
#define DEV_REGS(dev) \
	(DEV_CFG(dev)->uconf.regs)

#define USART_DEV ((volatile struct gd32_usart *)DEV_REGS(dev))

/* USART receiver configure */
#define RECEIVE_ENABLE          1UL                             /*!< enable receiver */
#define RECEIVE_DISABLE         0UL                             /*!< disable receiver */

/* USART transmitter configure */
#define TRANSMIT_ENABLE         1UL                             /*!< enable transmitter */
#define TRANSMIT_DISABLE        0UL                             /*!< disable transmitter */

/* USART parity bits definitions */
#define PM_NONE                 0UL                             /*!< no parity */
#define PM_EVEN                 2UL                             /*!< even parity */
#define PM_ODD                  3UL                             /*!< odd parity */

/* USART wakeup method in mute mode */
#define WM_IDLE                 0UL                             /*!< idle line */
#define WM_ADDR                 1UL                             /*!< address match */

/* USART word length definitions */
#define WL_8BIT                 0UL                             /*!< 8 bits */
#define WL_9BIT                 1UL                             /*!< 9 bits */

/* USART stop bits definitions */
#define STB_1BIT                0UL                             /*!< 1 bit */
#define STB_0_5BIT              1UL                             /*!< 0.5 bit */
#define STB_2BIT                2UL                             /*!< 2 bits */
#define STB_1_5BIT              3UL                             /*!< 1.5 bits */

/* USART LIN break frame length */
#define LBLEN_10B               0UL                     /*!< 10 bits */
#define LBLEN_11B               1UL                     /*!< 11 bits */

/* USART CK length */
#define CLEN_NONE               0UL                             /*!< there are 7 CK pulses for an 8 bit frame and 8 CK pulses for a 9 bit frame */
#define CLEN_EN                 1UL                             /*!< there are 8 CK pulses for an 8 bit frame and 9 CK pulses for a 9 bit frame */

/* USART clock phase */
#define CPH_1CK                 0UL                             /*!< first clock transition is the first data capture edge */
#define CPH_2CK                 1UL                             /*!< second clock transition is the first data capture edge */

/* USART clock polarity */
#define CPL_LOW                 0UL                             /*!< steady low value on CK pin */
#define CPL_HIGH                1UL                             /*!< steady high value on CK pin */

/* USART DMA request for receive configure */
#define DENR_ENABLE             1UL                             /*!< DMA request enable for reception */
#define DENR_DISABLE            0UL                             /*!< DMA request disable for reception */

/* USART DMA request for transmission configure */
#define DENT_ENABLE             1UL                             /*!< DMA request enable for transmission */
#define DENT_DISABLE            0UL                             /*!< DMA request disable for transmission */

/* USART RTS configure */
#define RTS_ENABLE              1UL                     /*!< RTS enable */
#define RTS_DISABLE             0UL                     /*!< RTS disable */

/* USART CTS configure */
#define CTS_ENABLE              1UL                     /*!< CTS enable */
#define CTS_DISABLE             0UL                     /*!< CTS disable */

/* USART IrDA low-power enable */
#define IRLP_LOW                1UL                             /*!< low-power */
#define IRLP_NORMAL             0UL                             /*!< normal */


#define HWFC_NONE               0UL                             /*!< HWFC disable */
#define HWFC_RTS                1UL                             /*!< RTS enable */
#define HWFC_CTS                2UL                             /*!< CTS enable */
#define HWFC_RTSCTS             3UL                             /*!< RTS&CTS enable */


/* device config */
struct uart_gd32_config {
	struct uart_device_config uconf;
	/* clock subsystem driving this peripheral */
	struct gd32_pclken pclken;
	/* initial hardware flow control, 1 for RTS/CTS */
	bool hw_flow_control;
	/* initial parity, 0 for none, 1 for odd, 2 for even */
	int parity;
	const struct soc_gpio_pinctrl *pinctrl_list;
	size_t pinctrl_list_size;
};

/* driver data */
struct uart_gd32_data {
	/* Baud rate */
	uint32_t baud_rate;
	/* clock device */
	const struct device *clock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
};


/**
 * Convert parity  value (zephyr to register)
 */
static inline uint32_t cfg2reg_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return PM_ODD;
	case UART_CFG_PARITY_EVEN:
		return PM_EVEN;
	case UART_CFG_PARITY_NONE:
	default:
		return PM_NONE;
	}
}

/**
 * Convert parity  value (register to zephyr)
 */
static inline enum uart_config_parity reg2cfg_parity(uint32_t parity)
{
	switch (parity) {
	case PM_ODD:
		return UART_CFG_PARITY_ODD;
	case PM_EVEN:
		return UART_CFG_PARITY_EVEN;
	case PM_NONE:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

/**
 * Convert stop-bits value (zephyr to register)
 */
static inline uint32_t cfg2reg_stopbits(enum uart_config_stop_bits sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef STB_0_5BIT
	case UART_CFG_STOP_BITS_0_5:
		return STB_0_5BIT;
#endif  /* STB_0_5BIT */
	case UART_CFG_STOP_BITS_1:
		return STB_1BIT;
/* Some MCU's don't support 1.5 stop bits */
#ifdef STB_1_5BIT
	case UART_CFG_STOP_BITS_1_5:
		return STB_1_5BIT;
#endif  /* STB_1_5BIT */
	case UART_CFG_STOP_BITS_2:
	default:
		return STB_2BIT;
	}
}

/**
 * Convert stop-bits value (register to zephyr)
 */
static inline enum uart_config_stop_bits reg2cfg_stopbits(uint32_t sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef STB_0_5BIT
	case STB_0_5BIT:
		return UART_CFG_STOP_BITS_0_5;
#endif  /* STB_0_5BIT */
	case STB_1BIT:
		return UART_CFG_STOP_BITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef STB_1_5BIT
	case STB_1_5BIT:
		return UART_CFG_STOP_BITS_1_5;
#endif  /* STB_1_5BIT */
	case STB_2BIT:
	default:
		return UART_CFG_STOP_BITS_2;
	}
}

/**
 * Convert data-width value (zephyr to register)
 */
static inline uint32_t cfg2reg_databits(enum uart_config_data_bits db)
{
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef WL_7BIT
	case UART_CFG_DATA_BITS_7:
		return WL_7BIT;
#endif  /* WL_7BIT */
#ifdef WL_9BIT
	case UART_CFG_DATA_BITS_9:
		return WL_9BIT;
#endif  /* WL_9BIT */
	case UART_CFG_DATA_BITS_8:
	default:
		return WL_8BIT;
	}
}

/**
 * Convert data-width value (register to zephyr)
 */
static inline enum uart_config_data_bits reg2cfg_databits(uint32_t db)
{
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef WL_7BIT
	case WL_7BIT:
		return UART_CFG_DATA_BITS_7;
#endif  /* WL_7BIT */
#ifdef WL_9BIT
	case WL_9BIT:
		return UART_CFG_DATA_BITS_9;
#endif  /* WL_9BIT */
	case WL_8BIT:
	default:
		return UART_CFG_DATA_BITS_8;
	}
}

/**
 * @brief  Get GD32 hardware flow control define from
 *         Zephyr hardware flow control option.
 * @note   Supports only UART_CFG_FLOW_CTRL_RTS_CTS.
 * @param  fc: Zephyr hardware flow control option.
 * @retval HWFC_RTSCTS, or HWFC_NONE.
 */
static inline uint32_t cfg2reg_hwctrl(enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return HWFC_RTSCTS;
	}

	return HWFC_NONE;
}

/**
 * @brief  Get Zephyr hardware flow control option from
 *         hardware flow control define.
 * @note   Supports only HWFC_RTSCTS.
 * @param  fc: hardware flow control definition.
 * @retval UART_CFG_FLOW_CTRL_RTS_CTS, or UART_CFG_FLOW_CTRL_NONE.
 */
static inline enum uart_config_flow_control reg2cfg_hwctrl(uint32_t fc)
{
	if (fc == HWFC_RTSCTS) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_PARITY_NONE;
}

static inline void uart_gd32_set_baudrate(const struct device *dev,
					  uint32_t baudval)
{
	uint32_t uclk = 0U, udiv = 0U;

	int err;

	static const int _CK_APB1 = GD32_CLOCK_BUS_APB1;
	static const int _CK_APB2 = GD32_CLOCK_BUS_APB2;

	switch (DEV_REGS(dev)) {
	/* get clock frequency */
	case USART0:
		/* get USART0 clock */
		err = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(rcu)), (clock_control_subsys_t)&_CK_APB2, &uclk);
		break;
	case USART1:
		/* get USART1 clock */
		err = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(rcu)), (clock_control_subsys_t)&_CK_APB1, &uclk);
		break;
	case USART2:
		/* get USART2 clock */
		err = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(rcu)), (clock_control_subsys_t)&_CK_APB1, &uclk);
		break;
	case UART3:
		/* get UART3 clock */
		err = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(rcu)), (clock_control_subsys_t)&_CK_APB1, &uclk);
		break;
	case UART4:
		/* get UART4 clock */
		err = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(rcu)), (clock_control_subsys_t)&_CK_APB1, &uclk);
		break;
	default:
		break;
	}
	/* oversampling by 16, configure the value of USART_BAUD */
	udiv = (uclk + baudval / 2U) / baudval;
	USART_DEV->BAUD.INTDIV = (udiv >> 4) & (0x00000fffU);
	USART_DEV->BAUD.FRADIV = udiv & (0x0000000fU);
}

static int uart_gd32_configure(const struct device *dev,
			       const struct uart_config *cfg)
{
	struct uart_gd32_data *data = DEV_DATA(dev);
	const uint32_t parity = cfg2reg_parity(cfg->parity);
	const uint32_t stopbits = cfg2reg_stopbits(cfg->stop_bits);
	const uint32_t databits = cfg2reg_databits(cfg->data_bits);
	const uint32_t flowctrl = cfg2reg_hwctrl(cfg->flow_ctrl);

	/* Hardware doesn't support mark or space parity */
	if ((cfg->parity == UART_CFG_PARITY_MARK) ||
	    (cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOTSUP;
	}

	/* Driver does not supports parity + 9 databits */
	if ((cfg->parity != UART_CFG_PARITY_NONE) &&
	    (cfg->data_bits == UART_CFG_DATA_BITS_9)) {
		return -ENOTSUP;
	}

	if (cfg->stop_bits == UART_CFG_STOP_BITS_0_5) {
		return -ENOTSUP;
	}

	if (cfg->stop_bits == UART_CFG_STOP_BITS_1_5) {
		return -ENOTSUP;
	}

	/* Driver doesn't support 5 or 6 databits and potentially 7 or 9 */
	if ((cfg->data_bits == UART_CFG_DATA_BITS_5) ||
	    (cfg->data_bits == UART_CFG_DATA_BITS_6)
#ifndef WL_7BIT
	    || (cfg->data_bits == UART_CFG_DATA_BITS_7)
#endif /* WL_7BIT */
	    || (cfg->data_bits == UART_CFG_DATA_BITS_9)) {
		return -ENOTSUP;
	}

	/* Driver supports only RTS CTS flow control */
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		if (!IS_UART_HWFLOW_INSTANCE(DEV_REGS(dev)) ||
		    cfg->flow_ctrl != UART_CFG_FLOW_CTRL_RTS_CTS) {
			return -ENOTSUP;
		}
	}

	USART_DEV->CTL0.UEN = 0;

	USART_DEV->CTL0.PARITY = parity;
	USART_DEV->CTL1.STB = stopbits;
	USART_DEV->CTL0.WL = databits;
	USART_DEV->CTL2.HWFC = flowctrl;

	if (cfg->baudrate != data->baud_rate) {
		uart_gd32_set_baudrate(dev, cfg->baudrate);
		data->baud_rate = cfg->baudrate;
	}

	USART_DEV->CTL0.UEN = 1;
	return 0;
};

static int uart_gd32_config_get(const struct device *dev,
				struct uart_config *cfg)
{
	cfg->baudrate = DEV_DATA(dev)->baud_rate;
	cfg->parity = reg2cfg_parity(USART_DEV->CTL0.PARITY);
	cfg->stop_bits = reg2cfg_stopbits(USART_DEV->CTL1.STB);
	cfg->data_bits = reg2cfg_databits(USART_DEV->CTL0.WL);
	cfg->flow_ctrl = reg2cfg_hwctrl(USART_DEV->CTL2.HWFC);
	return 0;
}

static int uart_gd32_poll_in(const struct device *dev, unsigned char *c)
{
	/* Clear overrun error flag */
	if (USART_DEV->STAT.ORERR) {
		USART_DEV->STAT.ORERR = 0;
	}

	if ((!USART_DEV->STAT.RBNE)) {
		return -1;
	}

	*c = (unsigned char)USART_DEV->DATA.DATA;

	return 0;
}

static void uart_gd32_poll_out(const struct device *dev,
			       unsigned char c)
{
	/* Wait for TXE flag to be raised */
	while ((!USART_DEV->STAT.TBE)) {
	}

	USART_DEV->STAT.TC = 0;

	USART_DEV->DATA.DATA = (uint8_t)c;
}

static int uart_gd32_err_check(const struct device *dev)
{
	uint32_t err = 0U;

	/* Check for errors, but don't clear them here.
	 * Some SoC clear all error flags when at least
	 * one is cleared. (e.g. F4X, F1X, and F2X)
	 */
	if ((USART_DEV->STAT.ORERR)) {
		err |= UART_ERROR_OVERRUN;
	}

	if ((USART_DEV->STAT.PERR)) {
		err |= UART_ERROR_PARITY;
	}

	if ((USART_DEV->STAT.FERR)) {
		err |= UART_ERROR_FRAMING;
	}

	if (err & UART_ERROR_OVERRUN) {
		USART_DEV->STAT.ORERR = 0;
	}

	if (err & UART_ERROR_PARITY) {
		USART_DEV->STAT.PERR = 0;
	}

	if (err & UART_ERROR_FRAMING) {
		USART_DEV->STAT.FERR = 0;
	}

	/* Clear noise error as well,
	 * it is not represented by the errors enum
	 */
	USART_DEV->STAT.NERR = 0;

	return err;
}

static inline void __uart_gd32_get_clock(const struct device *dev)
{
	struct uart_gd32_data *data = DEV_DATA(dev);
	const struct device *clk = DEVICE_DT_GET(DT_NODELABEL(rcu));

	data->clock = clk;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_gd32_irq_tx_ready(const struct device *dev)
{
	return (USART_DEV->STAT.TBE && USART_DEV->CTL0.TBEIE);
}

static int uart_gd32_irq_tx_complete(const struct device *dev)
{
	return (USART_DEV->STAT.TC && USART_DEV->CTL0.TCIE);
}

static int uart_gd32_irq_rx_ready(const struct device *dev)
{
	return (USART_DEV->STAT.RBNE && USART_DEV->CTL0.RBNEIE);
}

static int uart_gd32_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data,
			       int size)
{
	uint8_t num_tx = 0U;

	while ((size - num_tx > 0) && uart_gd32_irq_tx_ready(dev)) {
		/* TBE flag will be cleared with byte write to DATA register */

		/* Send a character (8bit , parity none) */
		USART_DEV->DATA.DATA = tx_data[num_tx++];
	}

	return num_tx;
}

static int uart_gd32_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int size)
{
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) && uart_gd32_irq_rx_ready(dev)) {
		/* RBNE flag will be cleared upon read from DATA register */

		/* Receive a character (8bit , parity none) */
		rx_data[num_rx++] = USART_DEV->DATA.DATA;

		/* Clear overrun error flag */
		if (USART_DEV->CTL2.ERRIE && USART_DEV->STAT.ORERR) {
			USART_DEV->CTL2.ERRIE = 0;
		}
	}

	return num_rx;
}

static void uart_gd32_irq_tx_enable(const struct device *dev)
{
	USART_DEV->CTL0.TBEIE = 1;
}

static void uart_gd32_irq_tx_disable(const struct device *dev)
{
	USART_DEV->CTL0.TBEIE = 0;
}

static void uart_gd32_irq_rx_enable(const struct device *dev)
{
	USART_DEV->CTL0.RBNEIE = 1;
}

static void uart_gd32_irq_rx_disable(const struct device *dev)
{
	USART_DEV->CTL0.RBNEIE = 0;
}

static void uart_gd32_irq_err_enable(const struct device *dev)
{
	/* Enable Error interruptions */
	USART_DEV->CTL2.ERRIE = 1;
	/* Enable Line break detection */
	if (IS_UART_LIN_INSTANCE(DEV_REGS(dev))) {
		USART_DEV->CTL1.LBDIE = 1;
	}
	/* Enable parity error interruption */
	USART_DEV->CTL0.PERRIE = 1;
}

static void uart_gd32_irq_err_disable(const struct device *dev)
{
	/* Disable Error interruptions */
	USART_DEV->CTL2.ERRIE = 0;
	/* Disable Line break detection */
	if (IS_UART_LIN_INSTANCE(DEV_REGS(dev))) {
		USART_DEV->CTL1.LBDIE = 0;
	}
	/* Disable parity error interruption */
	USART_DEV->CTL0.PERRIE = 0;
}

static int uart_gd32_irq_is_pending(const struct device *dev)
{
	return (uart_gd32_irq_rx_ready(dev) || uart_gd32_irq_tx_ready(dev));
}

static int uart_gd32_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_gd32_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_gd32_data *data = DEV_DATA(dev);

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)

static void uart_gd32_isr(const struct device *dev)
{
	struct uart_gd32_data *data = DEV_DATA(dev);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}

#endif /* (CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API) */


static const struct uart_driver_api uart_gd32_driver_api = {
	.poll_in = uart_gd32_poll_in,
	.poll_out = uart_gd32_poll_out,
	.err_check = uart_gd32_err_check,
	.configure = uart_gd32_configure,
	.config_get = uart_gd32_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_gd32_fifo_fill,
	.fifo_read = uart_gd32_fifo_read,
	.irq_tx_enable = uart_gd32_irq_tx_enable,
	.irq_tx_disable = uart_gd32_irq_tx_disable,
	.irq_tx_ready = uart_gd32_irq_tx_ready,
	.irq_tx_complete = uart_gd32_irq_tx_complete,
	.irq_rx_enable = uart_gd32_irq_rx_enable,
	.irq_rx_disable = uart_gd32_irq_rx_disable,
	.irq_rx_ready = uart_gd32_irq_rx_ready,
	.irq_err_enable = uart_gd32_irq_err_enable,
	.irq_err_disable = uart_gd32_irq_err_disable,
	.irq_is_pending = uart_gd32_irq_is_pending,
	.irq_update = uart_gd32_irq_update,
	.irq_callback_set = uart_gd32_irq_callback_set,
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_gd32_init(const struct device *dev)
{
	const struct uart_gd32_config *config = DEV_CFG(dev);
	struct uart_gd32_data *data = DEV_DATA(dev);
	uint32_t ll_parity;
	uint32_t ll_datawidth;

	__uart_gd32_get_clock(dev);
	/* enable clock */
	if (clock_control_on(data->clock,
			     (clock_control_subsys_t *)&config->pclken) != 0) {
		return -EIO;
	}

	USART_DEV->CTL0.UEN = 0;

	/* TODO make configurable with pinctrl */
	clock_control_on(DEVICE_DT_GET(DT_NODELABEL(rcu)), (void *)&GD32_PCLK_EN_GPIOA);
	clock_control_on(DEVICE_DT_GET(DT_NODELABEL(rcu)), (void *)&GD32_PCLK_EN_USART0);
	gpio_gd32_configure(DEVICE_DT_GET(DT_NODELABEL(gpioa)), 9, GD32_CNF_AF_PP | GD32_MODE_OUTPUT_MAX_50, 0);
	gpio_gd32_configure(DEVICE_DT_GET(DT_NODELABEL(gpioa)), 10, GD32_CNF_IN_FLOAT | GD32_MODE_OUTPUT_MAX_50, 0);

	/* TX/RX direction */
	USART_DEV->CTL0.TEN = 1;
	USART_DEV->CTL0.REN = 1;

	/* Determine the datawidth and parity. If we use other parity than
	 * 'none' we must use datawidth = 9 (to get 8 databit + 1 parity bit).
	 */
	if (config->parity == 2) {
		/* 8 databit, 1 parity bit, parity even */
		ll_parity = PM_EVEN;
		ll_datawidth = WL_9BIT;
	} else if (config->parity == 1) {
		/* 8 databit, 1 parity bit, parity odd */
		ll_parity = PM_ODD;
		ll_datawidth = WL_9BIT;
	} else {  /* Default to 8N0, but show warning if invalid value */
		if (config->parity != 0) {
			LOG_WRN("Invalid parity setting '%d'."
				"Defaulting to 'none'.", config->parity);
		}
		/* 8 databit, parity none */
		ll_parity = PM_NONE;
		ll_datawidth = WL_8BIT;
	}

	/* Set datawidth and parity, 1 start bit, 1 stop bit  */
	USART_DEV->CTL0.PARITY = ll_parity;
	USART_DEV->CTL0.WL = ll_datawidth;
	USART_DEV->CTL1.STB = STB_1BIT;

	if (config->hw_flow_control) {
		USART_DEV->CTL2.HWFC = HWFC_RTSCTS;
	}

	/* Set the default baudrate */
	uart_gd32_set_baudrate(dev, data->baud_rate);

	USART_DEV->CTL0.UEN = 1;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->uconf.irq_config_func(dev);
#endif
	return 0;
}


#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define GD32_UART_IRQ_HANDLER_DECL(index) \
	static void uart_gd32_irq_config_func_##index(const struct device *dev)
#define GD32_UART_IRQ_HANDLER_FUNC(index) \
	.irq_config_func = uart_gd32_irq_config_func_##index,
#define GD32_UART_IRQ_HANDLER(index)						\
	static void uart_gd32_irq_config_func_##index(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(index),				\
			    DT_INST_IRQ(index, priority),			\
			    uart_gd32_isr, DEVICE_DT_INST_GET(index),		\
			    0);							\
		irq_enable(DT_INST_IRQN(index));				\
	}
#else
#define GD32_UART_IRQ_HANDLER_DECL(index)
#define GD32_UART_IRQ_HANDLER_FUNC(index)
#define GD32_UART_IRQ_HANDLER(index)
#endif

#define GD32_UART_INIT(index)							\
	GD32_UART_IRQ_HANDLER_DECL(index);					\
										\
	static const struct soc_gpio_pinctrl uart_pins_##index[] =		\
		GIGADEVICE_GD32_DT_INST_PINCTRL(index, 0);			\
										\
	static const struct uart_gd32_config uart_gd32_cfg_##index = {		\
		.uconf = {							\
			.regs = DT_INST_REG_ADDR(index),			\
			GD32_UART_IRQ_HANDLER_FUNC(index)			\
		},								\
		.pclken = { .bus = DT_INST_CLOCKS_CELL(index, bus),		\
			    .enr = DT_INST_CLOCKS_CELL(index, bits)		\
		},								\
		.hw_flow_control = DT_INST_PROP(index, hw_flow_control),	\
		.parity = DT_INST_PROP_OR(index, parity, UART_CFG_PARITY_NONE),	\
		.pinctrl_list = uart_pins_##index,				\
		.pinctrl_list_size = ARRAY_SIZE(uart_pins_##index),		\
	};									\
										\
	static struct uart_gd32_data uart_gd32_data_##index = {			\
		.baud_rate = DT_INST_PROP(index, current_speed),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(index,						\
			      &uart_gd32_init,					\
			      NULL,						\
			      &uart_gd32_data_##index, &uart_gd32_cfg_##index,	\
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &uart_gd32_driver_api);				\
										\
	GD32_UART_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(GD32_UART_INIT)
