/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_pl011
#define SBSA_COMPAT arm_sbsa_uart

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/device_mmio.h>

#ifdef CONFIG_CPU_CORTEX_M
#include <cmsis_compiler.h>
#endif

/*
 * UART PL011 register map structure
 */
struct pl011_regs {
	uint32_t dr;			/* data register */
	union {
		uint32_t rsr;
		uint32_t ecr;
	};
	uint32_t reserved_0[4];
	uint32_t fr;			/* flags register */
	uint32_t reserved_1;
	uint32_t ilpr;
	uint32_t ibrd;
	uint32_t fbrd;
	uint32_t lcr_h;
	uint32_t cr;
	uint32_t ifls;
	uint32_t imsc;
	uint32_t ris;
	uint32_t mis;
	uint32_t icr;
	uint32_t dmacr;
};

struct pl011_config {
	DEVICE_MMIO_ROM;
	uint32_t sys_clk_freq;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
};

/* Device data structure */
struct pl011_data {
	DEVICE_MMIO_RAM;
	uint32_t baud_rate;	/* Baud rate */
	bool sbsa;		/* SBSA mode */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
};

#define PL011_BIT_MASK(x, y) (((2 << x) - 1) << y)

/* PL011 Uart Flags Register */
#define PL011_FR_CTS		BIT(0)	/* clear to send - inverted */
#define PL011_FR_DSR		BIT(1)	/* data set ready - inverted */
#define PL011_FR_DCD		BIT(2)	/* data carrier detect - inverted */
#define PL011_FR_BUSY		BIT(3)	/* busy transmitting data */
#define PL011_FR_RXFE		BIT(4)	/* receive FIFO empty */
#define PL011_FR_TXFF		BIT(5)	/* transmit FIFO full */
#define PL011_FR_RXFF		BIT(6)	/* receive FIFO full */
#define PL011_FR_TXFE		BIT(7)	/* transmit FIFO empty */
#define PL011_FR_RI		BIT(8)	/* ring indicator - inverted */

/* PL011 Integer baud rate register */
#define PL011_IBRD_BAUD_DIVINT_MASK	0xff	/* 16 bits of divider */

/* PL011 Fractional baud rate register */
#define PL011_FBRD_BAUD_DIVFRAC		0x3f
#define PL011_FBRD_WIDTH		6u

/* PL011 Receive status register / error clear register */
#define PL011_RSR_ECR_FE	BIT(0)	/* framing error */
#define PL011_RSR_ECR_PE	BIT(1)	/* parity error */
#define PL011_RSR_ECR_BE	BIT(2)	/* break error */
#define PL011_RSR_ECR_OE	BIT(3)	/* overrun error */

#define PL011_RSR_ERROR_MASK	(PL011_RSR_ECR_FE | PL011_RSR_ECR_PE | \
		PL011_RSR_ECR_BE | PL011_RSR_ECR_OE)

/* PL011 Line Control Register  */
#define PL011_LCRH_BRK		BIT(0)	/* send break */
#define PL011_LCRH_PEN		BIT(1)	/* enable parity */
#define PL011_LCRH_EPS		BIT(2)	/* select even parity */
#define PL011_LCRH_STP2		BIT(3)	/* select two stop bits */
#define PL011_LCRH_FEN		BIT(4)	/* enable FIFOs */
#define PL011_LCRH_WLEN_SHIFT	5	/* word length */
#define PL011_LCRH_WLEN_WIDTH	2
#define PL011_LCRH_SPS		BIT(7)	/* stick parity bit */

#define PL011_LCRH_WLEN_SIZE(x) (x - 5)

#define PL011_LCRH_FORMAT_MASK	(PL011_LCRH_PEN | PL011_LCRH_EPS | \
		PL011_LCRH_SPS | \
		PL011_BIT_MASK(PL011_LCRH_WLEN_WIDTH, PL011_LCRH_WLEN_SHIFT))

#define PL011_LCRH_PARTIY_EVEN	(PL011_LCRH_PEN | PL011_LCRH_EPS)
#define PL011_LCRH_PARITY_ODD	(PL011_LCRH_PEN)
#define PL011_LCRH_PARITY_NONE	(0)

