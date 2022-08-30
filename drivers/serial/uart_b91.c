/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"
#include "clock.h"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>


/* Driver dts compatibility: telink,b91_uart */
#define DT_DRV_COMPAT telink_b91_uart

/* Get UART instance */
#define GET_UART(dev)      ((volatile struct uart_b91_t *) \
			    ((const struct uart_b91_config *)dev->config)->uart_addr)

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

/* B91 UART registers structure */
struct uart_b91_t {
	uint8_t data_buf[UART_DATA_SIZE];
	uint16_t clk_div;
	uint8_t ctrl0;
	uint8_t ctrl1;
	uint8_t ctrl2;
	uint8_t ctrl3;
	uint16_t rxtimeout;
	uint8_t bufcnt;
	uint8_t status;
	uint8_t txrx_status;
	uint8_t state;
};

/* B91 UART data structure */
struct uart_b91_data {
	uint8_t tx_byte_index;
	uint8_t rx_byte_index;
	struct uart_config cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

/* B91 UART config structure */
struct uart_b91_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t uart_addr;
	uint32_t baud_rate;
	void (*pirq_connect)(void);
};

/* rxtimeout register enums */
enum {
	UART_ERR_IRQ_MASK = BIT(15),
};

/* ctrl0 register enums */
enum {
	UART_RX_IRQ_MASK        = BIT(6),
	UART_TX_IRQ_MASK        = BIT(7),
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

/* status register enums */
enum {
	UART_IRQ_STATUS         = BIT(3),
	UART_RX_ERR_STATUS      = BIT(7),
};


/* Get tx fifo count */
static inline uint8_t uart_b91_get_tx_bufcnt(volatile struct uart_b91_t *uart)
{
	return (uart->bufcnt & FLD_UART_TX_BUF_CNT) >> FLD_UART_TX_BUF_CNT_OFFSET;
}

/* Get rx fifo count */
static inline uint8_t uart_b91_get_rx_bufcnt(volatile struct uart_b91_t *uart)
{
	return (uart->bufcnt & FLD_UART_RX_BUF_CNT) >> FLD_UART_RX_BUF_CNT_OFFSET;
}

/* Check for prime */
static uint8_t uart_b91_is_prime(uint32_t n)
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
static void uart_b91_cal_div_and_bwpc(uint32_t baudrate, uint32_t pclk,
				      uint16_t *divider, uint8_t *bwpc)
{
	uint8_t i = 0, j = 0;
	uint32_t primeInt = 0;
	uint8_t primeDec = 0;
	uint32_t D_intdec[13], D_int[13];
	uint8_t D_dec[13];

	primeInt = pclk / baudrate;
	primeDec = 10 * pclk / baudrate - 10 * primeInt;

	if (uart_b91_is_prime(primeInt)) {
		primeInt += 1;
	} else if (primeDec > 5) {
		primeInt += 1;
		if (uart_b91_is_prime(primeInt)) {
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
static void uart_b91_init(volatile struct uart_b91_t *uart, uint16_t divider,
			  uint8_t bwpc, uint8_t parity, uint8_t stop_bit)
{
	/* config clock */
	divider = divider | FLD_UART_CLK_DIV_EN;
	uart->ctrl0 = bwpc;
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
static void uart_b91_irq_handler(const struct device *dev)
{
#ifndef CONFIG_UART_INTERRUPT_DRIVEN
	ARG_UNUSED(dev);
#else
	struct uart_b91_data *data = dev->data;

	if (data->callback != NULL) {
		data->callback(dev, data->cb_data);
	}
#endif
}

/* API implementation: configure */
static int uart_b91_configure(const struct device *dev,
			      const struct uart_config *cfg)
{
	struct uart_b91_data *data = dev->data;
	uint16_t divider;
	uint8_t bwpc;
	uint8_t parity;
	uint8_t stop_bits;

	volatile struct uart_b91_t *uart = GET_UART(dev);

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
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	/* UART configure */
	uart_b91_cal_div_and_bwpc(cfg->baudrate, sys_clk.pclk * 1000 * 1000, &divider, &bwpc);
	uart_b91_init(uart, divider, bwpc, parity, stop_bits);

	/* save configuration */
	data->cfg = *cfg;

	return 0;
}

/* API implementation: config_get */
static int uart_b91_config_get(const struct device *dev,
			       struct uart_config *cfg)
{
	struct uart_b91_data *data = dev->data;

	*cfg = data->cfg;

	return 0;
}

/* API implementation: driver initialization */
static int uart_b91_driver_init(const struct device *dev)
{
	int status = 0;
	uint16_t divider = 0u;
	uint8_t bwpc = 0u;
	volatile struct uart_b91_t *uart = GET_UART(dev);
	const struct uart_b91_config *cfg = dev->config;
	struct uart_b91_data *data = dev->data;

	/* Reset Tx, Rx status before usage */
	uart->status |= UART_RX_RESET_BIT | UART_TX_RESET_BIT;
	data->rx_byte_index = 0;
	data->tx_byte_index = 0;

	/* configure pins */
	status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		return status;
	}

	uart_b91_cal_div_and_bwpc(cfg->baud_rate, sys_clk.pclk * 1000 * 1000, &divider, &bwpc);
	uart_b91_init(uart, divider, bwpc, UART_PARITY_NONE, UART_STOP_BIT_1);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->pirq_connect();
#endif

	return 0;
}

/* API implementation: poll_out */
static void uart_b91_poll_out(const struct device *dev, uint8_t c)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);
	struct uart_b91_data *data = dev->data;

	while (uart_b91_get_tx_bufcnt(uart) >= UART_TX_BUF_CNT) {
	};

	uart->data_buf[data->tx_byte_index] = c;
	data->tx_byte_index = (data->tx_byte_index + 1) % ARRAY_SIZE(uart->data_buf);
}

/* API implementation: poll_in */
static int uart_b91_poll_in(const struct device *dev, unsigned char *c)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);
	struct uart_b91_data *data = dev->data;

