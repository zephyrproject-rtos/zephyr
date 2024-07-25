/*
 * Copyright (c) 2021-2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_w91_i2c

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_telink);

#include <zephyr/drivers/i2c.h>
#include "i2c-priv.h"
#include <zephyr/drivers/pinctrl.h>
#include <ipc/ipc_based_driver.h>
#include <stdlib.h>

enum {
	IPC_DISPATCHER_I2C_CONFIGURE_EVENT = IPC_DISPATCHER_I2C,
	IPC_DISPATCHER_I2C_MASTER_READ_EVENT,
	IPC_DISPATCHER_I2C_MASTER_WRITE_EVENT,
};

enum i2c_role {
	I2C_ROLE_MASTER,
	I2C_ROLE_SLAVE,
};

enum i2c_addr_len {
	I2C_ADDR_LEN_7BIT,
	I2C_ADDR_LEN_10BIT,
};

struct i2c_ipc_cfg {
	enum i2c_role role;
	enum i2c_addr_len addr_len;
	uint32_t master_clock;
	uint16_t slave_addr;
	uint8_t dma_en;
	uint8_t pull_up_en;
};

struct i2c_master_tx_req {
	uint16_t addr;
	uint32_t tx_len;
	uint8_t *tx_buf;
};

struct i2c_master_rx_req {
	uint16_t addr;
	uint32_t rx_len;
};

struct i2c_master_rx_resp {
	int err;
	uint32_t len;
	uint8_t *buffer;
};

/* I2C configuration structure */
struct i2c_w91_cfg {
	uint32_t bitrate;
	const struct pinctrl_dev_config *pcfg;
	uint8_t instance_id;
};

/* I2C data structure */
struct i2c_w91_data {
	struct k_mutex mutex;
	struct ipc_based_driver ipc; /* ipc driver part */
};

/* APIs implementation: pin configure */
static size_t pack_i2c_w91_ipc_configure(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct i2c_ipc_cfg *p_i2c_cfg = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(p_i2c_cfg->role) +
			       sizeof(p_i2c_cfg->addr_len) + sizeof(p_i2c_cfg->dma_en) +
			       sizeof(p_i2c_cfg->master_clock) + sizeof(p_i2c_cfg->pull_up_en);
	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_I2C_CONFIGURE_EVENT, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_cfg->role);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_cfg->addr_len);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_cfg->dma_en);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_cfg->master_clock);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_cfg->pull_up_en);
	}
	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(i2c_w91_ipc_configure);

static int i2c_w91_ipc_configure(const struct device *dev, uint32_t clock_speed)
{
	int err = -1;
	struct i2c_ipc_cfg i2c_config;

	i2c_config.role = I2C_ROLE_MASTER;
	i2c_config.addr_len = I2C_ADDR_LEN_7BIT;
	i2c_config.dma_en = 0;
	i2c_config.master_clock = clock_speed;
	i2c_config.pull_up_en = 1;
	struct ipc_based_driver *ipc_data = &((struct i2c_w91_data *)dev->data)->ipc;

	uint8_t inst = ((struct i2c_w91_cfg *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		i2c_w91_ipc_configure, &i2c_config, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: master_rx */
static size_t pack_i2c_w91_ipc_master_read(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct i2c_master_rx_req *p_i2c_master_rx = unpack_data;
	size_t pack_data_len =
		sizeof(uint32_t) + sizeof(p_i2c_master_rx->addr) + sizeof(p_i2c_master_rx->rx_len);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_I2C_MASTER_READ_EVENT, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_master_rx->addr);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_master_rx->rx_len);
	}
	return pack_data_len;
}

static void unpack_i2c_w91_ipc_master_read(void *unpack_data, const uint8_t *pack_data,
					   size_t pack_data_len)
{
	struct i2c_master_rx_resp *p_rx_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_rx_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_rx_resp->len);
	size_t expect_len =
		sizeof(uint32_t) + sizeof(p_rx_resp->err) + sizeof(p_rx_resp->len) + p_rx_resp->len;

	if (expect_len != pack_data_len) {
		LOG_ERR("Invalid RX length (exp %d/ got %d)", expect_len, pack_data_len);
		return;
	}
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_rx_resp->buffer, p_rx_resp->len);
}

static int i2c_w91_ipc_master_read(const struct device *dev, uint16_t addr, uint8_t *rx_buf,
				   uint32_t len)
{
	struct i2c_master_rx_req rx_req = {.addr = addr, .rx_len = len};
	struct i2c_master_rx_resp rx_resp = {
		.err = -1,
		.buffer = rx_buf,
	};
	struct ipc_based_driver *ipc_data = &((struct i2c_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct i2c_w91_cfg *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, i2c_w91_ipc_master_read, &rx_req, &rx_resp,
				      CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	if (rx_resp.err != 0) {
		LOG_ERR("RX failed, ret(%d)", rx_resp.err);
	}
	return rx_resp.err;
}

/* APIs implementation: master_tx */
static size_t pack_i2c_w91_ipc_master_write(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct i2c_master_tx_req *p_i2c_master_tx = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) + sizeof(p_i2c_master_tx->addr) +
			       sizeof(p_i2c_master_tx->tx_len) + p_i2c_master_tx->tx_len;
	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_I2C_MASTER_WRITE_EVENT, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_master_tx->addr);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_i2c_master_tx->tx_len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_i2c_master_tx->tx_buf,
					  p_i2c_master_tx->tx_len);
	}
	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(i2c_w91_ipc_master_write);

