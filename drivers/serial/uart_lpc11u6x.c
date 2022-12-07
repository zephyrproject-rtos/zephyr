/*
 * Copyright (c) 2020, Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_lpc11u6x_uart

#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include "uart_lpc11u6x.h"

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
static int lpc11u6x_uart0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	if (!(cfg->uart0->lsr & LPC11U6X_UART0_LSR_RDR)) {
		return -1;
	}
	*c = cfg->uart0->rbr;

	return 0;
}

static void lpc11u6x_uart0_poll_out(const struct device *dev, unsigned char c)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	while (!(cfg->uart0->lsr & LPC11U6X_UART0_LSR_THRE)) {
	}
	cfg->uart0->thr = c;
}

static int lpc11u6x_uart0_err_check(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;
	uint32_t lsr;
	int ret = 0;

	lsr = cfg->uart0->lsr;
	if (lsr & LPC11U6X_UART0_LSR_OE) {
		ret |= UART_ERROR_OVERRUN;
	}
	if (lsr & LPC11U6X_UART0_LSR_PE) {
		ret |= UART_ERROR_PARITY;
	}
	if (lsr & LPC11U6X_UART0_LSR_FE) {
		ret |= UART_ERROR_FRAMING;
	}
	if (lsr & LPC11U6X_UART0_LSR_BI) {
		ret |= UART_BREAK;
	}

	return ret;
}

static void lpc11u6x_uart0_write_divisor(struct lpc11u6x_uart0_regs *uart0,
					 uint32_t div)
{
	/* Enable access to dll & dlm registers */
	uart0->lcr |= LPC11U6X_UART0_LCR_DLAB;
	uart0->dll = div & 0xFF;
	uart0->dlm = (div >> 8) & 0xFF;
	uart0->lcr &= ~LPC11U6X_UART0_LCR_DLAB;
}

static void lpc11u6x_uart0_write_fdr(struct lpc11u6x_uart0_regs *uart0,
				     uint32_t div, uint32_t mul)
{
	uart0->fdr = (div & 0xF) | ((mul & 0xF) << 4);
}

