/*
 * Copyright (c) 2021-2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"
#include "clock.h"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/pm/device.h>

/* Driver dts compatibility: telink,b9x_uart */
#define DT_DRV_COMPAT telink_b9x_uart

/* Get UART instance */
#define GET_UART(dev)      ((volatile struct uart_b9x_t *) \
			    ((const struct uart_b9x_config *)dev->config)->uart_addr)

/* UART TX buffer count max value */
#define UART_TX_BUF_CNT    ((uint8_t)8u)

/* UART TX/RX data registers size */
#define UART_DATA_SIZE     ((uint8_t)4u)

/* Parity type */
#define UART_PARITY_NONE   ((uint8_t)0u)
#define UART_PARITY_EVEN   ((uint8_t)1u)
#define UART_PARITY_ODD    ((uint8_t)2u)

/* Stop bits length */
#define UART_STOP_BIT_1    ((uint8_t)0u)
#define UART_STOP_BIT_1P5  BIT(4)
#define UART_STOP_BIT_2    BIT(5)

/* TX RX reset bits */
#define UART_RX_RESET_BIT BIT(6)
#define UART_TX_RESET_BIT BIT(7)

/* b9x UART registers structure */
struct __packed uart_b9x_t {
	uint8_t data_buf[UART_DATA_SIZE];
	uint16_t clk_div;
	uint8_t ctrl0;
	uint8_t ctrl1;
	uint8_t ctrl2;
	uint8_t ctrl3;
	uint8_t rxtimeoutL;
	uint8_t rxtimeoutH;
	uint8_t bufcnt;
	uint8_t status;
	uint8_t txrx_status;
	uint8_t state;
#if CONFIG_SOC_RISCV_TELINK_B92
	uint8_t ctrl4;
#endif
};

/* b9x UART data structure */
struct uart_b9x_data {
	uint8_t tx_byte_index;
	uint8_t rx_byte_index;
	struct uart_config cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

/* b9x UART config structure */
struct uart_b9x_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t uart_addr;
	uint32_t baud_rate;
	void (*pirq_connect)(void);
	bool hw_flow_control;
};

/* ctrl3 register enums */
enum {
	FLD_UART_RX_IRQ_TRIQ_LEV_OFFSET = 0,
	FLD_UART_TX_IRQ_TRIQ_LEV_OFFSET = 4,
};

/* bufcnt register enums */
enum {
	FLD_UART_RX_BUF_CNT_OFFSET      = 0,
	FLD_UART_TX_BUF_CNT_OFFSET      = 4,
};

/* Get tx fifo count */
static inline uint8_t uart_b9x_get_tx_bufcnt(volatile struct uart_b9x_t *uart)
{
	return (uart->bufcnt & FLD_UART_TX_BUF_CNT) >> FLD_UART_TX_BUF_CNT_OFFSET;
}

/* Get rx fifo count */
static inline uint8_t uart_b9x_get_rx_bufcnt(volatile struct uart_b9x_t *uart)
{
	return (uart->bufcnt & FLD_UART_RX_BUF_CNT) >> FLD_UART_RX_BUF_CNT_OFFSET;
}

/* Check for prime */
static uint8_t uart_b9x_is_prime(uint32_t n)
{
	uint32_t i = 5;

	if (n <= 3) {
		return 1;
	} else if ((n % 2 == 0) || (n % 3 == 0)) {
		return 0;
	}

	for (i = 5; i * i < n; i += 6) {
		if ((n % i == 0) || (n % (i + 2)) == 0) {
			return 0;
		}
	}

	return 1;
}