static int i2c_w91_ipc_master_write(const struct device *dev, uint16_t addr, uint8_t *tx_buf,
				    uint32_t len)
{
	int err = -1;
	struct i2c_master_tx_req i2c_master_tx = {
		.addr = addr,
		.tx_len = len,
		.tx_buf = tx_buf,
	};
	struct ipc_based_driver *ipc_data = &((struct i2c_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct i2c_w91_cfg *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, i2c_w91_ipc_master_write, &i2c_master_tx,
				      &err, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	return err;
}

/* API implementation: configure */
static int i2c_w91_configure(const struct device *dev, uint32_t dev_config)
{
	ARG_UNUSED(dev);

	uint32_t i2c_speed = 0u;

	/* check address size */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bits address is not supported");
		return -ENOTSUP;
	}

	/* check I2C Master/Slave configuration */
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("I2C slave is not implemented");
		return -ENOTSUP;
	}

	/* check i2c speed */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_speed = 100000u;
		break;

	case I2C_SPEED_FAST:
		i2c_speed = 400000U;
		break;

	case I2C_SPEED_FAST_PLUS:
	case I2C_SPEED_HIGH:
	case I2C_SPEED_ULTRA:
	default:
		LOG_ERR("Unsupported I2C speed requested");
		return -ENOTSUP;
	}

	return i2c_w91_ipc_configure(dev, i2c_speed);
}

/* API implementation: transfer */
static int i2c_w91_transfer(const struct device *dev,
			    struct i2c_msg *msgs,
			    uint8_t num_msgs,
			    uint16_t addr)
{
	int status = 0;
	struct i2c_w91_data *data = dev->data;

	/* get the mutex */
	k_mutex_lock(&data->mutex, K_FOREVER);

	/* loop through all messages */
	for (int i = 0; i < num_msgs; i++) {
		/* check addr size */
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			LOG_ERR("10-bits address is not supported");
			k_mutex_unlock(&data->mutex);
			return -ENOTSUP;
		}

		/* transfer data */
		if (msgs[i].flags & I2C_MSG_READ) {
			status = i2c_w91_ipc_master_read(dev, addr, msgs[i].buf, msgs[i].len);
		} else {
			status = i2c_w91_ipc_master_write(dev, addr, msgs[i].buf, msgs[i].len);
		}

		/* check status */
		if (status) {
			LOG_ERR("Failed to transfer I2C messages\n");
			k_mutex_unlock(&data->mutex);
			return -EIO;
		}
	}

	/* release the mutex */
	k_mutex_unlock(&data->mutex);

	return status;
};

/* API implementation: init */
static int i2c_w91_init(const struct device *dev)
{
	int status = 0;
	const struct i2c_w91_cfg *cfg = dev->config;
	struct i2c_w91_data *data = dev->data;

	ipc_based_driver_init(&data->ipc);

	uint32_t dev_config = (I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(cfg->bitrate));

	/* init mutex */
	k_mutex_init(&data->mutex);

	/* configure pins */
	status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (status != 0) {
		LOG_ERR("Failed to configure I2C pins");
		return status;
	}

	/* config i2c on startup */
	status = i2c_w91_configure(dev, dev_config);
	if (status != 0) {
		LOG_ERR("Failed to configure I2C on init");
		return status;
	}

	return status;
}

/* I2C driver APIs structure */
static const struct i2c_driver_api i2c_w91_api = {
	.configure = i2c_w91_configure,
	.transfer = i2c_w91_transfer,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "unsupported I2C instance");

/* I2C driver registration */
#define I2C_W91_INIT(inst)					      \
								      \
	PINCTRL_DT_INST_DEFINE(inst);				      \
								      \
	static struct i2c_w91_data i2c_w91_data_##inst;		      \
								      \
	const static struct i2c_w91_cfg i2c_w91_cfg_##inst = {	      \
		.bitrate = DT_INST_PROP(inst, clock_frequency),	      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	      \
		.instance_id = inst,	      \
	};							      \
								      \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_w91_init,		      \
				  NULL,				      \
				  &i2c_w91_data_##inst,		      \
				  &i2c_w91_cfg_##inst,		      \
				  POST_KERNEL,			      \
				  CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,	      \
				  &i2c_w91_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_W91_INIT)