static void lpc11u6x_uart0_config_baudrate(const struct device *clk_drv,
					   const struct lpc11u6x_uart0_config *cfg,
					   uint32_t baudrate)
{
	uint32_t div = 1, mul, dl;
	uint32_t pclk;

	/* Compute values for fractional baud rate generator. We need to have
	 * a clock that is as close as possible to a multiple of
	 * LPC11U6X_UART0_CLK so that we can have every baudrate that is
	 * a multiple of 9600
	 */
	clock_control_get_rate(clk_drv, (clock_control_subsys_t) cfg->clkid,
			       &pclk);
	mul = pclk / (pclk % LPC11U6X_UART0_CLK);

	dl = pclk / (16 * baudrate + 16 * baudrate / mul);

	/* Configure clock divisor and fractional baudrate generator */
	lpc11u6x_uart0_write_divisor(cfg->uart0, dl);
	lpc11u6x_uart0_write_fdr(cfg->uart0, div, mul);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int lpc11u6x_uart0_configure(const struct device *dev,
				    const struct uart_config *cfg)
{
	const struct  lpc11u6x_uart0_config *dev_cfg = dev->config;
	struct lpc11u6x_uart0_data *data = dev->data;
	uint32_t flags = 0;

	/* Check that the baudrate is a multiple of 9600 */
	if (cfg->baudrate % 9600) {
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
		flags |= LPC11U6X_UART0_LCR_PARTIY_ENABLE |
			LPC11U6X_UART0_LCR_PARTIY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		flags |= LPC11U6X_UART0_LCR_PARTIY_ENABLE |
			LPC11U6X_UART0_LCR_PARTIY_EVEN;
		break;
	case UART_CFG_PARITY_MARK:
		__fallthrough;
	case UART_CFG_PARITY_SPACE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_0_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_1:
		flags |= LPC11U6X_UART0_LCR_STOP_1BIT;
		break;
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_2:
		flags |= LPC11U6X_UART0_LCR_STOP_2BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		flags |= LPC11U6X_UART0_LCR_WLS_5BITS;
		break;
	case UART_CFG_DATA_BITS_6:
		flags |= LPC11U6X_UART0_LCR_WLS_6BITS;
		break;
	case UART_CFG_DATA_BITS_7:
		flags |= LPC11U6X_UART0_LCR_WLS_7BITS;
		break;
	case UART_CFG_DATA_BITS_8:
		flags |= LPC11U6X_UART0_LCR_WLS_8BITS;
		break;
	case UART_CFG_DATA_BITS_9:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	lpc11u6x_uart0_config_baudrate(dev_cfg->clock_dev, dev_cfg, cfg->baudrate);
	dev_cfg->uart0->lcr = flags;

	data->baudrate = cfg->baudrate;
	data->stop_bits = cfg->stop_bits;
	data->data_bits = cfg->data_bits;
	data->flow_ctrl = cfg->flow_ctrl;
	data->parity = cfg->parity;

	return 0;
}

static int lpc11u6x_uart0_config_get(const struct device *dev,
				     struct uart_config *cfg)
{
	struct lpc11u6x_uart0_data *data = dev->data;

	cfg->baudrate = data->baudrate;
	cfg->parity = data->parity;
	cfg->stop_bits = data->stop_bits;
	cfg->data_bits = data->data_bits;
	cfg->flow_ctrl = data->flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int lpc11u6x_uart0_fifo_fill(const struct device *dev,
				    const uint8_t *data,
				    const int size)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;
	int nr_sent = 0;

	while (nr_sent < size && (cfg->uart0->lsr & LPC11U6X_UART0_LSR_THRE)) {
		cfg->uart0->thr = data[nr_sent++];
	}

	return nr_sent;
}

static int lpc11u6x_uart0_fifo_read(const struct device *dev, uint8_t *data,
				    const int size)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;
	int nr_rx = 0;

	while (nr_rx < size && (cfg->uart0->lsr & LPC11U6X_UART0_LSR_RDR)) {
		data[nr_rx++] = cfg->uart0->rbr;
	}

	return nr_rx;
}

static void lpc11u6x_uart0_irq_tx_enable(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	cfg->uart0->ier = (cfg->uart0->ier & LPC11U6X_UART0_IER_MASK) |
		LPC11U6X_UART0_IER_THREINTEN;

	/* Due to hardware limitations, first TX interrupt is not triggered when
	 * enabling it in the IER register. We have to trigger it.
	 */
	NVIC_SetPendingIRQ(DT_INST_IRQN(0));
}

static void lpc11u6x_uart0_irq_tx_disable(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	cfg->uart0->ier = (cfg->uart0->ier & LPC11U6X_UART0_IER_MASK) &
		~LPC11U6X_UART0_IER_THREINTEN;
}

static int lpc11u6x_uart0_irq_tx_complete(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	return (cfg->uart0->lsr & LPC11U6X_UART0_LSR_TEMT) != 0;
}

static int lpc11u6x_uart0_irq_tx_ready(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	return (cfg->uart0->lsr & LPC11U6X_UART0_LSR_THRE) &&
		(cfg->uart0->ier & LPC11U6X_UART0_IER_THREINTEN);
}

static void lpc11u6x_uart0_irq_rx_enable(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	cfg->uart0->ier = (cfg->uart0->ier & LPC11U6X_UART0_IER_MASK) |
		LPC11U6X_UART0_IER_RBRINTEN;
}

static void lpc11u6x_uart0_irq_rx_disable(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	cfg->uart0->ier = (cfg->uart0->ier & LPC11U6X_UART0_IER_MASK) &
		~LPC11U6X_UART0_IER_RBRINTEN;
}

static int lpc11u6x_uart0_irq_rx_ready(const struct device *dev)
{
	struct lpc11u6x_uart0_data *data = dev->data;

	return (LPC11U6X_UART0_IIR_INTID(data->cached_iir) ==
		LPC11U6X_UART0_IIR_INTID_RDA) ||
		(LPC11U6X_UART0_IIR_INTID(data->cached_iir) ==
		LPC11U6X_UART0_IIR_INTID_CTI);
}

static void lpc11u6x_uart0_irq_err_enable(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	cfg->uart0->ier = (cfg->uart0->ier & LPC11U6X_UART0_IER_MASK) |
		LPC11U6X_UART0_IER_RLSINTEN;
}

static void lpc11u6x_uart0_irq_err_disable(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;

	cfg->uart0->ier = (cfg->uart0->ier & LPC11U6X_UART0_IER_MASK) &
		~LPC11U6X_UART0_IER_RLSINTEN;
}

static int lpc11u6x_uart0_irq_is_pending(const struct device *dev)
{
	struct lpc11u6x_uart0_data *data = dev->data;

	return !(data->cached_iir & LPC11U6X_UART0_IIR_STATUS);
}

static int lpc11u6x_uart0_irq_update(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;
	struct lpc11u6x_uart0_data *data = dev->data;

	data->cached_iir = cfg->uart0->iir;
	return 1;
}

static void lpc11u6x_uart0_irq_callback_set(const struct device *dev,
					    uart_irq_callback_user_data_t cb,
					    void *user_data)
{
	struct lpc11u6x_uart0_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;
}

static void lpc11u6x_uart0_isr(const struct device *dev)
{
	struct lpc11u6x_uart0_data *data = dev->data;

	if (data->cb) {
		data->cb(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int lpc11u6x_uart0_init(const struct device *dev)
{
	const struct lpc11u6x_uart0_config *cfg = dev->config;
	struct lpc11u6x_uart0_data *data = dev->data;
	int err;

	/* Apply default pin control state to select RX and TX pins */
	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	clock_control_on(cfg->clock_dev, (clock_control_subsys_t) cfg->clkid);

	/* Configure baudrate, parity and stop bits */
	lpc11u6x_uart0_config_baudrate(cfg->clock_dev, cfg, cfg->baudrate);

	cfg->uart0->lcr |= LPC11U6X_UART0_LCR_WLS_8BITS; /* 8N1 */

	data->baudrate = cfg->baudrate;
	data->parity = UART_CFG_PARITY_NONE;
	data->stop_bits = UART_CFG_STOP_BITS_1;
	data->data_bits = UART_CFG_DATA_BITS_8;
	data->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	/* Configure FIFO */
	cfg->uart0->fcr = LPC11U6X_UART0_FCR_FIFO_EN;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void lpc11u6x_uart0_isr_config(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

PINCTRL_DT_DEFINE(DT_NODELABEL(uart0));

static const struct lpc11u6x_uart0_config uart0_config = {
	.uart0 = (struct lpc11u6x_uart0_regs *)
	DT_REG_ADDR(DT_NODELABEL(uart0)),
	.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(uart0))),
	.pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(uart0)),
	.clkid = DT_PHA_BY_IDX(DT_NODELABEL(uart0), clocks, 0, clkid),
	.baudrate = DT_PROP(DT_NODELABEL(uart0), current_speed),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = lpc11u6x_uart0_isr_config,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static const struct uart_driver_api uart0_api = {
	.poll_in = lpc11u6x_uart0_poll_in,
	.poll_out = lpc11u6x_uart0_poll_out,
	.err_check = lpc11u6x_uart0_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = lpc11u6x_uart0_configure,
	.config_get = lpc11u6x_uart0_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = lpc11u6x_uart0_fifo_fill,
	.fifo_read = lpc11u6x_uart0_fifo_read,
	.irq_tx_enable = lpc11u6x_uart0_irq_tx_enable,
	.irq_tx_disable = lpc11u6x_uart0_irq_tx_disable,
	.irq_tx_ready = lpc11u6x_uart0_irq_tx_ready,
	.irq_tx_complete = lpc11u6x_uart0_irq_tx_complete,
	.irq_rx_enable = lpc11u6x_uart0_irq_rx_enable,
	.irq_rx_disable = lpc11u6x_uart0_irq_rx_disable,
	.irq_rx_ready = lpc11u6x_uart0_irq_rx_ready,
	.irq_err_enable = lpc11u6x_uart0_irq_err_enable,
	.irq_err_disable = lpc11u6x_uart0_irq_err_disable,
	.irq_is_pending = lpc11u6x_uart0_irq_is_pending,
	.irq_update = lpc11u6x_uart0_irq_update,
	.irq_callback_set = lpc11u6x_uart0_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static struct lpc11u6x_uart0_data uart0_data;

DEVICE_DT_DEFINE(DT_NODELABEL(uart0),
		    &lpc11u6x_uart0_init,
		    NULL,
		    &uart0_data, &uart0_config,
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
		    &uart0_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void lpc11u6x_uart0_isr_config(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(uart0)),
		    DT_IRQ(DT_NODELABEL(uart0), priority),
		    lpc11u6x_uart0_isr, DEVICE_DT_GET(DT_NODELABEL(uart0)), 0);

	irq_enable(DT_IRQN(DT_NODELABEL(uart0)));
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) ||		\
	DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) ||	\
	DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay) ||	\
	DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)

static int lpc11u6x_uartx_poll_in(const struct device *dev, unsigned char *c)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	if (!(cfg->base->stat & LPC11U6X_UARTX_STAT_RXRDY)) {
		return -1;
	}
	*c = cfg->base->rx_dat;
	return 0;
}

