/*
 * Copyright (c) 2023, Ionut Pavel <iocapa@iocapa.com>
 * Copyright (c) 2022, Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico_util.h>

/* PIO registers */
#include <hardware/structs/pio.h>

#define DT_DRV_COMPAT	raspberrypi_pico_uart_pio

struct pio_uart_config {
	const struct device *parent;
	const struct pinctrl_dev_config *pcfg;
	pio_hw_t *pio_regs;
	uint32_t clock_frequency;
#if CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config;
	uint8_t irq_idx;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	uint8_t tx_gpio;
	uint8_t rx_gpio;
};

struct pio_uart_data {
#if CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	struct uart_config uart_config;
	uint8_t tx_sm;
	uint8_t tx_sm_mask;
	uint8_t tx_prg;
	uint8_t rx_sm;
	uint8_t rx_sm_mask;
	uint8_t rx_shift;
	uint8_t rx_prg;
};

/* Number of bits to shift */
#define PIO_UART_TXRX_BITS_CNT(bits)		((bits) - 1)

/**
 * 8N1 initial configuration
 * OUT pin 0 and side-set pin 0 mapped to the TX pin
 * Scratch Y for number of bits to shift (-1)
 * SMx TXNFULL IRQ for signalling
 * SMx clk_div = baud * 4
 */

/* .side_set 1 opt */
#define PIO_UART_TX_DSS(opt, ss, delay)		PIO_ASM_SIDE(1, opt, 2, ss, delay)

/* Patching parameters */
#define PIO_UART_TX_STOP_OFFSET			0
#define PIO_UART_TX_STOP_0_5			1
#define PIO_UART_TX_STOP_1			3
#define PIO_UART_TX_STOP_1_5			5
#define PIO_UART_TX_STOP_2			7

/* TX instructions */
static const uint16_t pio_uart_tx_prg[] = {
	/* wrap_bot */
	/* [stop bits : delay] => [0.5 : 1], [1 : 3], [1.5 : 5], [2 : 7]*/
	/* pull		side 1 [2] */
	PIO_ASM_PULL(0, 1, PIO_UART_TX_DSS(1, 1, 3)),
	/* mov x, y	side 0 [3] */
	PIO_ASM_MOV(PIO_ASM_MOV_DST_X, PIO_ASM_MOV_OP_NONE, PIO_ASM_MOV_SRC_Y,
									PIO_UART_TX_DSS(1, 0, 3)),
	/* loop: */
	/* out pins, 1 */
	PIO_ASM_OUT(PIO_ASM_OUT_DST_PINS, 1, PIO_UART_TX_DSS(0, 0, 0)),
	/* jmp x-- loop	[2] */
	PIO_ASM_JMP(PIO_ASM_JMP_COND_DECX, PIO_ASM_ADDR(0, 2), PIO_UART_TX_DSS(0, 0, 2)),
	/* wrap_top */
};

/**
 * 8N1 initial configuration
 * IN pin 0 and JMP pin mapped to the RX pin
 * Scratch Y for number of bits to shift (-1)
 * SMx IRQ for framing error
 * SMx RXNEMPTY IRQ for signalling
 * SMx clk_div = baud * 8
 */

/* Shift count due to right alignment */
#define PIO_UART_RX_SHIFT_CNT(bits)		(16 - (bits))

/* Default */
#define PIO_UART_RX_DSS(delay)			PIO_ASM_SIDE(0, 0, 0, 0, delay)

/* RX program */
static const uint16_t pio_uart_rx_prg[] = {
	/* wrap bot */
	/* start: */
	/* wait 0 pin 0 */
	PIO_ASM_WAIT(0, PIO_ASM_WAIT_SRC_PIN, 0, PIO_UART_RX_DSS(0)),
	/* mov x, y	[10] */
	PIO_ASM_MOV(PIO_ASM_MOV_DST_X, PIO_ASM_MOV_OP_NONE, PIO_ASM_MOV_SRC_Y, PIO_UART_RX_DSS(10)),
	/* bitloop: */
	/* in pins, 1 */
	PIO_ASM_IN(PIO_ASM_IN_SRC_PINS, 1, PIO_UART_RX_DSS(0)),
	/* jmp x--, bitloop [6] */
	PIO_ASM_JMP(PIO_ASM_JMP_COND_DECX, PIO_ASM_ADDR(0, 2), PIO_UART_RX_DSS(6)),
	/* jmp pin, stop */
	PIO_ASM_JMP(PIO_ASM_JMP_COND_PIN, PIO_ASM_ADDR(0, 8), PIO_UART_RX_DSS(0)),
	/* irq 0 rel */
	PIO_ASM_IRQ(0, 0, PIO_ASM_INDEX(true, 0), PIO_UART_RX_DSS(0)),
	/* wait 1 pin 0 */
	PIO_ASM_WAIT(1, PIO_ASM_WAIT_SRC_PIN, 0, PIO_UART_RX_DSS(0)),
	/* jmp start */
	PIO_ASM_JMP(PIO_ASM_JMP_COND_ALWAYS, PIO_ASM_ADDR(0, 0), PIO_UART_RX_DSS(0)),
	/* stop: */
	/* push */
	PIO_ASM_PUSH(0, 0, PIO_UART_RX_DSS(0)),
	/* wrap top */
};