	if (uart_b91_get_rx_bufcnt(uart) == 0) {
		return -1;
	}

	*c = uart->data_buf[data->rx_byte_index];
	data->rx_byte_index = (data->rx_byte_index + 1) % ARRAY_SIZE(uart->data_buf);

	return 0;
}

/* API implementation: err_check */
static int uart_b91_err_check(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	return ((uart->status & UART_RX_ERR_STATUS) != 0) ? 1 : 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* API implementation: fifo_fill */
static int uart_b91_fifo_fill(const struct device *dev,
			      const uint8_t *tx_data,
			      int size)
{
	int i = 0;
	volatile struct uart_b91_t *uart = GET_UART(dev);

	if (size > UART_DATA_SIZE) {
		size = UART_DATA_SIZE;
	}

	for (i = 0; i < size; i++) {
		if (uart_b91_get_rx_bufcnt(uart) != 0) {
			break;
		}

		uart_b91_poll_out(dev, tx_data[i]);
	}

	return i;
}

/* API implementation: fifo_read */
static int uart_b91_fifo_read(const struct device *dev,
			      uint8_t *rx_data,
			      const int size)
{
	int rx_count;
	volatile struct uart_b91_t *uart = GET_UART(dev);

	for (rx_count = 0; rx_count < size; rx_count++) {
		if (uart_b91_get_rx_bufcnt(uart) == 0) {
			break;
		}

		uart_b91_poll_in(dev, &rx_data[rx_count]);
	}

	return rx_count;
}

/* API implementation: irq_tx_enable */
static void uart_b91_irq_tx_enable(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	uart->ctrl3 = (uart->ctrl3 & (~FLD_UART_TX_IRQ_TRIQ_LEV)) |
		      BIT(FLD_UART_TX_IRQ_TRIQ_LEV_OFFSET);
	uart->ctrl0 |= UART_TX_IRQ_MASK;
}

/* API implementation: irq_tx_disable */
static void uart_b91_irq_tx_disable(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	uart->ctrl0 &= ~UART_TX_IRQ_MASK;
}

/* API implementation: irq_tx_ready */
static int uart_b91_irq_tx_ready(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	return ((uart_b91_get_tx_bufcnt(uart) < UART_TX_BUF_CNT) &&
		((uart->ctrl0 & UART_TX_IRQ_MASK) != 0)) ? 1 : 0;
}

/* API implementation: irq_tx_complete */
static int uart_b91_irq_tx_complete(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	return (uart_b91_get_tx_bufcnt(uart) == 0) ? 1 : 0;
}

/* API implementation: irq_rx_enable */
static void uart_b91_irq_rx_enable(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	uart->ctrl3 = (uart->ctrl3 & (~FLD_UART_RX_IRQ_TRIQ_LEV)) |
		      BIT(FLD_UART_RX_IRQ_TRIQ_LEV_OFFSET);
	uart->ctrl0 |= UART_RX_IRQ_MASK;
}

/* API implementation: irq_rx_disable */
static void uart_b91_irq_rx_disable(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	uart->ctrl0 &= ~UART_RX_IRQ_MASK;
}

/* API implementation: irq_rx_ready */
static int uart_b91_irq_rx_ready(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	return (uart_b91_get_rx_bufcnt(uart) > 0) ? 1 : 0;
}

/* API implementation: irq_err_enable */
static void uart_b91_irq_err_enable(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	uart->rxtimeout |= UART_ERR_IRQ_MASK;
}

/* API implementation: irq_err_disable*/
static void uart_b91_irq_err_disable(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	uart->rxtimeout &= ~UART_ERR_IRQ_MASK;
}

/* API implementation: irq_is_pending */
static int uart_b91_irq_is_pending(const struct device *dev)
{
	volatile struct uart_b91_t *uart = GET_UART(dev);

	return ((uart->status & UART_IRQ_STATUS) != 0) ? 1 : 0;
}

/* API implementation: irq_update */
static int uart_b91_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* nothing to be done */
	return 1;
}

