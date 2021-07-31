/*
 * Copyright (c) 2021 Murali Tejeshwar Janaswami
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <drivers/sensor.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "si1133.h"

#define DT_DRV_COMPAT silabs_si1133

#define SQUARE(x) ((x) * (x))

LOG_MODULE_REGISTER(SI1133, CONFIG_SENSOR_LOG_LEVEL);

static inline int si1133_write_i2c_reg(const struct si1133_data *const drv_data,
				       const uint8_t i2c_reg_addr,
				       const uint8_t data_in)
{
	return i2c_reg_write_byte(drv_data->i2c_master, drv_data->i2c_addr,
				  i2c_reg_addr, data_in);
}

static inline int si1133_read_i2c_reg(const struct si1133_data *const drv_data,
				      const uint8_t i2c_reg_addr,
				      uint8_t * const data_out)
{
	return i2c_reg_read_byte(drv_data->i2c_master, drv_data->i2c_addr,
				 i2c_reg_addr, data_out);
}

static inline int read_response0(const struct si1133_data *const drv_data,
				 uint8_t * const response0_out)
{
	return si1133_read_i2c_reg(drv_data, SI1133_I2C_REG_RESPONSE0,
				   response0_out);
}

static inline int read_response1(const struct si1133_data *const drv_data,
				 uint8_t * const response1_out)
{
	return si1133_read_i2c_reg(drv_data, SI1133_I2C_REG_RESPONSE1,
				   response1_out);
}

static int read_cmd_ctr(const struct si1133_data *const drv_data,
			uint8_t * const cmd_ctr_out)
{
	int rv;

	rv = read_response0(drv_data, cmd_ctr_out);
	if (rv) {
		LOG_ERR("read RESPONSE0 register failed: 0x%x\n", rv);
		return rv;
	}

	*cmd_ctr_out = *cmd_ctr_out & SI1133_RESPONSE0_BITS_CMD_CTR;

	return 0;
}

static int reset_and_read_cmd_ctr(const struct si1133_data *const drv_data,
				  uint8_t * const cmd_ctr_out)
{
	int rv;

	rv = si1133_write_i2c_reg(drv_data, SI1133_I2C_REG_COMMAND,
				  SI1133_CMD_REG_RST_CMD_CTR);
	if (rv) {
		LOG_ERR("write to COMMAND register failed: 0x%x\n", rv);
		return rv;
	}

	return read_cmd_ctr(drv_data, cmd_ctr_out);
}

static int poll_cmd_ctr_increment(const struct si1133_data *const drv_data,
				  const uint8_t cmd_ctr)
{
	uint8_t response0;
	int rv;

	while (true) {
		rv = read_response0(drv_data, &response0);
		if (rv) {
			LOG_ERR("read RESPONSE0 register failed: 0x%x\n", rv);
			return rv;
		}
		if (response0 & SI1133_RESPONSE0_BIT_CMD_ERR) {
			LOG_ERR("command failed with error: 0x%x\n",
				response0 & SI1133_RESPONSE0_BITS_CMD_ERR);
			return -EIO;
		}
		if ((response0 & SI1133_RESPONSE0_BITS_CMD_CTR) == cmd_ctr + 1) {
			break;
		}
	}

	return 0;
}

static int param_tbl_access_op(const struct si1133_data *const drv_data,
			       const uint8_t from_i2c_reg,
			       const uint8_t * const buf,
			       const uint32_t num_bytes,
			       uint8_t * const response1_out)
{
	uint8_t cmd_ctr;
	int rv;

	rv = reset_and_read_cmd_ctr(drv_data, &cmd_ctr);
	if (rv) {
		return rv;
	}

	rv = i2c_burst_write(drv_data->i2c_master, drv_data->i2c_addr,
			     from_i2c_reg, buf, num_bytes);
	if (rv) {
		LOG_ERR("access parameter table failed: 0x%x\n", rv);
		return rv;
	}

	rv = poll_cmd_ctr_increment(drv_data, cmd_ctr);
	if (rv) {
		return rv;
	}

	rv = read_response1(drv_data, response1_out);
	if (rv) {
		LOG_ERR("read RESPONSE1 register failed: 0x%x\n", rv);
	}

	return rv;
}

static int set_up_channel(const struct si1133_data *const drv_data,
			  const struct si1133_chan_config *const chan_config)
{
	uint8_t chan_list, get_op, set_op[2], verify_op;
	int rv;

	/* read the current CHAN_LIST configuration */
	get_op = SI1133_CMD_REG_PRM_QRY_PRFX | SI1133_PRM_TBL_CHAN_LIST;
	rv = param_tbl_access_op(drv_data, SI1133_I2C_REG_COMMAND, &get_op,
				 sizeof(get_op), &chan_list);
	if (rv) {
		return rv;
	}

	/* set up CHAN_LIST with new configuration */
	chan_list |= chan_config->enable_chan;
	set_op[0] = chan_list;
	set_op[1] = SI1133_CMD_REG_PRM_SET_PRFX | SI1133_PRM_TBL_CHAN_LIST;
	rv = param_tbl_access_op(drv_data, SI1133_I2C_REG_HOSTIN0, set_op,
				 sizeof(set_op), &verify_op);
	if (rv) {
		return rv;
	}
	if (verify_op != set_op[0]) {
		return -EIO;
	}

	/* set up ADCCONFIG for this channel */
	set_op[0] = chan_config->adcconfig_val;
	set_op[1] = SI1133_CMD_REG_PRM_SET_PRFX | chan_config->adcconfig_addr;
	rv = param_tbl_access_op(drv_data, SI1133_I2C_REG_HOSTIN0, set_op,
				 sizeof(set_op), &verify_op);
	if (rv) {
		return rv;
	}
	if (verify_op != set_op[0]) {
		return -EIO;
	}

	/* set up ADCSENS for this channel */
	set_op[0] = chan_config->adcsens_val;
	set_op[1] = SI1133_CMD_REG_PRM_SET_PRFX | chan_config->adcsens_addr;
	rv = param_tbl_access_op(drv_data, SI1133_I2C_REG_HOSTIN0, set_op,
				 sizeof(set_op), &verify_op);
	if (rv) {
		return rv;
	}
	if (verify_op != set_op[0]) {
		return -EIO;
	}

	/* set up ADCPOST for this channel */
	set_op[0] = chan_config->adcpost_val;
	set_op[1] = SI1133_CMD_REG_PRM_SET_PRFX | chan_config->adcpost_addr;
	rv = param_tbl_access_op(drv_data, SI1133_I2C_REG_HOSTIN0, set_op,
				 sizeof(set_op), &verify_op);
	if (rv) {
		return rv;
	}
	if (verify_op != set_op[0]) {
		return -EIO;
	}

	return 0;
}