static void lpc11u6x_uartx_poll_out(const struct device *dev, unsigned char c)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	while (!(cfg->base->stat & LPC11U6X_UARTX_STAT_TXRDY)) {
	}
	cfg->base->tx_dat = c;
}

static int lpc11u6x_uartx_err_check(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;
	int ret = 0;

	if (cfg->base->stat & LPC11U6X_UARTX_STAT_OVERRUNINT) {
		ret |= UART_ERROR_OVERRUN;
	}
	if (cfg->base->stat & LPC11U6X_UARTX_STAT_FRAMERRINT) {
		ret |= UART_ERROR_FRAMING;
	}
	if (cfg->base->stat & LPC11U6X_UARTX_STAT_PARITYERRINT) {
		ret |= UART_ERROR_PARITY;
	}

	return ret;
}

static void lpc11u6x_uartx_config_baud(const struct lpc11u6x_uartx_config *cfg,
				       uint32_t baudrate)
{
	uint32_t clk_rate;
	uint32_t div;
	const struct device *clk_drv = cfg->clock_dev;

	clock_control_get_rate(clk_drv, (clock_control_subsys_t) cfg->clkid,
			       &clk_rate);

	div = clk_rate / (16 * baudrate);
	if (div != 0) {
		div -= 1;
	}
	cfg->base->brg = div & LPC11U6X_UARTX_BRG_MASK;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int lpc11u6x_uartx_configure(const struct device *dev,
				    const struct uart_config *cfg)
{
	const struct lpc11u6x_uartx_config *dev_cfg = dev->config;
	struct lpc11u6x_uartx_data *data = dev->data;
	uint32_t flags = 0;

	/* We only support baudrates that are multiple of 9600 */
	if (cfg->baudrate % 9600) {
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		flags |= LPC11U6X_UARTX_CFG_PARITY_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		flags |= LPC11U6X_UARTX_CFG_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		flags |= LPC11U6X_UARTX_CFG_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_MARK:
		__fallthrough;
	case UART_CFG_PARITY_SPACE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_0_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_1:
		flags |= LPC11U6X_UARTX_CFG_STOP_1BIT;
		break;
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_2:
		flags |= LPC11U6X_UARTX_CFG_STOP_2BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		__fallthrough;
	case UART_CFG_DATA_BITS_6:
		return -ENOTSUP;
	case UART_CFG_DATA_BITS_7:
		flags |= LPC11U6X_UARTX_CFG_DATALEN_7BIT;
		break;
	case UART_CFG_DATA_BITS_8:
		flags |= LPC11U6X_UARTX_CFG_DATALEN_8BIT;
		break;
	case UART_CFG_DATA_BITS_9:
		flags |= LPC11U6X_UARTX_CFG_DATALEN_9BIT;
		break;
	default:
		return -EINVAL;
	}

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	/* Disable UART */
	dev_cfg->base->cfg = 0;

	/* Update baudrate */
	lpc11u6x_uartx_config_baud(dev_cfg, cfg->baudrate);

	/* Set parity, data bits, stop bits and re-enable UART interface */
	dev_cfg->base->cfg = flags | LPC11U6X_UARTX_CFG_ENABLE;

	data->baudrate = cfg->baudrate;
	data->parity = cfg->parity;
	data->stop_bits = cfg->stop_bits;
	data->data_bits = cfg->data_bits;
	data->flow_ctrl = cfg->flow_ctrl;

	return 0;
}

static int lpc11u6x_uartx_config_get(const struct device *dev,
				     struct uart_config *cfg)
{
	const struct lpc11u6x_uartx_data *data = dev->data;

	cfg->baudrate = data->baudrate;
	cfg->parity = data->parity;
	cfg->stop_bits = data->stop_bits;
	cfg->data_bits = data->data_bits;
	cfg->flow_ctrl = data->flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int lpc11u6x_uartx_fifo_fill(const struct device *dev,
				    const uint8_t *data,
				    int size)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;
	int tx_size = 0;

	while (tx_size < size &&
	       (cfg->base->stat & LPC11U6X_UARTX_STAT_TXRDY)) {
		cfg->base->tx_dat = data[tx_size++];
	}
	return tx_size;
}

static int lpc11u6x_uartx_fifo_read(const struct device *dev, uint8_t *data,
				    int size)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;
	int rx_size = 0;

	while (rx_size < size &&
	       (cfg->base->stat & LPC11U6X_UARTX_STAT_RXRDY)) {
		data[rx_size++] = cfg->base->rx_dat;
	}
	return rx_size;
}