/* API implementation: irq_callback_set */
static void uart_b91_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_b91_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_b91_driver_api = {
	.poll_in = uart_b91_poll_in,
	.poll_out = uart_b91_poll_out,
	.err_check = uart_b91_err_check,
	.configure = uart_b91_configure,
	.config_get = uart_b91_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_b91_fifo_fill,
	.fifo_read = uart_b91_fifo_read,
	.irq_tx_enable = uart_b91_irq_tx_enable,
	.irq_tx_disable = uart_b91_irq_tx_disable,
	.irq_tx_ready = uart_b91_irq_tx_ready,
	.irq_tx_complete = uart_b91_irq_tx_complete,
	.irq_rx_enable = uart_b91_irq_rx_enable,
	.irq_rx_disable = uart_b91_irq_rx_disable,
	.irq_rx_ready = uart_b91_irq_rx_ready,
	.irq_err_enable = uart_b91_irq_err_enable,
	.irq_err_disable = uart_b91_irq_err_disable,
	.irq_is_pending = uart_b91_irq_is_pending,
	.irq_update = uart_b91_irq_update,
	.irq_callback_set = uart_b91_irq_callback_set,
#endif
};


#define UART_B91_INIT(n)							    \
										    \
	static void uart_b91_irq_connect_##n(void);				    \
										    \
	PINCTRL_DT_INST_DEFINE(n);						    \
										    \
	static const struct uart_b91_config uart_b91_cfg_##n =			    \
	{									    \
		.uart_addr = DT_INST_REG_ADDR(n),				    \
		.baud_rate = DT_INST_PROP(n, current_speed),			    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			    \
		.pirq_connect = uart_b91_irq_connect_##n			    \
	};									    \
										    \
	static struct uart_b91_data uart_b91_data_##n;				    \
										    \
	DEVICE_DT_INST_DEFINE(n, uart_b91_driver_init,				    \
			      NULL,						    \
			      &uart_b91_data_##n,				    \
			      &uart_b91_cfg_##n,				    \
			      PRE_KERNEL_1,					    \
			      CONFIG_SERIAL_INIT_PRIORITY,			    \
			      (void *)&uart_b91_driver_api);			    \
										    \
	static void uart_b91_irq_connect_##n(void)				    \
	{									    \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		    \
			    uart_b91_irq_handler,				    \
			    DEVICE_DT_INST_GET(n), 0);				    \
										    \
		riscv_plic_irq_enable(DT_INST_IRQN(n));				    \
		riscv_plic_set_priority(DT_INST_IRQN(n), DT_INST_IRQ(n, priority)); \
	}

DT_INST_FOREACH_STATUS_OKAY(UART_B91_INIT)
