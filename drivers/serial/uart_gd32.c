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
#include <drivers/clock_control.h>


#include <linker/sections.h>
#include "gd32vf103_usart.h"
#include "gd32vf103_gpio.h"

#include "gigadevice_gd32_dt.h"

#include <linker/sections.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_gd32);

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct uart_gd32_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct uart_gd32_data *const)(dev)->data)
#define DEV_REGS(dev) \
	(DEV_CFG(dev)->uconf.regs)

#define CLT2_HWFC(regval)             (BITS(8, 9) & ((uint32_t)(regval) << 8))
#define USART_HWFC_NONE               CLT2_HWFC(0)                      /*!< HWFC disable */
#define USART_HWFC_RTS                CLT2_HWFC(1)                      /*!< RTS enable */
#define USART_HWFC_CTS                CLT2_HWFC(2)                      /*!< CTS enable */
#define USART_HWFC_RTSCTS             CLT2_HWFC(3)                      /*!< RTS&CTS enable */
#define USART_CTL2_HWFC               BITS(8, 9)                        /*!< RTS&CTS enable */

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

static inline int usart_interrupt_flag_enabled(uint32_t usart_periph, uint32_t int_flag)
{
	return (USART_REG_VAL(usart_periph, int_flag) >> USART_BIT_POS(int_flag)) & 0x1;
}

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

static inline uint32_t uart_gd32_cfg2ll_parity(enum uart_config_parity parity);
static inline enum uart_config_parity uart_gd32_ll2cfg_parity(uint32_t parity);
static inline uint32_t uart_gd32_cfg2ll_stopbits(enum uart_config_stop_bits sb);
static inline enum uart_config_stop_bits uart_gd32_ll2cfg_stopbits(uint32_t sb);
static inline uint32_t uart_gd32_cfg2ll_databits(enum uart_config_data_bits db);
static inline enum uart_config_data_bits uart_gd32_ll2cfg_databits(uint32_t db);
static inline enum uart_config_flow_control uart_gd32_ll2cfg_hwctrl(uint32_t fc);

static inline void uart_gd32_set_baudrate(const struct device *dev,
					  uint32_t baud_rate)
{
	uint32_t regs = DEV_REGS(dev);

	usart_baudrate_set(regs, baud_rate);
}

static inline void uart_gd32_set_parity(const struct device *dev,
					uint32_t parity)
{
	uint32_t regs = DEV_REGS(dev);

	usart_parity_config(regs, uart_gd32_cfg2ll_parity(parity));
}

static inline uint32_t uart_gd32_get_parity(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	return uart_gd32_ll2cfg_parity((USART_CTL0(regs) & USART_CTL0_PM) >> 9);
}

static inline void uart_gd32_set_stopbits(const struct device *dev,
					  uint32_t stopbits)
{
	uint32_t regs = DEV_REGS(dev);

	usart_stop_bit_set(regs, stopbits);
}

static inline uint32_t uart_gd32_get_stopbits(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	return uart_gd32_ll2cfg_stopbits((USART_CTL1(regs) & USART_CTL1_STB) >> 12);
}

static inline void uart_gd32_set_databits(const struct device *dev,
					  uint32_t databits)
{
	uint32_t regs = DEV_REGS(dev);

	usart_word_length_set(regs, databits);
}

static inline uint32_t uart_gd32_get_databits(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	return uart_gd32_ll2cfg_databits((USART_CTL0(regs) & USART_CTL0_WL) >> 12);
}

static inline void uart_gd32_set_hwctrl(const struct device *dev,
					uint32_t hwctrl)
{
	uint32_t regs = DEV_REGS(dev);

	usart_hardware_flow_rts_config(regs, (hwctrl >> 0 & 0x1));
	usart_hardware_flow_cts_config(regs, (hwctrl >> 1 & 0x1));
}

static inline uint32_t uart_gd32_get_hwctrl(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	return uart_gd32_ll2cfg_hwctrl((USART_CTL0(regs) & USART_CTL2_HWFC) >> 12);
}

static inline uint32_t uart_gd32_cfg2ll_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return USART_PM_ODD;
	case UART_CFG_PARITY_EVEN:
		return USART_PM_EVEN;
	case UART_CFG_PARITY_NONE:
	default:
		return USART_PM_NONE;
	}
}

