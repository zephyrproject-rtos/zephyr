/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_litei2c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_litex_litei2c, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#include <soc.h>

#define I2C_LITEX_ANY_HAS_IRQ DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
#define I2C_LITEX_ALL_HAS_IRQ DT_ALL_INST_HAS_PROP_STATUS_OKAY(interrupts)

#define I2C_LITEX_HAS_IRQ UTIL_OR(I2C_LITEX_ALL_HAS_IRQ, config->has_irq)

#define MASTER_STATUS_TX_READY_OFFSET 0x0
#define MASTER_STATUS_RX_READY_OFFSET 0x1
#define MASTER_STATUS_NACK_OFFSET     0x8

struct i2c_litex_litei2c_config {
	uint32_t phy_speed_mode_addr;
	uint32_t master_active_addr;
	uint32_t master_settings_addr;
	uint32_t master_addr_addr;
	uint32_t master_rxtx_addr;
	uint32_t master_status_addr;
	uint32_t bitrate;
#if I2C_LITEX_ANY_HAS_IRQ
	uint32_t master_ev_pending_addr;
	uint32_t master_ev_enable_addr;
	void (*irq_config_func)(const struct device *dev);
#if !I2C_LITEX_ALL_HAS_IRQ
	bool has_irq;
#endif /* !I2C_LITEX_ALL_HAS_IRQ */
#endif /* I2C_LITEX_ANY_HAS_IRQ */
};

struct i2c_context {
	struct i2c_msg *msg;
	uint32_t buf_idx;
	uint8_t num_msgs;
	uint8_t num_msgs_idx;
};

struct i2c_litex_litei2c_data {
	struct k_mutex mutex;
	struct i2c_context context;
	uint8_t len_rx;
#if I2C_LITEX_ANY_HAS_IRQ
	struct k_sem sem_rx_ready;
	int ret;
#endif /* I2C_LITEX_ANY_HAS_IRQ */
};

static int i2c_litex_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;

	if (I2C_ADDR_10_BITS & dev_config) {
		return -ENOTSUP;
	}

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Setup speed to use */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		litex_write8(0, config->phy_speed_mode_addr);
		break;
	case I2C_SPEED_FAST:
		litex_write8(1, config->phy_speed_mode_addr);
		break;
	case I2C_SPEED_FAST_PLUS:
		litex_write8(2, config->phy_speed_mode_addr);
		break;
	default:
		k_mutex_unlock(&data->mutex);
		return -ENOTSUP;
	}

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int i2c_litex_get_config(const struct device *dev, uint32_t *config)
{
	const struct i2c_litex_litei2c_config *dev_config = dev->config;

	*config = I2C_MODE_CONTROLLER;

	switch (litex_read8(dev_config->phy_speed_mode_addr)) {
	case 0:
		*config |= I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case 1:
		*config |= I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	case 2:
		*config |= I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
		break;
	default:
		break;
	}

	return 0;
}

static int i2c_litex_write_settings(const struct device *dev, uint8_t len_tx, uint8_t len_rx,
				    bool recover)
{
	const struct i2c_litex_litei2c_config *config = dev->config;

	uint32_t settings = len_tx | (len_rx << 8) | (recover << 16);

	litex_write32(settings, config->master_settings_addr);

	return 0;
}

