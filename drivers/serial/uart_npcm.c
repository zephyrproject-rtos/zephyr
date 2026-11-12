/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_uart

#include <zephyr/sys/__assert.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_npcm, CONFIG_UART_LOG_LEVEL);

/*
 * Universal Asynchronous Receiver-Transmitter (UART) device registers
 */
struct uart_reg {
	/* 0x000: Transmit Data Buffer */
	uint8_t UTBUF;
	uint8_t reserved1;
	/* 0x002: Receive Data Buffer */
	uint8_t URBUF;
	uint8_t reserved2;
	/* 0x004: Interrupt Control */
	uint8_t UICTRL;
	uint8_t reserved3;
	/* 0x006: Status */
	uint8_t USTAT;
	uint8_t reserved4;
	/* 0x008: Frame Select */
	uint8_t UFRS;
	uint8_t reserved5;
	/* 0x00A: Mode Select */
	uint8_t UMDSL;
	uint8_t reserved6;
	/* 0x00C: Baud Rate Divisor */
	uint8_t UBAUD;
	uint8_t reserved7;
	/* 0x00E: Baud Rate Prescaler */
	uint8_t UPSR;
	uint8_t reserved8[7];
	/* 0x016: FIFO Control */
	uint8_t UFCTRL;
	uint8_t reserved9;
	/* 0x018: TX FIFO Current Level */
	uint8_t UTXFLV;
	uint8_t reserved10;
	/* 0x01A: RX FIFO Current Level */
	uint8_t URXFLV;
	uint8_t reserved11[12];
};

/* UART register fields */
/* UICTRL bit 0 is TBE in buffer mode and TFNF in FIFO mode. */
#define NPCM_UICTRL_TBE          0
/* UICTRL bit 1 is RBF in buffer mode and RFLT in FIFO mode. */
#define NPCM_UICTRL_RBF          1
#define NPCM_UICTRL_ETI          5
#define NPCM_UICTRL_ERI          6
#define NPCM_UICTRL_EEI          7
#define NPCM_USTAT_PE            0
#define NPCM_USTAT_FE            1
#define NPCM_USTAT_DOE           2
#define NPCM_USTAT_ERR           3
#define NPCM_USTAT_BKD           4
#define NPCM_USTAT_RB9           5
#define NPCM_USTAT_XMIP          6
#define NPCM_UFRS_CHAR_FIELD     GENMASK(1, 0)
/* UFRS.CHAR: 00=8-bit, 01=7-bit, 10=9-bit, 11=diagnostic(9-bit). */
#define NPCM_UFRS_CHAR_8BITS     0U
#define NPCM_UFRS_CHAR_7BITS     1U
#define NPCM_UFRS_CHAR_9BITS     2U
#define NPCM_UFRS_STP            2
#define NPCM_UFRS_XB9            3
#define NPCM_UFRS_PSEL_FIELD     GENMASK(5, 4)
/* UFRS.PSEL: 00=odd, 01=even, 10=mark, 11=space. */
#define NPCM_UFRS_PSEL_ODD       0U
#define NPCM_UFRS_PSEL_EVEN      1U
#define NPCM_UFRS_PSEL_MARK      2U
#define NPCM_UFRS_PSEL_SPACE     3U
#define NPCM_UFRS_PEN            6
#define NPCM_UPSR_UDIV10_8_FIELD GENMASK(2, 0)
#define NPCM_UPSR_UPSC_FIELD     GENMASK(7, 3)
#define NPCM_UPSC_NO_CLOCK       0U
#define NPCM_UFCTRL_RXFTH_FIELD  GENMASK(7, 6)
#define NPCM_UFCTRL_FIFOEN       0
#define NPCM_UFCTRL_EXTLOOPBACK  1
#define NPCM_UTXFLV_TFL          GENMASK(4, 0)
#define NPCM_URXFLV_RFL          GENMASK(4, 0)

#define GET_FIELD(reg, field) FIELD_GET(field, reg)