static inline enum uart_config_parity uart_gd32_ll2cfg_parity(uint32_t parity)
{
	switch (parity) {
	case USART_PM_ODD:
		return UART_CFG_PARITY_ODD;
	case USART_PM_EVEN:
		return UART_CFG_PARITY_EVEN;
	case USART_PM_NONE:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static inline uint32_t uart_gd32_cfg2ll_stopbits(enum uart_config_stop_bits sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef USART_STB_0_5BIT
	case UART_CFG_STOP_BITS_0_5:
		return USART_STB_0_5BIT;
#endif  /* USART_STB_0_5BIT */
	case UART_CFG_STOP_BITS_1:
		return USART_STB_1BIT;
/* Some MCU's don't support 1.5 stop bits */
#ifdef USART_STB_1_5BIT
	case UART_CFG_STOP_BITS_1_5:
		return USART_STB_1_5BIT;
#endif  /* USART_STB_1_5BIT */
	case UART_CFG_STOP_BITS_2:
	default:
		return USART_STB_2BIT;
	}
}

static inline enum uart_config_stop_bits uart_gd32_ll2cfg_stopbits(uint32_t sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef USART_STB_0_5BIT
	case USART_STB_0_5BIT:
		return UART_CFG_STOP_BITS_0_5;
#endif  /* USART_STB_0_5BIT */
	case USART_STB_1BIT:
		return UART_CFG_STOP_BITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef USART_STB_1_5BIT
	case USART_STB_1_5BIT:
		return UART_CFG_STOP_BITS_1_5;
#endif  /* USART_STB_1_5BIT */
	case USART_STB_2BIT:
	default:
		return UART_CFG_STOP_BITS_2;
	}
}

static inline uint32_t uart_gd32_cfg2ll_databits(enum uart_config_data_bits db)
{
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef USART_WL_7BIT
	case UART_CFG_DATA_BITS_7:
		return USART_WL_7BIT;
#endif  /* USART_WL_7BIT */
#ifdef USART_WL_9BIT
	case UART_CFG_DATA_BITS_9:
		return USART_WL_9BIT;
#endif  /* USART_WL_9BIT */
	case UART_CFG_DATA_BITS_8:
	default:
		return USART_WL_8BIT;
	}
}

static inline enum uart_config_data_bits uart_gd32_ll2cfg_databits(uint32_t db)
{
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef USART_WL_7BIT
	case USART_WL_7BIT:
		return UART_CFG_DATA_BITS_7;
#endif  /* USART_WL_7BIT */
#ifdef USART_WL_9BIT
	case USART_WL_9BIT:
		return UART_CFG_DATA_BITS_9;
#endif  /* USART_WL_9BIT */
	case USART_WL_8BIT:
	default:
		return UART_CFG_DATA_BITS_8;
	}
}

/**
 * @brief  Get GD32 hardware flow control define from
 *         Zephyr hardware flow control option.
 * @note   Supports only UART_CFG_FLOW_CTRL_RTS_CTS.
 * @param  fc: Zephyr hardware flow control option.
 * @retval USART_HWFC_RTSCTS, or USART_HWFC_NONE.
 */
static inline uint32_t uart_gd32_cfg2ll_hwctrl(enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return USART_HWFC_RTSCTS;
	}

	return USART_HWFC_NONE;
}

/**
 * @brief  Get Zephyr hardware flow control option from
 *         hardware flow control define.
 * @note   Supports only USART_HWFC_RTSCTS.
 * @param  fc: hardware flow control definition.
 * @retval UART_CFG_FLOW_CTRL_RTS_CTS, or UART_CFG_FLOW_CTRL_NONE.
 */
static inline enum uart_config_flow_control uart_gd32_ll2cfg_hwctrl(uint32_t fc)
{
	if (fc == USART_HWFC_RTSCTS) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_PARITY_NONE;
}

static int uart_gd32_configure(const struct device *dev,
			       const struct uart_config *cfg)
{
	struct uart_gd32_data *data = DEV_DATA(dev);
	uint32_t regs = DEV_REGS(dev);
	const uint32_t parity = uart_gd32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_gd32_cfg2ll_stopbits(cfg->stop_bits);
	const uint32_t databits = uart_gd32_cfg2ll_databits(cfg->data_bits);

	const uint32_t flowctrl = uart_gd32_cfg2ll_hwctrl(cfg->flow_ctrl);

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
#ifndef USART_WL_7BIT
	    || (cfg->data_bits == UART_CFG_DATA_BITS_7)
#endif /* USART_WL_7BIT */
	    || (cfg->data_bits == UART_CFG_DATA_BITS_9)) {
		return -ENOTSUP;
	}

	/* Driver supports only RTS CTS flow control */
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		if (!IS_UART_HWFLOW_INSTANCE(regs) ||
		    cfg->flow_ctrl != UART_CFG_FLOW_CTRL_RTS_CTS) {
			return -ENOTSUP;
		}
	}

	usart_disable(regs);

	if (parity != uart_gd32_get_parity(dev)) {
		uart_gd32_set_parity(dev, parity);
	}

	if (stopbits != uart_gd32_get_stopbits(dev)) {
		uart_gd32_set_stopbits(dev, stopbits);
	}

	if (databits != uart_gd32_get_databits(dev)) {
		uart_gd32_set_databits(dev, databits);
	}

	if (flowctrl != uart_gd32_get_hwctrl(dev)) {
		uart_gd32_set_hwctrl(dev, flowctrl);
	}

	if (cfg->baudrate != data->baud_rate) {
		uart_gd32_set_baudrate(dev, cfg->baudrate);
		data->baud_rate = cfg->baudrate;
	}

	usart_enable(regs);
	return 0;
};