static void lpc11u6x_uartx_irq_tx_enable(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	cfg->base->int_en_set = (cfg->base->int_en_set &
				 LPC11U6X_UARTX_INT_EN_SET_MASK) |
		LPC11U6X_UARTX_INT_EN_SET_TXRDYEN;
}

static void lpc11u6x_uartx_irq_tx_disable(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	cfg->base->int_en_clr = LPC11U6X_UARTX_INT_EN_CLR_TXRDYCLR;
}

static int lpc11u6x_uartx_irq_tx_ready(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	return (cfg->base->stat & LPC11U6X_UARTX_STAT_TXRDY) &&
		(cfg->base->int_en_set & LPC11U6X_UARTX_INT_EN_SET_TXRDYEN);
}

static int lpc11u6x_uartx_irq_tx_complete(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	return (cfg->base->stat & LPC11U6X_UARTX_STAT_TXIDLE) != 0;
}

static void lpc11u6x_uartx_irq_rx_enable(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	cfg->base->int_en_set = (cfg->base->int_en_set &
				 LPC11U6X_UARTX_INT_EN_SET_MASK) |
		LPC11U6X_UARTX_INT_EN_SET_RXRDYEN;
}

static void lpc11u6x_uartx_irq_rx_disable(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	cfg->base->int_en_clr = LPC11U6X_UARTX_INT_EN_CLR_RXRDYCLR;
}