#define UART_NPCM_PM_TX_DRAIN_TIMEOUT_MS 20

static void uart_set_npcm_baud_rate(const struct device *dev, int baud_rate, uint32_t src_clk);

static inline uint8_t npcm_reg_read(uint8_t *reg)
{
	return sys_read8((mem_addr_t)reg);
}

static inline uint8_t npcm_set_field(uint8_t reg, uint8_t field, uint8_t value)
{
	return (reg & ~field) | FIELD_PREP(field, value);
}

static inline void npcm_reg_write(uint8_t *reg, uint8_t val)
{
	sys_write8(val, (mem_addr_t)reg);
}

static inline void npcm_reg_set_bits(uint8_t *reg, uint8_t bits)
{
	npcm_reg_write(reg, npcm_reg_read(reg) | bits);
}

static inline void npcm_reg_clr_bits(uint8_t *reg, uint8_t bits)
{
	npcm_reg_write(reg, npcm_reg_read(reg) & ~bits);
}

#define NPCM_REG_READ(reg_ptr)           npcm_reg_read((reg_ptr))
#define NPCM_REG_WRITE(reg_ptr, val)     npcm_reg_write((reg_ptr), (uint8_t)(val))
#define NPCM_REG_SET_BITS(reg_ptr, bits) npcm_reg_set_bits((reg_ptr), (bits))
#define NPCM_REG_CLR_BITS(reg_ptr, bits) npcm_reg_clr_bits((reg_ptr), (bits))

/* Driver config */
struct uart_npcm_config {
	struct uart_reg *base;
	/* clock configuration */
	uint32_t clk_cfg;
	/* encoded RX FIFO threshold (UFCTRL.RXFTH) */
	uint8_t rx_fifo_threshold;
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
};

/* Driver data */
struct uart_npcm_data {
	/* Baud rate */
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct k_spinlock lock;
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_npcm_tx_fifo_ready(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	/* True if the Tx FIFO contains some space available */
	return !(GET_FIELD(NPCM_REG_READ(&inst->UTXFLV), NPCM_UTXFLV_TFL) >= 16);
}

static int uart_npcm_rx_fifo_available(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	/* True if at least one byte is in the Rx FIFO */
	return !(GET_FIELD(NPCM_REG_READ(&inst->URXFLV), NPCM_URXFLV_RFL) == 0);
}

static void uart_npcm_dis_all_tx_interrupts(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	/* Disable ETI (Enable Transmit Interrupt) interrupt */
	NPCM_REG_CLR_BITS(&inst->UICTRL, BIT(NPCM_UICTRL_ETI));
}

static void uart_npcm_clear_rx_fifo(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	/* Read all dummy bytes out from Rx FIFO */
	while (uart_npcm_rx_fifo_available(dev)) {
		(void)NPCM_REG_READ(&inst->URBUF);
	}
}

static int uart_npcm_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	struct uart_npcm_data *data = dev->data;
	uint8_t tx_bytes = 0U;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* If Tx FIFO is still ready to send */
	while ((size - tx_bytes > 0) && uart_npcm_tx_fifo_ready(dev)) {
		/* Put a character into Tx FIFO */
		NPCM_REG_WRITE(&inst->UTBUF, tx_data[tx_bytes++]);
	}

	k_spin_unlock(&data->lock, key);

	return tx_bytes;
}

static int uart_npcm_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	unsigned int rx_bytes = 0U;

	/* If least one byte is in the Rx FIFO */
	while ((size - rx_bytes > 0) && uart_npcm_rx_fifo_available(dev)) {
		/* Receive one byte from Rx FIFO */
		rx_data[rx_bytes++] = NPCM_REG_READ(&inst->URBUF);
	}

	return rx_bytes;
}

static void uart_npcm_irq_tx_enable(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	struct uart_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	NPCM_REG_SET_BITS(&inst->UICTRL, BIT(NPCM_UICTRL_ETI));
	k_spin_unlock(&data->lock, key);
}