/* Calculate the best bit width */
static void uart_b9x_cal_div_and_bwpc(uint32_t baudrate, uint32_t pclk,
				      uint16_t *divider, uint8_t *bwpc)
{
	uint8_t i = 0, j = 0;
	uint32_t primeInt = 0;
	uint8_t primeDec = 0;
	uint32_t D_intdec[13], D_int[13];
	uint8_t D_dec[13];

	primeInt = pclk / baudrate;
	primeDec = 10 * pclk / baudrate - 10 * primeInt;

	if (uart_b9x_is_prime(primeInt)) {
		primeInt += 1;
	} else if (primeDec > 5) {
		primeInt += 1;
		if (uart_b9x_is_prime(primeInt)) {
			primeInt -= 1;
		}
	}

	for (i = 3; i <= 15; i++) {
		D_intdec[i - 3] = (10 * primeInt) / (i + 1);
		D_dec[i - 3] = D_intdec[i - 3] - 10 * (D_intdec[i - 3] / 10);
		D_int[i - 3] = D_intdec[i - 3] / 10;
	}

	/* find the max and min one decimation point */
	uint8_t position_min = 0, position_max = 0;
	uint32_t min = 0xffffffff, max = 0x00;

	for (j = 0; j < 13; j++) {
		if ((D_dec[j] <= min) && (D_int[j] != 0x01)) {
			min = D_dec[j];
			position_min = j;
		}
		if (D_dec[j] >= max) {
			max = D_dec[j];
			position_max = j;
		}
	}

	if ((D_dec[position_min] < 5) && (D_dec[position_max] >= 5)) {
		if (D_dec[position_min] < (10 - D_dec[position_max])) {
			*bwpc = position_min + 3;
			*divider = D_int[position_min] - 1;
		} else {
			*bwpc = position_max + 3;
			*divider = D_int[position_max];
		}
	} else if ((D_dec[position_min] < 5) && (D_dec[position_max] < 5)) {
		*bwpc = position_min + 3;
		*divider = D_int[position_min] - 1;
	} else {
		*bwpc = position_max + 3;
		*divider = D_int[position_max];
	}
}

/* Initializes the UART instance */
static void uart_b9x_init(volatile struct uart_b9x_t *uart, uint16_t divider,
			  uint8_t bwpc, uint8_t parity, uint8_t stop_bit)
{
	uart->ctrl0 = bwpc;

	/* config clock */
	divider = divider | FLD_UART_CLK_DIV_EN;
#if CONFIG_SOC_RISCV_TELINK_B92
	uart->ctrl0 &= ~(FLD_UART_RX_CLR_EN | FLD_UART_NDMA_RXDONE_EN |
		FLD_UART_RXTIMEOUT_RTS_EN | FLD_UART_S7816_EN);
	uart->ctrl4 &= ~FLD_UART_RXDONE_RTS_EN;
#endif
	uart->clk_div = divider;

	/* config parity */
	if (parity) {
		/* enable parity function */
		uart->ctrl1 |= FLD_UART_PARITY_ENABLE;

		if (parity == UART_PARITY_EVEN) {
			/* enable even parity */
			uart->ctrl1 &= (~FLD_UART_PARITY_POLARITY);
		} else if (parity == UART_PARITY_ODD) {
			/* enable odd parity */
			uart->ctrl1 |= FLD_UART_PARITY_POLARITY;
		}
	} else {
		uart->ctrl1 &= (~FLD_UART_PARITY_ENABLE); /* disable parity function */
	}

	/* stop bit config */
	uart->ctrl1 &= (~FLD_UART_STOP_SEL);
	uart->ctrl1 |= stop_bit;
}