static int pio_uart_init_tx(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	pio_sm_hw_t *pio_sm;
	int rc;

	/* Allocate state machine */
	rc = pio_rpi_pico_alloc_sm(config->parent, 1, &data->tx_sm);
	if (rc < 0) {
		return rc;
	}

	pio_sm = &pio_hw->sm[data->tx_sm];
	data->tx_sm_mask = BIT(data->tx_sm);

	/* Allocate instructions */
	rc = pio_rpi_pico_alloc_instr(config->parent, ARRAY_SIZE(pio_uart_tx_prg), &data->tx_prg);
	if (rc < 0) {
		return rc;
	}

	/* Load initial program */
	pio_rpi_pico_util_load_prg(pio_hw->instr_mem, data->tx_prg,
				   pio_uart_tx_prg, ARRAY_SIZE(pio_uart_tx_prg));

	/**
	 * 2 side set count (1 extra for enable)
	 * 1 set count
	 * 1 out count
	 * tx_gpio sideset base
	 * tx_gpio set base
	 * tx_gpio out base
	 */
	pio_sm->pinctrl = (2u << PIO_SM0_PINCTRL_SIDESET_COUNT_LSB)
			| (1u << PIO_SM0_PINCTRL_SET_COUNT_LSB)
			| (1u << PIO_SM0_PINCTRL_OUT_COUNT_LSB)
			| (config->tx_gpio << PIO_SM0_PINCTRL_SIDESET_BASE_LSB)
			| (config->tx_gpio << PIO_SM0_PINCTRL_SET_BASE_LSB)
			| (config->tx_gpio << PIO_SM0_PINCTRL_OUT_BASE_LSB);

	/* Force pin to 1 */
	pio_sm->instr = PIO_ASM_SET(PIO_ASM_SET_DST_PINS, 1, PIO_UART_TX_DSS(0, 0, 0));

	/* Force direction to output */
	pio_sm->instr = PIO_ASM_SET(PIO_ASM_SET_DST_PINDIRS, 1, PIO_UART_TX_DSS(0, 0, 0));

	/**
	 * Enable side bit
	 * Set wraps
	 */
	pio_sm->execctrl = PIO_SM0_EXECCTRL_SIDE_EN_BITS
			 | (data->tx_prg << PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB)
			 | ((data->tx_prg + ARRAY_SIZE(pio_uart_tx_prg) - 1)
					 << PIO_SM0_EXECCTRL_WRAP_TOP_LSB);

	/**
	 * Join TX
	 * Out right shift
	 */
	pio_sm->shiftctrl = PIO_SM0_SHIFTCTRL_FJOIN_TX_BITS
			  | PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS;

	return 0;
}

