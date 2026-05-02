/*
 * Copyright (c) 2021,2025 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT neorv32_uart

#include <zephyr/device.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>

#include <soc.h>

LOG_MODULE_REGISTER(uart_neorv32, CONFIG_UART_LOG_LEVEL);

/* NEORV32 UART registers offsets */
#define NEORV32_UART_CTRL 0x00
#define NEORV32_UART_DATA 0x04

/* NEORV32 UART CTRL register bits */
#define NEORV32_UART_CTRL_EN               BIT(0)
#define NEORV32_UART_CTRL_SIM_MODE         BIT(1)
#define NEORV32_UART_CTRL_HWFC_EN          BIT(2)
#define NEORV32_UART_CTRL_PRSC             GENMASK(5, 3)
#define NEORV32_UART_CTRL_BAUD             GENMASK(15, 6)
#define NEORV32_UART_CTRL_RX_NEMPTY        BIT(16)
#define NEORV32_UART_CTRL_RX_HALF          BIT(17)
#define NEORV32_UART_CTRL_RX_FULL          BIT(18)
#define NEORV32_UART_CTRL_TX_EMPTY         BIT(19)
#define NEORV32_UART_CTRL_TX_NHALF         BIT(20)
#define NEORV32_UART_CTRL_TX_FULL          BIT(21)
#define NEORV32_UART_CTRL_IRQ_RX_NEMPTY    BIT(22)
#define NEORV32_UART_CTRL_IRQ_RX_HALF      BIT(23)
#define NEORV32_UART_CTRL_IRQ_RX_FULL      BIT(24)
#define NEORV32_UART_CTRL_IRQ_TX_EMPTY     BIT(25)
#define NEORV32_UART_CTRL_IRQ_TX_NHALF     BIT(26)
#define NEORV32_UART_CTRL_UART_CTRL_RX_CLR BIT(28)
#define NEORV32_UART_CTRL_UART_CTRL_TX_CLR BIT(29)
#define NEORV32_UART_CTRL_RX_OVER          BIT(30)
#define NEORV32_UART_CTRL_TX_BUSY          BIT(31)

/* NEORV32 UART DATA register bits */
#define NEORV32_UART_DATA_RTX          GENMASK(7, 0)
#define NEORV32_UART_DATA_RX_FIFO_SIZE GENMASK(11, 8)
#define NEORV32_UART_DATA_TX_FIFO_SIZE GENMASK(15, 12)

struct neorv32_uart_config {
	const struct device *syscon;
	uint32_t feature_mask;
	mm_reg_t base;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct neorv32_uart_data {
	struct uart_config uart_cfg;
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *callback_data;
	uint32_t last_ctrl;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static inline uint32_t neorv32_uart_read_ctrl(const struct device *dev)
{
	const struct neorv32_uart_config *config = dev->config;

	return sys_read32(config->base + NEORV32_UART_CTRL);
}

static inline void neorv32_uart_write_ctrl(const struct device *dev, uint32_t ctrl)
{
	const struct neorv32_uart_config *config = dev->config;

	sys_write32(ctrl, config->base + NEORV32_UART_CTRL);
}

static inline uint32_t neorv32_uart_read_data(const struct device *dev)
{
	const struct neorv32_uart_config *config = dev->config;

	return sys_read32(config->base + NEORV32_UART_DATA);
}

static inline void neorv32_uart_write_data(const struct device *dev, uint32_t data)
{
	const struct neorv32_uart_config *config = dev->config;

	sys_write32(data, config->base + NEORV32_UART_DATA);
}

static int neorv32_uart_poll_in(const struct device *dev, unsigned char *c)
{
	uint32_t ctrl;

	ctrl = neorv32_uart_read_ctrl(dev);
	if ((ctrl & NEORV32_UART_CTRL_RX_NEMPTY) != 0U) {
		*c = neorv32_uart_read_data(dev) & NEORV32_UART_DATA_RTX;
		return 0;
	}

	return -1;
}

static void neorv32_uart_poll_out(const struct device *dev, unsigned char c)
{
	while ((neorv32_uart_read_ctrl(dev) & NEORV32_UART_CTRL_TX_FULL) != 0U) {
	}

	neorv32_uart_write_data(dev, c);
}

static int neorv32_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	const uint16_t baudxx_max = FIELD_GET(NEORV32_UART_CTRL_BAUD, NEORV32_UART_CTRL_BAUD);
	const uint8_t prscx_max = FIELD_GET(NEORV32_UART_CTRL_PRSC, NEORV32_UART_CTRL_PRSC);
	const struct neorv32_uart_config *config = dev->config;
	struct neorv32_uart_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t baudxx = 0;
	uint8_t prscx = 0;
	uint32_t ctrl;
	bool hwfc_en;
	uint32_t clk;
	int err;

	__ASSERT_NO_MSG(cfg != NULL);

	if (cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		LOG_ERR("hardware only supports one stop bit");
		return -ENOTSUP;
	}

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		LOG_ERR("hardware only supports 8 data bits");
		return -ENOTSUP;
	}