/* PL011 Control Register */
#define PL011_CR_UARTEN		BIT(0)	/* enable uart operations */
#define PL011_CR_SIREN		BIT(1)	/* enable IrDA SIR */
#define PL011_CR_SIRLP		BIT(2)	/* IrDA SIR low power mode */
#define PL011_CR_LBE		BIT(7)	/* loop back enable */
#define PL011_CR_TXE		BIT(8)	/* transmit enable */
#define PL011_CR_RXE		BIT(9)	/* receive enable */
#define PL011_CR_DTR		BIT(10)	/* data transmit ready */
#define PL011_CR_RTS		BIT(11)	/* request to send */
#define PL011_CR_Out1		BIT(12)
#define PL011_CR_Out2		BIT(13)
#define PL011_CR_RTSEn		BIT(14)	/* RTS hw flow control enable */
#define PL011_CR_CTSEn		BIT(15)	/* CTS hw flow control enable */

/* PL011 Interrupt Fifo Level Select Register */
#define PL011_IFLS_TXIFLSEL_SHIFT	0	/* bits 2:0 */
#define PL011_IFLS_TXIFLSEL_WIDTH	3
#define PL011_IFLS_RXIFLSEL_SHIFT	3	/* bits 5:3 */
#define PL011_IFLS_RXIFLSEL_WIDTH	3

/* PL011 Interrupt Mask Set/Clear Register */
#define PL011_IMSC_RIMIM	BIT(0)	/* RTR modem interrupt mask */
#define PL011_IMSC_CTSMIM	BIT(1)	/* CTS modem interrupt mask */
#define PL011_IMSC_DCDMIM	BIT(2)	/* DCD modem interrupt mask */
#define PL011_IMSC_DSRMIM	BIT(3)	/* DSR modem interrupt mask */
#define PL011_IMSC_RXIM		BIT(4)	/* receive interrupt mask */
#define PL011_IMSC_TXIM		BIT(5)	/* transmit interrupt mask */
#define PL011_IMSC_RTIM		BIT(6)	/* receive timeout interrupt mask */
#define PL011_IMSC_FEIM		BIT(7)	/* framing error interrupt mask */
#define PL011_IMSC_PEIM		BIT(8)	/* parity error interrupt mask */
#define PL011_IMSC_BEIM		BIT(9)	/* break error interrupt mask */
#define PL011_IMSC_OEIM		BIT(10)	/* overrun error interrupt mask */

#define PL011_IMSC_ERROR_MASK	(PL011_IMSC_FEIM | \
		PL011_IMSC_PEIM | PL011_IMSC_BEIM | \
		PL011_IMSC_OEIM)

#define PL011_IMSC_MASK_ALL (PL011_IMSC_OEIM | PL011_IMSC_BEIM | \
		PL011_IMSC_PEIM | PL011_IMSC_FEIM | \
		PL011_IMSC_RIMIM | PL011_IMSC_CTSMIM | \
		PL011_IMSC_DCDMIM | PL011_IMSC_DSRMIM | \
		PL011_IMSC_RXIM | PL011_IMSC_TXIM | \
		PL011_IMSC_RTIM)

static inline
volatile struct pl011_regs *const get_uart(const struct device *dev)
{
	return (volatile struct pl011_regs *const)DEVICE_MMIO_GET(dev);
}

static void pl011_enable(const struct device *dev)
{
	get_uart(dev)->cr |=  PL011_CR_UARTEN;
}

static void pl011_disable(const struct device *dev)
{
	get_uart(dev)->cr &= ~PL011_CR_UARTEN;
}

static void pl011_enable_fifo(const struct device *dev)
{
	get_uart(dev)->lcr_h |= PL011_LCRH_FEN;
}

static void pl011_disable_fifo(const struct device *dev)
{
	get_uart(dev)->lcr_h &= ~PL011_LCRH_FEN;
}

static int pl011_set_baudrate(const struct device *dev,
			      uint32_t clk, uint32_t baudrate)
{
	/* Avoiding float calculations, bauddiv is left shifted by 6 */
	uint64_t bauddiv = (((uint64_t)clk) << PL011_FBRD_WIDTH)
				/ (baudrate * 16U);

	/* Valid bauddiv value
	 * uart_clk (min) >= 16 x baud_rate (max)
	 * uart_clk (max) <= 16 x 65535 x baud_rate (min)
	 */
	if ((bauddiv < (1u << PL011_FBRD_WIDTH))
		|| (bauddiv > (65535u << PL011_FBRD_WIDTH))) {
		return -EINVAL;
	}

	get_uart(dev)->ibrd = bauddiv >> PL011_FBRD_WIDTH;
	get_uart(dev)->fbrd = bauddiv & ((1u << PL011_FBRD_WIDTH) - 1u);

	__DMB();

	/* In order to internally update the contents of ibrd or fbrd, a
	 * lcr_h write must always be performed at the end
	 * ARM DDI 0183F, Pg 3-13
	 */
	get_uart(dev)->lcr_h = get_uart(dev)->lcr_h;

	return 0;
}

