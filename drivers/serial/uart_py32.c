/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT puya_py32_uart

#include <soc.h>
#include <py32f0xx_ll_usart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/py32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

struct uart_py32_config {
	USART_TypeDef *uart;
	const struct device *clk_dev;
	struct py32_pclken pclken;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pinctrl;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_configure)(const struct device *dev);
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
struct uart_py32_data {
	uart_irq_callback_user_data_t callback;
	void *user_data;
};
#endif

static int uart_py32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_py32_config *cfg = dev->config;

	if (LL_USART_IsActiveFlag_RXNE(cfg->uart) == 0U) {
		return -1;
	}
	*c = LL_USART_ReceiveData8(cfg->uart);
	return 0;
}

static void uart_py32_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_py32_config *cfg = dev->config;

	while (LL_USART_IsActiveFlag_TXE(cfg->uart) == 0U) {
	}
	LL_USART_TransmitData8(cfg->uart, c);
}

static int uart_py32_err_check(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;
	uint32_t status = cfg->uart->SR;
	int errors = 0;

	if ((status & USART_SR_ORE) != 0U) {
		errors |= UART_ERROR_OVERRUN;
	}
	if ((status & USART_SR_PE) != 0U) {
		errors |= UART_ERROR_PARITY;
	}
	if ((status & USART_SR_FE) != 0U) {
		errors |= UART_ERROR_FRAMING;
	}
	if ((status & USART_SR_NE) != 0U) {
		errors |= UART_ERROR_NOISE;
	}
	if (errors != 0) {
		/* Reading SR followed by DR clears the receive error flags. */
		LL_USART_ClearFlag_ORE(cfg->uart);
	}
	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_py32_fifo_fill(const struct device *dev, const uint8_t *data, int size)
{
	const struct uart_py32_config *cfg = dev->config;
	int sent = 0;

	while ((sent < size) && (LL_USART_IsActiveFlag_TXE(cfg->uart) != 0U)) {
		LL_USART_TransmitData8(cfg->uart, data[sent++]);
	}
	return sent;
}

static int uart_py32_fifo_read(const struct device *dev, uint8_t *data, const int size)
{
	const struct uart_py32_config *cfg = dev->config;
	int received = 0;

	while ((received < size) && (LL_USART_IsActiveFlag_RXNE(cfg->uart) != 0U)) {
		data[received++] = LL_USART_ReceiveData8(cfg->uart);
	}
	return received;
}

static void uart_py32_irq_tx_enable(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	LL_USART_EnableIT_TXE(cfg->uart);
}

static void uart_py32_irq_tx_disable(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	LL_USART_DisableIT_TXE(cfg->uart);
}

static int uart_py32_irq_tx_ready(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	return (LL_USART_IsEnabledIT_TXE(cfg->uart) != 0U) &&
	       (LL_USART_IsActiveFlag_TXE(cfg->uart) != 0U);
}

static int uart_py32_irq_tx_complete(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	return LL_USART_IsActiveFlag_TC(cfg->uart) != 0U;
}

static void uart_py32_irq_rx_enable(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	LL_USART_EnableIT_RXNE(cfg->uart);
}

static void uart_py32_irq_rx_disable(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	LL_USART_DisableIT_RXNE(cfg->uart);
}

static int uart_py32_irq_rx_ready(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	return (LL_USART_IsEnabledIT_RXNE(cfg->uart) != 0U) &&
	       (LL_USART_IsActiveFlag_RXNE(cfg->uart) != 0U);
}

static void uart_py32_irq_err_enable(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	LL_USART_EnableIT_PE(cfg->uart);
	LL_USART_EnableIT_ERROR(cfg->uart);
}

static void uart_py32_irq_err_disable(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;

	LL_USART_DisableIT_PE(cfg->uart);
	LL_USART_DisableIT_ERROR(cfg->uart);
}

static int uart_py32_irq_is_pending(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;
	uint32_t status = cfg->uart->SR;

	return (uart_py32_irq_tx_ready(dev) != 0) || (uart_py32_irq_rx_ready(dev) != 0) ||
	       ((LL_USART_IsEnabledIT_PE(cfg->uart) != 0U) && ((status & USART_SR_PE) != 0U)) ||
	       ((LL_USART_IsEnabledIT_ERROR(cfg->uart) != 0U) &&
		((status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) != 0U));
}

static void uart_py32_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_py32_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *user_data)
{
	struct uart_py32_data *data = dev->data;
	unsigned int key = irq_lock();

	data->callback = cb;
	data->user_data = user_data;
	irq_unlock(key);
}