/* API implementation: irq handler */
static void uart_b9x_irq_handler(const struct device *dev)
{
#ifndef CONFIG_UART_INTERRUPT_DRIVEN
	ARG_UNUSED(dev);
#else
	struct uart_b9x_data *data = dev->data;

	if (data->callback != NULL) {
		data->callback(dev, data->cb_data);
	}
#endif
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
/* API implementation: configure */
static int uart_b9x_configure(const struct device *dev,
			      const struct uart_config *cfg)
{
	struct uart_b9x_data *data = dev->data;
	uint16_t divider;
	uint8_t bwpc;
	uint8_t parity;
	uint8_t stop_bits;

	volatile struct uart_b9x_t *uart = GET_UART(dev);

	/* check parity */
	if (cfg->parity == UART_CFG_PARITY_NONE) {
		parity = UART_PARITY_NONE;
	} else if (cfg->parity == UART_CFG_PARITY_ODD) {
		parity = UART_PARITY_ODD;
	} else if (cfg->parity == UART_CFG_PARITY_EVEN) {
		parity = UART_PARITY_EVEN;
	} else {
		return -ENOTSUP;
	}

	/* check stop bits */
	if (cfg->stop_bits == UART_CFG_STOP_BITS_1) {
		stop_bits = UART_STOP_BIT_1;
	} else if (cfg->stop_bits == UART_CFG_STOP_BITS_1_5) {
		stop_bits = UART_STOP_BIT_1P5;
	} else if (cfg->stop_bits == UART_CFG_STOP_BITS_2) {
		stop_bits = UART_STOP_BIT_2;
	} else {
		return -ENOTSUP;
	}

	/* check flow control */
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE &&
		cfg->flow_ctrl != UART_CFG_FLOW_CTRL_RTS_CTS) {
		return -ENOTSUP;
	}

	/* UART configure */
	uart_b9x_cal_div_and_bwpc(cfg->baudrate, sys_clk.pclk * 1000 * 1000, &divider, &bwpc);
	uart_b9x_init(uart, divider, bwpc, parity, stop_bits);

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		uart->ctrl1 |= FLD_UART_TX_CTS_ENABLE | FLD_UART_TX_CTS_POLARITY;
		uart->ctrl2 |= FLD_UART_RTS_EN | FLD_UART_RTS_POLARITY;
		uart->ctrl2 &= (~(FLD_UART_RTS_MANUAL_M | FLD_UART_RTS_TRIQ_LEV));
		uart->ctrl2 |= ARRAY_SIZE(uart->data_buf) - 1;
	} else {
		uart->ctrl1 &= (~FLD_UART_TX_CTS_ENABLE);
		uart->ctrl2 &= (~FLD_UART_RTS_EN);
	}

	/* save configuration */
	data->cfg = *cfg;

	return 0;
}

/* API implementation: config_get */
static int uart_b9x_config_get(const struct device *dev,
			       struct uart_config *cfg)
{
	struct uart_b9x_data *data = dev->data;

	*cfg = data->cfg;

	return 0;
}
#endif

/* API implementation: driver initialization */
static int uart_b9x_driver_init(const struct device *dev)
{
	int status = 0;
	uint16_t divider = 0u;
	uint8_t bwpc = 0u;
	volatile struct uart_b9x_t *uart = GET_UART(dev);
	const struct uart_b9x_config *cfg = dev->config;
	struct uart_b9x_data *data = dev->data;

	/* configure pins */
	status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		return status;
	}

	/* Reset Tx, Rx status before usage */
#if CONFIG_SOC_RISCV_TELINK_B91
	uart->status |= UART_RX_RESET_BIT | UART_TX_RESET_BIT;
#elif CONFIG_SOC_RISCV_TELINK_B92
	uart->txrx_status |= FLD_UART_RX_BUF_IRQ | FLD_UART_TX_BUF_IRQ;
#endif
	data->rx_byte_index = 0;
	data->tx_byte_index = 0;

	uart_b9x_cal_div_and_bwpc(cfg->baud_rate, sys_clk.pclk * 1000 * 1000, &divider, &bwpc);
	uart_b9x_init(uart, divider, bwpc, UART_PARITY_NONE, UART_STOP_BIT_1);

	data->cfg.baudrate = cfg->baud_rate;
	data->cfg.parity = UART_CFG_PARITY_NONE;
	data->cfg.stop_bits = UART_CFG_STOP_BITS_1;
	data->cfg.data_bits = UART_CFG_DATA_BITS_8;

	if (cfg->hw_flow_control) {
		uart->ctrl1 |= FLD_UART_TX_CTS_ENABLE | FLD_UART_TX_CTS_POLARITY;
		uart->ctrl2 |= FLD_UART_RTS_EN | FLD_UART_RTS_POLARITY;
		uart->ctrl2 &= (~(FLD_UART_RTS_MANUAL_M | FLD_UART_RTS_TRIQ_LEV));
		uart->ctrl2 |= ARRAY_SIZE(uart->data_buf) - 1;
		data->cfg.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	} else {
		uart->ctrl1 &= (~FLD_UART_TX_CTS_ENABLE);
		uart->ctrl2 &= (~FLD_UART_RTS_EN);
		data->cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->pirq_connect();
#endif

	return 0;
}