static inline uint8_t get_write_bytes_from_i2c_msg(struct i2c_context *context, uint8_t *data,
						   uint8_t data_len, bool *with_stop)
{
	uint8_t idx = 0;
	uint8_t to_copy;
	uint32_t msg_len;

	while ((data_len > 0) && (context->num_msgs_idx < context->num_msgs) &&
	       !(context->msg[context->num_msgs_idx].flags & I2C_MSG_READ)) {

		msg_len = context->msg[context->num_msgs_idx].len - context->buf_idx;
		to_copy = min(data_len, msg_len);

		memcpy(&data[idx], &context->msg[context->num_msgs_idx].buf[context->buf_idx],
		       to_copy);

		idx += to_copy;
		data_len -= to_copy;
		if (to_copy >= msg_len) {
			context->num_msgs_idx++;
			context->buf_idx = 0;

			if (context->msg[context->num_msgs_idx - 1].flags & I2C_MSG_STOP) {
				*with_stop = true;
				return idx;
			}
		} else {
			context->buf_idx += to_copy;
		}
	}

	if ((data_len == 0) && (context->num_msgs_idx < context->num_msgs) &&
	    !(context->msg[context->num_msgs_idx].flags & I2C_MSG_READ)) {
		/* Signal that there is more data to write */
		return idx + 1;
	}

	return idx;
}

static inline bool next_msg_is_available(struct i2c_context *context)
{
	return (context->num_msgs_idx < context->num_msgs);
}

static inline uint8_t get_read_bytes_len_from_i2c_msg(struct i2c_context *context,
						      uint8_t max_data_len)
{
	uint32_t counter = 0;
	uint32_t buf_idx = context->buf_idx;
	uint8_t num_msgs_idx = context->num_msgs_idx;

	while ((counter < max_data_len) && (num_msgs_idx < context->num_msgs) &&
	       (context->msg[num_msgs_idx].flags & I2C_MSG_READ)) {
		counter += context->msg[num_msgs_idx].len - buf_idx;

		if (context->msg[num_msgs_idx].flags & I2C_MSG_STOP) {
			break;
		}
		num_msgs_idx++;
		buf_idx = 0;
	}

	return min(counter, max_data_len);
}

static inline uint8_t set_read_bytes_from_i2c_msg(struct i2c_context *context, uint8_t *data,
						  uint8_t data_len)
{
	uint8_t idx = 0;
	uint8_t to_copy;
	uint32_t msg_len;

	while ((data_len > 0) && (context->num_msgs_idx < context->num_msgs) &&
	       (context->msg[context->num_msgs_idx].flags & I2C_MSG_READ)) {

		msg_len = context->msg[context->num_msgs_idx].len - context->buf_idx;
		to_copy = min(data_len, msg_len);

		memcpy(&context->msg[context->num_msgs_idx].buf[context->buf_idx], &data[idx],
		       to_copy);

		idx += to_copy;
		data_len -= to_copy;
		if (to_copy >= msg_len) {
			context->num_msgs_idx++;
			context->buf_idx = 0;
		} else {
			context->buf_idx += to_copy;
		}
	}

	return idx;
}

static void i2c_litex_i2c_do_tx(const struct device *dev)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;
	uint32_t txd = 0U;
	uint8_t len_rx = 0;
	uint8_t len_tx;
	uint8_t tx_buf[4] = {0};
	bool with_stop = false;

	len_tx = get_write_bytes_from_i2c_msg(&data->context, tx_buf, sizeof(tx_buf), &with_stop);

	switch (len_tx) {
	case 5:
	case 4:
		txd = sys_get_be32(tx_buf);
		break;
	case 3:
		txd = sys_get_be24(tx_buf);
		break;
	case 2:
		txd = sys_get_be16(tx_buf);
		break;
	default:
		txd = tx_buf[0];
		break;
	}

	if (!with_stop) {
		len_rx = get_read_bytes_len_from_i2c_msg(&data->context, 5);
	}

	data->len_rx = min(len_rx, 4);

	LOG_DBG("len_tx: %d, len_rx: %d", len_tx, len_rx);
	i2c_litex_write_settings(dev, len_tx, len_rx, false);

	LOG_DBG("txd: 0x%x", txd);
	litex_write32(txd, config->master_rxtx_addr);
}

