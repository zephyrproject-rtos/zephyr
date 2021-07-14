/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_iuart

#include <device.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
#include <errno.h>
#include <soc.h>

struct imx8m_uart_regs {
	uint32_t urxd;
	uint8_t reserved_0[60];
	uint32_t utxd;
	uint8_t reserved_1[60];
	uint32_t ucr1;
	uint32_t ucr2;
	uint32_t ucr3;
	uint32_t ucr4;
	uint32_t ufcr;
	uint32_t usr1;
	uint32_t usr2;
	uint32_t uesc;
	uint32_t utim;
	uint32_t ubir;
	uint32_t ubmr;
	uint32_t ubrc;
	uint32_t onems;
	uint32_t uts;
	uint32_t umcr;
};

struct imx8m_uart_config {
	struct imx8m_uart_regs *base;
	uint32_t sys_clk_freq;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct imx8m_uart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#define DEV_CFG(dev)						\
	((const struct imx8m_uart_config * const)(dev)->config)

#define DEV_REGS(dev) \
	((volatile struct imx8m_uart_regs  *)(DEV_CFG(dev))->base)

#define	UART_URXD_RX_DATA_MASK		0xFF
#define UART_UTXD_TX_DATA_MASK		0xFF

#define UART_UFCR_RFDIV_MASK		0x380
#define UART_UFCR_RFDIV_SHIFT		7
#define UART_UFCR_RFDIV(x)		(((x) << UART_UFCR_RFDIV_SHIFT) & UART_UFCR_RFDIV_MASK)
#define UART_UFCR_TXTL_MASK		0xFC00
#define UART_UFCR_RXTL_MASK		0x3F
#define UART_UFCR_TXTL_SHIFT		10
#define UART_UFCR_TXTL(x)		(((x) << UART_UFCR_TXTL_SHIFT) & UART_UFCR_TXTL_MASK)
#define UART_UFCR_RXTL_SHIFT		0
#define UART_UFCR_RXTL(x)		(((x) << UART_UFCR_RXTL_SHIFT) & UART_UFCR_RXTL_MASK)

#define UART_UBIR_INC_MASK		0xFFFF
#define UART_UBIR_INC_SHIFT		0
#define UART_UBIR_INC(x)		(((x) << UART_UBIR_INC_SHIFT) & UART_UBIR_INC_MASK)

#define UART_UBMR_MOD_MASK		0xFFFF
#define UART_UBMR_MOD_SHIFT		0
#define UART_UBMR_MOD(x)		(((x) << UART_UBMR_MOD_SHIFT) & UART_UBMR_MOD_MASK)

#define UART_ONEMS_ONEMS_MASK		0xFFFFFF
#define UART_ONEMS_ONEMS_SHIFT		0
#define UART_ONEMS_ONEMS(x)		(((x) << UART_ONEMS_ONEMS_SHIFT) & UART_ONEMS_ONEMS_MASK)

#define UART_UCR1_UARTEN_MASK		0x1
#define UART_UCR1_TXMPTYEN_MASK		BIT(6)
#define UART_UCR2_SRST_MASK		0x1
#define UART_UCR2_WS_MASK		BIT(5)
#define UART_UCR2_WS_SHIFT		5
#define UART_UCR2_WS(x)			(((x) << UART_UCR2_WS_SHIFT) & UART_UCR2_WS_MASK)
#define UART_UCR2_STPB_MASK		BIT(6)
#define UART_UCR2_STPB_SHIFT		6
#define UART_UCR2_STPB(x)		(((x) << UART_UCR2_STPB_SHIFT) & UART_UCR2_STPB_MASK)
#define UART_UCR2_IRTS_MASK		BIT(14)
#define UART_UCR2_TXEN_MASK		BIT(2)
#define UART_UCR2_RXEN_MASK		BIT(1)
#define UART_UCR3_RXDMUXSEL_MASK	BIT(2)
#define UART_UCR3_DSR_MASK		BIT(10)
#define UART_UCR3_DCD_MASK		BIT(9)
#define UART_UCR3_RI_MASK		BIT(8)
#define UART_UCR3_FRAERREN_MASK		BIT(11)
#define UART_UCR3_PARERREN_MASK		BIT(12)
#define UART_UCR4_DREN_MASK		BIT(0)
#define UART_UCR4_OREN_MASK		BIT(1)
#define UART_UCR4_CTSTL_MASK		0xFC00
#define UART_UCR4_CTSTL_SHIFT		10
#define UART_UCR4_CTSTL(x)		(((x) << UART_UCR4_CTSTL_SHIFT) & UART_UCR4_CTSTL_MASK)

#define UART_USR1_FRAMERR_MASK		BIT(10)
#define UART_USR1_TRDY_MASK		BIT(13)
#define UART_USR1_PARITYERR_MASK	BIT(15)
#define UART_USR2_RDR_MASK		BIT(0)
#define UART_USR2_ORE_MASK		BIT(1)
#define UART_USR2_TXFE_MASK		BIT(14)

#define UART_UESC_ESC_CHAR_MASK		0xFF
#define UART_UESC_ESC_CHAR_SHIFT	0
#define UART_UESC_ESC_CHAR(x)	(((x) << UART_UESC_ESC_CHAR_SHIFT) & UART_UESC_ESC_CHAR_MASK)

#define UART_UTS_TXEMPTY_MASK		BIT(6)
#define UART_UTS_RXEMPTY_MASK		BIT(5)

static inline uint32_t imx8m_uart_getstatus1(const struct device *dev)
{
	return DEV_REGS(dev)->usr1;
}

static inline uint32_t imx8m_uart_getstatus2(const struct device *dev)
{
	return DEV_REGS(dev)->usr2;
}

static inline uint8_t imx8m_uart_readbyte(const struct device *dev)
{
	return (uint8_t)(DEV_REGS(dev)->urxd & UART_URXD_RX_DATA_MASK);
}

static int imx8m_uart_poll_in(const struct device *dev, unsigned char *c)
{
	int ret = -1;

	if (imx8m_uart_getstatus2(dev) & UART_USR2_RDR_MASK) {
		*c = imx8m_uart_readbyte(dev);
		ret = 0;
	}

	return ret;
}

static inline void imx8m_uart_writebyte(const struct device *dev, uint8_t data)
{
	DEV_REGS(dev)->utxd = data & UART_UTXD_TX_DATA_MASK;
}

static void imx8m_uart_poll_out(const struct device *dev, unsigned char c)
{
	while (!(imx8m_uart_getstatus1(dev) & UART_USR1_TRDY_MASK)) {
	}

	imx8m_uart_writebyte(dev, c);
}

static int imx8m_uart_err_check(const struct device *dev)
{
	int err = 0;
	uint32_t usr1, usr2;

	usr1 = imx8m_uart_getstatus1(dev);
	usr2 = imx8m_uart_getstatus2(dev);

	if (usr2 & UART_USR2_ORE_MASK) {
		err |= UART_ERROR_OVERRUN;
		DEV_REGS(dev)->usr2 |= UART_USR2_ORE_MASK;
	}

	if (usr1 & UART_USR1_PARITYERR_MASK) {
		err |= UART_ERROR_PARITY;
		DEV_REGS(dev)->usr1 |= UART_USR1_PARITYERR_MASK;
	}

	if (usr1 & UART_USR1_FRAMERR_MASK) {
		err |= UART_ERROR_FRAMING;
		DEV_REGS(dev)->usr1 |= UART_USR1_FRAMERR_MASK;
	}

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int imx8m_uart_fifo_fill(const struct device *dev,
				const uint8_t *tx_data,
				int len)
{
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (imx8m_uart_getstatus2(dev) & UART_USR2_TXFE_MASK)) {

		imx8m_uart_writebyte(dev, tx_data[num_tx++]);
	}

	return num_tx;
}

static int imx8m_uart_fifo_read(const struct device *dev, uint8_t *rx_data,
				const int len)
{
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (imx8m_uart_getstatus2(dev) & UART_USR2_RDR_MASK)) {

		rx_data[num_rx++] = imx8m_uart_readbyte(dev);
	}

