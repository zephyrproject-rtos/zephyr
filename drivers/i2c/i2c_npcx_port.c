/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_i2c_port

/**
 * @file
 * @brief Nuvoton NPCX smb/i2c port driver
 *
 * This file contains the driver of SMBus/I2C buses (ports) which provides
 * pin-muxing for each i2c io-pads. In order to support "SMBus Multi-Bus"
 * feature, please refer the diagram below, the driver alsp provides connection
 * between Zephyr i2c api functions and i2c controller driver which provides
 * full support for SMBus/I2C transactions.
 *
 *                           Port SEL
 *                             |
 *                           |\|
 *           SCL_N Port 0----| \     +--------------+
 *           SDA_N Port 0----|  |----|   SMB/I2C    |
 *                           |  |----| Controller N |
 *           SCL_N Port 1----|  |    +--------------+
 *           SDA_N Port 1----| /
 *                           |/
 *
 */

#include <assert.h>
#include <drivers/clock_control.h>
#include <drivers/i2c.h>
#include <dt-bindings/i2c/i2c.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_npcx_port, LOG_LEVEL_ERR);

#include "i2c_npcx_controller.h"
#include "i2c-priv.h"

/* Device config */
struct i2c_npcx_port_config {
	/* pinmux configuration */
	const uint8_t   alts_size;
	const struct npcx_alt *alts_list;
	uint32_t bitrate;
	uint8_t port;
	const struct device *i2c_ctrl;
};

/* Driver convenience defines */
#define DRV_CONFIG(dev) \
	((const struct i2c_npcx_port_config *)(dev)->config)

/* I2C api functions */
static int i2c_npcx_port_configure(const struct device *dev,
							uint32_t dev_config)
{
	const struct i2c_npcx_port_config *const config = DRV_CONFIG(dev);

	if (config->i2c_ctrl == NULL) {
		LOG_ERR("Cannot find i2c controller on port%02x!",
								config->port);
		return -EIO;
	}

	if (!(dev_config & I2C_MODE_MASTER)) {
		return -ENOTSUP;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	/* Configure i2c controller */
	return npcx_i2c_ctrl_configure(config->i2c_ctrl, dev_config);
}

static int i2c_npcx_port_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_npcx_port_config *const config = DRV_CONFIG(dev);
	uint32_t speed;
	int ret;

	if (config->i2c_ctrl == NULL) {
		LOG_ERR("Cannot find i2c controller on port%02x!", config->port);
		return -EIO;
	}

	ret = npcx_i2c_ctrl_get_speed(config->i2c_ctrl, &speed);
	if (!ret) {
		*dev_config = (I2C_MODE_MASTER | speed);
	}

	return ret;
}

static int i2c_npcx_port_transfer(const struct device *dev,
		struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_npcx_port_config *const config = DRV_CONFIG(dev);
	int ret = 0;
	int idx_ctrl = (config->port & 0xF0) >> 4;
	int idx_port = (config->port & 0x0F);

	if (config->i2c_ctrl == NULL) {
		LOG_ERR("Cannot find i2c controller on port%02x!",
								config->port);
		return -EIO;
	}

	/* Lock mutex of i2c/smb controller */
	npcx_i2c_ctrl_mutex_lock(config->i2c_ctrl);

	/* Switch correct port for i2c controller first */
	npcx_pinctrl_i2c_port_sel(idx_ctrl, idx_port);

	/* Start transaction with i2c controller */
	ret = npcx_i2c_ctrl_transfer(config->i2c_ctrl, msgs, num_msgs, addr,
								config->port);

	/* Unlock mutex of i2c/smb controller */
	npcx_i2c_ctrl_mutex_unlock(config->i2c_ctrl);

	return ret;
}

/* I2C driver registration */
static int i2c_npcx_port_init(const struct device *dev)
{
	const struct i2c_npcx_port_config *const config = DRV_CONFIG(dev);
	uint32_t i2c_config;
	int ret;

	/* Configure pin-mux for I2C device */
	npcx_pinctrl_mux_configure(config->alts_list, config->alts_size, 1);

	/* Setup initial i2c configuration */
	i2c_config = (I2C_MODE_MASTER | i2c_map_dt_bitrate(config->bitrate));
	ret = i2c_npcx_port_configure(dev, i2c_config);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static const struct i2c_driver_api i2c_port_npcx_driver_api = {
	.configure = i2c_npcx_port_configure,
	.get_config = i2c_npcx_port_get_config,
	.transfer = i2c_npcx_port_transfer,
};

/* I2C port init macro functions */
#define NPCX_I2C_PORT_INIT(inst)                                               \
	static const struct npcx_alt i2c_port_alts##inst[] =                   \
					NPCX_DT_ALT_ITEMS_LIST(inst);          \
									       \
	static const struct i2c_npcx_port_config i2c_npcx_port_cfg_##inst = {  \
		.port = DT_INST_PROP(inst, port),                              \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                \
		.alts_size = ARRAY_SIZE(i2c_port_alts##inst),                  \
		.alts_list = i2c_port_alts##inst,                              \
		.i2c_ctrl = DEVICE_DT_GET(DT_INST_PHANDLE(inst, controller)),  \
	};                                                                     \
									       \
	DEVICE_DT_INST_DEFINE(inst,                                            \
			    i2c_npcx_port_init,                                \
			    NULL, NULL,                                        \
			    &i2c_npcx_port_cfg_##inst,                         \
			    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			    &i2c_port_npcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPCX_I2C_PORT_INIT)