static void uart_npcm_irq_tx_disable(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	struct uart_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	NPCM_REG_CLR_BITS(&inst->UICTRL, BIT(NPCM_UICTRL_ETI));
	k_spin_unlock(&data->lock, key);
}

static bool uart_npcm_irq_tx_is_enabled(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	return IS_BIT_SET(NPCM_REG_READ(&inst->UICTRL), NPCM_UICTRL_ETI);
}

static int uart_npcm_irq_tx_ready(const struct device *dev)
{
	return uart_npcm_tx_fifo_ready(dev) && uart_npcm_irq_tx_is_enabled(dev);
}

static int uart_npcm_irq_tx_complete(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	/* Tx FIFO is empty or last byte is sending */
	return !IS_BIT_SET(NPCM_REG_READ(&inst->USTAT), NPCM_USTAT_XMIP);
}

static void uart_npcm_irq_rx_enable(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	struct uart_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	NPCM_REG_SET_BITS(&inst->UICTRL, BIT(NPCM_UICTRL_ERI));
	k_spin_unlock(&data->lock, key);
}

static void uart_npcm_irq_rx_disable(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	struct uart_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	NPCM_REG_CLR_BITS(&inst->UICTRL, BIT(NPCM_UICTRL_ERI));
	k_spin_unlock(&data->lock, key);
}

static bool uart_npcm_irq_rx_is_enabled(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	return IS_BIT_SET(NPCM_REG_READ(&inst->UICTRL), NPCM_UICTRL_ERI);
}

static int uart_npcm_irq_rx_ready(const struct device *dev)
{
	return uart_npcm_rx_fifo_available(dev);
}

static void uart_npcm_irq_err_enable(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	struct uart_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	NPCM_REG_SET_BITS(&inst->UICTRL, BIT(NPCM_UICTRL_EEI));
	k_spin_unlock(&data->lock, key);
}

static void uart_npcm_irq_err_disable(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	struct uart_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	NPCM_REG_CLR_BITS(&inst->UICTRL, BIT(NPCM_UICTRL_EEI));
	k_spin_unlock(&data->lock, key);
}

static int uart_npcm_irq_is_pending(const struct device *dev)
{
	return uart_npcm_irq_tx_ready(dev) ||
	       (uart_npcm_irq_rx_ready(dev) && uart_npcm_irq_rx_is_enabled(dev));
}

static void uart_npcm_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_npcm_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->user_cb = cb;
	data->user_data = cb_data;

	k_spin_unlock(&data->lock, key);
}

static void uart_npcm_isr(const struct device *dev)
{
	struct uart_npcm_data *data = dev->data;
	uart_irq_callback_user_data_t cb;
	void *cb_data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	cb = data->user_cb;
	cb_data = data->user_data;
	k_spin_unlock(&data->lock, key);

	if (cb) {
		cb(dev, cb_data);
	}
}

/*
 * Poll-in implementation for interrupt driven config, forward call to
 * uart_npcm_fifo_read().
 */
static int uart_npcm_poll_in(const struct device *dev, unsigned char *c)
{
	return uart_npcm_fifo_read(dev, c, 1) ? 0 : -1;
}

/*
 * Poll-out implementation for interrupt driven config, forward call to
 * uart_npcm_fifo_fill().
 */
static void uart_npcm_poll_out(const struct device *dev, unsigned char c)
{
	while (!uart_npcm_fifo_fill(dev, &c, 1)) {
		continue;
	}
}

#else  /* !CONFIG_UART_INTERRUPT_DRIVEN */

/*
 * Poll-in implementation for byte mode config, read byte from URBUF if
 * available.
 */
static int uart_npcm_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	if (!IS_BIT_SET(NPCM_REG_READ(&inst->UICTRL), NPCM_UICTRL_RBF)) {
		return -1;
	}

	*c = NPCM_REG_READ(&inst->URBUF);
	return 0;
}