static int pio_uart_init_rx(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	pio_sm_hw_t *pio_sm;
	int rc;

	/* Allocate state machine */
	rc = pio_rpi_pico_alloc_sm(config->parent, 1, &data->rx_sm);
	if (rc < 0) {
		return rc;
	}

	pio_sm = &pio_hw->sm[data->rx_sm];
	data->rx_sm_mask = BIT(data->rx_sm);

	/* Allocate shared instructions */
	rc = pio_rpi_pico_alloc_shared_instr(config->parent, STRINGIFY(DT_DRV_COMPAT),
					     ARRAY_SIZE(pio_uart_rx_prg), &data->rx_prg);
	if ((rc < 0) && (rc != -EALREADY)) {
		return rc;
	}

	if (rc != -EALREADY) {
		/* Load initial program */
		pio_rpi_pico_util_load_prg(pio_hw->instr_mem, data->rx_prg,
					pio_uart_rx_prg, ARRAY_SIZE(pio_uart_rx_prg));
	}

	/**
	 * rx_gpio in base
	 * rx_gpio set base
	 */
	pio_sm->pinctrl = (1u << PIO_SM0_PINCTRL_SET_COUNT_LSB)
			| (config->rx_gpio << PIO_SM0_PINCTRL_SET_BASE_LSB)
			| (config->rx_gpio << PIO_SM0_PINCTRL_IN_BASE_LSB);

	/* Force direction to input */
	pio_sm->instr = PIO_ASM_SET(PIO_ASM_SET_DST_PINDIRS, 0, PIO_UART_RX_DSS(0));

	/**
	 * Set JMP pin
	 * Set wraps
	 */
	pio_sm->execctrl = (config->rx_gpio << PIO_SM0_EXECCTRL_JMP_PIN_LSB)
			 | (data->rx_prg << PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB)
			 | ((data->rx_prg + ARRAY_SIZE(pio_uart_rx_prg) - 1)
					 << PIO_SM0_EXECCTRL_WRAP_TOP_LSB);

	/**
	 * Join RX
	 * In right shift
	 */
	pio_sm->shiftctrl = PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS
			  | PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_BITS;

	return 0;
}

static int pio_uart_configure_txrx(const struct device *dev, const struct uart_config *cfg)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	pio_sm_hw_t *tx_sm = &pio_hw->sm[data->tx_sm];
	pio_sm_hw_t *rx_sm = &pio_hw->sm[data->rx_sm];
	uint32_t scratch_y, tx_delay, patch;
	uint8_t rx_shift;

	/* Flow control not supported */
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	/* Check parity */
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;

	default:
		return -ENOTSUP;
	}

	/* Baud cannot be 0 */
	if (cfg->baudrate == 0) {
		return -EINVAL;
	}

	/* Check and set data bits (see program description) */
	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		scratch_y = PIO_UART_TXRX_BITS_CNT(5);
		rx_shift = PIO_UART_RX_SHIFT_CNT(5);
		break;

	case UART_CFG_DATA_BITS_6:
		scratch_y = PIO_UART_TXRX_BITS_CNT(6);
		rx_shift = PIO_UART_RX_SHIFT_CNT(6);
		break;

	case UART_CFG_DATA_BITS_7:
		scratch_y = PIO_UART_TXRX_BITS_CNT(7);
		rx_shift = PIO_UART_RX_SHIFT_CNT(7);
		break;

	case UART_CFG_DATA_BITS_8:
		scratch_y = PIO_UART_TXRX_BITS_CNT(8);
		rx_shift = PIO_UART_RX_SHIFT_CNT(8);
		break;

	default:
		return -ENOTSUP;
	}

	/* Check and set stop bits (see program description) */
	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_0_5:
		tx_delay = PIO_UART_TX_STOP_0_5;
		break;

	case UART_CFG_STOP_BITS_1:
		tx_delay = PIO_UART_TX_STOP_1;
		break;

	case UART_CFG_STOP_BITS_1_5:
		tx_delay = PIO_UART_TX_STOP_1_5;
		break;

	case UART_CFG_STOP_BITS_2:
		tx_delay = PIO_UART_TX_STOP_2;
		break;

	default:
		return -ENOTSUP;
	}

	/* TX configuration (including stop bits patch) */
	tx_sm->clkdiv = PIO_SM_CLKDIV(config->clock_frequency, (cfg->baudrate * 4));
	tx_sm->instr = PIO_ASM_SET(PIO_ASM_SET_DST_Y, scratch_y, PIO_UART_TX_DSS(0, 0, 0));
	patch = PIO_ASM_PULL(0, 1, PIO_UART_TX_DSS(1, 1, tx_delay));
	pio_hw->instr_mem[PIO_UART_TX_STOP_OFFSET + data->tx_prg] = patch;

	/* RX configuration */
	rx_sm->clkdiv = PIO_SM_CLKDIV(config->clock_frequency, (cfg->baudrate * 8));
	rx_sm->instr = PIO_ASM_SET(PIO_ASM_SET_DST_Y, scratch_y, PIO_UART_RX_DSS(0));
	data->rx_shift = rx_shift;

	return 0;
}

