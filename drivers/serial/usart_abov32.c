/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT abov_abov32_usart

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

#include <hll_usart.h>

struct usart_abov32_config {
	USART_ID_e id;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct usart_abov32_data {
	struct uart_config cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int usart_abov32_data_bits(uint8_t data_bits, USART_DATA_e *out)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		*out = USART_DATA_5;
		break;
	case UART_CFG_DATA_BITS_6:
		*out = USART_DATA_6;
		break;
	case UART_CFG_DATA_BITS_7:
		*out = USART_DATA_7;
		break;
	case UART_CFG_DATA_BITS_8:
		*out = USART_DATA_8;
		break;
	case UART_CFG_DATA_BITS_9:
		*out = USART_DATA_9;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int usart_abov32_parity(uint8_t parity, USART_PARITY_e *out)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		*out = USART_PARITY_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		*out = USART_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		*out = USART_PARITY_EVEN;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int usart_abov32_stop_bits(uint8_t stop_bits, USART_STOP_e *out)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1:
		*out = USART_STOP_1;
		break;
	case UART_CFG_STOP_BITS_2:
		*out = USART_STOP_2;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int usart_abov32_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct usart_abov32_config *config = dev->config;
	struct usart_abov32_data *data = dev->data;
	USART_DATA_e data_bits;
	USART_PARITY_e parity;
	USART_STOP_e stop_bits;
	uint32_t bdr, bfr;
	int ret;

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	ret = usart_abov32_data_bits(cfg->data_bits, &data_bits);
	if (ret != 0) {
		return ret;
	}

	ret = usart_abov32_parity(cfg->parity, &parity);
	if (ret != 0) {
		return ret;
	}

	ret = usart_abov32_stop_bits(cfg->stop_bits, &stop_bits);
	if (ret != 0) {
		return ret;
	}

	HLL_USART_ClearControl(config->id);
	HLL_USART_SetMode(config->id, USART_MODE_UART);
	HLL_USART_SetUartFormat(config->id, parity, data_bits, stop_bits, false);

	HLL_USART_CalcUartBaud(cfg->baudrate, false, &bdr, &bfr);
	HLL_USART_SetBaudRate(config->id, bdr, bfr);

	HLL_USART_SetTxEnable(config->id, true);
	HLL_USART_SetRxEnable(config->id, true);
	HLL_USART_SetEnable(config->id, true);

	data->cfg = *cfg;

	return 0;
}

static int usart_abov32_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct usart_abov32_data *data = dev->data;

	*cfg = data->cfg;

	return 0;
}

static int usart_abov32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct usart_abov32_config *config = dev->config;

	if (!HLL_USART_GetRxReady(config->id)) {
		return -1;
	}

	*c = (unsigned char)HLL_USART_ReceiveByte(config->id);

	return 0;
}

static void usart_abov32_poll_out(const struct device *dev, unsigned char c)
{
	const struct usart_abov32_config *config = dev->config;

	HLL_USART_TransmitByte(config->id, c);
	while (!HLL_USART_GetTxReady(config->id)) {
	}
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int usart_abov32_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct usart_abov32_config *config = dev->config;
	int num_tx = 0;

	while (num_tx < len && HLL_USART_GetTxReady(config->id)) {
		HLL_USART_TransmitByte(config->id, tx_data[num_tx]);
		num_tx++;
	}

	return num_tx;
}

static int usart_abov32_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct usart_abov32_config *config = dev->config;
	int num_rx = 0;

	while (num_rx < size && HLL_USART_GetRxReady(config->id)) {
		rx_data[num_rx++] = (uint8_t)HLL_USART_ReceiveByte(config->id);
	}

	return num_rx;
}

static void usart_abov32_irq_tx_enable(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;

	HLL_USART_SetTxIntrEnable(config->id, true);
}

static void usart_abov32_irq_tx_disable(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;

	HLL_USART_SetTxIntrEnable(config->id, false);
}

