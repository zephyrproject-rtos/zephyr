/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <em_usart.h>

#define DT_DRV_COMPAT silabs_usart_uart

/**
 * @brief Config struct for UART
 */

struct uart_silabs_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	USART_TypeDef *base;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_silabs_data {
	struct uart_config *uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int uart_silabs_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_StatusGet(config->base);

	if (flags & USART_STATUS_RXDATAV) {
		*c = USART_Rx(config->base);
		return 0;
	}

	return -1;
}

static void uart_silabs_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_silabs_config *config = dev->config;

	USART_Tx(config->base, c);
}

static int uart_silabs_err_check(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_IntGet(config->base);
	int err = 0;

	if (flags & USART_IF_RXOF) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & USART_IF_PERR) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & USART_IF_FERR) {
		err |= UART_ERROR_FRAMING;
	}

	USART_IntClear(config->base, USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_silabs_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_silabs_config *config = dev->config;
	int i = 0;

	while ((i < len) && (config->base->STATUS & USART_STATUS_TXBL)) {
		config->base->TXDATA = tx_data[i++];
	}

	return i;
}

static int uart_silabs_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	const struct uart_silabs_config *config = dev->config;
	int i = 0;

	while ((i < len) && (config->base->STATUS & USART_STATUS_RXDATAV)) {
		rx_data[i++] = (uint8_t)config->base->RXDATA;
	}

	return i;
}

static void uart_silabs_irq_tx_enable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntEnable(config->base, USART_IEN_TXBL | USART_IEN_TXC);
}

static void uart_silabs_irq_tx_disable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntDisable(config->base, USART_IEN_TXBL | USART_IEN_TXC);
}

static int uart_silabs_irq_tx_complete(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_IntGet(config->base);

	USART_IntClear(config->base, USART_IF_TXC);

	return !!(flags & USART_IF_TXC);
}

static int uart_silabs_irq_tx_ready(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_IntGetEnabled(config->base);

	return !!(flags & USART_IF_TXBL);
}

static void uart_silabs_irq_rx_enable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntEnable(config->base, USART_IEN_RXDATAV);
}

static void uart_silabs_irq_rx_disable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntDisable(config->base, USART_IEN_RXDATAV);
}

static int uart_silabs_irq_rx_full(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_IntGet(config->base);

	return !!(flags & USART_IF_RXDATAV);
}

static int uart_silabs_irq_rx_ready(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	return (config->base->IEN & USART_IEN_RXDATAV) && uart_silabs_irq_rx_full(dev);
}

static void uart_silabs_irq_err_enable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntEnable(config->base, USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR);
}

static void uart_silabs_irq_err_disable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntDisable(config->base, USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR);
}

static int uart_silabs_irq_is_pending(const struct device *dev)
{
	return uart_silabs_irq_tx_ready(dev) || uart_silabs_irq_rx_ready(dev);
}

static int uart_silabs_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_silabs_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_silabs_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_silabs_isr(const struct device *dev)
{
	struct uart_silabs_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static inline USART_Parity_TypeDef uart_silabs_cfg2ll_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return usartOddParity;
	case UART_CFG_PARITY_EVEN:
		return usartEvenParity;
	case UART_CFG_PARITY_NONE:
	default:
		return usartNoParity;
	}
}