static bool pl011_is_readable(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	if (!data->sbsa &&
	    (!(get_uart(dev)->cr & PL011_CR_UARTEN) ||
	     !(get_uart(dev)->cr & PL011_CR_RXE)))
		return false;

	return (get_uart(dev)->fr & PL011_FR_RXFE) == 0U;
}

static int pl011_poll_in(const struct device *dev, unsigned char *c)
{
	if (!pl011_is_readable(dev)) {
		return -1;
	}

	/* got a character */
	*c = (unsigned char)get_uart(dev)->dr;

	return get_uart(dev)->rsr & PL011_RSR_ERROR_MASK;
}

static void pl011_poll_out(const struct device *dev,
					     unsigned char c)
{
	/* Wait for space in FIFO */
	while (get_uart(dev)->fr & PL011_FR_TXFF) {
		; /* Wait */
	}

	/* Send a character */
	get_uart(dev)->dr = (uint32_t)c;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int pl011_fifo_fill(const struct device *dev,
				    const uint8_t *tx_data, int len)
{
	uint8_t num_tx = 0U;

	while (!(get_uart(dev)->fr & PL011_FR_TXFF) && (len - num_tx > 0)) {
		get_uart(dev)->dr = tx_data[num_tx++];
	}
	return num_tx;
}

static int pl011_fifo_read(const struct device *dev,
				    uint8_t *rx_data, const int len)
{
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) && !(get_uart(dev)->fr & PL011_FR_RXFE)) {
		rx_data[num_rx++] = get_uart(dev)->dr;
	}

	return num_rx;
}

static void pl011_irq_tx_enable(const struct device *dev)
{
	get_uart(dev)->imsc |= PL011_IMSC_TXIM;
}

static void pl011_irq_tx_disable(const struct device *dev)
{
	get_uart(dev)->imsc &= ~PL011_IMSC_TXIM;
}

static int pl011_irq_tx_complete(const struct device *dev)
{
	/* check for TX FIFO empty */
	return get_uart(dev)->fr & PL011_FR_TXFE;
}

static int pl011_irq_tx_ready(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	if (!data->sbsa && !(get_uart(dev)->cr & PL011_CR_TXE))
		return false;

	return ((get_uart(dev)->imsc & PL011_IMSC_TXIM) &&
		pl011_irq_tx_complete(dev));
}

static void pl011_irq_rx_enable(const struct device *dev)
{
	get_uart(dev)->imsc |= PL011_IMSC_RXIM | PL011_IMSC_RTIM;
}

static void pl011_irq_rx_disable(const struct device *dev)
{
	get_uart(dev)->imsc &= ~(PL011_IMSC_RXIM | PL011_IMSC_RTIM);
}

static int pl011_irq_rx_ready(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	if (!data->sbsa && !(get_uart(dev)->cr & PL011_CR_RXE))
		return false;

	return ((get_uart(dev)->imsc & PL011_IMSC_RXIM) &&
		(!(get_uart(dev)->fr & PL011_FR_RXFE)));
}

static void pl011_irq_err_enable(const struct device *dev)
{
	/* enable framing, parity, break, and overrun */
	get_uart(dev)->imsc |= PL011_IMSC_ERROR_MASK;
}

static void pl011_irq_err_disable(const struct device *dev)
{
	get_uart(dev)->imsc &= ~PL011_IMSC_ERROR_MASK;
}

static int pl011_irq_is_pending(const struct device *dev)
{
	return pl011_irq_rx_ready(dev) || pl011_irq_tx_ready(dev);
}

static int pl011_irq_update(const struct device *dev)
{
	return 1;
}

