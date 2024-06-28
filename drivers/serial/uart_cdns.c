/*
 * Copyright 2022 Meta Platforms, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdns_uart

/**
 * @brief Serial Driver for cadence UART IP6528
 */

#include "uart_cdns.h"

#define DEV_UART(dev) ((struct uart_cdns_regs *) \
			((const struct uart_cdns_device_config *const)(dev)->config)->port)

/** Check if tx FIFO is full */
bool uart_cdns_is_tx_fifo_full(struct uart_cdns_regs *uart_regs)
{
	return ((uart_regs->channel_status & CSR_TFUL_MASK) != 0);
}

/** Check if tx FIFO is empty */
bool uart_cdns_is_tx_fifo_empty(struct uart_cdns_regs *uart_regs)
{
	return ((uart_regs->channel_status & CSR_TEMPTY_MASK) != 0);
}

/** Check if rx FIFO is empty */
bool uart_cdns_is_rx_fifo_empty(struct uart_cdns_regs *uart_regs)
{
	return ((uart_regs->channel_status & CSR_REMPTY_MASK) != 0);
}

/** Set the baudrate */
void uart_cdns_set_baudrate(struct uart_cdns_regs *uart_regs,
			       const struct uart_cdns_device_config *const dev_cfg,
			       uint32_t baud_rate)
{
	uart_regs->baud_rate_div = dev_cfg->bdiv;

	/*
	 * baud_rate is calculated by hardware as below
	 *
	 * baud_rate = sel_clk / ((bdiv + 1) * clock_divisor)
	 * i.e. clock_divisor = sel_clk / ((bdiv + 1) * baud_rate)
	 *
	 * However to round to a nearest integer we use this:
	 * clock_divisor = (sel_clk + ((bdiv + 1) * baud_rate) / 2) / ((bdiv + 1) * baud_rate)
	 */
	uart_regs->baud_rate_gen = (dev_cfg->sys_clk_freq + ((dev_cfg->bdiv + 1) * baud_rate) / 2) /
				   ((dev_cfg->bdiv + 1) * baud_rate);
}

static void uart_cdns_poll_out(const struct device *dev, unsigned char out_char)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);
	/* Wait while TX FIFO is full */
	while (uart_cdns_is_tx_fifo_full(uart_regs)) {
	}
	uart_regs->rx_tx_fifo = (uint32_t)out_char;
}

/** @brief Poll the device for input. */
int uart_cdns_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	if (uart_cdns_is_rx_fifo_empty(uart_regs)) {
		return -1;
	}

	*p_char = (unsigned char)(uart_regs->rx_tx_fifo & RXDATA_MASK);
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_cdns_fill_fifo(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	int i = 0;

	for (i = 0; i < len && (!uart_cdns_is_tx_fifo_full(uart_regs)); i++) {
		uart_regs->rx_tx_fifo = tx_data[i];
		while (!uart_cdns_is_tx_fifo_empty(uart_regs)) {
		}
	}
	return i;
}

static int uart_cdns_read_fifo(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	int i = 0;

	for (i = 0; i < size && (!uart_cdns_is_rx_fifo_empty(uart_regs)); i++) {
		rx_data[i] = uart_regs->rx_tx_fifo;
	}
	if (i > 0) {
		uart_regs->ctrl |= CTRL_RSTTO_MASK;
	}
	return i;
}

void uart_cdns_enable_tx_irq(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	uart_regs->intr_enable |= CSR_TTRIG_MASK;
}

void uart_cdns_disable_tx_irq(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	uart_regs->intr_disable |= CSR_TTRIG_MASK;
}

static int uart_cdns_irq_tx_ready(const struct device *dev)
{
	return !uart_cdns_is_tx_fifo_full(DEV_UART(dev));
}

static int uart_cdns_irq_tx_complete(const struct device *dev)
{
	return uart_cdns_is_tx_fifo_empty(DEV_UART(dev));
}

void uart_cdns_enable_rx_irq(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	uart_regs->rx_timeout = DEFAULT_RTO_PERIODS_FACTOR;
	uart_regs->intr_enable |= (CSR_RTRIG_MASK | CSR_RBRK_MASK | CSR_TOUT_MASK);
}

/** Disable RX UART interrupt */
void uart_cdns_disable_rx_irq(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	uart_regs->intr_disable |= (CSR_RTRIG_MASK | CSR_RBRK_MASK | CSR_TOUT_MASK);
}

static int uart_cdns_irq_rx_ready(const struct device *dev)
{
	return !uart_cdns_is_rx_fifo_empty(DEV_UART(dev));
}

static void uart_cdns_enable_irq_err(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	uart_regs->intr_enable |=
		(CSR_TOVR_MASK | CSR_TOUT_MASK | CSR_PARE_MASK | CSR_FRAME_MASK | CSR_ROVR_MASK);
}

