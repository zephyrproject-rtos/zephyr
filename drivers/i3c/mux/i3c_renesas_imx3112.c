/* renesas_imx3112_i3c.c - I3C file for Renesas I3C 1:2 bus multiplexer */

/*
 * Copyright (c) 2022 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "i3c_renesas_imx3112_regs.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/i3c/devicetree.h>

#define LOG_LEVEL CONFIG_I3C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_renesas_imx3112);

/* Number of ports that can be accessed by device */
#define NUM_CHANNELS      2
/* Timeout for locking in a channel - 5 seconds is more than sufficient */
#define CHAN_LOCK_TIMEOUT K_MSEC(5000)

/*
 * Different transmission modes. These are determined at compile time based on devicetree
 * configuration
 */
enum transmission_mode {
	MODE_I3C = 0,
	MODE_I2C = 1,
};

/*
 * Resistor values for SCL/SDA lines.
 * Not used if has_external_resistor is true
 */
struct channel_resistance {
	uint8_t scl;
	uint8_t sda;
};

/*
 * Devicetree values used to configure the mux during initalization.
 * Determine the electrical characteristics of the mux channels
 */
struct mux_hw_configuration {
	/*
	 * Indicates if mux has external pullup resistors or
	 * internal programmable resistors for SCL/SDA lines
	 */
	bool has_external_resistor;
	/* IO voltage level - at which SCL/SDA lines are driven */
	uint8_t io_voltage;
	/* Channel resistances */
	struct channel_resistance ch_resistance[NUM_CHANNELS];
};

/* Operations for reading and writing to the mux */
struct bus_operations {
	int (*read_reg)(const struct device *mux_dev, uint8_t *val, uint8_t addr);
	int (*write_reg)(const struct device *mux_dev, uint8_t addr, uint8_t val);
};

/*
 * I3C device configuration at initialization phase
 *
 * These are mostly configured with device tree information.
 */
struct i3c_renesas_imx3112_mux_config {
	/* I3C bus controller */
	const struct device *bus;

	/* Device id of the mux in the  bus */
	union {
		const struct i3c_device_id dev_id_i3c;
		const uint16_t dev_id_i2c;
	};

	/* Hw config for the mux */
	struct mux_hw_configuration hw_config;

	/* Indicates if the mux is meant to run in i2c or i3c mode */
	const enum transmission_mode transmission_mode;

	/* transmission mode specific read/write operations */
	const struct bus_operations bus_ops;
};

/*
 * I3C device data updated at runtime
 */
struct i3c_renesas_imx3112_mux_data {
	struct {
		/* Is the mux enabled - is at least one of the channels active */
		bool enabled;
		/* Currently enabled channel mask - not relevant if enabled is false */
		uint8_t channel_mask;
	} mux_state;

	/*
	 * I3C device description, updated after controller init. Runtime equivalent of
	 * i2c_dt_spec
	 */
	struct i3c_device_desc *mux_desc_i3c;

	/* mutex for imx3112 accesses */
	struct k_mutex lock;
};

struct i3c_renesas_imx3112_channel_config {
	/* Pointer to parent driver */
	const struct device *mux;
	/* Will be written directly to MR64/MR65 */
	uint8_t channel_mask;
	/* List of devices attached to this channel */
	/* Includes both I3C and I2C devices */
	struct i3c_dev_list device_list;
};

static inline struct i3c_renesas_imx3112_mux_data *
get_mux_data_from_channel(const struct device *dev)
{
	const struct i3c_renesas_imx3112_channel_config *channel_config = dev->config;

	return channel_config->mux->data;
}

static inline const struct i3c_renesas_imx3112_mux_config *
get_mux_config_from_channel(const struct device *dev)
{
	const struct i3c_renesas_imx3112_channel_config *channel_config = dev->config;

	return channel_config->mux->config;
}

static inline int i3c_renesas_imx3112_i2c_read_reg(const struct device *mux_dev, uint8_t *val,
						   uint8_t addr)
{
	const struct i3c_renesas_imx3112_mux_config *config = mux_dev->config;
	uint8_t write_buf[1];

	write_buf[0] = FIELD_GET(GENMASK(7, 0), addr);

	return i2c_write_read(config->bus, config->dev_id_i2c, write_buf, sizeof(write_buf), val,
			      sizeof(uint8_t));
}

