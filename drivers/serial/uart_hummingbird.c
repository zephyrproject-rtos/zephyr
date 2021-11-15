/*
 * Copyright (c) 2021, TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuclei_hummingbird_uart

/**
 * @brief Nuclei HummingBird UART Driver
 *
 * This is driver for Nuclei HummingBird SoC peripheral.
 */

#include <drivers/uart.h>

#define GPIO_CTRL_ADDR          0x10012000UL
#define IOF0_UART0_MASK         0x00030000UL
#define _REG32(p, i)            (*(volatile uint32_t *) ((p) + (i)))
#define GPIO_REG(offset)        _REG32(GPIO_CTRL_ADDR, offset)

#define GPIO_IOF_EN     (0x38)
#define GPIO_IOF_SEL    (0x3C)

/* TXCTRL register */
#define UART_TXEN               0x1
#define UART_TXWM(x)            (((x) & 0xffff) << 16)

/* RXCTRL register */
#define UART_RXEN               0x1
#define UART_RXWM(x)            (((x) & 0xffff) << 16)

/* IP register */
#define UART_IP_TXWM            0x1
#define UART_IP_RXWM            0x2

#define UART_TXFIFO_FULL        (1 << 31)
#define UART_RXFIFO_EMPTY       (1 << 31)

#define UART_TXCTRL_TXCNT_OFS   (16)
#define UART_TXCTRL_TXCNT_MASK  (0x7 << UART_TXCTRL_TXCNT_OFS)
#define UART_TXCTRL_TXEN_OFS    (0)
#define UART_TXCTRL_TXEN_MASK   (0x1 << UART_TXCTRL_TXEN_OFS)
#define UART_TXCTRL_NSTOP_OFS   (1)
#define UART_TXCTRL_NSTOP_MASK  (0x1 << UART_TXCTRL_TXEN_OFS)

#define UART_RXCTRL_RXCNT_OFS   (16)
#define UART_RXCTRL_RXCNT_MASK  (0x7 << UART_RXCTRL_RXCNT_OFS)
#define UART_RXCTRL_RXEN_OFS    (0)
#define UART_RXCTRL_RXEN_MASK   (0x1 << UART_RXCTRL_RXEN_OFS)

#define UART_IE_TXIE_OFS        (0)
#define UART_IE_TXIE_MASK       (0x1 << UART_IE_TXIE_OFS)
#define UART_IE_RXIE_OFS        (1)
#define UART_IE_RXIE_MASK       (0x1 << UART_IE_RXIE_OFS)

#define UART_IP_TXIP_OFS        (0)
#define UART_IP_TXIP_MASK       (0x1 << UART_IP_TXIP_OFS)
#define UART_IP_RXIP_OFS        (1)
#define UART_IP_RXIP_MASK       (0x1 << UART_IP_RXIP_OFS)

#define CFG2UART(cfg) ((struct UART_TypeDef *)((const struct hbird_uart_config *)dev->config)->reg)

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

/**
 * @brief Nuclei HummingBird UART Register map structure
 */
struct UART_TypeDef {
	volatile uint32_t TXFIFO;
	volatile uint32_t RXFIFO;
	volatile uint32_t TXCTRL;
	volatile uint32_t RXCTRL;
	volatile uint32_t IE;
	volatile uint32_t IP;
	volatile uint32_t DIV;
};

struct hbird_uart_config {
	uint32_t reg;
};

struct hbird_uart_data {
	uint32_t baud_rate;
};


static int uart_hbird_init(const struct device *dev)
{
	struct hbird_uart_data *const data = dev->data;
	struct UART_TypeDef *uart = CFG2UART(cfg);

	GPIO_REG(GPIO_IOF_SEL) &= ~IOF0_UART0_MASK;
	GPIO_REG(GPIO_IOF_EN) |= IOF0_UART0_MASK;

	uart->DIV = CPU_FREQ / data->baud_rate - 1;
	uart->TXCTRL |= UART_TXEN;

	return 0;
}

static int uart_hbird_poll_in(const struct device *dev, unsigned char *c)
{
	struct UART_TypeDef *uart = CFG2UART(cfg);
	uint32_t reg = uart->RXFIFO;

	if (reg & UART_RXFIFO_EMPTY) {
		return -1;
	}

	*c = (reg & 0xFF);
	return 0;
}

static void uart_hbird_poll_out(const struct device *dev, unsigned char c)
{
	struct UART_TypeDef *uart = CFG2UART(cfg);

	while (uart->TXFIFO & UART_TXFIFO_FULL) {
	}
	uart->TXFIFO = c;
}

static int uart_hbird_err_check(const struct device *dev)
{
	return 0;
}

static const struct uart_driver_api uart_hbird_driver_api = {
	.poll_in = uart_hbird_poll_in,
	.poll_out = uart_hbird_poll_out,
	.err_check = uart_hbird_err_check,
};

#define HBIRD_UART_INIT(n)						     \
	static struct hbird_uart_data uart##n##_hbird_data = {	     \
		.baud_rate = DT_INST_PROP(n, current_speed),			     \
	};									     \
	static const struct hbird_uart_config uart##n##_hbird_config = { \
		.reg = DT_INST_REG_ADDR(n),					     \
	};									     \
	DEVICE_DT_INST_DEFINE(n, &uart_hbird_init,			     \
			      NULL,						     \
			      &uart##n##_hbird_data,			     \
			      &uart##n##_hbird_config, PRE_KERNEL_1,	     \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		     \
			      &uart_hbird_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HBIRD_UART_INIT)
