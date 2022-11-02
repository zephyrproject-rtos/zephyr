/*
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.winsen-sensor.com/sensors/co2-sensor/mh-z19b.html
 */

#define DT_DRV_COMPAT winsen_mhz19b

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/drivers/sensor/mhz19b.h>
#include "mhz19b.h"

LOG_MODULE_REGISTER(mhz19b, CONFIG_SENSOR_LOG_LEVEL);

/* Table of supported MH-Z19B commands with precomputed checksum */
static const uint8_t mhz19b_cmds[MHZ19B_CMD_IDX_MAX][MHZ19B_BUF_LEN] = {
	[MHZ19B_CMD_IDX_GET_CO2] = {
		MHZ19B_HEADER, MHZ19B_RESERVED, MHZ19B_CMD_GET_CO2, MHZ19B_NULL_COUNT(5), 0x79
	},
	[MHZ19B_CMD_IDX_GET_RANGE] = {
		MHZ19B_HEADER, MHZ19B_RESERVED, MHZ19B_CMD_GET_RANGE, MHZ19B_NULL_COUNT(5), 0x64
	},
	[MHZ19B_CMD_IDX_GET_ABC] = {
		MHZ19B_HEADER, MHZ19B_RESERVED, MHZ19B_CMD_GET_ABC, MHZ19B_NULL_COUNT(5), 0x82
	},
	[MHZ19B_CMD_IDX_SET_ABC_ON] = {
		MHZ19B_HEADER, MHZ19B_RESERVED, MHZ19B_CMD_SET_ABC, MHZ19B_ABC_ON,
		MHZ19B_NULL_COUNT(4), 0xE6
	},
	[MHZ19B_CMD_IDX_SET_ABC_OFF] = {
		MHZ19B_HEADER, MHZ19B_RESERVED, MHZ19B_CMD_SET_ABC, MHZ19B_ABC_OFF,
		MHZ19B_NULL_COUNT(4), 0x86
	},
	[MHZ19B_CMD_IDX_SET_RANGE_2000] = {
		MHZ19B_HEADER, MHZ19B_RESERVED, MHZ19B_CMD_SET_RANGE, MHZ19B_NULL_COUNT(3),
		MHZ19B_RANGE_2000, 0x8F
	},
	[MHZ19B_CMD_IDX_SET_RANGE_5000] = {
		MHZ19B_HEADER, MHZ19B_RESERVED, MHZ19B_CMD_SET_RANGE, MHZ19B_NULL_COUNT(3),
		MHZ19B_RANGE_5000, 0xCB
	},
	[MHZ19B_CMD_IDX_SET_RANGE_10000] = {
		MHZ19B_HEADER, MHZ19B_RESERVED, MHZ19B_CMD_SET_RANGE, MHZ19B_NULL_COUNT(3),
		MHZ19B_RANGE_10000, 0x2F
	},
};

static void mhz19b_uart_flush(const struct device *uart_dev)
{
	uint8_t c;

	while (uart_fifo_read(uart_dev, &c, 1) > 0) {
		continue;
	}
}

static uint8_t mhz19b_checksum(const uint8_t *data)
{
	uint8_t cs = 0;

	for (uint8_t i = 1; i < MHZ19B_BUF_LEN - 1; i++) {
		cs += data[i];
	}

	return 0xff - cs + 1;
}

static int mhz19b_send_cmd(const struct device *dev, enum mhz19b_cmd_idx cmd_idx, bool has_rsp)
{
	struct mhz19b_data *data = dev->data;
	const struct mhz19b_cfg *cfg = dev->config;
	int ret;

	/* Make sure last command has been transferred */
	ret = k_sem_take(&data->tx_sem, MHZ19B_WAIT);
	if (ret) {
		return ret;
	}

	data->cmd_idx = cmd_idx;
	data->has_rsp = has_rsp;
	k_sem_reset(&data->rx_sem);

	uart_irq_tx_enable(cfg->uart_dev);

	if (has_rsp) {
		uart_irq_rx_enable(cfg->uart_dev);
		ret = k_sem_take(&data->rx_sem, MHZ19B_WAIT);
	}

	return ret;
}

static inline int mhz19b_send_config(const struct device *dev, enum mhz19b_cmd_idx cmd_idx)
{
	struct mhz19b_data *data = dev->data;
	int ret;

	ret = mhz19b_send_cmd(dev, cmd_idx, true);
	if (ret < 0) {
		return ret;
	}

	if (data->rd_data[MHZ19B_RX_CMD_IDX] != mhz19b_cmds[data->cmd_idx][MHZ19B_TX_CMD_IDX]) {
		return -EINVAL;
	}

	return 0;
}