/* API implementation: poll_out */
static void uart_b9x_poll_out(const struct device *dev, uint8_t c)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);
	struct uart_b9x_data *data = dev->data;

	while (uart_b9x_get_tx_bufcnt(uart) >= UART_TX_BUF_CNT) {
	}

	uart->data_buf[data->tx_byte_index] = c;
	data->tx_byte_index = (data->tx_byte_index + 1) % ARRAY_SIZE(uart->data_buf);

#if CONFIG_SOC_RISCV_TELINK_B91
	while (!(uart->txrx_status & FLD_UART_TX_DONE)) {
	}
#elif CONFIG_SOC_RISCV_TELINK_B92
	while (!(uart->txrx_status & FLD_UART_TXDONE_IRQ)) {
	}
#endif
}

/* API implementation: poll_in */
static int uart_b9x_poll_in(const struct device *dev, unsigned char *c)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);
	struct uart_b9x_data *data = dev->data;

	if (uart_b9x_get_rx_bufcnt(uart) == 0) {
		return -1;
	}

	*c = uart->data_buf[data->rx_byte_index];
	data->rx_byte_index = (data->rx_byte_index + 1) % ARRAY_SIZE(uart->data_buf);

	return 0;
}

/* API implementation: err_check */
static int uart_b9x_err_check(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

#if CONFIG_SOC_RISCV_TELINK_B91
	return ((uart->status & FLD_UART_RX_ERR) != 0) ? 1 : 0;
#elif CONFIG_SOC_RISCV_TELINK_B92
	return ((uart->txrx_status & FLD_UART_RX_ERR_IRQ) != 0) ? 1 : 0;
#endif
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* API implementation: fifo_fill */
static int uart_b9x_fifo_fill(const struct device *dev,
			      const uint8_t *tx_data,
			      int size)
{
	int i = 0;
	volatile struct uart_b9x_t *uart = GET_UART(dev);
	struct uart_b9x_data *data = dev->data;

	if (size > UART_DATA_SIZE) {
		size = UART_DATA_SIZE;
	}

	for (i = 0; i < size; i++) {
		if (uart_b9x_get_rx_bufcnt(uart) != 0) {
			break;
		}

		while (uart_b9x_get_tx_bufcnt(uart) >= UART_TX_BUF_CNT) {
		}

		uart->data_buf[data->tx_byte_index] = tx_data[i];
		data->tx_byte_index = (data->tx_byte_index + 1) % ARRAY_SIZE(uart->data_buf);
	}

	return i;
}

/* API implementation: fifo_read */
static int uart_b9x_fifo_read(const struct device *dev,
			      uint8_t *rx_data,
			      const int size)
{
	int rx_count;
	volatile struct uart_b9x_t *uart = GET_UART(dev);
	struct uart_b9x_data *data = dev->data;

	for (rx_count = 0; rx_count < size; rx_count++) {
		if (uart_b9x_get_rx_bufcnt(uart) == 0) {
			break;
		}

		rx_data[rx_count] = uart->data_buf[data->rx_byte_index];
		data->rx_byte_index = (data->rx_byte_index + 1) % ARRAY_SIZE(uart->data_buf);
	}

	return rx_count;
}

/* API implementation: irq_tx_enable */
static void uart_b9x_irq_tx_enable(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

	uart->ctrl3 = (uart->ctrl3 & (~FLD_UART_TX_IRQ_TRIQ_LEV)) |
		      BIT(FLD_UART_TX_IRQ_TRIQ_LEV_OFFSET);

#if CONFIG_SOC_RISCV_TELINK_B91
	uart->ctrl0 |= FLD_UART_MASK_TX_IRQ;
#elif CONFIG_SOC_RISCV_TELINK_B92
	uart->rxtimeoutH |= FLD_UART_MASK_TX_IRQ;
#endif
}

/* API implementation: irq_tx_disable */
static void uart_b9x_irq_tx_disable(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

#if CONFIG_SOC_RISCV_TELINK_B91
	uart->ctrl0 &= ~FLD_UART_MASK_TX_IRQ;
#elif CONFIG_SOC_RISCV_TELINK_B92
	uart->rxtimeoutH &= ~FLD_UART_MASK_TX_IRQ;
#endif
}

/* API implementation: irq_tx_ready */
static int uart_b9x_irq_tx_ready(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

#if CONFIG_SOC_RISCV_TELINK_B91
	return ((uart_b9x_get_tx_bufcnt(uart) < UART_TX_BUF_CNT) &&
		((uart->ctrl0 & FLD_UART_MASK_TX_IRQ) != 0)) ? 1 : 0;
#elif CONFIG_SOC_RISCV_TELINK_B92
	return ((uart_b9x_get_tx_bufcnt(uart) < UART_TX_BUF_CNT) &&
		((uart->rxtimeoutH & FLD_UART_MASK_TX_IRQ) != 0)) ? 1 : 0;
#endif
}

/* API implementation: irq_tx_complete */
static int uart_b9x_irq_tx_complete(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

	return (uart_b9x_get_tx_bufcnt(uart) == 0) ? 1 : 0;
}

/* API implementation: irq_rx_enable */
static void uart_b9x_irq_rx_enable(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

	uart->ctrl3 = (uart->ctrl3 & (~FLD_UART_RX_IRQ_TRIQ_LEV)) |
		      BIT(FLD_UART_RX_IRQ_TRIQ_LEV_OFFSET);

#if CONFIG_SOC_RISCV_TELINK_B91
	uart->ctrl0 |= FLD_UART_MASK_RX_IRQ;
#elif CONFIG_SOC_RISCV_TELINK_B92
	uart->rxtimeoutH |= FLD_UART_MASK_RX_IRQ;
#endif
}

/* API implementation: irq_rx_disable */
static void uart_b9x_irq_rx_disable(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

#if CONFIG_SOC_RISCV_TELINK_B91
	uart->ctrl0 &= ~FLD_UART_MASK_RX_IRQ;
#elif CONFIG_SOC_RISCV_TELINK_B92
	uart->rxtimeoutH &= ~FLD_UART_MASK_RX_IRQ;
#endif
}

/* API implementation: irq_rx_ready */
static int uart_b9x_irq_rx_ready(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

	return (uart_b9x_get_rx_bufcnt(uart) > 0) ? 1 : 0;
}

/* API implementation: irq_err_enable */
static void uart_b9x_irq_err_enable(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

	uart->rxtimeoutH |= FLD_UART_MASK_ERR_IRQ;
}

/* API implementation: irq_err_disable*/
static void uart_b9x_irq_err_disable(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

	uart->rxtimeoutH &= ~FLD_UART_MASK_ERR_IRQ;
}

/* API implementation: irq_is_pending */
static int uart_b9x_irq_is_pending(const struct device *dev)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);

	return ((uart->status & FLD_UART_IRQ_O) != 0) ? 1 : 0;
}

