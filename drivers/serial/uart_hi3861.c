/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hisilicon_hi3861_uart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#include <reg/uart_hi3861_regs.h>

LOG_MODULE_REGISTER(uart_hi3861, CONFIG_UART_LOG_LEVEL);

#define HI3861_UART_USE_IRQ                                                                        \
	(IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN) && IS_ENABLED(CONFIG_SYS_CLOCK_EXISTS))

/* Bound polling before the system timer starts. */
#define IDLE_POLL_LIMIT 1000000U

struct hi3861_uart_config {
	DEVICE_MMIO_ROM;
	uint32_t clock_frequency;
	struct uart_config initial_config;
	bool hw_flow_control_supported;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pin_config;
#endif
#if HI3861_UART_USE_IRQ
	uart_irq_config_func_t irq_config;
#endif
};

struct hi3861_uart_data {
	DEVICE_MMIO_RAM;
	struct k_spinlock lock;
	struct uart_config config;
	uint32_t errors;
#if HI3861_UART_USE_IRQ
	uart_irq_callback_user_data_t callback;
	void *callback_data;
	const struct device *dev;
	struct k_timer tx_kick_timer;
	uint32_t tx_count;
	uint32_t rx_count;
	bool servicing;
	bool tx_kick;
#endif
};

static uint32_t uart_read(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + offset);
}

static void uart_write(const struct device *dev, uint32_t offset, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_GET(dev) + offset);
}