static inline int i3c_renesas_imx3112_i3c_read_reg(const struct device *mux_dev, uint8_t *val,
						   uint8_t addr)
{
	struct i3c_renesas_imx3112_mux_data *data = mux_dev->data;
	struct i3c_device_desc *mux_desc_i3c = data->mux_desc_i3c;
	uint8_t write_buf[2];

	/* Prepare message - address is split into two bytes */
	write_buf[0] = FIELD_GET(GENMASK(6, 0), addr);
	write_buf[1] = FIELD_GET(BIT(7), addr);

	return i3c_write_read(mux_desc_i3c, write_buf, sizeof(write_buf), val, sizeof(uint8_t));
}

static int i3c_renesas_imx3112_read_reg(const struct device *mux_dev, uint8_t *val, uint8_t addr)
{

	LOG_DBG("Reading from %s : address 0x%X", mux_dev->name, addr);
	const struct i3c_renesas_imx3112_mux_config *config = mux_dev->config;
	int err;

	err = config->bus_ops.read_reg(mux_dev, val, addr);

	if (err) {
		LOG_ERR("Failed to read from %s : address 0x%X", mux_dev->name, addr);
	}
	return err;
}

static inline int i3c_renesas_imx3112_i2c_write_reg(const struct device *mux_dev, uint8_t addr,
						    uint8_t val)
{
	const struct i3c_renesas_imx3112_mux_config *config = mux_dev->config;
	uint8_t write_buf[2];

	write_buf[0] = FIELD_GET(GENMASK(7, 0), addr);
	write_buf[1] = val;

	return i2c_write(config->bus, write_buf, sizeof(write_buf), config->dev_id_i2c);
}

static inline int i3c_renesas_imx3112_i3c_write_reg(const struct device *mux_dev, uint8_t addr,
						    uint8_t val)
{
	struct i3c_renesas_imx3112_mux_data *data = mux_dev->data;
	struct i3c_device_desc *mux_desc_i3c = data->mux_desc_i3c;
	uint8_t write_buf[3];

	/* Prepare message - address is split into two bytes */
	write_buf[0] = FIELD_GET(GENMASK(6, 0), addr);
	write_buf[1] = FIELD_GET(BIT(7), addr);
	write_buf[2] = val;

	return i3c_write(mux_desc_i3c, write_buf, sizeof(write_buf));
}

static int i3c_renesas_imx3112_write_reg(const struct device *mux_dev, uint8_t addr, uint8_t val)
{
	LOG_DBG("Writing 0x%X to %s : address 0x%X", val, mux_dev->name, addr);
	const struct i3c_renesas_imx3112_mux_config *config = mux_dev->config;
	uint8_t actual_val;
	int err;

	err = config->bus_ops.write_reg(mux_dev, addr, val);

	if (err) {
		LOG_ERR("Failed to write to %s : address 0x%X", mux_dev->name, addr);
		return err;
	}

	if (IS_ENABLED(CONFIG_I3C_LOG_LEVEL_DBG)) {
		/* Check that the correct value was written to the correct address */
		err = i3c_renesas_imx3112_read_reg(mux_dev, &actual_val, addr);
		if (err) {
			return err;
		}
		if (val != actual_val) {
			LOG_ERR("Read value from %s : address 0x%X was 0x%X, expected 0x%X",
				mux_dev->name, addr, actual_val, val);
		}
	}

	return 0;
}

static int i3c_renesas_imx3112_set_channel(const struct device *mux_dev, uint8_t select_mask)
{
	struct i3c_renesas_imx3112_mux_data *data = mux_dev->data;
	uint8_t select_mask_reg;
	int err = 0;

	/*
	 * Only select the channel if its different from the last channel or if
	 * the mux hasn't been enabled yet
	 */
	LOG_DBG("Mux dev %s attempting mask change. Current channel mask is: 0x%X, "
		"new channel mask is 0x%X",
		mux_dev->name, data->mux_state.channel_mask, select_mask);
	if (!data->mux_state.enabled || (data->mux_state.channel_mask != select_mask)) {
		/* Offset the select_mask to the correct field */
		select_mask_reg = FIELD_PREP(RF_SCL_SDA_0_EN | RF_SCL_SDA_1_EN, select_mask);
		/* Deactivate the mux temporarily while we select a new channel mask */
		err = i3c_renesas_imx3112_write_reg(
			mux_dev, offsetof(struct i3c_renesas_imx3112_registers, mux_config), 0);
		if (err) {
			return err;
		}

		/* Select new channel mask */
		err = i3c_renesas_imx3112_write_reg(
			mux_dev, offsetof(struct i3c_renesas_imx3112_registers, mux_select),
			select_mask_reg);
		if (err) {
			return err;
		}

		/* Re-enable mux */
		err = i3c_renesas_imx3112_write_reg(
			mux_dev, offsetof(struct i3c_renesas_imx3112_registers, mux_config),
			select_mask_reg);
		if (err) {
			return err;
		}

		/* Update runtime data */
		data->mux_state.enabled = true;
		data->mux_state.channel_mask = select_mask;
	}
	return 0;
}