static int initiate_measurements(struct si1133_data *const drv_data)
{
	uint8_t cmd_ctr, irq_status, data[SI1133_CFG_TOTAL_OUTPUT_BYTES];
	uint16_t uv_light_raw;
	int rv;

	rv = reset_and_read_cmd_ctr(drv_data, &cmd_ctr);
	if (rv) {
		return rv;
	}

	rv = si1133_write_i2c_reg(drv_data, SI1133_I2C_REG_COMMAND,
				  SI1133_CMD_REG_FORCE);
	if (rv) {
		LOG_ERR("initiate measurements failed: 0x%x\n", rv);
		return rv;
	}

	rv = poll_cmd_ctr_increment(drv_data, cmd_ctr);
	if (rv) {
		return rv;
	}

	irq_status = 0;
	while (!irq_status) {
		rv = si1133_read_i2c_reg(drv_data, SI1133_I2C_REG_IRQ_STATUS,
					 &irq_status);
		if (rv) {
			LOG_ERR("read IRQ_STATUS register failed: 0x%x\n", rv);
			return rv;
		}
	}

	rv = i2c_burst_read(drv_data->i2c_master, drv_data->i2c_addr,
			    SI1133_I2C_REG_HOSTOUT_BASE, data,
			    SI1133_CFG_TOTAL_OUTPUT_BYTES);
	if (rv) {
		LOG_ERR("read HOSTOUTx registers failed: 0x%x\n", rv);
		return rv;
	}
#ifdef CONFIG_SI1133_OPERATING_MODE_BL
	drv_data->data.visible_light =
	    (data[0] << 16) | (data[1] << 8) | data[2];
	drv_data->data.infrared_light =
	    (data[3] << 16) | (data[4] << 8) | data[5];
	uv_light_raw = (data[6] << 8) | data[7];
#else
	drv_data->data.visible_light = (data[0] << 8) | data[1];
	drv_data->data.infrared_light = (data[2] << 8) | data[3];
	uv_light_raw = (data[4] << 8) | data[5];
#endif
	/* convert the sensor's raw UV measurement into UVI scale */
	drv_data->data.ultraviolet_light =
	    0.0187 * ((0.00391 * SQUARE(uv_light_raw)) + uv_light_raw);

	return 0;
}

static int si1133_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct si1133_data *drv_data = dev->data;

	return initiate_measurements(drv_data);
}

