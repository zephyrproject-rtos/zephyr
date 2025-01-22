/*
 * Copyright (c) 2024 Google LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_usart

#include <errno.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <ch32fun.h>

struct usart_wch_config {
	USART_TypeDef *regs;
	const struct device *clock_dev;
	uint32_t current_speed;
	uint8_t parity;
	uint8_t clock_id;
	const struct pinctrl_dev_config *pin_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct usart_wch_data {
	uart_irq_callback_user_data_t cb;
	void *user_data;
};

static int usart_wch_init(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;
	uint32_t ctlr1 = USART_CTLR1_TE | USART_CTLR1_RE | USART_CTLR1_UE;
	uint32_t clock_rate;
	clock_control_subsys_t clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clock_id;
	uint32_t divn;
	int err;

	clock_control_on(config->clock_dev, clock_sys);

	err = clock_control_get_rate(config->clock_dev, clock_sys, &clock_rate);
	if (err != 0) {
		return err;
	}
	divn = (clock_rate + config->current_speed / 2) / config->current_speed;

	switch (config->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
		ctlr1 |= USART_CTLR1_PCE | USART_CTLR1_PS;
		break;
	case UART_CFG_PARITY_EVEN:
		ctlr1 |= USART_CTLR1_PCE;
		break;
	default:
		return -EINVAL;
	}

	regs->BRR = divn;
	regs->CTLR1 = ctlr1;
	regs->CTLR2 = 0;
	regs->CTLR3 = 0;

	err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static int usart_wch_poll_in(const struct device *dev, unsigned char *ch)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	if ((regs->STATR & USART_STATR_RXNE) == 0) {
		return -1;
	}

	*ch = regs->DATAR;
	return 0;
}

static void usart_wch_poll_out(const struct device *dev, unsigned char ch)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	while ((regs->STATR & USART_STATR_TXE) == 0) {
	}

	regs->DATAR = ch;
}

static int usart_wch_err_check(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;
	uint32_t statr = regs->STATR;
	enum uart_rx_stop_reason errors = 0;

	if ((statr & USART_STATR_PE) != 0) {
		errors |= UART_ERROR_PARITY;
	}
	if ((statr & USART_STATR_LBD) != 0) {
		errors |= UART_BREAK;
	}
	if ((statr & USART_STATR_FE) != 0) {
		errors |= UART_ERROR_FRAMING;
	}
	if ((statr & USART_STATR_NE) != 0) {
		errors |= UART_ERROR_NOISE;
	}
	if ((statr & USART_STATR_ORE) != 0) {
		errors |= UART_ERROR_OVERRUN;
	}

	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static void usart_wch_isr(const struct device *dev)
{
	struct usart_wch_data *data = dev->data;

	if (data->cb) {
		data->cb(dev, data->user_data);
	}
}

static int usart_wch_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	if (!len || !(regs->STATR & USART_STATR_TXE)) {
		return 0;
	}

	regs->DATAR = tx_data[0];
	return 1;
}

static int usart_wch_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	if (!size || !(regs->STATR & USART_STATR_RXNE)) {
		return 0;
	}

	rx_data[0] = regs->DATAR;
	return 1;
}

static void usart_wch_irq_tx_enable(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	regs->CTLR1 |= (USART_CTLR1_TXEIE | USART_CTLR1_TCIE);
}

static void usart_wch_irq_tx_disable(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	regs->CTLR1 &= ~(USART_CTLR1_TXEIE | USART_CTLR1_TCIE);
}

static int usart_wch_irq_tx_ready(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	return (regs->STATR & USART_STATR_TXE) > 0;
}

static void usart_wch_irq_rx_enable(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	regs->CTLR1 |= USART_CTLR1_RXNEIE;
}

static void usart_wch_irq_rx_disable(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	regs->CTLR1 &= ~USART_CTLR1_RXNEIE;
}

static int usart_wch_irq_tx_complete(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	return (regs->STATR & USART_STATR_TC) > 0;
}

static int usart_wch_irq_rx_ready(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	return (regs->STATR & USART_STATR_RXNE) > 0;
}

static void usart_wch_irq_err_enable(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	regs->CTLR1 |= USART_CTLR1_PEIE;
	regs->CTLR2 |= USART_CTLR2_LBDIE;
	regs->CTLR3 |= USART_CTLR3_EIE;
}

static void usart_wch_irq_err_disable(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	regs->CTLR1 &= ~USART_CTLR1_PEIE;
	regs->CTLR2 &= ~USART_CTLR2_LBDIE;
	regs->CTLR3 &= ~USART_CTLR3_EIE;
}

static int usart_wch_irq_is_pending(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;
	uint16_t ctlr1 = regs->CTLR1;
	uint16_t ctlr2 = regs->CTLR2;
	uint16_t ctlr3 = regs->CTLR3;
	uint16_t statr = regs->STATR;
	uint16_t stat_mask = 0;

	stat_mask |= (ctlr1 & USART_CTLR1_TXEIE) ? USART_STATR_TXE : 0;
	stat_mask |= (ctlr1 & USART_CTLR1_TCIE) ? USART_STATR_TC : 0;
	stat_mask |= (ctlr1 & USART_CTLR1_RXNEIE) ? USART_STATR_RXNE | USART_STATR_ORE : 0;
	stat_mask |= (ctlr1 & USART_CTLR1_IDLEIE) ? USART_STATR_IDLE : 0;
	stat_mask |= (ctlr1 & USART_CTLR1_PEIE) ? USART_STATR_PE : 0;

	stat_mask |= (ctlr2 & USART_CTLR2_LBDIE) ? USART_STATR_LBD : 0;
	stat_mask |=
		(ctlr3 & USART_CTLR3_EIE) ? USART_STATR_NE | USART_STATR_ORE | USART_STATR_FE : 0;
	stat_mask |= (ctlr3 & USART_CTLR3_CTSIE) ? USART_STATR_CTS : 0;

	return (statr & stat_mask) > 0;
}

static int usart_wch_irq_update(const struct device *dev)
{
	return 1;
}

static void usart_wch_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *user_data)
{
	struct usart_wch_data *data = dev->data;

	data->cb = cb;
	data->user_data = user_data;
}
#endif /*CONFIG_UART_INTERRUPT_DRIVEN*/