static int i2c_litex_i2c_do_rx(const struct device *dev)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;
	uint32_t rxd;
	uint8_t rx_buf[4] = {0};

	if (litex_read16(config->master_status_addr) & BIT(MASTER_STATUS_NACK_OFFSET)) {
		/* NACK received, clear RX FIFO */
		(void)litex_read32(config->master_rxtx_addr);

		return -EIO;
	}

	rxd = litex_read32(config->master_rxtx_addr);

	LOG_DBG("rxd: 0x%x", rxd);

	switch (data->len_rx) {
	case 4:
		sys_put_be32(rxd, rx_buf);
		break;
	case 3:
		sys_put_be24(rxd, rx_buf);
		break;
	case 2:
		sys_put_be16(rxd, rx_buf);
		break;
	case 1:
		rx_buf[0] = rxd;
		break;
	default:
		return 0;
	}

	set_read_bytes_from_i2c_msg(&data->context, rx_buf, data->len_rx);

	return 0;
}

static int i2c_litex_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	data->context.msg = msgs;
	data->context.num_msgs = num_msgs;
	data->context.num_msgs_idx = 0;
	data->context.buf_idx = 0;

	litex_write8(1, config->master_active_addr);

	/* Flush RX buffer */
	while ((litex_read8(config->master_status_addr) &
		BIT(MASTER_STATUS_RX_READY_OFFSET))) {
		(void)litex_read32(config->master_rxtx_addr);
	}

	while (!(litex_read8(config->master_status_addr) &
		BIT(MASTER_STATUS_TX_READY_OFFSET))) {
		(void)litex_read32(config->master_rxtx_addr);
	}

	LOG_DBG("addr: 0x%x", addr);
	litex_write8((uint8_t)addr, config->master_addr_addr);

#if I2C_LITEX_ANY_HAS_IRQ
	if (I2C_LITEX_HAS_IRQ) {
		litex_write8(BIT(0), config->master_ev_pending_addr);
		litex_write8(BIT(0), config->master_ev_enable_addr);
		k_sem_reset(&data->sem_rx_ready);

		i2c_litex_i2c_do_tx(dev);

		k_sem_take(&data->sem_rx_ready, K_FOREVER);

		ret = data->ret;

		k_mutex_unlock(&data->mutex);

		return ret;
	}
#endif /* I2C_LITEX_ANY_HAS_IRQ */

	do {
		i2c_litex_i2c_do_tx(dev);

		while (!(litex_read8(config->master_status_addr) &
			BIT(MASTER_STATUS_RX_READY_OFFSET))) {
			/* Wait until RX is ready */
		}

		ret = i2c_litex_i2c_do_rx(dev);
	} while ((ret == 0) && next_msg_is_available(&data->context));

	litex_write8(0, config->master_active_addr);

	k_mutex_unlock(&data->mutex);

	return ret;
}

static int i2c_litex_recover_bus(const struct device *dev)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	litex_write8(1, config->master_active_addr);

	i2c_litex_write_settings(dev, 0, 0, true);

	while (!(litex_read8(config->master_status_addr) & BIT(MASTER_STATUS_TX_READY_OFFSET))) {
		/* Wait for TX ready */
	}

	litex_write32(0, config->master_rxtx_addr);

	while (!(litex_read8(config->master_status_addr) & BIT(MASTER_STATUS_RX_READY_OFFSET))) {
		/* Wait for RX data */
	}

	(void)litex_read32(config->master_rxtx_addr);

	litex_write8(0, config->master_active_addr);

	k_mutex_unlock(&data->mutex);

	return 0;
}

#if I2C_LITEX_ANY_HAS_IRQ
static void i2c_litex_irq_handler(const struct device *dev)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;
	int ret;

	if (litex_read8(config->master_ev_pending_addr) & BIT(0)) {
		ret = i2c_litex_i2c_do_rx(dev);

		/* ack reader irq */
		litex_write8(BIT(0), config->master_ev_pending_addr);

		if ((ret == 0) && next_msg_is_available(&data->context)) {
			i2c_litex_i2c_do_tx(dev);
		} else {
			litex_write8(0, config->master_ev_enable_addr);
			litex_write8(0, config->master_active_addr);

			data->ret = ret;

			k_sem_give(&data->sem_rx_ready);
		}
	}
}
#endif /* I2C_LITEX_ANY_HAS_IRQ */