static int uart_gd32_config_get(const struct device *dev,
				struct uart_config *cfg)
{
	struct uart_gd32_data *data = DEV_DATA(dev);

	cfg->baudrate = data->baud_rate;
	cfg->parity = uart_gd32_ll2cfg_parity(uart_gd32_get_parity(dev));
	cfg->stop_bits = uart_gd32_ll2cfg_stopbits(
		uart_gd32_get_stopbits(dev));
	cfg->data_bits = uart_gd32_ll2cfg_databits(
		uart_gd32_get_databits(dev));
	cfg->flow_ctrl = uart_gd32_ll2cfg_hwctrl(
		uart_gd32_get_hwctrl(dev));
	return 0;
}

static int uart_gd32_poll_in(const struct device *dev, unsigned char *c)
{
	uint32_t regs = DEV_REGS(dev);

	/* Clear overrun error flag */
	if (usart_flag_get(regs, USART_FLAG_ORERR)) {
		usart_flag_clear(regs, USART_FLAG_ORERR);
	}

	if (!usart_flag_get(regs, USART_FLAG_RBNE)) {
		return -1;
	}

	*c = (unsigned char)usart_data_receive(regs);

	return 0;
}

static void uart_gd32_poll_out(const struct device *dev,
			       unsigned char c)
{
	uint32_t regs = DEV_REGS(dev);

	/* Wait for TXE flag to be raised */
	while (!usart_flag_get(regs, USART_FLAG_TBE)) {
	}

	usart_flag_clear(regs, USART_FLAG_TC);

	usart_data_transmit(regs, (uint8_t) c);
}

static int uart_gd32_err_check(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);
	uint32_t err = 0U;

	/* Check for errors, but don't clear them here.
	 * Some SoC clear all error flags when at least
	 * one is cleared. (e.g. F4X, F1X, and F2X)
	 */
	if (usart_flag_get(regs, USART_FLAG_ORERR)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (usart_flag_get(regs, USART_FLAG_PERR)) {
		err |= UART_ERROR_PARITY;
	}

	if (usart_flag_get(regs, USART_FLAG_FERR)) {
		err |= UART_ERROR_FRAMING;
	}

	if (err & UART_ERROR_OVERRUN) {
		usart_flag_clear(regs, USART_FLAG_ORERR);
	}

	if (err & UART_ERROR_PARITY) {
		usart_flag_clear(regs, USART_FLAG_PERR);
	}

	if (err & UART_ERROR_FRAMING) {
		usart_flag_clear(regs, USART_FLAG_FERR);
	}

	/* Clear noise error as well,
	 * it is not represented by the errors enum
	 */
	usart_flag_clear(regs, USART_FLAG_NERR);

	return err;
}

static inline void __uart_gd32_get_clock(const struct device *dev)
{
	struct uart_gd32_data *data = DEV_DATA(dev);
	const struct device *clk = DEVICE_DT_GET(GD32_CLOCK_CONTROL_NODE);

	data->clock = clk;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_gd32_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data,
			       int size)
{
	uint32_t regs = DEV_REGS(dev);
	uint8_t num_tx = 0U;

	while ((size - num_tx > 0) &&
	       usart_interrupt_flag_get(regs, USART_INT_FLAG_TBE)) {
		/* TBE flag will be cleared with byte write to DATA register */

		/* Send a character (8bit , parity none) */
		usart_data_transmit(regs, tx_data[num_tx++]);
	}

	return num_tx;
}

static int uart_gd32_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int size)
{
	uint32_t regs = DEV_REGS(dev);
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) &&
	       usart_interrupt_flag_get(regs, USART_INT_FLAG_RBNE)) {
		/* RBNE flag will be cleared upon read from DATA register */

		/* Receive a character (8bit , parity none) */
		rx_data[num_rx++] = usart_data_receive(regs);

		/* Clear overrun error flag */
		if (usart_interrupt_flag_get(regs, USART_INT_FLAG_ERR_ORERR)) {
			usart_interrupt_flag_clear(regs, USART_INT_FLAG_ERR_ORERR);
		}
	}

	return num_rx;
}