static int lpc11u6x_uartx_irq_rx_ready(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	return (cfg->base->stat & LPC11U6X_UARTX_STAT_RXRDY) &&
		(cfg->base->int_en_set & LPC11U6X_UARTX_INT_EN_SET_RXRDYEN);
}

static void lpc11u6x_uartx_irq_err_enable(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	cfg->base->int_en_set = (cfg->base->int_en_set &
				 LPC11U6X_UARTX_INT_EN_SET_MASK) |
		(LPC11U6X_UARTX_INT_EN_SET_RXRDYEN |
		 LPC11U6X_UARTX_INT_EN_SET_FRAMERREN |
		 LPC11U6X_UARTX_INT_EN_SET_PARITYERREN);
}

static void lpc11u6x_uartx_irq_err_disable(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	cfg->base->int_en_clr = LPC11U6X_UARTX_INT_EN_CLR_OVERRUNCLR |
		LPC11U6X_UARTX_INT_EN_CLR_FRAMERRCLR |
		LPC11U6X_UARTX_INT_EN_CLR_PARITYERRCLR;
}

static int lpc11u6x_uartx_irq_is_pending(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;

	if ((cfg->base->stat & LPC11U6X_UARTX_STAT_RXRDY) &&
	    (cfg->base->int_stat & LPC11U6X_UARTX_INT_STAT_RXRDY)) {
		return 1;
	}
	if ((cfg->base->stat & LPC11U6X_UARTX_STAT_TXRDY) &&
	    cfg->base->int_stat & LPC11U6X_UARTX_INT_STAT_TXRDY) {
		return 1;
	}
	if (cfg->base->stat & (LPC11U6X_UARTX_STAT_OVERRUNINT |
			       LPC11U6X_UARTX_STAT_FRAMERRINT |
			       LPC11U6X_UARTX_STAT_PARITYERRINT)) {
		return 1;
	}
	return 0;
}