	return num_rx;
}

static void imx8m_uart_irq_tx_enable(const struct device *dev)
{
	DEV_REGS(dev)->ucr1 |= UART_UCR1_TXMPTYEN_MASK;
}

static void imx8m_uart_irq_tx_disable(const struct device *dev)
{
	DEV_REGS(dev)->ucr1 &= ~UART_UCR1_TXMPTYEN_MASK;
}

static int imx8m_uart_irq_tx_complete(const struct device *dev)
{
	return !!(imx8m_uart_getstatus2(dev) & UART_USR2_TXFE_MASK);
}

static int imx8m_uart_irq_tx_ready(const struct device *dev)
{
	return (DEV_REGS(dev)->ucr1 & UART_UCR1_TXMPTYEN_MASK)
		&& imx8m_uart_irq_tx_complete(dev);
}

static void imx8m_uart_irq_rx_enable(const struct device *dev)
{
	DEV_REGS(dev)->ucr4 |= UART_UCR4_DREN_MASK;
}

static void imx8m_uart_irq_rx_disable(const struct device *dev)
{
	DEV_REGS(dev)->ucr4 &= ~UART_UCR4_DREN_MASK;
}

static int imx8m_uart_irq_rx_full(const struct device *dev)
{
	return !!(imx8m_uart_getstatus2(dev) & UART_USR2_RDR_MASK);
}