/*
 * Poll-out implementation for byte mode config, write byte to UTBUF if empty.
 */
static void uart_npcm_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;

	while (!IS_BIT_SET(NPCM_REG_READ(&inst->UICTRL), NPCM_UICTRL_TBE)) {
		continue;
	}
	NPCM_REG_WRITE(&inst->UTBUF, c);
}
#endif /* !CONFIG_UART_INTERRUPT_DRIVEN */

/* UART api functions */
static int uart_npcm_err_check(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	uint32_t err = 0U;

	uint8_t stat = NPCM_REG_READ(&inst->USTAT);

	if (IS_BIT_SET(stat, NPCM_USTAT_DOE)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (IS_BIT_SET(stat, NPCM_USTAT_PE)) {
		err |= UART_ERROR_PARITY;
	}

	if (IS_BIT_SET(stat, NPCM_USTAT_FE)) {
		err |= UART_ERROR_FRAMING;
	}

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static inline bool uart_cfg2npcm_parity(uint8_t *mode_reg, enum uart_config_parity parity)
{
	if (parity != UART_CFG_PARITY_NONE) {
		uint8_t psel;

		*mode_reg |= BIT(NPCM_UFRS_PEN); /* Enable parity */

		switch (parity) {
		case UART_CFG_PARITY_NONE:
			return false;
		case UART_CFG_PARITY_ODD:
			psel = NPCM_UFRS_PSEL_ODD;
			break;
		case UART_CFG_PARITY_EVEN:
			psel = NPCM_UFRS_PSEL_EVEN;
			break;
		case UART_CFG_PARITY_MARK:
			psel = NPCM_UFRS_PSEL_MARK;
			break;
		case UART_CFG_PARITY_SPACE:
			psel = NPCM_UFRS_PSEL_SPACE;
			break;
		default:
			return false;
		}

		*mode_reg = npcm_set_field(*mode_reg, NPCM_UFRS_PSEL_FIELD, psel);
	} else {
		*mode_reg &= ~BIT(NPCM_UFRS_PEN); /* Disable parity */
	}

	return true;
}

static inline bool uart_cfg2npcm_stopbits(uint8_t *mode_reg, enum uart_config_stop_bits stopbits)
{
	switch (stopbits) {
	case UART_CFG_STOP_BITS_0_5:
	case UART_CFG_STOP_BITS_1_5:
		return false;
	case UART_CFG_STOP_BITS_1:
		*mode_reg &= ~BIT(NPCM_UFRS_STP); /* 1 stop bit */
		break;
	case UART_CFG_STOP_BITS_2:
		*mode_reg |= BIT(NPCM_UFRS_STP); /* 2 stop bits */
		break;
	}

	return true;
}

static inline bool uart_cfg2npcm_databits(uint8_t *mode_reg, enum uart_config_data_bits databits)
{
	switch (databits) {
	case UART_CFG_DATA_BITS_5:
	case UART_CFG_DATA_BITS_6:
		/* Controller doesn't support 5 or 6 data bits */
		return false;
	case UART_CFG_DATA_BITS_7:
		*mode_reg = npcm_set_field(*mode_reg, NPCM_UFRS_CHAR_FIELD, NPCM_UFRS_CHAR_7BITS);
		break;
	case UART_CFG_DATA_BITS_8:
		*mode_reg = npcm_set_field(*mode_reg, NPCM_UFRS_CHAR_FIELD, NPCM_UFRS_CHAR_8BITS);
		break;
	case UART_CFG_DATA_BITS_9:
	default:
		*mode_reg = npcm_set_field(*mode_reg, NPCM_UFRS_CHAR_FIELD, NPCM_UFRS_CHAR_9BITS);
		break;
	}

	return true;
}

static int uart_npcm_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_npcm_data *const data = dev->data;
	struct uart_reg *const inst = config->base;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	uint32_t uart_rate;
	uint8_t mode_reg = NPCM_REG_READ(&inst->UFRS);
	int ret;

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	if (cfg->baudrate == 0U) {
		return -EINVAL;
	}

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("UART clock device not ready");
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on UART clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clk_cfg, &uart_rate);
	if (ret < 0) {
		LOG_ERR("Get UART clock rate error %d", ret);
		return ret;
	}

	/* Assemble the updated registers, validity checks contained within */
	if ((!uart_cfg2npcm_parity(&mode_reg, cfg->parity)) ||
	    (!uart_cfg2npcm_stopbits(&mode_reg, cfg->stop_bits)) ||
	    (!uart_cfg2npcm_databits(&mode_reg, cfg->data_bits))) {
		return -ENOTSUP;
	}

	/* Set baud rate */
	uart_set_npcm_baud_rate(dev, cfg->baudrate, uart_rate);
	NPCM_REG_WRITE(&inst->UFRS, mode_reg);
	data->baud_rate = cfg->baudrate;

	return 0;
}