static int lpc11u6x_uartx_irq_update(const struct device *dev)
{
	return 1;
}

static void lpc11u6x_uartx_irq_callback_set(const struct device *dev,
					    uart_irq_callback_user_data_t cb,
					    void *user_data)
{
	struct lpc11u6x_uartx_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;
}

static void lpc11u6x_uartx_isr(const struct device *dev)
{
	struct lpc11u6x_uartx_data *data = dev->data;

	if (data->cb) {
		data->cb(dev, data->cb_data);
	}
}

static void lpc11u6x_uartx_shared_isr(const void *arg)
{
	struct lpc11u6x_uartx_shared_irq *shared_irq =
		(struct lpc11u6x_uartx_shared_irq *)arg;
	int i;

	for (i = 0; i < ARRAY_SIZE(shared_irq->devices); i++) {
		if (shared_irq->devices[i]) {
			lpc11u6x_uartx_isr(shared_irq->devices[i]);
		}
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int lpc11u6x_uartx_init(const struct device *dev)
{
	const struct lpc11u6x_uartx_config *cfg = dev->config;
	struct lpc11u6x_uartx_data *data = dev->data;
	int err;

	/* Apply default pin control state to select RX and TX pins */
	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	clock_control_on(cfg->clock_dev, (clock_control_subsys_t) cfg->clkid);

	/* Configure baudrate, parity and stop bits */
	lpc11u6x_uartx_config_baud(cfg, cfg->baudrate);
	cfg->base->cfg = LPC11U6X_UARTX_CFG_DATALEN_8BIT; /* 8N1 */

	data->baudrate = cfg->baudrate;
	data->parity = UART_CFG_PARITY_NONE;
	data->stop_bits = UART_CFG_STOP_BITS_1;
	data->data_bits = UART_CFG_DATA_BITS_8;
	data->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	/* Enable UART */
	cfg->base->cfg = (cfg->base->cfg & LPC11U6X_UARTX_CFG_MASK) |
		LPC11U6X_UARTX_CFG_ENABLE;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) || \
	DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)
	lpc11u6x_uartx_isr_config_1(dev);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) ||
	* DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)
	*/

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) || \
	DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay)
	lpc11u6x_uartx_isr_config_2(dev);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) ||
	* DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay)
	*/
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	return 0;
}

static const struct uart_driver_api uartx_api = {
	.poll_in = lpc11u6x_uartx_poll_in,
	.poll_out = lpc11u6x_uartx_poll_out,
	.err_check = lpc11u6x_uartx_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = lpc11u6x_uartx_configure,
	.config_get = lpc11u6x_uartx_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = lpc11u6x_uartx_fifo_fill,
	.fifo_read = lpc11u6x_uartx_fifo_read,
	.irq_tx_enable = lpc11u6x_uartx_irq_tx_enable,
	.irq_tx_disable = lpc11u6x_uartx_irq_tx_disable,
	.irq_tx_ready = lpc11u6x_uartx_irq_tx_ready,
	.irq_tx_complete = lpc11u6x_uartx_irq_tx_complete,
	.irq_rx_enable = lpc11u6x_uartx_irq_rx_enable,
	.irq_rx_disable = lpc11u6x_uartx_irq_rx_disable,
	.irq_rx_ready = lpc11u6x_uartx_irq_rx_ready,
	.irq_err_enable = lpc11u6x_uartx_irq_err_enable,
	.irq_err_disable = lpc11u6x_uartx_irq_err_disable,
	.irq_is_pending = lpc11u6x_uartx_irq_is_pending,
	.irq_update = lpc11u6x_uartx_irq_update,
	.irq_callback_set = lpc11u6x_uartx_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};