static void uart_cdns_disable_irq_err(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	uart_regs->intr_disable |=
		(CSR_TOVR_MASK | CSR_TOUT_MASK | CSR_PARE_MASK | CSR_FRAME_MASK | CSR_ROVR_MASK);
}

static int uart_cdns_is_irq_pending(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	return (uart_regs->channel_intr_status != 0);
}

/** Check for IRQ updates */
static int uart_cdns_update_irq(const struct device *dev)
{
	return 1;
}

/** Set the callback function pointer for IRQ. */
void uart_cdns_set_irq_callback(const struct device *dev, uart_irq_callback_user_data_t cb,
				   void *cb_data)
{
	struct uart_cdns_data *data;

	data = dev->data;
	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_cdns_irq_handler(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);
	uint32_t key = irq_lock();
	uint32_t isr_status;
	struct uart_cdns_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}

	/* clear events */
	/* need to make local copy of the status */
	isr_status = uart_regs->channel_intr_status;

	irq_unlock(key);
}

#endif

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static enum uart_config_parity parity_from_cdns(enum csr_parity_val parity)
{
	switch (parity) {
	case EVEN_PARITY_VAL:
		return UART_CFG_PARITY_EVEN;
	case ODD_PARITY_VAL:
		return UART_CFG_PARITY_ODD;
	case SPACE_PARITY_VAL:
		return UART_CFG_PARITY_SPACE;
	case MARK_PARITY_VAL:
		return UART_CFG_PARITY_MARK;
	case NO_PARITY_VAL:
		return UART_CFG_PARITY_NONE;
	}
	return UART_CFG_PARITY_NONE;
}

static int32_t parity_to_cdns(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		return NO_PARITY_VAL;
	case UART_CFG_PARITY_ODD:
		return ODD_PARITY_VAL;
	case UART_CFG_PARITY_EVEN:
		return EVEN_PARITY_VAL;
	case UART_CFG_PARITY_MARK:
		return MARK_PARITY_VAL;
	case UART_CFG_PARITY_SPACE:
		return SPACE_PARITY_VAL;
	}
	return -1;
}

static int32_t stop_bits_to_cdns(enum uart_config_stop_bits stop_bits)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_0_5:
		return -1;
	case UART_CFG_STOP_BITS_1:
		return 0;
	case UART_CFG_STOP_BITS_1_5:
		return 1;
	case UART_CFG_STOP_BITS_2:
		return 2;
	}
	return -1;
}

static int32_t data_bits_to_cdns(enum uart_config_data_bits data_bits)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		return -1;
	case UART_CFG_DATA_BITS_6:
		return 3;
	case UART_CFG_DATA_BITS_7:
		return 2;
	case UART_CFG_DATA_BITS_8:
		return 0;
	case UART_CFG_DATA_BITS_9:
		return -1;
	}
	return -1;
}

static int32_t flow_ctrl_to_cdns(enum uart_config_flow_control flow_ctrl)
{
	switch (flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		return 0;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		return 1;
	case UART_CFG_FLOW_CTRL_DTR_DSR:
	case UART_CFG_FLOW_CTRL_RS485:
		return -1;
	}
	return -1;
}

static void uart_cdns_config_init(const struct device *dev)
{
	const struct uart_cdns_device_config *const dev_cfg = dev->config;
	struct uart_cdns_data *dev_data = dev->data;
	dev_data->cfg.baudrate = dev_cfg->baud_rate;
	dev_data->cfg.parity = parity_from_cdns(dev_cfg->parity);
	dev_data->cfg.stop_bits = UART_CFG_STOP_BITS_1;
	dev_data->cfg.data_bits = UART_CFG_DATA_BITS_8;
	dev_data->cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
}

static int uart_cdns_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_cdns_data *dev_data = dev->data;
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);

	int32_t parity = parity_to_cdns(cfg->parity);
	int32_t stop_bits = stop_bits_to_cdns(cfg->stop_bits);
	int32_t data_bits = data_bits_to_cdns(cfg->data_bits);
	int32_t flow_ctrl = flow_ctrl_to_cdns(cfg->flow_ctrl);
	if (parity < 0 || stop_bits < 0 || data_bits < 0 || flow_ctrl < 0) {
		return -EINVAL;
	}

	uart_cdns_set_baudrate(uart_regs, dev->config, cfg->baudrate);

	uint32_t mode = uart_regs->mode;
	mode &= ~(MODE_NBSTOP_MASK | MODE_PAR_MASK | MODE_CHRL_MASK);
	mode |= (stop_bits << MODE_NBSTOP_SHIFT) | (parity << MODE_PAR_SHIFT) | (data_bits << MODE_CHRL_SHIFT);
	uart_regs->mode = mode;

	uint32_t modem_ctrl = uart_regs->modem_control;
	modem_ctrl &= ~MCR_FCM_MASK;
	modem_ctrl |= (flow_ctrl << MCR_FCM_SHIFT);
	uart_regs->modem_control = modem_ctrl;

	uart_regs->ctrl |= CTRL_TXRES_MASK | CTRL_RXRES_MASK;

	dev_data->cfg = *cfg;
	return 0;
}