static int hi3861_uart_configure(const struct device *dev, const struct uart_config *config)
{
	const struct hi3861_uart_config *cfg = dev->config;
	struct hi3861_uart_data *data = dev->data;
	uint32_t lcr = LCR_FEN;
	uint32_t cr = CR_UARTEN | CR_TXE | CR_RXE;
	uint64_t divisor;

	switch (config->data_bits) {
	case UART_CFG_DATA_BITS_5:
	case UART_CFG_DATA_BITS_6:
	case UART_CFG_DATA_BITS_7:
	case UART_CFG_DATA_BITS_8:
		lcr |= (config->data_bits - UART_CFG_DATA_BITS_5) << LCR_WLEN_SHIFT;
		break;
	default:
		LOG_ERR("Unsupported data bits: %u", config->data_bits);
		return -ENOTSUP;
	}

	switch (config->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		break;
	case UART_CFG_STOP_BITS_2:
		lcr |= LCR_STP2;
		break;
	default:
		LOG_ERR("Unsupported stop bits: %u", config->stop_bits);
		return -ENOTSUP;
	}

	switch (config->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
		lcr |= LCR_PEN;
		break;
	case UART_CFG_PARITY_EVEN:
		lcr |= LCR_PEN | LCR_EPS;
		break;
	case UART_CFG_PARITY_MARK:
		lcr |= LCR_PEN | LCR_SPS;
		break;
	case UART_CFG_PARITY_SPACE:
		lcr |= LCR_PEN | LCR_EPS | LCR_SPS;
		break;
	default:
		LOG_ERR("Unsupported parity: %u", config->parity);
		return -ENOTSUP;
	}

	switch (config->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		if (!cfg->hw_flow_control_supported) {
			LOG_ERR("RTS/CTS is not supported on %s", dev->name);
			return -ENOTSUP;
		}
		cr |= CR_RTSEN | CR_CTSEN;
		break;
	default:
		LOG_ERR("Unsupported flow control: %u", config->flow_ctrl);
		return -ENOTSUP;
	}

	if (config->baudrate == 0U) {
		LOG_ERR("Baud rate must be nonzero");
		return -EINVAL;
	}

	divisor = DIV_ROUND_CLOSEST((uint64_t)cfg->clock_frequency * 4U, config->baudrate);
	if (divisor < 64U || divisor > 65535U * 64U) {
		LOG_ERR("Baud rate %u is outside the divisor range", config->baudrate);
		return -EINVAL;
	}

	for (uint32_t i = 0; i < IDLE_POLL_LIMIT; i++) {
		k_spinlock_key_t key;

		if ((uart_read(dev, UART_FR) & FR_BUSY) != 0U) {
			continue;
		}
		key = k_spin_lock(&data->lock);
		if ((uart_read(dev, UART_FR) & FR_BUSY) != 0U) {
			k_spin_unlock(&data->lock, key);
			continue;
		}

		uart_write(dev, UART_CR, 0);
		uart_write(dev, UART_IBRD, divisor >> 6);
		uart_write(dev, UART_FBRD, divisor & 0x3fU);
		/* LCR_H latches both baud divisor registers. */
		uart_write(dev, UART_LCR_H, lcr);
		uart_write(dev, UART_CR, cr);
		data->config = *config;
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	LOG_ERR("UART remains busy during configuration");
	return -EBUSY;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int hi3861_uart_config_get(const struct device *dev, struct uart_config *config)
{
	struct hi3861_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	*config = data->config;
	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif

static int hi3861_uart_poll_in(const struct device *dev, unsigned char *character)
{
	struct hi3861_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = -1;

	if ((uart_read(dev, UART_FR) & FR_RXFE) == 0U) {
		uint32_t value = uart_read(dev, UART_DR);

		data->errors |= (value >> 8) & 0xfU;
		*character = value & 0xffU;
		ret = 0;
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static void hi3861_uart_poll_out(const struct device *dev, unsigned char character)
{
	struct hi3861_uart_data *data = dev->data;

	for (;;) {
		k_spinlock_key_t key;

		if ((uart_read(dev, UART_FR) & FR_TXFF) != 0U) {
			continue;
		}
		key = k_spin_lock(&data->lock);
		if ((uart_read(dev, UART_FR) & FR_TXFF) == 0U) {
			uart_write(dev, UART_DR, character);
			k_spin_unlock(&data->lock, key);
			return;
		}
		k_spin_unlock(&data->lock, key);
	}
}

static int hi3861_uart_err_check(const struct device *dev)
{
	struct hi3861_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t status = uart_read(dev, UART_RSR_ECR) | data->errors;
	int errors = 0;

	uart_write(dev, UART_RSR_ECR, 0);
	uart_write(dev, UART_ICR, INT_ERR);
	data->errors = 0;
	k_spin_unlock(&data->lock, key);

	if ((status & RSR_OE) != 0U) {
		errors |= UART_ERROR_OVERRUN;
	}
	if ((status & RSR_PE) != 0U) {
		errors |= UART_ERROR_PARITY;
	}
	if ((status & RSR_FE) != 0U) {
		errors |= UART_ERROR_FRAMING;
	}
	if ((status & RSR_BE) != 0U) {
		errors |= UART_BREAK;
	}

	return errors;
}

#if HI3861_UART_USE_IRQ
static int hi3861_uart_fifo_fill(const struct device *dev, const uint8_t *buffer, int length)
{
	struct hi3861_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int count = 0;

	while (count < length && (uart_read(dev, UART_FR) & FR_TXFF) == 0U) {
		uart_write(dev, UART_DR, buffer[count++]);
	}
	data->tx_count += count;
	k_spin_unlock(&data->lock, key);

	return count;
}

static int hi3861_uart_fifo_read(const struct device *dev, uint8_t *buffer, const int length)
{
	struct hi3861_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int count = 0;

	while (count < length && (uart_read(dev, UART_FR) & FR_RXFE) == 0U) {
		uint32_t value = uart_read(dev, UART_DR);

		data->errors |= (value >> 8) & 0xfU;
		buffer[count++] = value & 0xffU;
	}
	if ((uart_read(dev, UART_FR) & FR_RXFE) != 0U) {
		uart_write(dev, UART_ICR, INT_RX_MASK);
	}
	data->rx_count += count;
	k_spin_unlock(&data->lock, key);

	return count;
}

static void hi3861_uart_irq_mask(const struct device *dev, uint32_t mask, bool enable)
{
	struct hi3861_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t imsc = uart_read(dev, UART_IMSC);

	uart_write(dev, UART_IMSC, enable ? imsc | mask : imsc & ~mask);
	k_spin_unlock(&data->lock, key);
}

static int hi3861_uart_irq_tx_ready(const struct device *dev)
{
	return (uart_read(dev, UART_IMSC) & INT_TX) != 0U &&
	       (uart_read(dev, UART_FR) & FR_TXFF) == 0U;
}

static int hi3861_uart_irq_rx_ready(const struct device *dev)
{
	return (uart_read(dev, UART_IMSC) & INT_RX_MASK) != 0U &&
	       (uart_read(dev, UART_FR) & FR_RXFE) == 0U;
}

static int hi3861_uart_irq_is_pending(const struct device *dev)
{
	return hi3861_uart_irq_tx_ready(dev) || hi3861_uart_irq_rx_ready(dev) ||
	       (uart_read(dev, UART_MIS) & INT_ERR) != 0U;
}

static void hi3861_uart_irq_dispatch(const struct device *dev, bool kick)
{
	struct hi3861_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->tx_kick |= kick;
	if (data->servicing) {
		k_spin_unlock(&data->lock, key);
		return;
	}
	data->servicing = true;

	for (;;) {
		uint32_t pending = uart_read(dev, UART_MIS) & INT_ALL;
		uint32_t tx_count = data->tx_count;
		uint32_t rx_count = data->rx_count;
		uint32_t mask = uart_read(dev, UART_IMSC);
		uart_irq_callback_user_data_t callback = data->callback;
		void *callback_data = data->callback_data;

		if ((uart_read(dev, UART_FR) & FR_RXFE) != 0U) {
			uart_write(dev, UART_ICR, INT_RX_MASK);
			pending &= ~INT_RX_MASK;
		}
		if (pending == 0U && !(data->tx_kick && hi3861_uart_irq_tx_ready(dev))) {
			break;
		}
		data->tx_kick |= (pending & INT_TX) != 0U;
		uart_write(dev, UART_ICR, pending & (INT_TX | INT_ERR));
		if (callback == NULL) {
			uart_write(dev, UART_IMSC, 0);
			data->servicing = false;
			data->tx_kick = false;
			k_spin_unlock(&data->lock, key);
			LOG_ERR("Interrupt enabled without a UART callback");
			return;
		}
		k_spin_unlock(&data->lock, key);
		callback(dev, callback_data);
		key = k_spin_lock(&data->lock);
		if (tx_count == data->tx_count && rx_count == data->rx_count &&
		    mask == uart_read(dev, UART_IMSC) && hi3861_uart_irq_is_pending(dev)) {
			uart_write(dev, UART_IMSC, 0);
			data->servicing = false;
			data->tx_kick = false;
			k_spin_unlock(&data->lock, key);
			LOG_ERR("UART callback made no progress; interrupts disabled");
			return;
		}
	}

	data->servicing = false;
	data->tx_kick = false;
	k_spin_unlock(&data->lock, key);
}

static void hi3861_uart_tx_kick(struct k_timer *timer)
{
	struct hi3861_uart_data *data = CONTAINER_OF(timer, struct hi3861_uart_data, tx_kick_timer);

	hi3861_uart_irq_dispatch(data->dev, true);
}

static void hi3861_uart_irq_tx_enable(const struct device *dev)
{
	struct hi3861_uart_data *data = dev->data;

	hi3861_uart_irq_mask(dev, INT_TX, true);
	/* TX IRQs need a FIFO threshold crossing; seed the FIFO in ISR context. */
	if (k_is_in_isr()) {
		hi3861_uart_irq_dispatch(dev, true);
	} else {
		k_timer_start(&data->tx_kick_timer, K_TICKS(1), K_NO_WAIT);
	}
}

static void hi3861_uart_irq_tx_disable(const struct device *dev)
{
	struct hi3861_uart_data *data = dev->data;

	hi3861_uart_irq_mask(dev, INT_TX, false);
	k_timer_stop(&data->tx_kick_timer);
}

static int hi3861_uart_irq_tx_complete(const struct device *dev)
{
	return (uart_read(dev, UART_FR) & (FR_BUSY | FR_TXFE)) == FR_TXFE;
}

static void hi3861_uart_irq_rx_enable(const struct device *dev)
{
	hi3861_uart_irq_mask(dev, INT_RX_MASK, true);
}

static void hi3861_uart_irq_rx_disable(const struct device *dev)
{
	hi3861_uart_irq_mask(dev, INT_RX_MASK, false);
}

static void hi3861_uart_irq_err_enable(const struct device *dev)
{
	hi3861_uart_irq_mask(dev, INT_ERR, true);
}

static void hi3861_uart_irq_err_disable(const struct device *dev)
{
	hi3861_uart_irq_mask(dev, INT_ERR, false);
}

static void hi3861_uart_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t callback, void *user_data)
{
	struct hi3861_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->callback = callback;
	data->callback_data = user_data;
	k_spin_unlock(&data->lock, key);
}

static void hi3861_uart_isr(const struct device *dev)
{
	hi3861_uart_irq_dispatch(dev, false);
}
#endif

static int hi3861_uart_init(const struct device *dev)
{
	const struct hi3861_uart_config *cfg = dev->config;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
#ifdef CONFIG_PINCTRL
	if (cfg->pin_config != NULL) {
		ret = pinctrl_apply_state(cfg->pin_config, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Cannot configure UART pins: %d", ret);
			return ret;
		}
	}
#endif
	uart_write(dev, UART_IMSC, 0);
	uart_write(dev, UART_ICR, INT_ALL);
	uart_write(dev, UART_DMACR, 0);
	uart_write(dev, UART_RSR_ECR, 0);
	uart_write(dev, UART_IFLS, IFLS_TX_EIGHTH | IFLS_RX_HALF | IFLS_RTS_HALF);
	ret = hi3861_uart_configure(dev, &cfg->initial_config);
	if (ret < 0) {
		return ret;
	}
#if HI3861_UART_USE_IRQ
	struct hi3861_uart_data *data = dev->data;

	data->dev = dev;
	k_timer_init(&data->tx_kick_timer, hi3861_uart_tx_kick, NULL);
	cfg->irq_config(dev);
#endif

	return 0;
}

static DEVICE_API(uart, hi3861_uart_api) = {
	.poll_in = hi3861_uart_poll_in,
	.poll_out = hi3861_uart_poll_out,
	.err_check = hi3861_uart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = hi3861_uart_configure,
	.config_get = hi3861_uart_config_get,
#endif
#if HI3861_UART_USE_IRQ
	.fifo_fill = hi3861_uart_fifo_fill,
	.fifo_read = hi3861_uart_fifo_read,
	.irq_tx_enable = hi3861_uart_irq_tx_enable,
	.irq_tx_disable = hi3861_uart_irq_tx_disable,
	.irq_tx_ready = hi3861_uart_irq_tx_ready,
	.irq_tx_complete = hi3861_uart_irq_tx_complete,
	.irq_rx_enable = hi3861_uart_irq_rx_enable,
	.irq_rx_disable = hi3861_uart_irq_rx_disable,
	.irq_rx_ready = hi3861_uart_irq_rx_ready,
	.irq_err_enable = hi3861_uart_irq_err_enable,
	.irq_err_disable = hi3861_uart_irq_err_disable,
	.irq_is_pending = hi3861_uart_irq_is_pending,
	.irq_callback_set = hi3861_uart_irq_callback_set,
#endif
};

#if HI3861_UART_USE_IRQ
#define HI3861_IRQ_DEFINE(inst)                                                                    \
	static void hi3861_uart_irq_config_##inst(const struct device *dev)                        \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(inst), 0, hi3861_uart_isr, DEVICE_DT_INST_GET(inst), 0);  \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}
#define HI3861_IRQ_CONFIG(inst) .irq_config = hi3861_uart_irq_config_##inst,
#else
#define HI3861_IRQ_DEFINE(inst)
#define HI3861_IRQ_CONFIG(inst)
#endif

#ifdef CONFIG_PINCTRL
#define HI3861_PINCTRL_DEFINE(inst) PINCTRL_DT_INST_DEFINE(inst)
#define HI3861_PINCTRL_CONFIG(inst)                                                                \
	.pin_config = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, pinctrl_0),                          \
				 (PINCTRL_DT_INST_DEV_CONFIG_GET(inst)), (NULL)),
#else
#define HI3861_PINCTRL_DEFINE(inst)
#define HI3861_PINCTRL_CONFIG(inst)
#endif

#define HI3861_UART_DEFINE(inst)                                                                   \
	HI3861_IRQ_DEFINE(inst)                                                                    \
	HI3861_PINCTRL_DEFINE(inst);                                                               \
	static struct hi3861_uart_data hi3861_uart_data_##inst;                                    \
	static const struct hi3861_uart_config hi3861_uart_config_##inst = {                       \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.clock_frequency = DT_INST_PROP(inst, clock_frequency),                            \
		.hw_flow_control_supported = DT_INST_PROP(inst, hw_flow_control_supported),        \
		.initial_config =                                                                  \
			{                                                                          \
				.baudrate = DT_INST_PROP(inst, current_speed),                     \
				.parity = DT_INST_ENUM_IDX(inst, parity),                          \
				.stop_bits = DT_INST_ENUM_IDX_OR(inst, stop_bits,                  \
								 UART_CFG_STOP_BITS_1),            \
				.data_bits = DT_INST_PROP_OR(inst, data_bits, 8) - 5,              \
				.flow_ctrl = DT_INST_PROP(inst, hw_flow_control)                   \
						     ? UART_CFG_FLOW_CTRL_RTS_CTS                  \
						     : UART_CFG_FLOW_CTRL_NONE,                    \
			},                                                                         \
		HI3861_IRQ_CONFIG(inst) HI3861_PINCTRL_CONFIG(inst)};                              \
	DEVICE_DT_INST_DEFINE(inst, hi3861_uart_init, NULL, &hi3861_uart_data_##inst,              \
			      &hi3861_uart_config_##inst, PRE_KERNEL_1,                            \
			      CONFIG_SERIAL_INIT_PRIORITY, &hi3861_uart_api);

DT_INST_FOREACH_STATUS_OKAY(HI3861_UART_DEFINE)
