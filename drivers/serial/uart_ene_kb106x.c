/*
 * Copyright (c) 2025-2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/ring_buffer.h>
#include <reg/ser.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_ene_kb106x, CONFIG_UART_LOG_LEVEL);

struct kb106x_uart_config {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_cfg_func)(void);
#endif
	struct serial_regs *ser;
	const struct pinctrl_dev_config *pcfg;
};

#define RX_BUF_SIZE 16

struct kb106x_uart_data {
	uart_irq_callback_user_data_t callback;
	struct uart_config current_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void *callback_data;
	struct k_timer tx_timer;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	struct k_spinlock lock;
};

static int kb106x_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	uint16_t reg_baudrate = 0;
	int ret = 0;
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;

	reg_baudrate = ((DIVIDER_BASE_CLK + (cfg->baudrate / 2)) / cfg->baudrate) - 1;

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
	config->ser->SERCFG = (reg_baudrate << 16) | 0x04 | SERCFG_TX_ENABLE | SERCFG_RX_ENABLE;
	config->ser->SERCTRL = SERCTRL_MODE1;
	data->current_config = *cfg;
	return ret;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int kb106x_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct kb106x_uart_data *data = dev->data;

	*cfg = data->current_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void tx_irq_trigger(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;
	uint8_t pending_flag_data = 0;

	if (config->ser->SERIE & SERIE_TX_ENABLE) {
		config->ser->SERPF = SERPF_TX_EMPTY;
		pending_flag_data |= SERPF_TX_EMPTY;
	}
	if (data->callback && pending_flag_data) {
		data->callback(dev, data->callback_data);
	}
}

static int kb106x_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;
	uint16_t tx_bytes = 0U;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while (((size - tx_bytes) > 0) && (!(config->ser->SERSTS & SERSTS_TX_FULL))) {
		/* Put a character into	Tx FIFO	*/
		config->ser->SERTBUF = tx_data[tx_bytes++];
	}
	k_spin_unlock(&data->lock, key);

	return tx_bytes;
}

static void kb106x_uart_irq_tx_enable(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	config->ser->SERPF = SERPF_TX_EMPTY;
	config->ser->SERIE |= SERIE_TX_ENABLE;
	if (!(config->ser->SERSTS & SERSTS_TX_BUSY)) {
		k_timer_start(&data->tx_timer, K_NO_WAIT, K_FOREVER);
	}
	k_spin_unlock(&data->lock, key);
}

static void kb106x_uart_irq_tx_disable(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;

	k_timer_stop(&data->tx_timer);
	config->ser->SERIE &= ~SERIE_TX_ENABLE;
	config->ser->SERPF = SERPF_TX_EMPTY;
}

static int kb106x_uart_irq_tx_complete(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;

	return (config->ser->SERSTS & SERSTS_TX_BUSY) ? 0 : 1;
}

static int kb106x_uart_irq_tx_ready(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;

	if ((config->ser->SERIE & SERIE_TX_ENABLE) && !(config->ser->SERSTS & SERSTS_TX_FULL)) {
		return 1;
	}
	return 0;
}

static int kb106x_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct kb106x_uart_config *config = dev->config;
	int read_len = 0;

	while (config->ser->SERPF & SERPF_RX_CNT_FULL) {
		*rx_data++ = (uint8_t)config->ser->SERRBUF;
		config->ser->SERPF = SERPF_RX_CNT_FULL;
		if (++read_len >= size) {
			break;
		}
	}

	return read_len;
}

static void kb106x_uart_irq_rx_enable(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;

	config->ser->SERIE |= SERIE_RX_ENABLE;
}

static void kb106x_uart_irq_rx_disable(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;

	config->ser->SERIE &= ~SERIE_RX_ENABLE;
}

static int kb106x_uart_irq_rx_ready(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;

	if (config->ser->SERPF & SERPF_RX_CNT_FULL) {
		return 1;
	}
	return 0;
}

static int kb106x_uart_irq_is_pending(const struct device *dev)
{
	return kb106x_uart_irq_tx_ready(dev) || kb106x_uart_irq_rx_ready(dev);
}

static void kb106x_uart_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
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
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;
	uint8_t pending_flag_data = 0;

	if (config->ser->SERPF & SERPF_RX_CNT_FULL) {
		pending_flag_data |= SERPF_RX_CNT_FULL;
	}
	if ((config->ser->SERPF & SERPF_TX_EMPTY) && (config->ser->SERIE & SERIE_TX_ENABLE)) {
		config->ser->SERPF = SERPF_TX_EMPTY;
		pending_flag_data |= SERPF_TX_EMPTY;
	}
	if (data->callback && pending_flag_data) {
		data->callback(dev, data->callback_data);
		pending_flag_data = 0;
	}
	if ((config->ser->SERPF & SERPF_RX_CNT_FULL) && pending_flag_data) {
		config->ser->SERPF = SERPF_RX_CNT_FULL;
		LOG_WRN("UART %s: RX FIFO not empty after ISR.", dev->name);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int kb106x_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct kb106x_uart_config *config = dev->config;

	if (config->ser->SERPF & SERPF_RX_CNT_FULL) {
		*c = config->ser->SERRBUF;
		config->ser->SERPF = SERPF_RX_CNT_FULL;
		return 0;
	}

	return -1;
}

static void kb106x_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct kb106x_uart_config *config = dev->config;
	int try_count = 500;

	while (try_count > 0) {
		if (config->ser->SERSTS & SERSTS_TX_FULL) {
			try_count--;
			continue;
		}
		/* Put a character into	Tx FIFO */
		config->ser->SERTBUF = c;
		break;
	}
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
	.fifo_read = kb106x_uart_fifo_read,
	.irq_tx_enable = kb106x_uart_irq_tx_enable,
	.irq_tx_disable = kb106x_uart_irq_tx_disable,
	.irq_tx_ready = kb106x_uart_irq_tx_ready,
	.irq_tx_complete = kb106x_uart_irq_tx_complete,
	.irq_rx_enable = kb106x_uart_irq_rx_enable,
	.irq_rx_disable = kb106x_uart_irq_rx_disable,
	.irq_rx_ready = kb106x_uart_irq_rx_ready,
	.irq_is_pending = kb106x_uart_irq_is_pending,
	.irq_update = kb106x_uart_irq_update,
	.irq_callback_set = kb106x_uart_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* UART module instances */
#define KB106X_UART_DEV(inst) DEVICE_DT_INST_GET(inst),
static const struct device *const uart_devices[] = {DT_INST_FOREACH_STATUS_OKAY(KB106X_UART_DEV)};
static void kb106x_uart_isr_wrap(const struct device *dev)
{
	for (size_t i = 0; i < ARRAY_SIZE(uart_devices); i++) {
		const struct device *dev_ = uart_devices[i];
		const struct kb106x_uart_config *config = dev_->config;

		if (((config->ser->SERIE & SERIE_RX_ENABLE) &&
		     (config->ser->SERPF & SERPF_RX_CNT_FULL)) ||
		    ((config->ser->SERIE & SERIE_TX_ENABLE) &&
		     (config->ser->SERPF & SERPF_TX_EMPTY))) {
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
	k_timer_init(&data->tx_timer, tx_irq_trigger, NULL);
	k_timer_user_data_set(&data->tx_timer, (void *)dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void kb106x_uart_irq_init(void)
{
	static bool init_irq = true;

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
		.current_config =                                                                  \
			{                                                                          \
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