/* API implementation: irq_update */
static int uart_b9x_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* nothing to be done */
	return 1;
}

/* API implementation: irq_callback_set */
static void uart_b9x_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_b9x_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_DRV_CMD
#include <gpio.h>

int uart_b9x_drv_cmd(const struct device *dev, uint32_t cmd, uint32_t p)
{
	int result = -ENOTSUP;

	if (cmd == 0) {
		/* finish transmission */
		volatile struct uart_b9x_t *uart = GET_UART(dev);

		k_sched_lock();
		while (uart_b9x_get_tx_bufcnt(uart)) {
		}
#if CONFIG_SOC_RISCV_TELINK_B91
		while (!(uart->txrx_status & FLD_UART_TX_DONE)) {
		}
#elif CONFIG_SOC_RISCV_TELINK_B92
		while (!(uart->txrx_status & FLD_UART_TXDONE_IRQ)) {
		}
#endif
		/* CMD 0: Get logical zero level UART pins count */
		ARG_UNUSED(p);
		const struct uart_b9x_config *cfg = dev->config;
		const struct pinctrl_dev_config *pcfg = cfg->pcfg;

		result = 0;
		for (uint8_t i = 0; i < pcfg->state_cnt; i++) {
			const struct pinctrl_state *state = &pcfg->states[i];

			if (state->id == PINCTRL_STATE_DEFAULT) {
				for (uint8_t j = 0; j < state->pin_cnt; j++) {
					const pinctrl_soc_pin_t *pin = &state->pins[j];
					bool level = gpio_get_level(B9x_PINMUX_GET_PIN(*pin));

					if (!level) {
						result++;
					}
				}
				break;
			}
		}
		k_sched_unlock();
	}

	return result;
}
#endif /* CONFIG_UART_DRV_CMD */