static inline int mhz19b_poll_data(const struct device *dev, enum mhz19b_cmd_idx cmd_idx)
{
	struct mhz19b_data *data = dev->data;
	uint8_t checksum;
	int ret;

	ret = mhz19b_send_cmd(dev, cmd_idx, true);
	if (ret < 0) {
		return ret;
	}

	checksum = mhz19b_checksum(data->rd_data);
	if (checksum != data->rd_data[MHZ19B_CHECKSUM_IDX]) {
		LOG_DBG("Checksum mismatch: 0x%x != 0x%x", checksum,
			data->rd_data[MHZ19B_CHECKSUM_IDX]);
		return -EBADMSG;
	}

	switch (cmd_idx) {
	case MHZ19B_CMD_IDX_GET_CO2:
		data->data = sys_get_be16(&data->rd_data[2]);
		break;
	case MHZ19B_CMD_IDX_GET_RANGE:
		data->data = sys_get_be16(&data->rd_data[4]);
		break;
	case MHZ19B_CMD_IDX_GET_ABC:
		data->data = data->rd_data[7];
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mhz19b_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct mhz19b_data *data = dev->data;

	if (chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	val->val1 = (int32_t)data->data;
	val->val2 = 0;

	return 0;
}

static int mhz19b_attr_full_scale_cfg(const struct device *dev, int range)
{
	switch (range) {
	case 2000:
		LOG_DBG("Configure range to %d", range);
		return mhz19b_send_config(dev, MHZ19B_CMD_IDX_SET_RANGE_2000);
	case 5000:
		LOG_DBG("Configure range to %d", range);
		return mhz19b_send_config(dev, MHZ19B_CMD_IDX_SET_RANGE_5000);
	case 10000:
		LOG_DBG("Configure range to %d", range);
		return mhz19b_send_config(dev, MHZ19B_CMD_IDX_SET_RANGE_10000);
	default:
		return -ENOTSUP;
	}
}

static int mhz19b_attr_abc_cfg(const struct device *dev, bool on)
{
	if (on) {
		LOG_DBG("%s ABC", "Enable");
		return mhz19b_send_config(dev, MHZ19B_CMD_IDX_SET_ABC_ON);
	}

	LOG_DBG("%s ABC", "Disable");
	return mhz19b_send_config(dev, MHZ19B_CMD_IDX_SET_ABC_OFF);
}

static int mhz19b_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return mhz19b_attr_full_scale_cfg(dev, val->val1);

	case SENSOR_ATTR_MHZ19B_ABC:
		return mhz19b_attr_abc_cfg(dev, val->val1);

	default:
		return -ENOTSUP;
	}
}

static int mhz19b_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	struct mhz19b_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		ret = mhz19b_poll_data(dev, MHZ19B_CMD_IDX_GET_RANGE);
		break;
	case SENSOR_ATTR_MHZ19B_ABC:
		ret = mhz19b_poll_data(dev, MHZ19B_CMD_IDX_GET_ABC);
		break;
	default:
		return -ENOTSUP;
	}

	val->val1 = (int32_t)data->data;
	val->val2 = 0;

	return ret;
}

static int mhz19b_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if (chan == SENSOR_CHAN_CO2 || chan == SENSOR_CHAN_ALL) {
		return mhz19b_poll_data(dev, MHZ19B_CMD_IDX_GET_CO2);
	}

	return -ENOTSUP;
}

static const struct sensor_driver_api mhz19b_api_funcs = {
	.attr_set = mhz19b_attr_set,
	.attr_get = mhz19b_attr_get,
	.sample_fetch = mhz19b_sample_fetch,
	.channel_get = mhz19b_channel_get,
};

static void mhz19b_uart_isr(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	struct mhz19b_data *data = dev->data;

	ARG_UNUSED(user_data);

	if (uart_dev == NULL) {
		return;
	}

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (uart_irq_rx_ready(uart_dev)) {
		data->xfer_bytes += uart_fifo_read(uart_dev, &data->rd_data[data->xfer_bytes],
						   MHZ19B_BUF_LEN - data->xfer_bytes);

		if (data->xfer_bytes == MHZ19B_BUF_LEN) {
			data->xfer_bytes = 0;
			uart_irq_rx_disable(uart_dev);
			k_sem_give(&data->rx_sem);
			if (data->has_rsp) {
				k_sem_give(&data->tx_sem);
			}
		}
	}

	if (uart_irq_tx_ready(uart_dev)) {
		data->xfer_bytes +=
			uart_fifo_fill(uart_dev, &mhz19b_cmds[data->cmd_idx][data->xfer_bytes],
				       MHZ19B_BUF_LEN - data->xfer_bytes);

		if (data->xfer_bytes == MHZ19B_BUF_LEN) {
			data->xfer_bytes = 0;
			uart_irq_tx_disable(uart_dev);
			if (!data->has_rsp) {
				k_sem_give(&data->tx_sem);
			}
		}
	}
}

static int mhz19b_init(const struct device *dev)
{
	struct mhz19b_data *data = dev->data;
	const struct mhz19b_cfg *cfg = dev->config;
	int ret;

	uart_irq_rx_disable(cfg->uart_dev);
	uart_irq_tx_disable(cfg->uart_dev);

	mhz19b_uart_flush(cfg->uart_dev);

	uart_irq_callback_user_data_set(cfg->uart_dev, cfg->cb, (void *)dev);

	k_sem_init(&data->rx_sem, 0, 1);
	k_sem_init(&data->tx_sem, 1, 1);

	/* Configure default detection range */
	ret = mhz19b_attr_full_scale_cfg(dev, cfg->range);
	if (ret != 0) {
		LOG_ERR("Error setting default range %d", cfg->range);
		return ret;
	}

	/* Configure ABC logic */
	ret = mhz19b_attr_abc_cfg(dev, cfg->abc_on);
	if (ret != 0) {
		LOG_ERR("Error setting default ABC %s", cfg->abc_on ? "on" : "off");
	}

	return ret;
}

#define MHZ19B_INIT(inst)									\
												\
	static struct mhz19b_data mhz19b_data_##inst;						\
												\
	static const struct mhz19b_cfg mhz19b_cfg_##inst = {					\
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.range = DT_INST_PROP(inst, maximum_range),					\
		.abc_on = DT_INST_PROP(inst, abc_on),						\
		.cb = mhz19b_uart_isr,								\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mhz19b_init, NULL,					\
			      &mhz19b_data_##inst, &mhz19b_cfg_##inst,				\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &mhz19b_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(MHZ19B_INIT)