static int imx8m_uart_irq_rx_pending(const struct device *dev)
{
	return (DEV_REGS(dev)->ucr4 & UART_UCR4_DREN_MASK)
		&& imx8m_uart_irq_rx_full(dev);
}

static void imx8m_uart_irq_err_enable(const struct device *dev)
{
	DEV_REGS(dev)->ucr3 |= UART_UCR3_FRAERREN_MASK | UART_UCR3_PARERREN_MASK;
	DEV_REGS(dev)->ucr4 |= UART_UCR4_OREN_MASK;
}

static void imx8m_uart_irq_err_disable(const struct device *dev)
{
	DEV_REGS(dev)->ucr4 &= ~UART_UCR4_OREN_MASK;
	DEV_REGS(dev)->ucr3 &= ~(UART_UCR3_FRAERREN_MASK | UART_UCR3_PARERREN_MASK);
}

static int imx8m_uart_irq_is_pending(const struct device *dev)
{
	return imx8m_uart_irq_tx_ready(dev) || imx8m_uart_irq_rx_pending(dev);
}

static int imx8m_uart_irq_update(const struct device *dev)
{
	return 1;
}

static void imx8m_uart_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct imx8m_uart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void imx8m_uart_isr(const struct device *dev)
{
	struct imx8m_uart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static inline void imx8m_uart_disable(const struct device *dev)
{
	DEV_REGS(dev)->ucr1 &= ~UART_UCR1_UARTEN_MASK;
}

static inline void imx8m_uart_enable(const struct device *dev)
{
	DEV_REGS(dev)->ucr1 |= UART_UCR1_UARTEN_MASK;
}

static inline void imx8m_uart_softwarereset(const struct device *dev)
{
	DEV_REGS(dev)->ucr2 &= ~UART_UCR2_SRST_MASK;
	while ((DEV_REGS(dev)->ucr2 & UART_UCR2_SRST_MASK) == 0U) {
	}
}

static int imx8m_uart_setbaudrate(const struct device *dev,
				  uint32_t baudrate, uint32_t clk)
{
	uint32_t numerator, denominator, divisor, reffreqdiv;
	uint32_t divider, tempdenominator;
	uint64_t bauddiff, tempnumerator;

	/* get the approximately maximum divisor */
	numerator   = clk;
	denominator = baudrate << 4;
	divisor     = 1;

	while (denominator) {
		divisor     = denominator;
		denominator = numerator % denominator;
		numerator   = divisor;
	}

	numerator = clk / divisor;
	denominator = (baudrate << 4) / divisor;

	/* numerator ranges from 1 ~ 7 * 64k */
	/* denominator ranges from 1 ~ 64k */
	if ((numerator > (UART_UBIR_INC_MASK * 7)) || (denominator > UART_UBIR_INC_MASK)) {
		uint32_t m   = (numerator - 1) / (UART_UBIR_INC_MASK * 7) + 1;
		uint32_t n   = (denominator - 1) / UART_UBIR_INC_MASK + 1;
		uint32_t max = m > n ? m : n;

		numerator /= max;
		denominator /= max;
		if (!numerator) {
			numerator = 1;
		}

		if (!denominator) {
			denominator = 1;
		}
	}

	divider = (numerator - 1) / UART_UBIR_INC_MASK + 1;

	switch (divider) {
	case 1:
		reffreqdiv = 0x05;
		break;
	case 2:
		reffreqdiv = 0x04;
		break;
	case 3:
		reffreqdiv = 0x03;
		break;
	case 4:
		reffreqdiv = 0x02;
		break;
	case 5:
		reffreqdiv = 0x01;
		break;
	case 6:
		reffreqdiv = 0x00;
		break;
	case 7:
		reffreqdiv = 0x06;
		break;
	default:
		reffreqdiv = 0x05;
		break;
	}

	/*
	 * Compare the difference between baudrate and calculated baud rate.
	 * Baud Rate = Ref Freq / (16 * (UBMR + 1)/(UBIR+1)).
	 * bauddiff = (clk/divider)/( 16 * ((numerator / divider)/ denominator).
	 */
	tempnumerator   = (uint64_t)clk;
	tempdenominator = (numerator << 4);
	divisor         = 1;
	/* get the approximately maximum divisor */
	while (tempdenominator) {
		divisor         = tempdenominator;
		tempdenominator = (uint32_t)(tempnumerator % tempdenominator);
		tempnumerator   = (uint64_t)divisor;
	}
	tempnumerator   = (uint64_t)clk / (uint64_t)divisor;
	tempdenominator = (numerator << 4) / divisor;
	bauddiff        = (tempnumerator * (uint64_t)denominator) / (uint64_t)tempdenominator;
	bauddiff        = (bauddiff >= (uint64_t)baudrate) ? (bauddiff - (uint64_t)baudrate) :
		((uint64_t)baudrate - bauddiff);

	if (bauddiff < ((uint64_t)baudrate / 100 * 3)) {
		DEV_REGS(dev)->ufcr &= ~UART_UFCR_RFDIV_MASK;
		DEV_REGS(dev)->ufcr |= UART_UFCR_RFDIV(reffreqdiv);
		DEV_REGS(dev)->ubir  = UART_UBIR_INC(denominator - 1);
		DEV_REGS(dev)->ubmr  = UART_UBMR_MOD(numerator / divider - 1);
		DEV_REGS(dev)->onems = UART_ONEMS_ONEMS(clk / (1000 * divider));

		return 0;
	} else {
		return -EINVAL;
	}
}

static int imx8m_uart_init(const struct device *dev)
{
	const struct imx8m_uart_config *config = DEV_CFG(dev);

	imx8m_uart_disable(dev);

	imx8m_uart_softwarereset(dev);

	DEV_REGS(dev)->ucr1  = 0x0;
	DEV_REGS(dev)->ucr2  = UART_UCR2_SRST_MASK;
	DEV_REGS(dev)->ucr3  = UART_UCR3_DSR_MASK | UART_UCR3_DCD_MASK | UART_UCR3_RI_MASK;
	DEV_REGS(dev)->ucr4  = UART_UCR4_CTSTL(32);
	DEV_REGS(dev)->ufcr  = UART_UFCR_TXTL(2) | UART_UFCR_RXTL(1);
	DEV_REGS(dev)->uesc  = UART_UESC_ESC_CHAR(0x2B);
	DEV_REGS(dev)->utim  = 0x0;
	DEV_REGS(dev)->onems = 0x0;
	DEV_REGS(dev)->uts   = UART_UTS_TXEMPTY_MASK | UART_UTS_RXEMPTY_MASK;
	DEV_REGS(dev)->umcr  = 0x0;

	/*
	 * Set UART data word length, stop bit count, parity mode and communication
	 * direction according to uart init struct, disable RTS hardware flow control.
	 */
	DEV_REGS(dev)->ucr2 |= UART_UCR2_WS(1) | UART_UCR2_STPB(0) |
		UART_UCR2_TXEN_MASK | UART_UCR2_RXEN_MASK | UART_UCR2_IRTS_MASK;

	DEV_REGS(dev)->ucr3 |= UART_UCR3_RXDMUXSEL_MASK;

	DEV_REGS(dev)->ufcr = (DEV_REGS(dev)->ufcr & ~UART_UFCR_TXTL_MASK) | UART_UFCR_TXTL(2);
	DEV_REGS(dev)->ufcr = (DEV_REGS(dev)->ufcr & ~UART_UFCR_RXTL_MASK) | UART_UFCR_RXTL(1);

	imx8m_uart_setbaudrate(dev, config->baud_rate, config->sys_clk_freq);

	imx8m_uart_enable(dev);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api imx8m_uart_driver_api = {
	.poll_in = imx8m_uart_poll_in,
	.poll_out = imx8m_uart_poll_out,
	.err_check = imx8m_uart_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = imx8m_uart_fifo_fill,
	.fifo_read = imx8m_uart_fifo_read,
	.irq_tx_enable = imx8m_uart_irq_tx_enable,
	.irq_tx_disable = imx8m_uart_irq_tx_disable,
	.irq_tx_complete = imx8m_uart_irq_tx_complete,
	.irq_tx_ready = imx8m_uart_irq_tx_ready,
	.irq_rx_enable = imx8m_uart_irq_rx_enable,
	.irq_rx_disable = imx8m_uart_irq_rx_disable,
	.irq_rx_ready = imx8m_uart_irq_rx_full,
	.irq_err_enable = imx8m_uart_irq_err_enable,
	.irq_err_disable = imx8m_uart_irq_err_disable,
	.irq_is_pending = imx8m_uart_irq_is_pending,
	.irq_update = imx8m_uart_irq_update,
	.irq_callback_set = imx8m_uart_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define IMX8M_UART_IRQ_INIT(n, i)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq),		\
			    DT_INST_IRQ_BY_IDX(n, i, priority),		\
			    imx8m_uart_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));		\
	} while (0)