static inline enum uart_config_parity uart_silabs_ll2cfg_parity(USART_Parity_TypeDef parity)
{
	switch (parity) {
	case usartOddParity:
		return UART_CFG_PARITY_ODD;
	case usartEvenParity:
		return UART_CFG_PARITY_EVEN;
	case usartNoParity:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static inline USART_Stopbits_TypeDef uart_silabs_cfg2ll_stopbits(enum uart_config_stop_bits sb)
{
	switch (sb) {
	case UART_CFG_STOP_BITS_0_5:
		return usartStopbits0p5;
	case UART_CFG_STOP_BITS_1:
		return usartStopbits1;
	case UART_CFG_STOP_BITS_2:
		return usartStopbits2;
	case UART_CFG_STOP_BITS_1_5:
		return usartStopbits1p5;
	default:
		return usartStopbits1;
	}
}

static inline enum uart_config_stop_bits uart_silabs_ll2cfg_stopbits(USART_Stopbits_TypeDef sb)
{
	switch (sb) {
	case usartStopbits0p5:
		return UART_CFG_STOP_BITS_0_5;
	case usartStopbits1:
		return UART_CFG_STOP_BITS_1;
	case usartStopbits1p5:
		return UART_CFG_STOP_BITS_1_5;
	case usartStopbits2:
		return UART_CFG_STOP_BITS_2;
	default:
		return UART_CFG_STOP_BITS_1;
	}
}

static inline USART_Databits_TypeDef uart_silabs_cfg2ll_databits(enum uart_config_data_bits db,
								   enum uart_config_parity p)
{
	switch (db) {
	case UART_CFG_DATA_BITS_7:
		if (p == UART_CFG_PARITY_NONE) {
			return usartDatabits7;
		} else {
			return usartDatabits8;
		}
	case UART_CFG_DATA_BITS_9:
		return usartDatabits9;
	case UART_CFG_DATA_BITS_8:
	default:
		if (p == UART_CFG_PARITY_NONE) {
			return usartDatabits8;
		} else {
			return usartDatabits9;
		}
		return usartDatabits8;
	}
}

static inline enum uart_config_data_bits uart_silabs_ll2cfg_databits(USART_Databits_TypeDef db,
								    USART_Parity_TypeDef p)
{
	switch (db) {
	case usartDatabits7:
		if (p == usartNoParity) {
			return UART_CFG_DATA_BITS_7;
		} else {
			return UART_CFG_DATA_BITS_6;
		}
	case usartDatabits9:
		if (p == usartNoParity) {
			return UART_CFG_DATA_BITS_9;
		} else {
			return UART_CFG_DATA_BITS_8;
		}
	case usartDatabits8:
	default:
		if (p == usartNoParity) {
			return UART_CFG_DATA_BITS_8;
		} else {
			return UART_CFG_DATA_BITS_7;
		}
	}
}

static inline USART_HwFlowControl_TypeDef uart_silabs_cfg2ll_hwctrl(
	enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return usartHwFlowControlCtsAndRts;
	}

	return usartHwFlowControlNone;
}

static inline enum uart_config_flow_control uart_silabs_ll2cfg_hwctrl(
	USART_HwFlowControl_TypeDef fc)
{
	if (fc == usartHwFlowControlCtsAndRts) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_silabs_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	const struct uart_silabs_config *config = dev->config;
	USART_TypeDef *base = config->base;
	struct uart_silabs_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;
	USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;

	if ((cfg->parity == UART_CFG_PARITY_MARK) ||
	    (cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOSYS;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_DTR_DSR ||
	    cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) {
		return -ENOSYS;
	}

	*uart_cfg = *cfg;
	usartInit.baudrate = uart_cfg->baudrate;
	usartInit.parity = uart_silabs_cfg2ll_parity(uart_cfg->parity);
	usartInit.stopbits = uart_silabs_cfg2ll_stopbits(uart_cfg->stop_bits);
	usartInit.databits = uart_silabs_cfg2ll_databits(uart_cfg->data_bits,
								 uart_cfg->parity);
	usartInit.hwFlowControl = uart_silabs_cfg2ll_hwctrl(uart_cfg->flow_ctrl);

	USART_Enable(base, usartDisable);

	USART_InitAsync(base, &usartInit);

	USART_Enable(base, usartEnable);

	return 0;
};

static int uart_silabs_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_silabs_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;

	cfg->baudrate = uart_cfg->baudrate;
	cfg->parity = uart_cfg->parity;
	cfg->stop_bits = uart_cfg->stop_bits;
	cfg->data_bits = uart_cfg->data_bits;
	cfg->flow_ctrl = uart_cfg->flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_silabs_init(const struct device *dev)
{
	int err;
	const struct uart_silabs_config *config = dev->config;
	const struct uart_silabs_data *data = dev->data;
	const struct uart_config *uart_cfg = data->uart_cfg;
	USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;

	/* The peripheral and gpio clock are already enabled from soc and gpio driver */
	/* Enable USART clock */
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	usartInit.baudrate = uart_cfg->baudrate;
	usartInit.parity = uart_silabs_cfg2ll_parity(uart_cfg->parity);
	usartInit.stopbits = uart_silabs_cfg2ll_stopbits(uart_cfg->stop_bits);
	usartInit.databits = uart_silabs_cfg2ll_databits(uart_cfg->data_bits, uart_cfg->parity);
	usartInit.hwFlowControl =
		uart_cfg->flow_ctrl ? usartHwFlowControlCtsAndRts : usartHwFlowControlNone;

	USART_InitAsync(config->base, &usartInit);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int uart_silabs_pm_action(const struct device *dev, enum pm_device_action action)
{
	__maybe_unused const struct uart_silabs_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Wait for TX FIFO to flush before suspending */
		while (!(USART_StatusGet(config->base) & USART_STATUS_TXIDLE)) {
		}
		break;

	case PM_DEVICE_ACTION_RESUME:
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static DEVICE_API(uart, uart_silabs_driver_api) = {
	.poll_in = uart_silabs_poll_in,
	.poll_out = uart_silabs_poll_out,
	.err_check = uart_silabs_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_silabs_configure,
	.config_get = uart_silabs_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_silabs_fifo_fill,
	.fifo_read = uart_silabs_fifo_read,
	.irq_tx_enable = uart_silabs_irq_tx_enable,
	.irq_tx_disable = uart_silabs_irq_tx_disable,
	.irq_tx_complete = uart_silabs_irq_tx_complete,
	.irq_tx_ready = uart_silabs_irq_tx_ready,
	.irq_rx_enable = uart_silabs_irq_rx_enable,
	.irq_rx_disable = uart_silabs_irq_rx_disable,
	.irq_rx_ready = uart_silabs_irq_rx_ready,
	.irq_err_enable = uart_silabs_irq_err_enable,
	.irq_err_disable = uart_silabs_irq_err_disable,
	.irq_is_pending = uart_silabs_irq_is_pending,
	.irq_update = uart_silabs_irq_update,
	.irq_callback_set = uart_silabs_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define SILABS_USART_IRQ_HANDLER_FUNC(idx) .irq_config_func = usart_silabs_config_func_##idx,
#define SILABS_USART_IRQ_HANDLER(idx)                                                              \
	static void usart_silabs_config_func_##idx(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority), uart_silabs_isr,               \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority), uart_silabs_isr,               \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));                                     \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));                                     \
	}
#else
#define SILABS_USART_IRQ_HANDLER_FUNC(idx)
#define SILABS_USART_IRQ_HANDLER(idx)
#endif

#define SILABS_USART_INIT(idx)                                                                     \
	SILABS_USART_IRQ_HANDLER(idx);                                                             \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	PM_DEVICE_DT_INST_DEFINE(idx, uart_silabs_pm_action);                                      \
                                                                                                   \
	static struct uart_config uart_cfg_##idx = {                                               \
		.baudrate = DT_INST_PROP(idx, current_speed),                                      \
		.parity = DT_INST_ENUM_IDX(idx, parity),                                           \
		.stop_bits = DT_INST_ENUM_IDX(idx, stop_bits),                                     \
		.data_bits = DT_INST_ENUM_IDX(idx, data_bits),                                     \
		.flow_ctrl = DT_INST_PROP(idx, hw_flow_control) ? UART_CFG_FLOW_CTRL_RTS_CTS       \
								: UART_CFG_FLOW_CTRL_NONE,         \
	};                                                                                         \
                                                                                                   \
	static const struct uart_silabs_config uart_silabs_cfg_##idx = {                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.base = (USART_TypeDef *)DT_INST_REG_ADDR(idx),                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
		SILABS_USART_IRQ_HANDLER_FUNC(idx)                                                 \
	};                                                                                         \
                                                                                                   \
	static struct uart_silabs_data uart_silabs_data_##idx = {                                  \
		.uart_cfg = &uart_cfg_##idx,                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, uart_silabs_init, PM_DEVICE_DT_INST_GET(idx),                   \
			      &uart_silabs_data_##idx, &uart_silabs_cfg_##idx, PRE_KERNEL_1,       \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_silabs_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SILABS_USART_INIT)