static void uart_gd32_irq_tx_enable(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	usart_interrupt_enable(regs, USART_INT_TBE);
}

static void uart_gd32_irq_tx_disable(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	usart_interrupt_disable(regs, USART_INT_TBE);
}

static int uart_gd32_irq_tx_ready(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	return usart_interrupt_flag_get(regs, USART_INT_FLAG_TBE);
}

static int uart_gd32_irq_tx_complete(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	return usart_interrupt_flag_get(regs, USART_INT_FLAG_TC);
}

static void uart_gd32_irq_rx_enable(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	usart_interrupt_enable(regs, USART_INT_RBNE);
}

static void uart_gd32_irq_rx_disable(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	usart_interrupt_disable(regs, USART_INT_RBNE);
}

static int uart_gd32_irq_rx_ready(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	return usart_interrupt_flag_get(regs, USART_INT_FLAG_RBNE);
}

static void uart_gd32_irq_err_enable(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	/* Enable Error interruptions */
	usart_interrupt_enable(regs, USART_INT_ERR);
	/* Enable Line break detection */
	if (IS_UART_LIN_INSTANCE(regs)) {
		usart_interrupt_enable(regs, USART_INT_LBD);
	}
	/* Enable parity error interruption */
	usart_interrupt_enable(regs, USART_INT_PERR);
}

static void uart_gd32_irq_err_disable(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	/* Disable Error interruptions */
	usart_interrupt_disable(regs, USART_INT_ERR);
	/* Disable Line break detection */
	if (IS_UART_LIN_INSTANCE(regs)) {
		usart_interrupt_disable(regs, USART_INT_LBD);
	}
	/* Disable parity error interruption */
	usart_interrupt_disable(regs, USART_INT_PERR);
}

static int uart_gd32_irq_is_pending(const struct device *dev)
{
	uint32_t regs = DEV_REGS(dev);

	return ((usart_interrupt_flag_enabled(regs, USART_INT_FLAG_RBNE) &&
		 usart_interrupt_flag_get(regs, USART_INT_FLAG_RBNE)) ||
		(usart_interrupt_flag_enabled(regs, USART_INT_FLAG_TC) &&
		 usart_interrupt_flag_get(regs, USART_INT_FLAG_TC)));
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
	uint32_t regs = DEV_REGS(dev);
	uint32_t ll_parity;
	uint32_t ll_datawidth;

	__uart_gd32_get_clock(dev);
	/* enable clock */
	if (clock_control_on(data->clock,
			     (clock_control_subsys_t *)&config->pclken) != 0) {
		return -EIO;
	}

	usart_deinit(regs);

	/* TODO make configurable with pinctrl */
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_USART0);
	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
	gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

	/* TX/RX direction */
	usart_transmit_config(regs, USART_TRANSMIT_ENABLE);
	usart_receive_config(regs, USART_RECEIVE_ENABLE);

	/* Determine the datawidth and parity. If we use other parity than
	 * 'none' we must use datawidth = 9 (to get 8 databit + 1 parity bit).
	 */
	if (config->parity == 2) {
		/* 8 databit, 1 parity bit, parity even */
		ll_parity = USART_PM_EVEN;
		ll_datawidth = USART_WL_9BIT;
	} else if (config->parity == 1) {
		/* 8 databit, 1 parity bit, parity odd */
		ll_parity = USART_PM_ODD;
		ll_datawidth = USART_WL_9BIT;
	} else {  /* Default to 8N0, but show warning if invalid value */
		if (config->parity != 0) {
			LOG_WRN("Invalid parity setting '%d'."
				"Defaulting to 'none'.", config->parity);
		}
		/* 8 databit, parity none */
		ll_parity = USART_PM_NONE;
		ll_datawidth = USART_WL_8BIT;
	}

	/* Set datawidth and parity, 1 start bit, 1 stop bit  */
	uart_gd32_set_parity(dev, ll_parity);
	uart_gd32_set_databits(dev, ll_datawidth);
	uart_gd32_set_stopbits(dev, USART_STB_1BIT);

	if (config->hw_flow_control) {
		uart_gd32_set_hwctrl(dev, USART_HWFC_RTSCTS);
	}

	/* Set the default baudrate */
	uart_gd32_set_baudrate(dev, data->baud_rate);

	usart_enable(regs);

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