#define LPC11U6X_UARTX_INIT(idx)                                              \
PINCTRL_DT_DEFINE(DT_NODELABEL(uart##idx));                                   \
									      \
static const struct lpc11u6x_uartx_config uart_cfg_##idx = {	              \
	.base = (struct lpc11u6x_uartx_regs *)                                \
	DT_REG_ADDR(DT_NODELABEL(uart##idx)),			              \
	.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(uart##idx))),  \
	.clkid = DT_PHA_BY_IDX(DT_NODELABEL(uart##idx), clocks, 0, clkid),    \
	.pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(uart##idx)),         \
	.baudrate = DT_PROP(DT_NODELABEL(uart##idx), current_speed),	      \
};									      \
									      \
static struct lpc11u6x_uartx_data uart_data_##idx;                            \
									      \
DEVICE_DT_DEFINE(DT_NODELABEL(uart##idx), 				      \
		    &lpc11u6x_uartx_init, NULL,				      \
		    &uart_data_##idx, &uart_cfg_##idx,			      \
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		      \
		    &uartx_api)

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
LPC11U6X_UARTX_INIT(1);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
LPC11U6X_UARTX_INIT(2);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay)
LPC11U6X_UARTX_INIT(3);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)
LPC11U6X_UARTX_INIT(4);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay) */

#if CONFIG_UART_INTERRUPT_DRIVEN &&				\
	(DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) ||	\
	 DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay))

struct lpc11u6x_uartx_shared_irq lpc11u6x_uartx_shared_irq_info_1 = {
	.devices = {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
		DEVICE_DT_GET(DT_NODELABEL(uart1)),
#else
		NULL,
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)
		DEVICE_DT_GET(DT_NODELABEL(uart4)),
#else
		NULL,
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay) */
	},
};

static void lpc11u6x_uartx_isr_config_1(const struct device *dev)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(uart1)),
		    DT_IRQ(DT_NODELABEL(uart1), priority),
		    lpc11u6x_uartx_shared_isr,
		    &lpc11u6x_uartx_shared_irq_info_1,
		    0);
	irq_enable(DT_IRQN(DT_NODELABEL(uart1)));
#else
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(uart4)),
		    DT_IRQ(DT_NODELABEL(uart4), priority),
		    lpc11u6x_uartx_shared_isr,
		    &lpc11u6x_uartx_shared_irq_info_1,
		    0);
	irq_enable(DT_IRQN(DT_NODELABEL(uart4)));
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN &&
	* (DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) ||
	* DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay))
	*/

#if CONFIG_UART_INTERRUPT_DRIVEN && \
	(DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) || \
	 DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay))
struct lpc11u6x_uartx_shared_irq lpc11u6x_uartx_shared_irq_info_2 = {
	.devices = {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
		DEVICE_DT_GET(DT_NODELABEL(uart2)),
#else
		NULL,
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay)
		DEVICE_DT_GET(DT_NODELABEL(uart3)),
#else
		NULL,
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay) */
	},
};

static void lpc11u6x_uartx_isr_config_2(const struct device *dev)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(uart2)),
		    DT_IRQ(DT_NODELABEL(uart2), priority),
		    lpc11u6x_uartx_shared_isr,
		    &lpc11u6x_uartx_shared_irq_info_2,
		    0);
	irq_enable(DT_IRQN(DT_NODELABEL(uart2)));
#else
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(uart3)),
		    DT_IRQ(DT_NODELABEL(uart3), priority),
		    lpc11u6x_uartx_shared_isr,
		    &lpc11u6x_uartx_shared_irq_info_2,
		    0);
	irq_enable(DT_IRQN(DT_NODELABEL(uart3)));
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN &&
	* (DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) ||
	* DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay))
	*/
#endif  /* DT_NODE_EXISTS(DT_NODELABEL(uart1) ||
	 * DT_NODE_EXISTS(DT_NODELABEL(uart2) ||
	 * DT_NODE_EXISTS(DT_NODELABEL(uart3) ||
	 * DT_NODE_EXISTS(DT_NODELABEL(uart4)
	 */