static void pio_uart_enable_txrx(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	pio_sm_hw_t *tx_sm = &pio_hw->sm[data->tx_sm];
	pio_sm_hw_t *rx_sm = &pio_hw->sm[data->rx_sm];
	uint32_t mask = data->tx_sm_mask | data->rx_sm_mask;

	/* Go to start */
	tx_sm->instr = PIO_ASM_JMP(PIO_ASM_JMP_COND_ALWAYS, data->tx_prg, PIO_UART_TX_DSS(0, 0, 0));
	rx_sm->instr = PIO_ASM_JMP(PIO_ASM_JMP_COND_ALWAYS, data->rx_prg, PIO_UART_RX_DSS(0));

	/* Restart clocks */
	PIO_ATOMIC_SET(pio_hw->ctrl, mask << PIO_CTRL_CLKDIV_RESTART_LSB);

	/* Restart SMs */
	PIO_ATOMIC_SET(pio_hw->ctrl, mask << PIO_CTRL_SM_RESTART_LSB);

	/* Enable  SMs */
	PIO_ATOMIC_SET(pio_hw->ctrl, mask << PIO_CTRL_SM_ENABLE_LSB);
}

static void pio_uart_disable_txrx(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	uint32_t mask = data->tx_sm_mask | data->rx_sm_mask;

	/* Disable SMs */
	PIO_ATOMIC_CLR(pio_hw->ctrl, mask << PIO_CTRL_SM_ENABLE_LSB);
}

#if CONFIG_UART_INTERRUPT_DRIVEN
static int pio_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	uint32_t fmask = (uint32_t)data->tx_sm_mask << PIO_FSTAT_TXFULL_LSB;
	int cnt = 0;

	while (((len - cnt) > 0) && !(pio_hw->fstat & fmask)) {
		pio_hw->txf[data->tx_sm] = (uint32_t)tx_data[cnt++];
	}

	return cnt;
}

static int pio_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	uint32_t fmask = (uint32_t)data->rx_sm_mask << PIO_FSTAT_RXEMPTY_LSB;
	io_ro_16 *fifo = (io_ro_16 *)&pio_hw->rxf[data->rx_sm] + 1; /* Upper 16 bits */
	int cnt = 0;

	while (((len - cnt) > 0) && !(pio_hw->fstat & fmask)) {
		rx_data[cnt++] = (unsigned char)(*fifo >> data->rx_shift);
	}

	return cnt;
}

static inline void pio_uart_irq_generic_enable(const struct device *dev, uint32_t mask)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_irq_hw *irq_hw = PIO_IRQ_HW_INDEX(config->pio_regs, config->irq_idx);

	PIO_ATOMIC_SET(irq_hw->inte, mask);
}

static inline void pio_uart_irq_generic_disable(const struct device *dev, uint32_t mask)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_irq_hw *irq_hw = PIO_IRQ_HW_INDEX(config->pio_regs, config->irq_idx);

	PIO_ATOMIC_CLR(irq_hw->inte, mask);
}

static void pio_uart_irq_tx_enable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;
	uint32_t mask = (uint32_t)data->tx_sm_mask << PIO_IRQ0_INTE_SM0_TXNFULL_LSB;

	pio_uart_irq_generic_enable(dev, mask);
}

static void pio_uart_irq_rx_enable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;
	uint32_t mask = (uint32_t)data->rx_sm_mask << PIO_IRQ0_INTE_SM0_RXNEMPTY_LSB;

	pio_uart_irq_generic_enable(dev, mask);
}

static void pio_uart_irq_error_enable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;
	uint32_t mask = (uint32_t)data->rx_sm_mask << PIO_IRQ0_INTE_SM0_LSB;

	pio_uart_irq_generic_enable(dev, mask);
}

static void pio_uart_irq_tx_disable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;
	uint32_t mask = (uint32_t)data->tx_sm_mask << PIO_IRQ0_INTE_SM0_TXNFULL_LSB;

	pio_uart_irq_generic_disable(dev, mask);
}

static void pio_uart_irq_rx_disable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;
	uint32_t mask = (uint32_t)data->rx_sm_mask << PIO_IRQ0_INTE_SM0_RXNEMPTY_LSB;

	pio_uart_irq_generic_disable(dev, mask);
}

static void pio_uart_irq_error_disable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;
	uint32_t mask = (uint32_t)data->rx_sm_mask << PIO_IRQ0_INTE_SM0_LSB;

	pio_uart_irq_generic_disable(dev, mask);
}