	if (cfg->parity != UART_CFG_PARITY_NONE) {
		LOG_ERR("hardware only supports parity mode none");
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		hwfc_en = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		hwfc_en = true;
		break;
	default:
		LOG_ERR("unsupported flow control mode %d", cfg->flow_ctrl);
		return -ENOTSUP;
	}

	err = syscon_read_reg(config->syscon, NEORV32_SYSINFO_CLK, &clk);
	if (err < 0) {
		LOG_ERR("failed to determine clock rate (err %d)", err);
		return -EIO;
	}

	if (cfg->baudrate == 0) {
		LOG_ERR("invalid baud rate 0");
		return -EINVAL;
	}

	/*
	 * Calculate clock prescaler and baud prescaler. Initial prscx = 0 is
	 * clock / 2.
	 */
	baudxx = clk / (2 * cfg->baudrate);
	while (baudxx >= baudxx_max) {
		if ((prscx == 2) || (prscx == 4)) {
			baudxx >>= 3;
		} else {
			baudxx >>= 1;
		}

		prscx++;
	}

	if (prscx > prscx_max) {
		LOG_ERR("unsupported baud rate %d", cfg->baudrate);
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	ctrl = neorv32_uart_read_ctrl(dev);
	ctrl |= NEORV32_UART_CTRL_EN;

	if (hwfc_en) {
		ctrl |= NEORV32_UART_CTRL_HWFC_EN;
	} else {
		ctrl &= ~NEORV32_UART_CTRL_HWFC_EN;
	}

	ctrl &= ~(NEORV32_UART_CTRL_BAUD | NEORV32_UART_CTRL_PRSC);
	ctrl |= FIELD_PREP(NEORV32_UART_CTRL_BAUD, baudxx - 1U) |
		FIELD_PREP(NEORV32_UART_CTRL_PRSC, prscx);

	neorv32_uart_write_ctrl(dev, ctrl);
	data->uart_cfg = *cfg;

	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int neorv32_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct neorv32_uart_data *data = dev->data;

	__ASSERT_NO_MSG(cfg != NULL);

	*cfg = data->uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int neorv32_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	int count = 0;

	if (size <= 0) {
		return 0;
	}

	__ASSERT_NO_MSG(tx_data != NULL);

	while (count < size && (neorv32_uart_read_ctrl(dev) & NEORV32_UART_CTRL_TX_FULL) == 0U) {
		neorv32_uart_write_data(dev, tx_data[count++]);
	}

	return count;
}

static int neorv32_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	int count = 0;

	if (size <= 0) {
		return 0;
	}

	__ASSERT_NO_MSG(rx_data != NULL);

	while (count < size && (neorv32_uart_read_ctrl(dev) & NEORV32_UART_CTRL_RX_NEMPTY) != 0U) {
		rx_data[count++] = neorv32_uart_read_data(dev) & NEORV32_UART_DATA_RTX;
	}

	return count;
}

static void neorv32_uart_irq_tx_enable(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ctrl;

	key = k_spin_lock(&data->lock);

	ctrl = neorv32_uart_read_ctrl(dev);
	ctrl |= NEORV32_UART_CTRL_IRQ_TX_EMPTY;
	neorv32_uart_write_ctrl(dev, ctrl);

	k_spin_unlock(&data->lock, key);
}