static inline enum uart_config_parity uart_npcm_cfg_parity(uint8_t mode_reg)
{
	if (IS_BIT_SET(mode_reg, NPCM_UFRS_PEN)) {
		/* Parity enabled */
		switch (GET_FIELD(mode_reg, NPCM_UFRS_PSEL_FIELD)) {
		case NPCM_UFRS_PSEL_ODD:
			return UART_CFG_PARITY_ODD;
		case NPCM_UFRS_PSEL_EVEN:
			return UART_CFG_PARITY_EVEN;
		case NPCM_UFRS_PSEL_MARK:
			return UART_CFG_PARITY_MARK;
		case NPCM_UFRS_PSEL_SPACE:
			return UART_CFG_PARITY_SPACE;
		default:
			return UART_CFG_PARITY_NONE;
		}
	} else {
		/* Parity disabled */
		return UART_CFG_PARITY_NONE;
	}
}

static inline enum uart_config_stop_bits uart_npcm_cfg_stopbits(uint8_t mode_reg)
{
	if (IS_BIT_SET(mode_reg, NPCM_UFRS_STP)) {
		/* Stop bits enabled */
		return UART_CFG_STOP_BITS_2;
	} else {
		return UART_CFG_STOP_BITS_1;
	}
}

static inline enum uart_config_data_bits uart_npcm_cfg_databits(uint8_t mode_reg)
{
	switch (GET_FIELD(mode_reg, NPCM_UFRS_CHAR_FIELD)) {
	case NPCM_UFRS_CHAR_8BITS:
		return UART_CFG_DATA_BITS_8;
	case NPCM_UFRS_CHAR_7BITS:
		return UART_CFG_DATA_BITS_7;
	case NPCM_UFRS_CHAR_9BITS:
		return UART_CFG_DATA_BITS_9;
	case 3:
		return UART_CFG_DATA_BITS_9;
	default:
		return UART_CFG_DATA_BITS_8;
	}
}

static int uart_npcm_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_npcm_data *const data = dev->data;
	struct uart_reg *const inst = config->base;
	uint8_t mode_reg = NPCM_REG_READ(&inst->UFRS);

	cfg->baudrate = data->baud_rate;
	cfg->parity = uart_npcm_cfg_parity(mode_reg);
	cfg->stop_bits = uart_npcm_cfg_stopbits(mode_reg);
	cfg->data_bits = uart_npcm_cfg_databits(mode_reg);
	cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

/* UART driver registration */
static DEVICE_API(uart, uart_npcm_driver_api) = {
	.poll_in = uart_npcm_poll_in,
	.poll_out = uart_npcm_poll_out,
	.err_check = uart_npcm_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_npcm_fifo_fill,
	.fifo_read = uart_npcm_fifo_read,
	.irq_tx_enable = uart_npcm_irq_tx_enable,
	.irq_tx_disable = uart_npcm_irq_tx_disable,
	.irq_tx_ready = uart_npcm_irq_tx_ready,
	.irq_tx_complete = uart_npcm_irq_tx_complete,
	.irq_rx_enable = uart_npcm_irq_rx_enable,
	.irq_rx_disable = uart_npcm_irq_rx_disable,
	.irq_rx_ready = uart_npcm_irq_rx_ready,
	.irq_err_enable = uart_npcm_irq_err_enable,
	.irq_err_disable = uart_npcm_irq_err_disable,
	.irq_is_pending = uart_npcm_irq_is_pending,
	.irq_update = uart_npcm_irq_update,
	.irq_callback_set = uart_npcm_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_npcm_configure,
	.config_get = uart_npcm_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
};