static int i3c_renesas_imx3112_i2c_transfer(const struct device *channel_dev, struct i2c_msg *msgs,
					    uint8_t num_msgs, uint16_t addr)
{

	struct i3c_renesas_imx3112_mux_data *data = get_mux_data_from_channel(channel_dev);
	const struct i3c_renesas_imx3112_mux_config *config =
		get_mux_config_from_channel(channel_dev);
	const struct i3c_renesas_imx3112_channel_config *channel_config = channel_dev->config;
	const struct device *bus_dev = config->bus;
	int err;

	err = k_mutex_lock(&data->lock, CHAN_LOCK_TIMEOUT);
	if (err) {
		return err;
	}

	err = i3c_renesas_imx3112_set_channel(channel_config->mux, channel_config->channel_mask);
	if (err) {
		goto end_trans;
	}

	err = i2c_transfer(bus_dev, msgs, num_msgs, addr);

end_trans:
	k_mutex_unlock(&data->lock);
	if (err) {
		LOG_ERR("Mux dev %s transfer failed with error %d", channel_dev->name, err);
	}
	return err;
}

static int i3c_renesas_imx3112_transfer(const struct device *channel_dev,
					struct i3c_device_desc *target, struct i3c_msg *msgs,
					uint8_t num_msgs)
{
	struct i3c_renesas_imx3112_mux_data *data = get_mux_data_from_channel(channel_dev);
	const struct i3c_renesas_imx3112_mux_config *config =
		get_mux_config_from_channel(channel_dev);
	const struct i3c_renesas_imx3112_channel_config *channel_config = channel_dev->config;
	const struct device *bus_dev = config->bus;
	struct i3c_device_id pid = {.pid = target->pid};
	int err;

	err = k_mutex_lock(&data->lock, CHAN_LOCK_TIMEOUT);
	if (err) {
		return err;
	}

	err = i3c_renesas_imx3112_set_channel(channel_config->mux, channel_config->channel_mask);
	if (err) {
		goto end_trans;
	}
	/* Change the device bus from the mux to the controller to redirect the transaction */
	target = i3c_device_find(bus_dev, &pid);
	if (target == NULL) {
		LOG_ERR("Mux dev %s failed to find target: 0x%llX", channel_dev->name,
			(uint64_t)pid.pid);
		return -ENXIO;
	}
	err = i3c_transfer(target, msgs, num_msgs);

end_trans:
	k_mutex_unlock(&data->lock);
	if (err) {
		LOG_ERR("Mux dev %s transfer failed with error %d", channel_dev->name, err);
	}
	return err;
}

static int i3c_renesas_imx3112_configure(const struct device *dev, enum i3c_config_type type,
					 void *config)
{
	return -ENOSYS;
}

static int i3c_renesas_imx3112_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	return -ENOSYS;
}

static struct i3c_device_desc *i3c_renesas_imx3112_device_find(const struct device *channel_dev,
							       const struct i3c_device_id *id)
{
	const struct i3c_renesas_imx3112_channel_config *channel_config = channel_dev->config;

	return i3c_dev_list_find(&channel_config->device_list, id);
}

static int i3c_renesas_imx3112_channel_initialize(const struct device *channel_dev)
{
	LOG_DBG("Initalizing mux channel %s", channel_dev->name);
	const struct i3c_renesas_imx3112_channel_config *channel_cfg = channel_dev->config;
	const struct i3c_renesas_imx3112_mux_config *mux_cfg =
		get_mux_config_from_channel(channel_dev);
	int err;

	if (!device_is_ready(channel_cfg->mux)) {
		LOG_ERR("I3C mux root %s not ready", channel_cfg->mux->name);
		return -ENODEV;
	}

	/* Test channel selection */
	err = i3c_renesas_imx3112_set_channel(channel_cfg->mux, channel_cfg->channel_mask);
	if (err) {
		return err;
	}

	LOG_DBG("Mux channel %s initalization complete", channel_dev->name);
	return 0;
}