#define IMX8M_UART_CONFIG_FUNC(n)					\
	static void imx8m_uart_config_func_##n(const struct device *dev) \
	{								\
		IMX8M_UART_IRQ_INIT(n, 0);				\
									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 1),			\
			   (IMX8M_UART_IRQ_INIT(n, 1);))		\
	}
#define IMX8M_UART_IRQ_CFG_FUNC_INIT(n)				\
	.irq_config_func = imx8m_uart_config_func_##n
#define IMX8M_UART_INIT_CFG(n)						\
	IMX8M_UART_DECLARE_CFG(n, IMX8M_UART_IRQ_CFG_FUNC_INIT(n))
#else
#define IMX8M_UART_CONFIG_FUNC(n)
#define IMX8M_UART_IRQ_CFG_FUNC_INIT
#define IMX8M_UART_INIT_CFG(n)						\
	IMX8M_UART_DECLARE_CFG(n, IMX8M_UART_IRQ_CFG_FUNC_INIT)
#endif

#define IMX8M_UART_DECLARE_CFG(n, IRQ_FUNC_INIT)			\
static const struct imx8m_uart_config imx8m_uart_##n##_config = {	\
	.base = (struct imx8m_uart_regs *) DT_INST_REG_ADDR(n),		\
	.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(0, clocks, clock_frequency), \
	.baud_rate = DT_INST_PROP(n, current_speed),			\
	IRQ_FUNC_INIT							\
}

#define IMX8M_UART_INIT(n)						\
									\
	static struct imx8m_uart_data imx8m_uart_##n##_data;		\
									\
	static const struct imx8m_uart_config imx8m_uart_##n##_config;\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &imx8m_uart_init,				\
			    NULL,					\
			    &imx8m_uart_##n##_data,			\
			    &imx8m_uart_##n##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &imx8m_uart_driver_api);			\
									\
	IMX8M_UART_CONFIG_FUNC(n)					\
									\
	IMX8M_UART_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(IMX8M_UART_INIT)