static void uart_set_npcm_baud_rate(const struct device *dev, int baud_rate, uint32_t src_clk)
{
	uint32_t divisor, opt_dev, min_deviation, clk, calc_baudrate, deviation;
	const struct uart_npcm_config *const config = dev->config;
	struct uart_reg *const inst = config->base;
	uint8_t prescalar, opt_prescalar, i, upsr;

	/*
	 * baud_rate calculation equation:
	 *
	 * BR = APB2_CLK/(16 x DIV x P)
	 *
	 * P is the prescalar decided by UPSC field (5 bits) in UPSR register.
	 *
	 * The correspondences between the 5-bit prescaler select (UPSC) and
	 *
	 * prescaler divide factors are as follows.
	 *
	 * UPSC		Prescaler Factor
	 *
	 * 00000b	NO CLOCK
	 *
	 * 00001b	1
	 *
	 * 00010b	1.5
	 *
	 * 00011b	2
	 *
	 * 00100b	2.5
	 *
	 * 00101b	3
	 *
	 * 00110b	3.5
	 *
	 * 00111b	4
	 *
	 * .
	 * .
	 * .
	 *
	 * 11110b	15.5
	 *
	 * 11111b	16
	 *
	 * The prescaler increased by 0.5.
	 *
	 * For easier calculation, the prescalar times 10.
	 *
	 */

	/* Calculated UART baudrate , clock source from APB2 */
	opt_prescalar = 0;
	opt_dev = 0;
	prescalar = 10;
	min_deviation = 0xFFFFFFFF;
	clk = src_clk;

	for (i = 1; i <= 31; i++) {
		divisor = (clk * 10) / (16 * baud_rate * prescalar);
		if (divisor == 0) {
			divisor = 1;
		}

		calc_baudrate = (clk * 10) / (16 * divisor * prescalar);
		deviation = (calc_baudrate > baud_rate) ? (calc_baudrate - baud_rate)
							: (baud_rate - calc_baudrate);
		if (deviation < min_deviation) {
			min_deviation = deviation;
			opt_prescalar = i;
			opt_dev = divisor;
		}
		prescalar += 5;
	}

	/*
	 * Datasheet sequence: force prescaler to NO CLOCK before selecting
	 * a new baud rate to avoid transient receive/transmit corruption.
	 */
	upsr = NPCM_REG_READ(&inst->UPSR);
	upsr = npcm_set_field(upsr, NPCM_UPSR_UPSC_FIELD, NPCM_UPSC_NO_CLOCK);
	NPCM_REG_WRITE(&inst->UPSR, upsr);

	opt_dev--;
	NPCM_REG_WRITE(&inst->UBAUD, opt_dev);
	NPCM_REG_WRITE(&inst->UPSR, ((opt_prescalar << 3) & NPCM_UPSR_UPSC_FIELD) |
					    ((opt_dev >> 8) & NPCM_UPSR_UDIV10_8_FIELD));
}

static int uart_npcm_init(const struct device *dev)
{
	const struct uart_npcm_config *const config = dev->config;
	struct uart_npcm_data *const data = dev->data;
	struct uart_reg *const inst = config->base;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	uint32_t uart_rate;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("UART clock device not ready");
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on UART clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clk_cfg, &uart_rate);
	if (ret < 0) {
		LOG_ERR("Get UART clock rate error %d", ret);
		return ret;
	}

	uart_set_npcm_baud_rate(dev, data->baud_rate, uart_rate);

	/*
	 * 8-N-1, FIFO enabled.  Must be done after setting
	 * the divisor for the new divisor to take effect.
	 */
	NPCM_REG_WRITE(&inst->UFRS, 0x00);