static void uart_py32_isr(const struct device *dev)
{
	struct uart_py32_data *data = dev->data;

	if (data->callback != NULL) {
		data->callback(dev, data->user_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_py32_init(const struct device *dev)
{
	const struct uart_py32_config *cfg = dev->config;
	uint32_t rate;
	int ret;

	if (!device_is_ready(cfg->clk_dev) || !device_is_ready(cfg->reset.dev)) {
		return -ENODEV;
	}
	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&cfg->pclken);
	if (ret < 0) {
		return ret;
	}
	ret = reset_line_toggle_dt(&cfg->reset);
	if (ret < 0) {
		return ret;
	}
	ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	ret = clock_control_get_rate(cfg->clk_dev, (clock_control_subsys_t)&cfg->pclken, &rate);
	if (ret < 0) {
		return ret;
	}
	if ((cfg->baud_rate == 0U) || (rate / cfg->baud_rate < 16U) ||
	    (DIV_ROUND_CLOSEST(rate, cfg->baud_rate) > UINT16_MAX)) {
		return -EINVAL;
	}
	LL_USART_Disable(cfg->uart);
	LL_USART_ConfigCharacter(cfg->uart, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE,
				 LL_USART_STOPBITS_1);
	LL_USART_SetTransferDirection(cfg->uart, LL_USART_DIRECTION_TX_RX);
	/* PY32 keeps OVER8 in CR3 rather than CR1. */
	LL_USART_SetOverSampling(cfg->uart, LL_USART_OVERSAMPLING_16);
	LL_USART_SetBaudRate(cfg->uart, rate, LL_USART_OVERSAMPLING_16, cfg->baud_rate);
	LL_USART_Enable(cfg->uart);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_configure(dev);
#endif
	return 0;
}

static DEVICE_API(uart, uart_py32_api) = {
	.poll_in = uart_py32_poll_in,
	.poll_out = uart_py32_poll_out,
	.err_check = uart_py32_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_py32_fifo_fill,
	.fifo_read = uart_py32_fifo_read,
	.irq_tx_enable = uart_py32_irq_tx_enable,
	.irq_tx_disable = uart_py32_irq_tx_disable,
	.irq_tx_ready = uart_py32_irq_tx_ready,
	.irq_tx_complete = uart_py32_irq_tx_complete,
	.irq_rx_enable = uart_py32_irq_rx_enable,
	.irq_rx_disable = uart_py32_irq_rx_disable,
	.irq_rx_ready = uart_py32_irq_rx_ready,
	.irq_err_enable = uart_py32_irq_err_enable,
	.irq_err_disable = uart_py32_irq_err_disable,
	.irq_is_pending = uart_py32_irq_is_pending,
	.irq_update = uart_py32_irq_update,
	.irq_callback_set = uart_py32_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_PY32_IRQ_DEFINE(inst)                                                                 \
	static struct uart_py32_data uart_py32_data_##inst;                                        \
	static void uart_py32_irq_configure_##inst(const struct device *dev)                       \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), uart_py32_isr,        \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}
#define UART_PY32_DATA(inst)       &uart_py32_data_##inst
#define UART_PY32_IRQ_CONFIG(inst) .irq_configure = uart_py32_irq_configure_##inst,
#else
#define UART_PY32_IRQ_DEFINE(inst)
#define UART_PY32_DATA(inst) NULL
#define UART_PY32_IRQ_CONFIG(inst)
#endif

#define UART_PY32_DEFINE(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	UART_PY32_IRQ_DEFINE(inst)                                                                 \
	static const struct uart_py32_config uart_py32_config_##inst = {                           \
		.uart = (USART_TypeDef *)DT_INST_REG_ADDR(inst),                                   \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.pclken = {DT_INST_CLOCKS_CELL(inst, bus), DT_INST_CLOCKS_CELL(inst, bits)},       \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                   \
		.baud_rate = DT_INST_PROP(inst, current_speed),                                    \
		UART_PY32_IRQ_CONFIG(inst)};                                                       \
	DEVICE_DT_INST_DEFINE(inst, uart_py32_init, NULL, UART_PY32_DATA(inst),                    \
			      &uart_py32_config_##inst, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_py32_api);

DT_INST_FOREACH_STATUS_OKAY(UART_PY32_DEFINE)
