/*
 * Copyright (c) 2024, Yishai Jaffe
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_eusart_uart

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <em_eusart.h>

struct eusart_config {
	EUSART_TypeDef *eusart;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct eusart_data {
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int eusart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct eusart_config *config = dev->config;

	if (EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL) {
		*c = EUSART_Rx(config->eusart);
		return 0;
	}

	return -1;
}

static void eusart_poll_out(const struct device *dev, unsigned char c)
{
	const struct eusart_config *config = dev->config;

	/* EUSART_Tx function already waits for the transmit buffer being empty
	 * and waits for the bus to be free to transmit.
	 */
	EUSART_Tx(config->eusart, c);
}

static int eusart_err_check(const struct device *dev)
{
	const struct eusart_config *config = dev->config;
	uint32_t flags = EUSART_IntGet(config->eusart);
	int err = 0;

	if (flags & EUSART_IF_RXOF) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & EUSART_IF_PERR) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & EUSART_IF_FERR) {
		err |= UART_ERROR_FRAMING;
	}

	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int eusart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct eusart_config *config = dev->config;
	int i = 0;

	while ((i < len) && (EUSART_StatusGet(config->eusart) & EUSART_STATUS_TXFL)) {
		config->eusart->TXDATA = (uint32_t)tx_data[i++];
	}

	if (!(EUSART_StatusGet(config->eusart) & EUSART_STATUS_TXFL)) {
		EUSART_IntClear(config->eusart, EUSART_IF_TXFL);
	}

	return i;
}

static int eusart_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	const struct eusart_config *config = dev->config;
	int i = 0;

	while ((i < len) && (EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL)) {
		rx_data[i++] = (uint8_t)config->eusart->RXDATA;
	}

	if (!(EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL)) {
		EUSART_IntClear(config->eusart, EUSART_IF_RXFL);
	}

	return i;
}

static void eusart_irq_tx_enable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntClear(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
	EUSART_IntEnable(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
}

static void eusart_irq_tx_disable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
	EUSART_IntClear(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
}

static int eusart_irq_tx_complete(const struct device *dev)
{
	const struct eusart_config *config = dev->config;
	uint32_t flags = EUSART_IntGet(config->eusart);

	EUSART_IntClear(config->eusart, EUSART_IF_TXC);

	return !!(flags & EUSART_IF_TXC);
}

static int eusart_irq_tx_ready(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	return (config->eusart->IEN & EUSART_IEN_TXFL) &&
	       (EUSART_IntGet(config->eusart) & EUSART_IF_TXFL);
}

static void eusart_irq_rx_enable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntClear(config->eusart, EUSART_IEN_RXFL);
	EUSART_IntEnable(config->eusart, EUSART_IEN_RXFL);
}

static void eusart_irq_rx_disable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IEN_RXFL);
	EUSART_IntClear(config->eusart, EUSART_IEN_RXFL);
}

static int eusart_irq_rx_ready(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	return (config->eusart->IEN & EUSART_IEN_RXFL) &&
	       (EUSART_IntGet(config->eusart) & EUSART_IF_RXFL);
}

static void eusart_irq_err_enable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
	EUSART_IntEnable(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
}

static void eusart_irq_err_disable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
}

static int eusart_irq_is_pending(const struct device *dev)
{
	return eusart_irq_tx_ready(dev) || eusart_irq_rx_ready(dev);
}

static int eusart_irq_update(const struct device *dev)
{
	return 1;
}

static void eusart_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				    void *cb_data)
{
	struct eusart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void eusart_isr(const struct device *dev)
{
	struct eusart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static EUSART_Parity_TypeDef eusart_cfg2ll_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return eusartOddParity;
	case UART_CFG_PARITY_EVEN:
		return eusartEvenParity;
	case UART_CFG_PARITY_NONE:
	default:
		return eusartNoParity;
	}
}