static void pl011_irq_callback_set(const struct device *dev,
					    uart_irq_callback_user_data_t cb,
					    void *cb_data)
{
	struct pl011_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api pl011_driver_api = {
	.poll_in = pl011_poll_in,
	.poll_out = pl011_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = pl011_fifo_fill,
	.fifo_read = pl011_fifo_read,
	.irq_tx_enable = pl011_irq_tx_enable,
	.irq_tx_disable = pl011_irq_tx_disable,
	.irq_tx_ready = pl011_irq_tx_ready,
	.irq_rx_enable = pl011_irq_rx_enable,
	.irq_rx_disable = pl011_irq_rx_disable,
	.irq_tx_complete = pl011_irq_tx_complete,
	.irq_rx_ready = pl011_irq_rx_ready,
	.irq_err_enable = pl011_irq_err_enable,
	.irq_err_disable = pl011_irq_err_disable,
	.irq_is_pending = pl011_irq_is_pending,
	.irq_update = pl011_irq_update,
	.irq_callback_set = pl011_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int pl011_init(const struct device *dev)
{
	const struct pl011_config *config = dev->config;
	struct pl011_data *data = dev->data;
	int ret;
	uint32_t lcrh;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/*
	 * If working in SBSA mode, we assume that UART is already configured,
	 * or does not require configuration at all (if UART is emulated by
	 * virtualization software).
	 */
	if (!data->sbsa) {
		/* disable the uart */
		pl011_disable(dev);
		pl011_disable_fifo(dev);

		/* Set baud rate */
		ret = pl011_set_baudrate(dev, config->sys_clk_freq,
					 data->baud_rate);
		if (ret != 0) {
			return ret;
		}

		/* Setting the default character format */
		lcrh = get_uart(dev)->lcr_h & ~(PL011_LCRH_FORMAT_MASK);
		lcrh &= ~(BIT(0) | BIT(7));
		lcrh |= PL011_LCRH_WLEN_SIZE(8) << PL011_LCRH_WLEN_SHIFT;
		get_uart(dev)->lcr_h = lcrh;

		/* Enabling the FIFOs */
		pl011_enable_fifo(dev);
	}
	/* initialize all IRQs as masked */
	get_uart(dev)->imsc = 0U;
	get_uart(dev)->icr = PL011_IMSC_MASK_ALL;

	if (!data->sbsa) {
		get_uart(dev)->dmacr = 0U;
		__ISB();
		get_uart(dev)->cr &= ~(BIT(14) | BIT(15) | BIT(1));
		get_uart(dev)->cr |= PL011_CR_RXE | PL011_CR_TXE;
		__ISB();
	}
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif
	if (!data->sbsa)
		pl011_enable(dev);

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void pl011_isr(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define PL011_IRQ_CONFIG_FUNC_BODY(n, prop, i)		\
	{						\
		IRQ_CONNECT(DT_IRQ_BY_IDX(n, i, irq),	\
			DT_IRQ_BY_IDX(n, i, priority),	\
			pl011_isr,			\
			DEVICE_DT_GET(n),		\
			0);				\
		irq_enable(DT_IRQ_BY_IDX(n, i, irq));	\
	}

#define PL011_CONFIG_PORT(n)								\
	static void pl011_irq_config_func_##n(const struct device *dev)			\
	{										\
		DT_INST_FOREACH_PROP_ELEM(n, interrupt_names,				\
			PL011_IRQ_CONFIG_FUNC_BODY)					\
	};										\
											\
	static struct pl011_config pl011_cfg_port_##n = {				\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),	\
		.irq_config_func = pl011_irq_config_func_##n,				\
	};
#else
#define PL011_CONFIG_PORT(n)								\
	static struct pl011_config pl011_cfg_port_##n = {				\
		.uart = (volatile struct pl011_regs *)DT_INST_REG_ADDR(n),		\
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),	\
	};
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define PL011_INIT(n)						\
								\
	PL011_CONFIG_PORT(n)					\
								\
	static struct pl011_data pl011_data_port_##n = {	\
		.baud_rate = DT_INST_PROP(n, current_speed),	\
	};							\
								\
	DEVICE_DT_INST_DEFINE(n, &pl011_init,			\
			NULL,					\
			&pl011_data_port_##n,			\
			&pl011_cfg_port_##n,			\
			PRE_KERNEL_1,				\
			CONFIG_SERIAL_INIT_PRIORITY,		\
			&pl011_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PL011_INIT)

#ifdef CONFIG_UART_PL011_SBSA

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT SBSA_COMPAT

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void pl011_irq_config_func_sbsa(const struct device *dev);
#endif

static struct pl011_config pl011_cfg_sbsa = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = pl011_irq_config_func_sbsa,
#endif
};

static struct pl011_data pl011_data_sbsa = {
	.sbsa = true,
};

DEVICE_DT_INST_DEFINE(0,
		      &pl011_init,
		      NULL,
		      &pl011_data_sbsa,
		      &pl011_cfg_sbsa, PRE_KERNEL_1,
		      CONFIG_SERIAL_INIT_PRIORITY,
		      &pl011_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void pl011_irq_config_func_sbsa(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    pl011_isr,
		    DEVICE_GET(pl011_sbsa),
		    0);
	irq_enable(DT_INST_IRQN(0));
}
#endif

#endif /* CONFIG_UART_PL011_SBSA */