static int si1133_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct si1133_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = drv_data->data.visible_light;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_IR:
		val->val1 = drv_data->data.infrared_light;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_UV:
		val->val1 = drv_data->data.ultraviolet_light;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int si1133_init(const struct device *dev)
{
	struct si1133_data *drv_data = dev->data;
	const struct si1133_dev_config *cfg = dev->config;
	struct si1133_chan_config chan_config;
	uint8_t part_id;
	int rv;

	drv_data->i2c_addr = cfg->i2c_addr;
	drv_data->i2c_master = device_get_binding(cfg->i2c_master_name);
	if (!drv_data->i2c_master) {
		LOG_ERR("i2c master not found");
		return -ENODEV;
	}

	/* ensure the device is ready for i2c transactions */
	k_msleep(SI1133_VAL_STARTUP_TIME_MS);

	/* read and verify PART_ID */
	rv = si1133_read_i2c_reg(drv_data, SI1133_I2C_REG_PART_ID, &part_id);
	if (rv || part_id != SI1133_VAL_PART_ID) {
		LOG_ERR("read PART_ID failed: 0x%x\n", rv);
		return -ENODEV;
	}

	/* set up channel 0 for measuring visible light */
	chan_config.enable_chan = SI1133_CFG_ENABLE_CHANNEL_0;
	chan_config.adcconfig_addr = SI1133_PRM_TBL_ADCCONFIG0;
	chan_config.adcconfig_val =
	    SI1133_CFG_ADCCONFIG0_ADCMUX_WHITE |
	    SI1133_CFG_ADCCONFIG0_DECIM_RATE;
	chan_config.adcsens_addr = SI1133_PRM_TBL_ADCSENS0;
	chan_config.adcsens_val = SI1133_CFG_ADCSENS0_HW_GAIN;
	chan_config.adcpost_addr = SI1133_PRM_TBL_ADCPOST0;
	chan_config.adcpost_val = SI1133_CFG_ADCPOST0_24BIT_OUT;
	rv = set_up_channel(drv_data, &chan_config);
	if (rv) {
		LOG_ERR("set up channel 0 failed: 0x%x\n", rv);
		return rv;
	}

	/* set up channel 1 for measuring infrared light */
	chan_config.enable_chan = SI1133_CFG_ENABLE_CHANNEL_1;
	chan_config.adcconfig_addr = SI1133_PRM_TBL_ADCCONFIG1;
	chan_config.adcconfig_val =
	    SI1133_CFG_ADCCONFIG1_ADCMUX_SMALL_IR |
	    SI1133_CFG_ADCCONFIG1_DECIM_RATE;
	chan_config.adcsens_addr = SI1133_PRM_TBL_ADCSENS1;
	chan_config.adcsens_val = SI1133_CFG_ADCSENS1_HW_GAIN;
	chan_config.adcpost_addr = SI1133_PRM_TBL_ADCPOST1;
	chan_config.adcpost_val = SI1133_CFG_ADCPOST1_24BIT_OUT;
	rv = set_up_channel(drv_data, &chan_config);
	if (rv) {
		LOG_ERR("set up channel 1 failed: 0x%x\n", rv);
		return rv;
	}

	/* set up channel 2 for measuring ultraviolet light */
	chan_config.enable_chan = SI1133_CFG_ENABLE_CHANNEL_2;
	chan_config.adcconfig_addr = SI1133_PRM_TBL_ADCCONFIG2;
	chan_config.adcconfig_val =
	    SI1133_CFG_ADCCONFIG2_ADCMUX_UV | SI1133_CFG_ADCCONFIG2_DECIM_RATE;
	chan_config.adcsens_addr = SI1133_PRM_TBL_ADCSENS2;
	chan_config.adcsens_val = SI1133_CFG_ADCSENS2_HW_GAIN;
	chan_config.adcpost_addr = SI1133_PRM_TBL_ADCPOST2;
	chan_config.adcpost_val = SI1133_CFG_ADCPOST2_24BIT_OUT;
	rv = set_up_channel(drv_data, &chan_config);
	if (rv) {
		LOG_ERR("set up channel 2 failed: 0x%x\n", rv);
		return rv;
	}

	LOG_DBG("Si1133 init ok");

	return 0;
}

static const struct si1133_dev_config si1133_config = {
	.i2c_master_name = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0),
};

static const struct sensor_driver_api si1133_api_funcs = {
	.sample_fetch = si1133_sample_fetch,
	.channel_get = si1133_channel_get,
};

static struct si1133_data si1133_data;

DEVICE_DT_INST_DEFINE(0, si1133_init, device_pm_control_nop, &si1133_data,
		      &si1133_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		      &si1133_api_funcs);