static inline enum uart_config_parity eusart_ll2cfg_parity(EUSART_Parity_TypeDef parity)
{
	switch (parity) {
	case eusartOddParity:
		return UART_CFG_PARITY_ODD;
	case eusartEvenParity:
		return UART_CFG_PARITY_EVEN;
	case eusartNoParity:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static EUSART_Stopbits_TypeDef eusart_cfg2ll_stopbits(enum uart_config_stop_bits sb)
{
	switch (sb) {
	case UART_CFG_STOP_BITS_0_5:
		return eusartStopbits0p5;
	case UART_CFG_STOP_BITS_1:
		return eusartStopbits1;
	case UART_CFG_STOP_BITS_2:
		return eusartStopbits2;
	case UART_CFG_STOP_BITS_1_5:
		return eusartStopbits1p5;
	default:
		return eusartStopbits1;
	}
}

static inline enum uart_config_stop_bits eusart_ll2cfg_stopbits(EUSART_Stopbits_TypeDef sb)
{
	switch (sb) {
	case eusartStopbits0p5:
		return UART_CFG_STOP_BITS_0_5;
	case eusartStopbits1:
		return UART_CFG_STOP_BITS_1;
	case eusartStopbits1p5:
		return UART_CFG_STOP_BITS_1_5;
	case eusartStopbits2:
		return UART_CFG_STOP_BITS_2;
	default:
		return UART_CFG_STOP_BITS_1;
	}
}

static EUSART_Databits_TypeDef eusart_cfg2ll_databits(enum uart_config_data_bits db,
						      enum uart_config_parity p)
{
	switch (db) {
	case UART_CFG_DATA_BITS_7:
		if (p == UART_CFG_PARITY_NONE) {
			return eusartDataBits7;
		} else {
			return eusartDataBits8;
		}
	case UART_CFG_DATA_BITS_9:
		return eusartDataBits9;
	case UART_CFG_DATA_BITS_8:
	default:
		if (p == UART_CFG_PARITY_NONE) {
			return eusartDataBits8;
		} else {
			return eusartDataBits9;
		}
		return eusartDataBits8;
	}
}

static inline enum uart_config_data_bits eusart_ll2cfg_databits(EUSART_Databits_TypeDef db,
								EUSART_Parity_TypeDef p)
{
	switch (db) {
	case eusartDataBits7:
		if (p == eusartNoParity) {
			return UART_CFG_DATA_BITS_7;
		} else {
			return UART_CFG_DATA_BITS_6;
		}
	case eusartDataBits9:
		if (p == eusartNoParity) {
			return UART_CFG_DATA_BITS_9;
		} else {
			return UART_CFG_DATA_BITS_8;
		}
	case eusartDataBits8:
	default:
		if (p == eusartNoParity) {
			return UART_CFG_DATA_BITS_8;
		} else {
			return UART_CFG_DATA_BITS_7;
		}
	}
}

static EUSART_HwFlowControl_TypeDef eusart_cfg2ll_hwctrl(enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return eusartHwFlowControlCtsAndRts;
	}

	return eusartHwFlowControlNone;
}

static inline enum uart_config_flow_control eusart_ll2cfg_hwctrl(EUSART_HwFlowControl_TypeDef fc)
{
	if (fc == eusartHwFlowControlCtsAndRts) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

static void eusart_configure_peripheral(const struct device *dev, bool enable)
{
	const struct eusart_config *config = dev->config;
	const struct eusart_data *data = dev->data;
	const struct uart_config *uart_cfg = &data->uart_cfg;
	EUSART_UartInit_TypeDef eusartInit = EUSART_UART_INIT_DEFAULT_HF;
	EUSART_AdvancedInit_TypeDef advancedSettings = EUSART_ADVANCED_INIT_DEFAULT;

	eusartInit.baudrate = uart_cfg->baudrate;
	eusartInit.parity = eusart_cfg2ll_parity(uart_cfg->parity);
	eusartInit.stopbits = eusart_cfg2ll_stopbits(uart_cfg->stop_bits);
	eusartInit.databits = eusart_cfg2ll_databits(uart_cfg->data_bits, uart_cfg->parity);
	advancedSettings.hwFlowControl = eusart_cfg2ll_hwctrl(uart_cfg->flow_ctrl);
	eusartInit.advancedSettings = &advancedSettings;
	eusartInit.enable = eusartDisable;

	EUSART_UartInitHf(config->eusart, &eusartInit);

	if (enable) {
		EUSART_Enable(config->eusart, eusartEnable);
	}
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int eusart_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct eusart_config *config = dev->config;
	EUSART_TypeDef *eusart = config->eusart;
	struct eusart_data *data = dev->data;
	struct uart_config *uart_cfg = &data->uart_cfg;

	if (cfg->parity == UART_CFG_PARITY_MARK || cfg->parity == UART_CFG_PARITY_SPACE) {
		return -ENOSYS;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_DTR_DSR ||
	    cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) {
		return -ENOSYS;
	}

	*uart_cfg = *cfg;

	EUSART_Enable(eusart, eusartDisable);
	eusart_configure_peripheral(dev, true);

	return 0;
};

static int eusart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct eusart_data *data = dev->data;
	struct uart_config *uart_cfg = &data->uart_cfg;

	*cfg = *uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int eusart_init(const struct device *dev)
{
	int err;
	const struct eusart_config *config = dev->config;

	/* The peripheral and gpio clock are already enabled from soc and gpio driver */
	/* Enable EUSART clock */
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	eusart_configure_peripheral(dev, true);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int eusart_pm_action(const struct device *dev, enum pm_device_action action)
{
	__maybe_unused const struct eusart_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Wait for TX FIFO to flush before suspending */
		while (!(EUSART_StatusGet(config->eusart) & EUSART_STATUS_TXIDLE)) {
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

static DEVICE_API(uart, eusart_driver_api) = {
	.poll_in = eusart_poll_in,
	.poll_out = eusart_poll_out,
	.err_check = eusart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = eusart_configure,
	.config_get = eusart_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = eusart_fifo_fill,
	.fifo_read = eusart_fifo_read,
	.irq_tx_enable = eusart_irq_tx_enable,
	.irq_tx_disable = eusart_irq_tx_disable,
	.irq_tx_complete = eusart_irq_tx_complete,
	.irq_tx_ready = eusart_irq_tx_ready,
	.irq_rx_enable = eusart_irq_rx_enable,
	.irq_rx_disable = eusart_irq_rx_disable,
	.irq_rx_ready = eusart_irq_rx_ready,
	.irq_err_enable = eusart_irq_err_enable,
	.irq_err_disable = eusart_irq_err_disable,
	.irq_is_pending = eusart_irq_is_pending,
	.irq_update = eusart_irq_update,
	.irq_callback_set = eusart_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define SILABS_EUSART_IRQ_HANDLER_FUNC(idx) .irq_config_func = eusart_config_func_##idx,
#define SILABS_EUSART_IRQ_HANDLER(idx)                                                             \
	static void eusart_config_func_##idx(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority), eusart_isr,                    \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority), eusart_isr,                    \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));                                     \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));                                     \
	}
#else
#define SILABS_EUSART_IRQ_HANDLER_FUNC(idx)
#define SILABS_EUSART_IRQ_HANDLER(idx)
#endif

#define SILABS_EUSART_INIT(idx)                                                                    \
	SILABS_EUSART_IRQ_HANDLER(idx);                                                            \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	PM_DEVICE_DT_INST_DEFINE(idx, eusart_pm_action);                                           \
                                                                                                   \
	static const struct eusart_config eusart_cfg_##idx = {                                     \
		.eusart = (EUSART_TypeDef *)DT_INST_REG_ADDR(idx),                                 \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
		SILABS_EUSART_IRQ_HANDLER_FUNC(idx)                                                \
	};                                                                                         \
                                                                                                   \
	static struct eusart_data eusart_data_##idx = {                                            \
		.uart_cfg = {                                                                      \
			.baudrate  = DT_INST_PROP(idx, current_speed),                             \
			.parity    = DT_INST_ENUM_IDX(idx, parity),                                \
			.stop_bits = DT_INST_ENUM_IDX(idx, stop_bits),                             \
			.data_bits = DT_INST_ENUM_IDX(idx, data_bits),                             \
			.flow_ctrl = DT_INST_PROP(idx, hw_flow_control)                            \
						? UART_CFG_FLOW_CTRL_RTS_CTS                       \
						: UART_CFG_FLOW_CTRL_NONE,                         \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, eusart_init, PM_DEVICE_DT_INST_GET(idx), &eusart_data_##idx,    \
			      &eusart_cfg_##idx, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,        \
			      &eusart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SILABS_EUSART_INIT)