static int pio_uart_irq_tx_ready(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	struct pio_irq_hw *irq_hw = PIO_IRQ_HW_INDEX(config->pio_regs, config->irq_idx);
	uint32_t imask = (uint32_t)data->tx_sm_mask << PIO_IRQ0_INTE_SM0_TXNFULL_LSB;
	uint32_t fmask = (uint32_t)data->tx_sm_mask << PIO_FSTAT_TXFULL_LSB;

	/* True only if interrupts enabled and fifo not full */
	return ((irq_hw->inte & imask) && !(pio_hw->fstat & fmask)) ? 1 : 0;
}

static int pio_uart_irq_rx_ready(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	uint32_t fmask = (uint32_t)data->rx_sm_mask << PIO_FSTAT_RXEMPTY_LSB;

	/* True if fifo not empty */
	return !(pio_hw->fstat & fmask) ? 1 : 0;
}

static int pio_uart_irq_tx_complete(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	uint32_t mask = (uint32_t)data->tx_sm_mask << PIO_FSTAT_TXEMPTY_LSB;

	/* True if fifo empty */
	return (pio_hw->fstat & mask) ? 1 : 0;
}

static int pio_uart_irq_is_pending(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	uint32_t mask;

	mask  = (uint32_t)data->tx_sm_mask << PIO_INTR_SM0_TXNFULL_LSB
	      | (uint32_t)data->rx_sm_mask << PIO_INTR_SM0_RXNEMPTY_LSB
	      | (uint32_t)data->rx_sm_mask << PIO_INTR_SM0_LSB;

	/* True if any IRQ is pending */
	return (pio_hw->intr & mask) ? 1 : 0;
}

static int pio_uart_irq_update(const struct device *dev)
{
	return 1;
}

static void pio_uart_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct pio_uart_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

static void pio_uart_irq(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	uint32_t mask;

	/* Filter events */
	mask  = (uint32_t)data->tx_sm_mask << PIO_INTR_SM0_TXNFULL_LSB
	      | (uint32_t)data->rx_sm_mask << PIO_INTR_SM0_RXNEMPTY_LSB
	      | (uint32_t)data->rx_sm_mask << PIO_INTR_SM0_LSB;

	if ((pio_hw->intr & mask) && (data->irq_cb)) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if CONFIG_UART_USE_RUNTIME_CONFIGURE
static int pio_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct pio_uart_data *data = dev->data;
	int rc;

	__ASSERT_NO_MSG(cfg);

	/* No point if no changes */
	rc = memcmp(cfg, &data->uart_config, sizeof(struct uart_config));
	if (rc != 0) {
		/* Disable / config / enable. Will retain current config if invalid */
		pio_uart_disable_txrx(dev);
		rc = pio_uart_configure_txrx(dev, cfg);
		if (rc == 0) {
			data->uart_config = *cfg;
		}
		pio_uart_enable_txrx(dev);
	}

	return rc;
}

static int pio_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct pio_uart_data *data = dev->data;

	*cfg = data->uart_config;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int pio_uart_err_check(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	int err = 0;

	if (pio_hw->fdebug & ((uint32_t)data->rx_sm_mask << PIO_FDEBUG_RXSTALL_LSB)) {
		pio_hw->fdebug = (uint32_t)data->rx_sm_mask << PIO_FDEBUG_RXSTALL_LSB;
		err |= UART_ERROR_OVERRUN;
	}

	if (pio_hw->irq & data->rx_sm_mask) {
		pio_hw->irq = data->rx_sm_mask;
		err |= UART_ERROR_FRAMING;
	}

	return err;
}

static void pio_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;

	while (pio_hw->fstat & ((uint32_t)data->tx_sm_mask << PIO_FSTAT_TXFULL_LSB)) {
	}

	pio_hw->txf[data->tx_sm] = (uint32_t)c;
}

static int pio_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	pio_hw_t *pio_hw = config->pio_regs;
	io_ro_16 *fifo = (io_ro_16 *)&pio_hw->rxf[data->rx_sm] + 1; /* Upper 16 bits */

	if (pio_hw->fstat & ((uint32_t)data->rx_sm_mask << PIO_FSTAT_RXEMPTY_LSB)) {
		return -1;
	}

	*c = (unsigned char)(*fifo >> data->rx_shift);

	return 0;
}

static int pio_uart_init(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	int rc;

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	rc = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}

	rc = pio_uart_init_tx(dev);
	if (rc < 0) {
		return rc;
	}

	rc = pio_uart_init_rx(dev);
	if (rc < 0) {
		return rc;
	}

	rc = pio_uart_configure_txrx(dev, &data->uart_config);
	if (rc < 0) {
		return rc;
	}

