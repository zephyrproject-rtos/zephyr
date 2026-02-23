/*
 * Copyright (c) 2025-2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <reg/ser.h>

struct kb106x_uart_config {
	void (*irq_cfg_func)(void);
	struct serial_regs *ser;
	const struct pinctrl_dev_config *pcfg;
};

#define QUEUE_SIZE 16

struct uart_queue {
	uint8_t data[QUEUE_SIZE];
	int8_t front;
	int8_t rear;
};

struct kb106x_uart_data {
	uart_irq_callback_user_data_t callback;
	struct uart_config current_config;
	struct uart_queue rx_queue;
	bool rx_irq_enabled;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void *callback_data;
	uint8_t pending_flag_data;
	struct k_timer tx_timer;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static void uart_queue_init(struct uart_queue *q)
{
	q->front = q->rear = 0;
}

static bool uart_enqueue(struct uart_queue *q, uint8_t value)
{
	if ((q->rear + 1) % QUEUE_SIZE == q->front) {
		return false;
	}

	q->data[q->rear] = value;
	q->rear = (q->rear + 1) % QUEUE_SIZE;
	return true;
}

static bool uart_dequeue(struct uart_queue *q, uint8_t *value)
{
	if (q->front == q->rear) {
		return false;
	}

	*value = q->data[q->front];
	q->front = (q->front + 1) % QUEUE_SIZE;
	return true;
}

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
void tx_irq_triger(struct k_timer *timer)
{
	struct device *dev = k_timer_user_data_get(timer);
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;

	data->pending_flag_data = 0;
	if (config->ser->SERIE & SERIE_TX_ENABLE) {
		config->ser->SERPF = SERPF_TX_EMPTY;
		data->pending_flag_data |= SERPF_TX_EMPTY;
	}
	if (data->callback && data->pending_flag_data) {
		data->callback(dev, data->callback_data);
	}
}

static int kb106x_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct kb106x_uart_config *config = dev->config;
	uint16_t tx_bytes = 0U;
	unsigned int key;

	key = irq_lock();
	while (((size - tx_bytes) > 0) && (!(config->ser->SERSTS & SERSTS_TX_FULL))) {
		/* Put a character into	Tx FIFO	*/
		config->ser->SERTBUF = tx_data[tx_bytes++];
	}
	irq_unlock(key);

	return tx_bytes;
}

static void kb106x_uart_irq_tx_enable(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;

	config->ser->SERPF = SERPF_TX_EMPTY;
	config->ser->SERIE |= SERIE_TX_ENABLE;
	if (!(config->ser->SERSTS & SERSTS_TX_BUSY)) {
		k_timer_start(&data->tx_timer, K_NO_WAIT, K_FOREVER);
	}
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

	return (config->ser->SERSTS & SERSTS_TX_FULL) ? 0 : 1;
}

static int kb106x_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct kb106x_uart_data *data = dev->data;
	int read_len = 0;
	unsigned int key;

	key = irq_lock();
	while (read_len < size) {
		if (!uart_dequeue(&data->rx_queue, &rx_data[read_len])) {
			break;
		}
		read_len++;
	}
	irq_unlock(key);

	return read_len;
}

static void kb106x_uart_irq_rx_enable(const struct device *dev)
{
	struct kb106x_uart_data *data = dev->data;

	data->rx_irq_enabled = true;
}

static void kb106x_uart_irq_rx_disable(const struct device *dev)
{
	struct kb106x_uart_data *data = dev->data;

	data->rx_irq_enabled = false;
}

static int kb106x_uart_irq_rx_ready(const struct device *dev)
{
	struct kb106x_uart_data *data = dev->data;
	struct uart_queue *q = &data->rx_queue;
	bool empty;

	empty = q->front == q->rear;
	return empty ? 0 : 1;
}

static int kb106x_uart_irq_is_pending(const struct device *dev)
{
	struct kb106x_uart_data *data = dev->data;

	return (data->pending_flag_data) ? 1 : 0;
}

static int kb106x_uart_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
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
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;

	data->pending_flag_data = 0;
	if (config->ser->SERPF & SERPF_RX_CNT_FULL) {
		uint8_t rx_data;

		rx_data = (uint8_t)config->ser->SERRBUF;
		uart_enqueue(&data->rx_queue, rx_data);
		config->ser->SERPF = SERPF_RX_CNT_FULL;
		if (data->rx_irq_enabled) {
			data->pending_flag_data |= SERPF_RX_CNT_FULL;
		}
	}
	if ((config->ser->SERPF & SERPF_TX_EMPTY) && (config->ser->SERIE & SERIE_TX_ENABLE)) {
		config->ser->SERPF = SERPF_TX_EMPTY;
		data->pending_flag_data |= SERPF_TX_EMPTY;
	}
	if (data->callback && data->pending_flag_data) {
		data->callback(dev, data->callback_data);
	}
}
#else
static void kb106x_uart_irq_handler(const struct device *dev)
{
	const struct kb106x_uart_config *config = dev->config;
	struct kb106x_uart_data *data = dev->data;

	if (config->ser->SERPF & SERPF_RX_CNT_FULL) {
		uint8_t rx_data;

		rx_data = (uint8_t)config->ser->SERRBUF;
		uart_enqueue(&data->rx_queue, rx_data);
		config->ser->SERPF = SERPF_RX_CNT_FULL;
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int kb106x_uart_poll_in(const struct device *dev, unsigned char *c)
{
	struct kb106x_uart_data *data = dev->data;
	unsigned int key;
	int ret = -1;

	key = irq_lock();
	if (uart_dequeue(&data->rx_queue, c)) {
		ret = 0;
	}
	irq_unlock(key);

	return ret;
}

static void kb106x_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct kb106x_uart_config *config = dev->config;
	unsigned int key;

	key = irq_lock();
	/* Wait	Tx FIFO	not Full*/
	while (config->ser->SERSTS & SERSTS_TX_FULL) {
	}
	/* Put a character into	Tx FIFO */
	config->ser->SERTBUF = c;
	irq_unlock(key);
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
	uart_queue_init(&data->rx_queue);
	config->ser->SERPF = SERPF_RX_CNT_FULL;
	config->ser->SERIE |= SERIE_RX_ENABLE;
	config->irq_cfg_func();
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	k_timer_init(&data->tx_timer, tx_irq_triger, NULL);
	k_timer_user_data_set(&data->tx_timer, (void *)dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

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
		.irq_cfg_func = kb106x_uart_irq_init,                                              \
		.ser = (struct serial_regs *)DT_INST_REG_ADDR(n),                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &kb106x_uart_init, NULL, &kb106x_uart_data_##n,                   \
			      &kb106x_uart_config_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &kb106x_uart_api);

DT_INST_FOREACH_STATUS_OKAY(KB106X_UART_INIT)