#ifdef CONFIG_PM_DEVICE

static int uart_b9x_pm_action(const struct device *dev, enum pm_device_action action)
{
	volatile struct uart_b9x_t *uart = GET_UART(dev);
	struct uart_b9x_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#if CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION
		{
			extern volatile bool b9x_deep_sleep_retention;

			if (b9x_deep_sleep_retention) {
				uart_b9x_driver_init(dev);
			}
		}
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION */
		/* reset TX/RX byte index */
		data->tx_byte_index = 0;
		data->rx_byte_index = 0;
#if CONFIG_SOC_RISCV_TELINK_B91
		uart->status |= UART_RX_RESET_BIT | UART_TX_RESET_BIT;
#elif CONFIG_SOC_RISCV_TELINK_B92
		uart->txrx_status |= FLD_UART_RX_BUF_IRQ | FLD_UART_TX_BUF_IRQ;
#endif
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

#endif /* CONFIG_PM_DEVICE */

static const struct uart_driver_api uart_b9x_driver_api = {
	.poll_in = uart_b9x_poll_in,
	.poll_out = uart_b9x_poll_out,
	.err_check = uart_b9x_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_b9x_configure,
	.config_get = uart_b9x_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_b9x_fifo_fill,
	.fifo_read = uart_b9x_fifo_read,
	.irq_tx_enable = uart_b9x_irq_tx_enable,
	.irq_tx_disable = uart_b9x_irq_tx_disable,
	.irq_tx_ready = uart_b9x_irq_tx_ready,
	.irq_tx_complete = uart_b9x_irq_tx_complete,
	.irq_rx_enable = uart_b9x_irq_rx_enable,
	.irq_rx_disable = uart_b9x_irq_rx_disable,
	.irq_rx_ready = uart_b9x_irq_rx_ready,
	.irq_err_enable = uart_b9x_irq_err_enable,
	.irq_err_disable = uart_b9x_irq_err_disable,
	.irq_is_pending = uart_b9x_irq_is_pending,
	.irq_update = uart_b9x_irq_update,
	.irq_callback_set = uart_b9x_irq_callback_set,
#endif
#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd = uart_b9x_drv_cmd,
#endif
};


#define UART_B9X_INIT(n)							    \
										    \
	PM_DEVICE_DT_INST_DEFINE(n, uart_b9x_pm_action);			    \
										    \
	static void uart_b9x_irq_connect_##n(void);				    \
										    \
	PINCTRL_DT_INST_DEFINE(n);						    \
										    \
	static const struct uart_b9x_config uart_b9x_cfg_##n =			    \
	{									    \
		.uart_addr = DT_INST_REG_ADDR(n),				    \
		.baud_rate = DT_INST_PROP(n, current_speed),			    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			    \
		.pirq_connect = uart_b9x_irq_connect_##n,			    \
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control)		    \
	};									    \
										    \
	static struct uart_b9x_data uart_b9x_data_##n;				    \
										    \
	DEVICE_DT_INST_DEFINE(n, uart_b9x_driver_init,				    \
			      PM_DEVICE_DT_INST_GET(n),				    \
			      &uart_b9x_data_##n,				    \
			      &uart_b9x_cfg_##n,				    \
			      PRE_KERNEL_1,					    \
			      CONFIG_SERIAL_INIT_PRIORITY,			    \
			      (void *)&uart_b9x_driver_api);			    \
										    \
	static void uart_b9x_irq_connect_##n(void)				    \
	{									    \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		    \
			    uart_b9x_irq_handler,				    \
			    DEVICE_DT_INST_GET(n), 0);				    \
										    \
		riscv_plic_irq_enable(DT_INST_IRQN(n));	\
		riscv_plic_set_priority(DT_INST_IRQN(n),\
		DT_INST_IRQ(n, priority)); \
	}

DT_INST_FOREACH_STATUS_OKAY(UART_B9X_INIT)
