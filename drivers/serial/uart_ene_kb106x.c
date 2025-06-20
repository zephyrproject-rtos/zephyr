/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <reg/ser.h>

struct kb106x_uart_config {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_cfg_func)(void);
#endif
	struct serial_regs *ser;
	const struct pinctrl_dev_config *pcfg;
};

struct kb106x_uart_data {
	uart_irq_callback_user_data_t callback;
	struct uart_config current_config;
	void *callback_data;
	uint8_t pending_flag_data;
};

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int kb106x_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	uint16_t reg_baudrate = 0;
	int ret = 0;
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;

	reg_baudrate = (DIVIDER_BASE_CLK / cfg->baudrate) - 1;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
	case UART_CFG_PARITY_EVEN:
	case UART_CFG_PARITY_MARK:
	case UART_CFG_PARITY_SPACE:
	default:
		ret = -ENOTSUP;
		break;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		break;
	case UART_CFG_STOP_BITS_0_5:
	case UART_CFG_STOP_BITS_1_5:
	case UART_CFG_STOP_BITS_2:
	default:
		ret = -ENOTSUP;
		break;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_8:
		break;
	case UART_CFG_DATA_BITS_5:
	case UART_CFG_DATA_BITS_6:
	case UART_CFG_DATA_BITS_7:
	case UART_CFG_DATA_BITS_9:
	default:
		ret = -ENOTSUP;
		break;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
	case UART_CFG_FLOW_CTRL_DTR_DSR:
	default:
		ret = -ENOTSUP;
		break;
	}
	config->ser->SERCFG = (reg_baudrate << 16) | 0x04 | SERIE_TX_ENABLE;
	config->ser->SERCTRL = SERCTRL_MODE1;
	data->current_config = *cfg;
	return ret;
}

static int kb106x_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct kb106x_uart_data *data = dev->data;

	*cfg = data->current_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int kb106x_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct kb106x_uart_config *config = dev->config;
	uint16_t tx_bytes = 0U;

	while ((size - tx_bytes) > 0) {
		/* Check Tx FIFO not Full*/
		while (config->ser->SERSTS & SERSTS_TX_FULL) {
		}
		/* Put a character into	Tx FIFO	*/
		config->ser->SERTBUF = tx_data[tx_bytes];
		tx_bytes++;
	}
	return tx_bytes;
}

static void kb106x_uart_irq_tx_enable(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;

	config->ser->SERPF = SERPF_TX_EMPTY;
	config->ser->SERIE |= SERIE_TX_ENABLE;
}

static void kb106x_uart_irq_tx_disable(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;

	config->ser->SERIE &= ~SERIE_TX_ENABLE;
	config->ser->SERPF = SERPF_TX_EMPTY;
}

static int kb106x_uart_irq_tx_ready(const struct device *dev)
{
	struct kb106x_uart_data *data = dev->data;

	return (data->pending_flag_data & SERPF_TX_EMPTY) ? 1 : 0;
}

static int kb106x_uart_irq_is_pending(const struct device *dev)
{
	struct kb106x_uart_data *data = dev->data;

	return (data->pending_flag_data) ? 1 : 0;
}

static int kb106x_uart_irq_update(const struct device *dev)
{
	struct kb106x_uart_data *data = dev->data;
	const struct kb106x_uart_config *config = dev->config;

	data->pending_flag_data = (config->ser->SERPF) & (config->ser->SERIE);
	/*clear	pending	flag*/
	config->ser->SERPF = data->pending_flag_data;
	return 1;
}

static void kb106x_uart_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct kb106x_uart_data *data = dev->data;

	data->callback = cb;
	data->callback_data = cb_data;
}

static void kb106x_uart_irq_handler(const struct device *dev)
{
	struct kb106x_uart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int kb106x_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct kb106x_uart_config *config = dev->config;

	/* Check Rx FIFO not Empty*/
	if (config->ser->SERSTS & SERSTS_RX_BUSY) {
		return -1;
	}
	/* Put a character into Tx FIFO */
	*c = config->ser->SERRBUF;
	return 0;
}

static void kb106x_uart_poll_out(const struct device *dev, unsigned char c)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	kb106x_uart_fifo_fill(dev, &c, 1);
#else
	const struct kb106x_uart_config *config = dev->config;

	/* Wait	Tx FIFO	not Full*/
	while (config->ser->SERSTS & SERSTS_TX_FULL) {
	}
	/* Put a character into	Tx FIFO */
	config->ser->SERTBUF = c;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}

static DEVICE_API(uart, kb106x_uart_api) = {
	.poll_in = kb106x_uart_poll_in,
	.poll_out = kb106x_uart_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = kb106x_uart_configure,
	.config_get = kb106x_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = kb106x_uart_fifo_fill,
	.irq_tx_enable = kb106x_uart_irq_tx_enable,
	.irq_tx_disable = kb106x_uart_irq_tx_disable,
	.irq_tx_ready = kb106x_uart_irq_tx_ready,
	.irq_is_pending = kb106x_uart_irq_is_pending,
	.irq_update = kb106x_uart_irq_update,
	.irq_callback_set = kb106x_uart_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* GPIO module instances */
#define KB106X_UART_DEV(inst) DEVICE_DT_INST_GET(inst),
static const struct device *const uart_devices[] = {DT_INST_FOREACH_STATUS_OKAY(KB106X_UART_DEV)};
static void kb106x_uart_isr_wrap(const struct device *dev)
{
	for (size_t i = 0; i < ARRAY_SIZE(uart_devices); i++) {
		const struct device *dev_ = uart_devices[i];
		const struct kb106x_uart_config *config = dev_->config;

		if (config->ser->SERIE & config->ser->SERPF) {
			kb106x_uart_irq_handler(dev_);
		}
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int kb106x_uart_init(const struct device *dev)
{
	int ret;
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	kb106x_uart_configure(dev, &data->current_config);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_cfg_func();
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static bool init_irq = true;
static void kb106x_uart_irq_init(void)
{
	if (init_irq) {
		init_irq = false;
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), kb106x_uart_isr_wrap, NULL,
			    0);
		irq_enable(DT_INST_IRQN(0));
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define KB106X_UART_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct kb106x_uart_data kb106x_uart_data_##n = {                                    \
		.current_config = {                                                                \
				.baudrate = DT_INST_PROP(n, current_speed),                        \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
			},                                                                         \
	};                                                                                         \
	static const struct kb106x_uart_config kb106x_uart_config_##n = {                          \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (.irq_cfg_func = kb106x_uart_irq_init,))  \
		.ser = (struct serial_regs *)DT_INST_REG_ADDR(n),                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &kb106x_uart_init, NULL, &kb106x_uart_data_##n,                   \
			      &kb106x_uart_config_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &kb106x_uart_api);

DT_INST_FOREACH_STATUS_OKAY(KB106X_UART_INIT)