/* Additional initialization steps needed to set the mux to I3C mode */
static int i3c_renesas_imx3112_mux_configure_i3c_mode(const struct device *dev)
{
	const struct i3c_renesas_imx3112_mux_config *cfg = dev->config;
	struct i3c_renesas_imx3112_mux_data *data = dev->data;

	/*
	 * Need to grab the pointer to the I3C device descriptor or I2C device descriptor
	 * before we can configure the mux
	 */
	data->mux_desc_i3c = i3c_device_find(cfg->bus, &cfg->dev_id_i3c);
	if (data->mux_desc_i3c == NULL) {
		LOG_ERR("Cannot find I3C device descriptor");
		return -ENODEV;
	}

	return 0;
}

static int i3c_renesas_imx3112_mux_initialize(const struct device *dev)
{
	LOG_DBG("Initalizing mux %s", dev->name);
	const struct i3c_renesas_imx3112_mux_config *cfg = dev->config;
	uint8_t intf_cfg_val = 0;
	uint8_t res_cfg_val = 0;
	int err;

	if (cfg->bus == NULL || !device_is_ready(cfg->bus)) {
		LOG_ERR("Cannot find I3C controller");
		return -ENODEV;
	}

	if (cfg->transmission_mode == MODE_I3C) {
		err = i3c_renesas_imx3112_mux_configure_i3c_mode(dev);
		if (err) {
			LOG_ERR("Couldn't set mux to I3C mode");
			return err;
		}
	}

	/* Perform HW configuration for the mux based on devicetree defaults */
	intf_cfg_val |=
		FIELD_PREP(RF_LOCAL_INF_PULLUP_CONF, cfg->hw_config.has_external_resistor ? 1 : 0);
	intf_cfg_val |= FIELD_PREP(RF_LOCAL_INF_IO_LEVEL, cfg->hw_config.io_voltage);
	err = i3c_renesas_imx3112_write_reg(
		dev, offsetof(struct i3c_renesas_imx3112_registers, local_interface_cfg),
		intf_cfg_val);
	if (err) {
		LOG_ERR("Mux %s HW configuration failed", dev->name);
		return err;
	}

	res_cfg_val |= FIELD_PREP(RF_LSCL_0_PU_RES, cfg->hw_config.ch_resistance[0].scl);
	res_cfg_val |= FIELD_PREP(RF_LSDA_0_PU_RES, cfg->hw_config.ch_resistance[0].sda);
	res_cfg_val |= FIELD_PREP(RF_LSCL_1_PU_RES, cfg->hw_config.ch_resistance[1].scl);
	res_cfg_val |= FIELD_PREP(RF_LSDA_1_PU_RES, cfg->hw_config.ch_resistance[1].sda);
	err = i3c_renesas_imx3112_write_reg(
		dev, offsetof(struct i3c_renesas_imx3112_registers, pullup_resistor_config),
		res_cfg_val);

	if (err) {
		LOG_ERR("Mux %s HW configuration failed", dev->name);
		return 0;
	}

	LOG_DBG("Mux %s initalization complete", dev->name);
	return 0;
}

static const struct i3c_driver_api imx3112_api_funcs = {
	.i3c_xfers = i3c_renesas_imx3112_transfer,
	.i3c_device_find = i3c_renesas_imx3112_device_find,
	.configure = i3c_renesas_imx3112_configure,
	/* I2C API */
	.i2c_api.configure = i3c_renesas_imx3112_i2c_configure,
	.i2c_api.transfer = i3c_renesas_imx3112_i2c_transfer,
};

BUILD_ASSERT(CONFIG_I3C_RENESAS_CHANNEL_INIT_PRIORITY > CONFIG_I3C_RENESAS_MUX_INIT_PRIORITY,
	     "I3C multiplexer channels must be initialized after their root");

#define CHECK_MUX_CONFIG(n)                                                                        \
	COND_CODE_1(DT_INST_PROP(n, has_external_resistor),                                        \
		    (BUILD_ASSERT(DT_INST_ENUM_IDX_OR(n, io_voltage, 0) < 3)), ())