static void neorv32_uart_irq_tx_disable(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ctrl;

	key = k_spin_lock(&data->lock);

	ctrl = neorv32_uart_read_ctrl(dev);
	ctrl &= ~NEORV32_UART_CTRL_IRQ_TX_EMPTY;
	neorv32_uart_write_ctrl(dev, ctrl);

	k_spin_unlock(&data->lock, key);
}

static int neorv32_uart_irq_tx_ready(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;

	if ((data->last_ctrl & NEORV32_UART_CTRL_IRQ_TX_EMPTY) == 0U) {
		return 0;
	}

	return (data->last_ctrl & NEORV32_UART_CTRL_TX_FULL) == 0U;
}

static void neorv32_uart_irq_rx_enable(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ctrl;

	key = k_spin_lock(&data->lock);

	ctrl = neorv32_uart_read_ctrl(dev);
	ctrl |= NEORV32_UART_CTRL_IRQ_RX_NEMPTY;
	neorv32_uart_write_ctrl(dev, ctrl);

	k_spin_unlock(&data->lock, key);
}

static void neorv32_uart_irq_rx_disable(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ctrl;

	key = k_spin_lock(&data->lock);

	ctrl = neorv32_uart_read_ctrl(dev);
	ctrl &= ~NEORV32_UART_CTRL_IRQ_RX_NEMPTY;
	neorv32_uart_write_ctrl(dev, ctrl);

	k_spin_unlock(&data->lock, key);
}

static int neorv32_uart_irq_tx_complete(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;

	return (data->last_ctrl & NEORV32_UART_CTRL_TX_BUSY) == 0U;
}

static int neorv32_uart_irq_rx_ready(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;

	if ((data->last_ctrl & NEORV32_UART_CTRL_IRQ_RX_NEMPTY) == 0U) {
		return 0;
	}

	return (data->last_ctrl & NEORV32_UART_CTRL_RX_NEMPTY) != 0U;
}

static int neorv32_uart_irq_is_pending(const struct device *dev)
{
	return (neorv32_uart_irq_tx_ready(dev) ||
		neorv32_uart_irq_rx_ready(dev));
}

static int neorv32_uart_irq_update(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;

	/* Cache the CTRL register for use in the following functions:
	 * - neorv32_uart_irq_tx_complete()
	 * - neorv32_uart_irq_tx_ready()
	 * - neorv32_uart_irq_rx_ready()
	 */
	data->last_ctrl = neorv32_uart_read_ctrl(dev);

	return 1;
}

static void neorv32_uart_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb, void *user_data)
{
	struct neorv32_uart_data *data = dev->data;

	data->callback = cb;
	data->callback_data = user_data;
}