static int i2c_litex_init(const struct device *dev)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	int ret;

	ret = i2c_litex_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (ret != 0) {
		LOG_ERR("failed to configure I2C: %d", ret);
	}

#if I2C_LITEX_ANY_HAS_IRQ
	if (I2C_LITEX_HAS_IRQ) {
		/* Disable interrupts initially */
		litex_write8(0, config->master_ev_enable_addr);

		config->irq_config_func(dev);
	}
#endif /* I2C_LITEX_ANY_HAS_IRQ */

	return ret;
}

static DEVICE_API(i2c, i2c_litex_litei2c_driver_api) = {
	.configure = i2c_litex_configure,
	.get_config = i2c_litex_get_config,
	.transfer = i2c_litex_transfer,
	.recover_bus = i2c_litex_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

/* Device Instantiation */

#define I2C_LITEX_IRQ(n)									   \
	BUILD_ASSERT(DT_INST_REG_HAS_NAME(n, master_ev_pending) &&				   \
	DT_INST_REG_HAS_NAME(n, master_ev_enable), "registers for interrupts missing");		   \
												   \
	static void i2c_litex_irq_config##n(const struct device *dev)				   \
	{											   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_litex_irq_handler,	   \
			    DEVICE_DT_INST_GET(n), 0);						   \
												   \
		irq_enable(DT_INST_IRQN(n));							   \
	};

#define I2C_LITEC_IRQ_DATA(n)									   \
	.sem_rx_ready = Z_SEM_INITIALIZER(i2c_litex_litei2c_data_##n.sem_rx_ready, 0, 1),

#define I2C_LITEC_IRQ_CONFIG(n)									   \
	IF_DISABLED(I2C_LITEX_ALL_HAS_IRQ, (.has_irq = DT_INST_IRQ_HAS_IDX(n, 0),))		   \
	.master_ev_pending_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, master_ev_pending, 0),		   \
	.master_ev_enable_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, master_ev_enable, 0),		   \
	.irq_config_func = COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0),				   \
			   (i2c_litex_irq_config##n), (NULL)),

#define I2C_LITEX_INIT(n)                                                                          \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0), (I2C_LITEX_IRQ(n)))				   \
												   \
	static struct i2c_litex_litei2c_data i2c_litex_litei2c_data_##n = {			   \
		.mutex = Z_MUTEX_INITIALIZER(i2c_litex_litei2c_data_##n.mutex),			   \
		IF_ENABLED(I2C_LITEX_ANY_HAS_IRQ, (I2C_LITEC_IRQ_DATA(n)))			   \
	};											   \
												   \
	static const struct i2c_litex_litei2c_config i2c_litex_litei2c_config_##n = {              \
		.phy_speed_mode_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_speed_mode),                \
		.master_active_addr = DT_INST_REG_ADDR_BY_NAME(n, master_active),                  \
		.master_settings_addr = DT_INST_REG_ADDR_BY_NAME(n, master_settings),              \
		.master_addr_addr = DT_INST_REG_ADDR_BY_NAME(n, master_addr),                      \
		.master_rxtx_addr = DT_INST_REG_ADDR_BY_NAME(n, master_rxtx),                      \
		.master_status_addr = DT_INST_REG_ADDR_BY_NAME(n, master_status),                  \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		IF_ENABLED(I2C_LITEX_ANY_HAS_IRQ, (I2C_LITEC_IRQ_CONFIG(n)))                    \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_litex_init, NULL, &i2c_litex_litei2c_data_##n,            \
				  &i2c_litex_litei2c_config_##n, POST_KERNEL,                      \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_litex_litei2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_LITEX_INIT)