/* Select hardware analog parameters based on devicetree config */
#define MUX_HW_CONFIG(n)                                                                           \
	{                                                                                          \
		.has_external_resistor = DT_INST_PROP(n, has_external_resistor),                   \
		.io_voltage = DT_INST_ENUM_IDX_OR(n, io_voltage, 0),                               \
		.ch_resistance[0].scl =                                                            \
			DT_ENUM_IDX_OR(DT_INST_CHILD(n, mux_i2c_0), scl_resistance, 0),            \
		.ch_resistance[0].sda =                                                            \
			DT_ENUM_IDX_OR(DT_INST_CHILD(n, mux_i2c_0), sda_resistance, 0),            \
		.ch_resistance[1].scl =                                                            \
			DT_ENUM_IDX_OR(DT_INST_CHILD(n, mux_i2c_1), scl_resistance, 0),            \
		.ch_resistance[1].sda =                                                            \
			DT_ENUM_IDX_OR(DT_INST_CHILD(n, mux_i2c_1), sda_resistance, 0),            \
	}

#define I3C_BUS_OPS                                                                                \
	{                                                                                          \
		.read_reg = i3c_renesas_imx3112_i3c_read_reg,                                      \
		.write_reg = i3c_renesas_imx3112_i3c_write_reg                                     \
	}

#define I3C_MODE_CONFIG(node_id)                                                                   \
	.dev_id_i3c = I3C_DEVICE_ID_DT_INST(node_id), .transmission_mode = MODE_I3C,               \
	.bus_ops = I3C_BUS_OPS,

#define I2C_BUS_OPS                                                                                \
	{                                                                                          \
		.read_reg = i3c_renesas_imx3112_i2c_read_reg,                                      \
		.write_reg = i3c_renesas_imx3112_i2c_write_reg                                     \
	}

#define I2C_MODE_CONFIG(node_id)                                                                   \
	.dev_id_i2c = DT_INST_PROP_BY_IDX(node_id, reg, 0), .transmission_mode = MODE_I2C,         \
	.bus_ops = I2C_BUS_OPS,

/*
 * Select I3C or I2C configuration based on the presence on the provisioned ID
 * See i3c-device.yaml for details
 */
#define SELECT_TX_MODE_CONFIG(node_id)                                                             \
	COND_CODE_0(DT_INST_PROP_BY_IDX(node_id, reg, 1), (I2C_MODE_CONFIG(node_id)),              \
		    (I3C_MODE_CONFIG(node_id)))

/* Devicetree initialization macros for muxes on an I3C bus (with an I3C controller) */
#define DT_DRV_COMPAT renesas_imx3112_i3c
#define IMX3112_CHANNEL_INIT(parent_inst, node_id, ch_num)                                         \
	BUILD_ASSERT(DT_REG_ADDR(node_id) < NUM_CHANNELS);                                         \
	static struct i3c_device_desc i3c_renesas_imx3112_i3c_dev_arr_##parent_inst##_##ch_num[] = \
		I3C_DEVICE_ARRAY_DT(node_id);                                                      \
	static struct i3c_i2c_device_desc                                                          \
		i3c_renesas_imx3112_i2c_dev_arr_##parent_inst##_##ch_num[] =                       \
			I3C_I2C_DEVICE_ARRAY_DT(node_id);                                          \
	const struct i3c_renesas_imx3112_channel_config                                            \
		imx3112_channel_##parent_inst##_##ch_num##_config = {                              \
			.channel_mask = BIT(DT_REG_ADDR(node_id)),                                 \
			.mux = DEVICE_DT_GET(DT_PARENT(node_id)),                                  \
			.device_list.i3c =                                                         \
				i3c_renesas_imx3112_i3c_dev_arr_##parent_inst##_##ch_num,          \
			.device_list.num_i3c = ARRAY_SIZE(                                         \
				i3c_renesas_imx3112_i3c_dev_arr_##parent_inst##_##ch_num),         \
			.device_list.i2c =                                                         \
				i3c_renesas_imx3112_i2c_dev_arr_##parent_inst##_##ch_num,          \
			.device_list.num_i2c = ARRAY_SIZE(                                         \
				i3c_renesas_imx3112_i2c_dev_arr_##parent_inst##_##ch_num),         \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, i3c_renesas_imx3112_channel_initialize, NULL, NULL,              \
			 &imx3112_channel_##parent_inst##_##ch_num##_config, POST_KERNEL,          \
			 CONFIG_I3C_RENESAS_CHANNEL_INIT_PRIORITY, &imx3112_api_funcs);

#define I3C_DEVICE_INIT_RENESAS_IMX3112(n)                                                         \
	/* Internal resistors only regulate up to the VDD supply */                                \
	CHECK_MUX_CONFIG(n)                                                                        \
	static const struct i3c_renesas_imx3112_mux_config i3c_renesas_imx3112_mux_config_##n = {  \
		.bus = DEVICE_DT_GET(DT_INST_BUS(n)),                                              \
		.hw_config = MUX_HW_CONFIG(n),                                                     \
		SELECT_TX_MODE_CONFIG(n)};                                                         \
	static struct i3c_renesas_imx3112_mux_data i3c_renesas_imx3112_mux_data_##n = {            \
		.lock = Z_MUTEX_INITIALIZER(i3c_renesas_imx3112_mux_data_##n.lock),                \
		.mux_state = {.enabled = false, .channel_mask = 0},                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, i3c_renesas_imx3112_mux_initialize, NULL,                         \
			      &i3c_renesas_imx3112_mux_data_##n,                                   \
			      &i3c_renesas_imx3112_mux_config_##n, POST_KERNEL,                    \
			      CONFIG_I3C_RENESAS_MUX_INIT_PRIORITY, NULL);                         \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_INST_CHILD(n, mux_i3c_0), okay),                         \
		    (IMX3112_CHANNEL_INIT(n, DT_INST_CHILD(n, mux_i3c_0), 0)), ())                 \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_INST_CHILD(n, mux_i3c_1), okay),                         \
		    (IMX3112_CHANNEL_INIT(n, DT_INST_CHILD(n, mux_i3c_1), 1)), ())