#if CONFIG_UART_INTERRUPT_DRIVEN
	uint8_t ufctrl = NPCM_REG_READ(&inst->UFCTRL);

	ufctrl = npcm_set_field(ufctrl, NPCM_UFCTRL_RXFTH_FIELD, config->rx_fifo_threshold);
	ufctrl |= BIT(NPCM_UFCTRL_FIFOEN);
	NPCM_REG_WRITE(&inst->UFCTRL, ufctrl);

	/* Disable all UART tx FIFO interrupts */
	uart_npcm_dis_all_tx_interrupts(dev);

	/* Clear UART rx FIFO */
	uart_npcm_clear_rx_fifo(dev);

	/* Configure UART interrupts */
	config->irq_config_func(dev);
#endif

	/* Configure pin-mux for uart device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("UART pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static inline bool uart_npcm_device_is_transmitting(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
		/* The transmitted transaction is completed? */
		return !uart_npcm_irq_tx_complete(dev);
	}

	/* No need for polling mode */
	return 0;
}

static inline int uart_npcm_get_power_state(const struct device *dev, uint32_t *state)
{
	const struct uart_npcm_data *const data = dev->data;

	*state = data->pm_state;
	return 0;
}

static inline int uart_npcm_set_power_state(const struct device *dev, uint32_t next_state)
{
	struct uart_npcm_data *const data = dev->data;

	/* If next device power state is LOW or SUSPEND power state */
	if (next_state == PM_DEVICE_STATE_SUSPENDING || next_state == PM_DEVICE_STATE_SUSPENDED) {
		if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
			int64_t deadline = k_uptime_get() + UART_NPCM_PM_TX_DRAIN_TIMEOUT_MS;

			/* Wait for TX completion with timeout to avoid suspend hangs. */
			while (uart_npcm_device_is_transmitting(dev)) {
				if (k_uptime_get() >= deadline) {
					LOG_WRN("Suspend blocked: UART TX drain timeout");
					return -ETIMEDOUT;
				}

				k_busy_wait(50);
			}
		}
	}

	data->pm_state = next_state;
	return 0;
}

/* Implements the device power management action functionality */
static int uart_npcm_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = uart_npcm_set_power_state(dev, PM_DEVICE_STATE_SUSPENDED);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = uart_npcm_set_power_state(dev, PM_DEVICE_STATE_ACTIVE);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		ret = uart_npcm_set_power_state(dev, PM_DEVICE_STATE_OFF);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		ret = uart_npcm_set_power_state(dev, PM_DEVICE_STATE_ACTIVE);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)
PINCTRL_DT_INST_DEFINE(0);
#ifdef CONFIG_PM_DEVICE
PM_DEVICE_DT_INST_DEFINE(0, uart_npcm_pm_action);
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_npcm_irq_config_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), uart_npcm_isr, DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(0));
}
#endif
static const struct uart_npcm_config uart_npcm_cfg_0 = {
	.base = (struct uart_reg *)DT_INST_REG_ADDR(0),
	.clk_cfg = DT_INST_PHA(0, clocks, clk_id),
	.rx_fifo_threshold = DT_INST_PROP(0, rx_fifo_threshold),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_npcm_irq_config_0,
#endif
};

static struct uart_npcm_data uart_npcm_data_0 = {
	.baud_rate = DT_INST_PROP(0, current_speed),
};