static void neorv32_uart_isr(const struct device *dev)
{
	struct neorv32_uart_data *data = dev->data;
	uart_irq_callback_user_data_t callback = data->callback;

	if (callback) {
		callback(dev, data->callback_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int neorv32_uart_init(const struct device *dev)
{
	const struct neorv32_uart_config *config = dev->config;
	struct neorv32_uart_data *data = dev->data;
	uint32_t features;
	int err;

	if (!device_is_ready(config->syscon)) {
		LOG_ERR("syscon device not ready");
		return -EINVAL;
	}

	err = syscon_read_reg(config->syscon, NEORV32_SYSINFO_SOC, &features);
	if (err < 0) {
		LOG_ERR("failed to determine implemented features (err %d)", err);
		return -EIO;
	}

	if ((features & config->feature_mask) == 0) {
		LOG_ERR("neorv32 uart instance not supported");
		return -ENODEV;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return neorv32_uart_configure(dev, &data->uart_cfg);
}

#ifdef CONFIG_PM_DEVICE
static int neorv32_uart_pm_action(const struct device *dev,
				  enum pm_device_action action)
{
	struct neorv32_uart_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ctrl;

	key = k_spin_lock(&data->lock);
	ctrl = neorv32_uart_read_ctrl(dev);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ctrl &= ~(NEORV32_UART_CTRL_EN);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ctrl |= NEORV32_UART_CTRL_EN;
		break;
	default:
		return -ENOTSUP;
	}

	neorv32_uart_write_ctrl(dev, ctrl);
	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(uart, neorv32_uart_driver_api) = {
	.poll_in = neorv32_uart_poll_in,
	.poll_out = neorv32_uart_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = neorv32_uart_configure,
	.config_get = neorv32_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = neorv32_uart_fifo_fill,
	.fifo_read = neorv32_uart_fifo_read,
	.irq_tx_enable = neorv32_uart_irq_tx_enable,
	.irq_tx_disable = neorv32_uart_irq_tx_disable,
	.irq_tx_ready = neorv32_uart_irq_tx_ready,
	.irq_rx_enable = neorv32_uart_irq_rx_enable,
	.irq_rx_disable = neorv32_uart_irq_rx_disable,
	.irq_tx_complete = neorv32_uart_irq_tx_complete,
	.irq_rx_ready = neorv32_uart_irq_rx_ready,
	.irq_is_pending = neorv32_uart_irq_is_pending,
	.irq_update = neorv32_uart_irq_update,
	.irq_callback_set = neorv32_uart_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define NEORV32_UART_CONFIG_FUNC(node_id, n)	\
	static void neorv32_uart_config_func_##n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_IRQ_BY_NAME(node_id, tx, irq),		\
			    DT_IRQ_BY_NAME(node_id, tx, priority),	\
			    neorv32_uart_isr,				\
			    DEVICE_DT_GET(node_id), 0);			\
		irq_enable(DT_IRQ_BY_NAME(node_id, tx, irq));		\
									\
		IRQ_CONNECT(DT_IRQ_BY_NAME(node_id, rx, irq),		\
			    DT_IRQ_BY_NAME(node_id, rx, priority),	\
			    neorv32_uart_isr,				\
			    DEVICE_DT_GET(node_id), 0);			\
		irq_enable(DT_IRQ_BY_NAME(node_id, rx, irq));		\
	}
#define NEORV32_UART_CONFIG_INIT(node_id, n)				\
	.irq_config_func = neorv32_uart_config_func_##n,
#else
#define NEORV32_UART_CONFIG_FUNC(node_id, n)
#define NEORV32_UART_CONFIG_INIT(node_id, n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define NEORV32_UART_INIT(node_id, n)					\
	NEORV32_UART_CONFIG_FUNC(node_id, n)				\
									\
	static struct neorv32_uart_data neorv32_uart_##n##_data = {	\
		.uart_cfg = {						\
			.baudrate = DT_PROP(node_id, current_speed),	\
			.parity = DT_ENUM_IDX(node_id, parity),		\
			.stop_bits = UART_CFG_STOP_BITS_1,		\
			.data_bits = UART_CFG_DATA_BITS_8,		\
			.flow_ctrl = DT_PROP(node_id, hw_flow_control) ? \
				UART_CFG_FLOW_CTRL_RTS_CTS :		\
				UART_CFG_FLOW_CTRL_NONE,		\
		},							\
	};								\
									\
	static const struct neorv32_uart_config neorv32_uart_##n##_config = { \
		.syscon = DEVICE_DT_GET(DT_PHANDLE(node_id, syscon)),	\
		.feature_mask = NEORV32_SYSINFO_SOC_IO_UART##n,	\
		.base = DT_REG_ADDR(node_id),				\
		NEORV32_UART_CONFIG_INIT(node_id, n)			\
	};								\
									\
	PM_DEVICE_DT_DEFINE(node_id, neorv32_uart_pm_action);		\
									\
	DEVICE_DT_DEFINE(node_id, &neorv32_uart_init,			\
			 PM_DEVICE_DT_GET(node_id),			\
			 &neorv32_uart_##n##_data,			\
			 &neorv32_uart_##n##_config,			\
			 PRE_KERNEL_1,					\
			 CONFIG_SERIAL_INIT_PRIORITY,			\
			 &neorv32_uart_driver_api)

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(uart0), DT_DRV_COMPAT, okay)
NEORV32_UART_INIT(DT_NODELABEL(uart0), 0);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(uart1), DT_DRV_COMPAT, okay)
NEORV32_UART_INIT(DT_NODELABEL(uart1), 1);
#endif