static int usart_abov32_irq_tx_ready(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;

	return HLL_USART_GetTxReady(config->id) ? 1 : 0;
}

static int usart_abov32_irq_tx_complete(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;

	return (HLL_USART_GetStatus(config->id) & USART_STATUS_TXC) ? 1 : 0;
}

static void usart_abov32_irq_rx_enable(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;

	HLL_USART_SetRxIntrEnable(config->id, true);
}

static void usart_abov32_irq_rx_disable(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;

	HLL_USART_SetRxIntrEnable(config->id, false);
}

static int usart_abov32_irq_rx_ready(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;

	return HLL_USART_GetRxReady(config->id) ? 1 : 0;
}

static int usart_abov32_irq_is_pending(const struct device *dev)
{
	return usart_abov32_irq_tx_ready(dev) || usart_abov32_irq_rx_ready(dev);
}

static void usart_abov32_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct usart_abov32_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void usart_abov32_isr(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;
	struct usart_abov32_data *data = dev->data;
	uint32_t status = HLL_USART_GetStatus(config->id);

	HLL_USART_ClearStatus(config->id, status);

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, usart_abov32_driver_api) = {
	.poll_in = usart_abov32_poll_in,
	.poll_out = usart_abov32_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = usart_abov32_configure,
	.config_get = usart_abov32_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_abov32_fifo_fill,
	.fifo_read = usart_abov32_fifo_read,
	.irq_tx_enable = usart_abov32_irq_tx_enable,
	.irq_tx_disable = usart_abov32_irq_tx_disable,
	.irq_tx_ready = usart_abov32_irq_tx_ready,
	.irq_tx_complete = usart_abov32_irq_tx_complete,
	.irq_rx_enable = usart_abov32_irq_rx_enable,
	.irq_rx_disable = usart_abov32_irq_rx_disable,
	.irq_rx_ready = usart_abov32_irq_rx_ready,
	.irq_is_pending = usart_abov32_irq_is_pending,
	.irq_callback_set = usart_abov32_irq_callback_set,
#endif
};

static int usart_abov32_init(const struct device *dev)
{
	const struct usart_abov32_config *config = dev->config;
	struct usart_abov32_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	HLL_USART_SetClockEnable(config->id, true);

	ret = usart_abov32_configure(dev, &data->cfg);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define ABOV32_USART_IRQ_HANDLER(n)                                                               \
	static void usart_abov32_irq_config_##n(const struct device *dev)                        \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                  \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), usart_abov32_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                            \
		irq_enable(DT_INST_IRQN(n));                                                      \
	}
#define ABOV32_USART_IRQ_FUNC_INIT(n) .irq_config_func = usart_abov32_irq_config_##n,
#else
#define ABOV32_USART_IRQ_HANDLER(n)
#define ABOV32_USART_IRQ_FUNC_INIT(n)
#endif

#define ABOV32_USART_INIT(n)                                                                     \
	PINCTRL_DT_INST_DEFINE(n);                                                                \
	ABOV32_USART_IRQ_HANDLER(n)                                                               \
                                                                                                   \
	static const struct usart_abov32_config usart_abov32_cfg_##n = {                         \
		.id = (USART_ID_e)((DT_INST_REG_ADDR(n) - USART_REG_BASE) / USART_REG_OFFSET),    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                        \
		ABOV32_USART_IRQ_FUNC_INIT(n)                                                     \
	};                                                                                         \
                                                                                                   \
	static struct usart_abov32_data usart_abov32_data_##n = {                                \
		.cfg = {                                                                          \
			.baudrate = DT_INST_PROP(n, current_speed),                               \
			.parity = UART_CFG_PARITY_NONE,                                           \
			.stop_bits = UART_CFG_STOP_BITS_1,                                        \
			.data_bits = UART_CFG_DATA_BITS_8,                                        \
			.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                                     \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, usart_abov32_init, NULL, &usart_abov32_data_##n,                 \
			      &usart_abov32_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,   \
			      &usart_abov32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ABOV32_USART_INIT)