DEVICE_DT_INST_DEFINE(0, &uart_npcm_init,
		      COND_CODE_1(CONFIG_PM_DEVICE, (PM_DEVICE_DT_INST_GET(0)), (NULL)),
						    &uart_npcm_data_0, &uart_npcm_cfg_0,
						    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
						    &uart_npcm_driver_api);
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(1), okay)
PINCTRL_DT_INST_DEFINE(1);
#ifdef CONFIG_PM_DEVICE
PM_DEVICE_DT_INST_DEFINE(1, uart_npcm_pm_action);
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_npcm_irq_config_1(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(1), DT_INST_IRQ(1, priority), uart_npcm_isr, DEVICE_DT_INST_GET(1),
		    0);
	irq_enable(DT_INST_IRQN(1));
}
#endif
static const struct uart_npcm_config uart_npcm_cfg_1 = {
	.base = (struct uart_reg *)DT_INST_REG_ADDR(1),
	.clk_cfg = DT_INST_PHA(1, clocks, clk_id),
	.rx_fifo_threshold = DT_INST_PROP(1, rx_fifo_threshold),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(1),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_npcm_irq_config_1,
#endif
};

static struct uart_npcm_data uart_npcm_data_1 = {
	.baud_rate = DT_INST_PROP(1, current_speed),
};

DEVICE_DT_INST_DEFINE(1, &uart_npcm_init,
		      COND_CODE_1(CONFIG_PM_DEVICE, (PM_DEVICE_DT_INST_GET(1)), (NULL)),
						    &uart_npcm_data_1, &uart_npcm_cfg_1,
						    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
						    &uart_npcm_driver_api);
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(2), okay)
PINCTRL_DT_INST_DEFINE(2);
#ifdef CONFIG_PM_DEVICE
PM_DEVICE_DT_INST_DEFINE(2, uart_npcm_pm_action);
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_npcm_irq_config_2(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(2), DT_INST_IRQ(2, priority), uart_npcm_isr, DEVICE_DT_INST_GET(2),
		    0);
	irq_enable(DT_INST_IRQN(2));
}
#endif
static const struct uart_npcm_config uart_npcm_cfg_2 = {
	.base = (struct uart_reg *)DT_INST_REG_ADDR(2),
	.clk_cfg = DT_INST_PHA(2, clocks, clk_id),
	.rx_fifo_threshold = DT_INST_PROP(2, rx_fifo_threshold),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(2),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_npcm_irq_config_2,
#endif
};

static struct uart_npcm_data uart_npcm_data_2 = {
	.baud_rate = DT_INST_PROP(2, current_speed),
};

DEVICE_DT_INST_DEFINE(2, &uart_npcm_init,
		      COND_CODE_1(CONFIG_PM_DEVICE, (PM_DEVICE_DT_INST_GET(2)), (NULL)),
						    &uart_npcm_data_2, &uart_npcm_cfg_2,
						    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
						    &uart_npcm_driver_api);
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(3), okay)
PINCTRL_DT_INST_DEFINE(3);
#ifdef CONFIG_PM_DEVICE
PM_DEVICE_DT_INST_DEFINE(3, uart_npcm_pm_action);
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_npcm_irq_config_3(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(3), DT_INST_IRQ(3, priority), uart_npcm_isr, DEVICE_DT_INST_GET(3),
		    0);
	irq_enable(DT_INST_IRQN(3));
}
#endif
static const struct uart_npcm_config uart_npcm_cfg_3 = {
	.base = (struct uart_reg *)DT_INST_REG_ADDR(3),
	.clk_cfg = DT_INST_PHA(3, clocks, clk_id),
	.rx_fifo_threshold = DT_INST_PROP(3, rx_fifo_threshold),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(3),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_npcm_irq_config_3,
#endif
};

static struct uart_npcm_data uart_npcm_data_3 = {
	.baud_rate = DT_INST_PROP(3, current_speed),
};

DEVICE_DT_INST_DEFINE(3, &uart_npcm_init,
		      COND_CODE_1(CONFIG_PM_DEVICE, (PM_DEVICE_DT_INST_GET(3)), (NULL)),
						    &uart_npcm_data_3, &uart_npcm_cfg_3,
						    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
						    &uart_npcm_driver_api);
#endif