static int uart_cdns_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_cdns_data *dev_data = dev->data;
	*cfg = dev_data->cfg;
	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static const struct uart_driver_api uart_cdns_driver_api = {
	.poll_in = uart_cdns_poll_in,
	.poll_out = uart_cdns_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_cdns_configure,
	.config_get = uart_cdns_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_cdns_fill_fifo,
	.fifo_read = uart_cdns_read_fifo,
	.irq_tx_enable = uart_cdns_enable_tx_irq,
	.irq_tx_disable = uart_cdns_disable_tx_irq,
	.irq_tx_ready = uart_cdns_irq_tx_ready,
	.irq_tx_complete = uart_cdns_irq_tx_complete,
	.irq_rx_enable = uart_cdns_enable_rx_irq,
	.irq_rx_disable = uart_cdns_disable_rx_irq,
	.irq_rx_ready = uart_cdns_irq_rx_ready,
	.irq_err_enable = uart_cdns_enable_irq_err,
	.irq_err_disable = uart_cdns_disable_irq_err,
	.irq_is_pending = uart_cdns_is_irq_pending,
	.irq_update = uart_cdns_update_irq,
	.irq_callback_set = uart_cdns_set_irq_callback
#endif
};

/** Initialize the UART */
static int uart_cdns_init(const struct device *dev)
{
	struct uart_cdns_regs *uart_regs = DEV_UART(dev);
	const struct uart_cdns_device_config *const dev_cfg = dev->config;

	/* Reset RX and TX path */
	uart_regs->ctrl = (CTRL_RXRES_MASK | CTRL_TXRES_MASK);

	/* Disable TX and RX channels */
	uart_regs->ctrl = (CTRL_STPBRK_MASK | CTRL_TXDIS_MASK | CTRL_RXDIS_MASK);

	/* Configure Baud rate */
	uart_cdns_set_baudrate(uart_regs, dev_cfg, dev_cfg->baud_rate);

	/* Configure the mode */
	uart_regs->mode = (SET_VAL32(MODE_WSIZE, 1) | SET_VAL32(MODE_UCLKEN, 1) |
					  SET_VAL32(MODE_PAR, dev_cfg->parity));

	/* Disable all interrupts */
	uart_regs->intr_disable = 0xFFFFFFFF;

	/* Enable TX and RX Channels */
	uart_regs->ctrl = (CTRL_TXEN_MASK | CTRL_RXEN_MASK | CTRL_STPBRK_MASK);

	if (dev_cfg->cfg_func) {
		/* Setup IRQ handler */
		dev_cfg->cfg_func();
	}

#if CONFIG_UART_USE_RUNTIME_CONFIGURE
	uart_cdns_config_init(dev);
#endif

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define UART_CDNS_IRQ_CFG_FUNC(n)								   \
	static void uart_cdns_irq_cfg_func_##n(void)						   \
	{											   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_cdns_irq_handler,	   \
			    DEVICE_DT_INST_GET(n), 0);						   \
												   \
		irq_enable(DT_INST_IRQN(n));							   \
	}

#define UART_CDNS_IRQ_CFG_FUNC_INIT(n) .cfg_func = uart_cdns_irq_cfg_func_##n,

#else

#define UART_CDNS_IRQ_CFG_FUNC(n)
#define UART_CDNS_IRQ_CFG_FUNC_INIT(n)

#endif

#define UART_CDNS_INIT(n)									   \
	static struct uart_cdns_data uart_cdns_data_##n;					   \
												   \
	UART_CDNS_IRQ_CFG_FUNC(n)								   \
												   \
	static const struct uart_cdns_device_config uart_cdns_dev_cfg_##n = {			   \
		.port = DT_INST_REG_ADDR(n),							   \
		.bdiv = DT_INST_PROP(n, bdiv),							   \
		.sys_clk_freq = DT_INST_PROP(n, clock_frequency),				   \
		.baud_rate = DT_INST_PROP(n, current_speed),					   \
		.parity = CDNS_PARTITY_MAP(DT_ENUM_IDX(DT_DRV_INST(n), parity)),		   \
		UART_CDNS_IRQ_CFG_FUNC_INIT(n)};						   \
												   \
	DEVICE_DT_INST_DEFINE(n, uart_cdns_init, NULL, &uart_cdns_data_##n,			   \
			      &uart_cdns_dev_cfg_##n, PRE_KERNEL_1,				   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_cdns_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_CDNS_INIT)