DT_INST_FOREACH_STATUS_OKAY(I3C_DEVICE_INIT_RENESAS_IMX3112)

/* Devicetree initialization macros for muxes on an I2C bus (with an I2C controller) */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_imx3112_i2c

#define IMX3112_I2C_CHANNEL_INIT(parent_inst, node_id, ch_num)                                     \
	BUILD_ASSERT(DT_REG_ADDR(node_id) < NUM_CHANNELS);                                         \
	const struct i3c_renesas_imx3112_channel_config                                            \
		imx3112_channel_##parent_inst##_##ch_num##_config = {                              \
			.channel_mask = BIT(DT_REG_ADDR(node_id)),                                 \
			.mux = DEVICE_DT_GET(DT_PARENT(node_id)),                                  \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, i3c_renesas_imx3112_channel_initialize, NULL, NULL,              \
			 &imx3112_channel_##parent_inst##_##ch_num##_config, POST_KERNEL,          \
			 CONFIG_I3C_RENESAS_CHANNEL_PRIORITY, &imx3112_api_funcs);

#define I2C_DEVICE_INIT_RENESAS_IMX3112(n)                                                         \
	/* Internal resistors only regulate up to the VDD supply */                                \
	CHECK_MUX_CONFIG(n)                                                                        \
	static const struct i3c_renesas_imx3112_mux_config i2c_renesas_imx3112_mux_config_##n = {  \
		.bus = DEVICE_DT_GET(DT_INST_BUS(n)),                                              \
		.hw_config = MUX_HW_CONFIG(n),                                                     \
		I2C_MODE_CONFIG(n)};                                                               \
	static struct i3c_renesas_imx3112_mux_data i2c_renesas_imx3112_mux_data_##n = {            \
		.lock = Z_MUTEX_INITIALIZER(i2c_renesas_imx3112_mux_data_##n.lock),                \
		.mux_state = {.enabled = false, .channel_mask = 0},                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, i3c_renesas_imx3112_mux_initialize, NULL,                         \
			      &i2c_renesas_imx3112_mux_data_##n,                                   \
			      &i2c_renesas_imx3112_mux_config_##n, POST_KERNEL,                    \
			      CONFIG_I3C_RENESAS_MUX_PRIORITY, NULL);                              \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_INST_CHILD(n, mux_i2c_0), okay),                         \
		    (IMX3112_CHANNEL_INIT(n, DT_INST_CHILD(n, mux_i2c_0), 0)), ())                 \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_INST_CHILD(n, mux_i2c_1), okay),                         \
		    (IMX3112_CHANNEL_INIT(n, DT_INST_CHILD(n, mux_i2c_1), 1)), ())

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_RENESAS_IMX3112)