#if CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	pio_uart_enable_txrx(dev);

	return 0;
}

static const struct uart_driver_api pio_uart_driver_api = {
	.poll_in = pio_uart_poll_in,
	.poll_out = pio_uart_poll_out,
	.err_check = pio_uart_err_check,
#if CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = pio_uart_configure,
	.config_get = pio_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#if CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = pio_uart_fifo_fill,
	.fifo_read = pio_uart_fifo_read,
	.irq_tx_enable = pio_uart_irq_tx_enable,
	.irq_tx_disable = pio_uart_irq_tx_disable,
	.irq_tx_ready = pio_uart_irq_tx_ready,
	.irq_rx_enable = pio_uart_irq_rx_enable,
	.irq_rx_disable = pio_uart_irq_rx_disable,
	.irq_tx_complete = pio_uart_irq_tx_complete,
	.irq_rx_ready = pio_uart_irq_rx_ready,
	.irq_err_enable = pio_uart_irq_error_enable,
	.irq_err_disable = pio_uart_irq_error_disable,
	.irq_is_pending = pio_uart_irq_is_pending,
	.irq_update = pio_uart_irq_update,
	.irq_callback_set = pio_uart_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#if CONFIG_UART_INTERRUPT_DRIVEN
#define PIO_UART_IRQ_CONFIG(inst)								\
static void pio_uart_irq_config##inst(const struct device *dev)					\
{												\
	ARG_UNUSED(dev);									\
	static struct pio_rpi_pico_irq_cfg pio_uart_irq_cfg##inst = {				\
		.irq_func = pio_uart_irq,							\
		.irq_param = DEVICE_DT_GET(DT_DRV_INST(inst)),					\
		.irq_idx = DT_INST_PROP_BY_IDX(inst, pio_interrupts, 0),			\
	};											\
	pio_rpi_pico_irq_register(DEVICE_DT_GET(DT_INST_PARENT(inst)), &pio_uart_irq_cfg##inst);\
	pio_rpi_pico_irq_enable(DEVICE_DT_GET(DT_INST_PARENT(inst)), &pio_uart_irq_cfg##inst);	\
}

#define PIO_UART_GEN_IRQ_CONFIG(inst)								\
	.irq_config = pio_uart_irq_config##inst,						\
	.irq_idx = DT_INST_PROP_BY_IDX(inst, pio_interrupts, 0),

#else
#define PIO_UART_IRQ_CONFIG(inst)
#define PIO_UART_GEN_IRQ_CONFIG(inst)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define PIO_UART_INIT(inst) \
	BUILD_ASSERT(DT_INST_ON_BUS(inst, pio_rpi_pico),					\
		"node " DT_NODE_PATH(DT_DRV_INST(inst)) " is not assigned to a PIO instance");	\
												\
	PIO_UART_IRQ_CONFIG(inst)								\
												\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	static struct pio_uart_data pio_uart_data##inst = {					\
		.uart_config = {								\
			.baudrate = DT_INST_PROP_OR(inst, current_speed, 115200),		\
			.parity = DT_INST_ENUM_IDX_OR(inst, parity, UART_CFG_PARITY_NONE),	\
			.stop_bits = DT_INST_ENUM_IDX_OR(inst, stop_bits, UART_CFG_STOP_BITS_1),\
			.data_bits = DT_INST_ENUM_IDX_OR(inst, data_bits, UART_CFG_DATA_BITS_8),\
			.flow_ctrl = 0,								\
		},										\
	};											\
												\
	static const struct pio_uart_config pio_uart_config##inst = {				\
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),					\
		.pio_regs = (pio_hw_t *)DT_INST_PIO_RPI_PICO_REG_ADDR(inst),			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
		.clock_frequency = DT_INST_PIO_RPI_PICO_CLOCK_FREQ_HZ(inst),			\
		.tx_gpio =  DT_INST_PIO_RPI_PICO_PIN_BY_NAME(inst, default, 0, tx_gpio, 0),	\
		.rx_gpio =  DT_INST_PIO_RPI_PICO_PIN_BY_NAME(inst, default, 0, rx_gpio, 0),	\
		PIO_UART_GEN_IRQ_CONFIG(inst)							\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, &pio_uart_init,						\
		NULL,										\
		&pio_uart_data##inst,								\
		&pio_uart_config##inst,								\
		PRE_KERNEL_1,									\
		CONFIG_SERIAL_INIT_PRIORITY,							\
		&pio_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PIO_UART_INIT)