static DEVICE_API(uart, usart_wch_driver_api) = {
	.poll_in = usart_wch_poll_in,
	.poll_out = usart_wch_poll_out,
	.err_check = usart_wch_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_wch_fifo_fill,
	.fifo_read = usart_wch_fifo_read,
	.irq_tx_enable = usart_wch_irq_tx_enable,
	.irq_tx_disable = usart_wch_irq_tx_disable,
	.irq_tx_ready = usart_wch_irq_tx_ready,
	.irq_rx_enable = usart_wch_irq_rx_enable,
	.irq_rx_disable = usart_wch_irq_rx_disable,
	.irq_tx_complete = usart_wch_irq_tx_complete,
	.irq_rx_ready = usart_wch_irq_rx_ready,
	.irq_err_enable = usart_wch_irq_err_enable,
	.irq_err_disable = usart_wch_irq_err_disable,
	.irq_is_pending = usart_wch_irq_is_pending,
	.irq_update = usart_wch_irq_update,
	.irq_callback_set = usart_wch_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define USART_WCH_IRQ_HANDLER_DECL(idx)                                                            \
	static void usart_wch_irq_config_func_##idx(const struct device *dev);

#define USART_WCH_IRQ_HANDLER_FUNC(idx) .irq_config_func = usart_wch_irq_config_func_##idx,

#define USART_WCH_IRQ_HANDLER(idx)                                                                 \
	static void usart_wch_irq_config_func_##idx(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(index, priority), usart_wch_isr,        \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQN(idx));                                                     \
	}
#else
#define USART_WCH_IRQ_HANDLER_DECL(idx)
#define USART_WCH_IRQ_HANDLER_FUNC(idx)
#define USART_WCH_IRQ_HANDLER(idx)
#endif

#define USART_WCH_INIT(idx)                                                                        \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	USART_WCH_IRQ_HANDLER_DECL(idx)                                                            \
	static struct usart_wch_data usart_wch_##idx##_data;                                       \
	static const struct usart_wch_config usart_wch_##idx##_config = {                          \
		.regs = (USART_TypeDef *)DT_INST_REG_ADDR(idx),                                    \
		.current_speed = DT_INST_PROP(idx, current_speed),                                 \
		.parity = DT_INST_ENUM_IDX(idx, parity),                                           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_id = DT_INST_CLOCKS_CELL(idx, id),                                          \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                    \
		USART_WCH_IRQ_HANDLER_FUNC(idx)};                                                  \
	DEVICE_DT_INST_DEFINE(idx, &usart_wch_init, NULL, &usart_wch_##idx##_data,                 \
			      &usart_wch_##idx##_config, PRE_KERNEL_1,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, &usart_wch_driver_api);                 \
	USART_WCH_IRQ_HANDLER(idx)

DT_INST_FOREACH_STATUS_OKAY(USART_WCH_INIT)
