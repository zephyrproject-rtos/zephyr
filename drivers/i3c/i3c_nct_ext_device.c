/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_i3c_ext_device

#include "i3c_nct.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nct_i3c_ext_target, CONFIG_I3C_LOG_LEVEL);

/* APIs */
typedef int (*nct_i3c_i2c_read_ptr)(void *handle, uint8_t reg_addr, uint8_t *value, uint8_t len);
typedef int (*nct_i3c_i2c_write_ptr)(void *handle, uint8_t reg_addr, uint8_t *value, uint8_t len);
typedef int (*nct_i3c_read_ptr)(void *handle, uint8_t reg_addr, uint8_t *value, uint8_t len);
typedef int (*nct_i3c_write_ptr)(void *handle, uint8_t reg_addr, uint8_t *value, uint8_t len);

int ext_dev_i3c_i2c_read(void *handle, uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	return -ENOTSUP;
}

int ext_dev_i3c_i2c_write(void *handle, uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	return -ENOTSUP;
}

int ext_dev_i3c_read(void *handle, uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	struct i3c_device_desc *target = **(struct i3c_device_desc ***)handle;

	return i3c_burst_read(target, reg_addr, value, len);
}

int ext_dev_i3c_write(void *handle, uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	struct i3c_device_desc *target = **(struct i3c_device_desc ***)handle;

	return i3c_burst_write(target, reg_addr, value, len);
}

typedef struct {
	nct_i3c_i2c_read_ptr	i3c_i2c_read;
	nct_i3c_i2c_write_ptr	i3c_i2c_write;
	nct_i3c_read_ptr		i3c_read;
	nct_i3c_write_ptr		i3c_write;

	void *handle;
} dev_ctx_t;

struct nct_i3c_ext_dev_config {
	dev_ctx_t ctx;
	union {
		const struct i2c_dt_spec i2c;
		struct i3c_device_desc **i3c;
	} dev_cfg;

//#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
//	struct {
		const struct device *bus;
		const struct i3c_device_id dev_id;
		const uint8_t static_addr;
		const uint8_t dynamic_addr;
//	} i3c;
//#endif	
};

struct nct_i3c_ext_dev_data
{
	struct i3c_device_desc *i3c_dev;
};

static int i3c_target_init(const struct device *dev)
{
	const struct nct_i3c_ext_dev_config *const config = dev->config;
	struct nct_i3c_ext_dev_data *data = dev->data;

	if (config->bus != NULL) {
		/*
		 * Need to grab the pointer to the I3C device descriptor
		 * before we can talk to the device.
		 */
		data->i3c_dev = i3c_device_find(config->bus, &config->dev_id);
		if (data->i3c_dev == NULL) {
			LOG_ERR("Cannot find I3C device descriptor");
			return -ENODEV;
		}
	}

    return 0;
}

/* APIs */
int nct_i3c_i2c_ext_dev_api_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr)
{
	return -ENOTSUP;
}

int nct_i3c_ext_dev_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	if (type != I3C_CONFIG_TARGET) {
		LOG_ERR("Should be target mode only");
		return -EINVAL;
	}

	return i3c_target_init(dev);
}

int nct_i3c_ext_dev_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	if ((dev == NULL) || (config == NULL)) {
		return -EINVAL;
	}

	if (type != I3C_CONFIG_TARGET) {
		return -ENOTSUP;
	}

	// struct nct_i3c_data *dev_data = dev->data;
	// struct nct_i3c_config_target *cfg = config;

	return -ENOTSUP;
}

int nct_i3c_ext_dev_xfers(const struct device *dev, struct i3c_device_desc *target, \
	struct i3c_msg *msgs, uint8_t num_msgs)
{
	return -ENOTSUP;
}

int nct_i3c_ext_dev_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	return -ENOTSUP;
}

int nct_i3c_ext_dev_target_register(const struct device *dev, struct i3c_target_config *cfg)
{
	return -ENOTSUP;
}

int nct_i3c_ext_dev_target_unregister(const struct device *dev, struct i3c_target_config *cfg)
{
	return -ENOTSUP;
}

int nct_i3c_ext_dev_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len, \
	uint8_t hdr_mode)
{
	return -ENOTSUP;
}

static const struct i3c_driver_api i3c_target_driver_api = {
	.i2c_api = {
		.transfer = nct_i3c_i2c_ext_dev_api_transfer,	/* Transfer data to/from I2C device */
	},
	.configure = nct_i3c_ext_dev_configure,
	.config_get = nct_i3c_ext_dev_config_get,
	.i3c_xfers = nct_i3c_ext_dev_xfers,						/* Transfer messages in I3C mode */
	.ibi_raise = nct_i3c_ext_dev_ibi_raise,					/* Raise IBI */
	.target_register = nct_i3c_ext_dev_target_register,		/* Register target device */
	.target_unregister = nct_i3c_ext_dev_target_unregister,	/* Unregister target device */
	.target_tx_write = nct_i3c_ext_dev_target_tx_write,		/* Write data to controller */
};

/* External target device */
#define NCT_I3C_EXTERNAL_DEVICE_INIT(inst)                               \
	static struct nct_i3c_ext_dev_data i3c_target_data_##inst;							\
    static const struct nct_i3c_ext_dev_config i3c_target_config_##inst = {				\
		/* Add support APIs here */														\
		.ctx.i3c_i2c_read = ext_dev_i3c_i2c_read,										\
		.ctx.i3c_i2c_write = ext_dev_i3c_i2c_write,										\
		.ctx.i3c_read = ext_dev_i3c_read,												\
		.ctx.i3c_write = ext_dev_i3c_write,												\
		\
		/* reserve a space to restore device handle to be used in APIs */				\
		.ctx.handle = (void *)&i3c_target_config_##inst.dev_cfg,						\
		\
		/* pointer to the target device node in i3c_target_init */						\
		.dev_cfg.i3c = &i3c_target_data_##inst.i3c_dev,									\
		\
		/* data used to search the device node */										\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),									\
		.dev_id = I3C_DEVICE_ID_DT_INST(inst),										\
		.static_addr = DT_PROP_BY_IDX(DT_DRV_INST(inst), reg, 0),									\
		.dynamic_addr = DT_INST_PROP(inst, assigned_address),										\
	};																					\
	DEVICE_DT_INST_DEFINE(inst, i3c_target_init, NULL,									\
		&i3c_target_data_##inst, &i3c_target_config_##inst,								\
		POST_KERNEL, CONFIG_I3C_TARGET_INIT_PRIORITY, 				\
		&i3c_target_driver_api);

#define I3C_EXTERNAL_DEVICE_INIT(inst, init_priority)				\
	COND_CODE_0(DT_INST_PROP_BY_IDX(inst, reg, 1),	\
		(/* NCT_I2C_EXTERNAL_DEVICE_INIT(inst) */),	\
		(NCT_I3C_EXTERNAL_DEVICE_INIT(inst, init_priority)));

DT_INST_FOREACH_STATUS_OKAY(NCT_I3C_EXTERNAL_DEVICE_INIT)