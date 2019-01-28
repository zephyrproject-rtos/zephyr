/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * LIS2MDL mag driver
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <misc/__assert.h>
#include <misc/byteorder.h>
#include <sensor.h>
#include <string.h>

#include "lis2mdl.h"

struct lis2mdl_data lis2mdl_device_data;

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LIS2MDL);

#ifdef CONFIG_LIS2MDL_MAG_ODR_RUNTIME
static const struct {
	u16_t odr;
	u8_t regval;
	} lis2mdl_odr_reg[] = {
		{
			.odr = 10,
			.regval = LIS2MDL_ODR10_HZ,
		},
		{
			.odr = 20,
			.regval = LIS2MDL_ODR20_HZ,
		},
		{
			.odr = 50,
			.regval = LIS2MDL_ODR50_HZ,
		},
		{
			.odr = 100,
			.regval = LIS2MDL_ODR100_HZ,
		},
	};

static int lis2mdl_freq_to_odr_val(u16_t odr)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lis2mdl_odr_reg); i++) {
		if (odr == lis2mdl_odr_reg[i].odr) {
			return i;
		}
	}

	return -EINVAL;
}

static int lis2mdl_set_odr(struct device *dev, u16_t odr)
{
	int odr_idx;
	struct lis2mdl_data *lis2mdl = dev->driver_data;

	/* check if power off */
	if (odr == 0) {
		/* power off mag */
		return i2c_reg_update_byte(lis2mdl->i2c,
					   lis2mdl->i2c_addr,
					   LIS2MDL_CFG_REG_A,
					   LIS2MDL_MAG_MODE_MASK,
					   LIS2MDL_MD_IDLE1_MODE);
	}

	odr_idx = lis2mdl_freq_to_odr_val(odr);
	if (odr_idx < 0) {
		return odr_idx;
	}

	return i2c_reg_update_byte(lis2mdl->i2c,
				   lis2mdl->i2c_addr,
				   LIS2MDL_CFG_REG_A,
				   LIS2MDL_ODR_MASK,
				   lis2mdl_odr_reg[odr_idx].regval);
}
#endif /* CONFIG_LIS2MDL_MAG_ODR_RUNTIME */

static int lis2mdl_set_hard_iron(struct device *dev, enum sensor_channel chan,
				   const struct sensor_value *val)
{
	struct lis2mdl_data *lis2mdl = dev->driver_data;
	u8_t regs = LIS2MDL_OFFSET_X_REG_L;
	u8_t i;
	s16_t offs;
	int ret;

	for (i = 0U; i < LIS2MDL_NUM_AXIS; i++) {
		offs = sys_cpu_to_le16(val->val1);
		ret = i2c_burst_write(lis2mdl->i2c, lis2mdl->i2c_addr,
				      regs, (u8_t *)&offs, sizeof(offs));
		if (ret < 0) {
			return ret;
		}

		regs += sizeof(offs);
		val++;
	}

	return 0;
}

static void lis2mdl_channel_get_mag(struct device *dev,
				      enum sensor_channel chan,
				      struct sensor_value *val)
{
	s32_t cval;
	int i;
	u8_t ofs_start, ofs_stop;
	struct lis2mdl_data *lis2mdl = dev->driver_data;
	struct sensor_value *pval = val;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_MAGN_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_MAGN_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop; i++) {
		cval = lis2mdl->mag[i] * lis2mdl->mag_fs_sensitivity;
		pval->val1 = cval / 1000000;
		pval->val2 = cval % 1000000;
		pval++;
	}
}

/* read internal temperature */
static void lis2mdl_channel_get_temp(struct device *dev,
				       struct sensor_value *val)
{
	struct lis2mdl_data *drv_data = dev->driver_data;

	val->val1 = drv_data->temp_sample / 100;
	val->val2 = drv_data->temp_sample % 100;
}

static int lis2mdl_channel_get(struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		lis2mdl_channel_get_mag(dev, chan, val);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		lis2mdl_channel_get_temp(dev, val);
		break;
	default:
		LOG_DBG("Channel not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2mdl_config(struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
#ifdef CONFIG_LIS2MDL_MAG_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2mdl_set_odr(dev, val->val1);
#endif /* CONFIG_LIS2MDL_MAG_ODR_RUNTIME */
	case SENSOR_ATTR_OFFSET:
		return lis2mdl_set_hard_iron(dev, chan, val);
	default:
		LOG_DBG("Mag attribute not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2mdl_attr_set(struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		return lis2mdl_config(dev, chan, attr, val);
	default:
		LOG_DBG("attr_set() not supported on %d channel", chan);
		return -ENOTSUP;
	}

	return 0;
}

static int lis2mdl_sample_fetch_mag(struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->driver_data;

	union {
		u8_t raw[LIS2MDL_OUT_REG_SIZE];
		struct {
			s16_t m_axis[3];
		};
	} buf __aligned(2);

	/* fetch raw data sample */
	if (i2c_burst_read(lis2mdl->i2c, lis2mdl->i2c_addr,
			   LIS2MDL_OUT_REG, buf.raw,
			   sizeof(buf)) < 0) {
		LOG_DBG("Failed to fetch raw mag sample");
		return -EIO;
	}

	lis2mdl->mag[0] = sys_le16_to_cpu(buf.m_axis[0]);
	lis2mdl->mag[1] = sys_le16_to_cpu(buf.m_axis[1]);
	lis2mdl->mag[2] = sys_le16_to_cpu(buf.m_axis[2]);

	return 0;
}

static int lis2mdl_sample_fetch_temp(struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->driver_data;
	u16_t temp_raw;
	s32_t temp;
	int ret;

	/* fetch raw temperature sample */
	ret = i2c_burst_read(lis2mdl->i2c, lis2mdl->i2c_addr,
			     LIS2MDL_TEMP_OUT_L_REG,
			     (u8_t *)&temp_raw, sizeof(temp_raw));
	if (ret < 0) {
		LOG_DBG("Failed to fetch raw temp sample");
		return -EIO;
	}

	/* formula is temp = 25 + (temp / 8) C */
	temp = (sys_le16_to_cpu(temp_raw) & 0x8FFF);
	lis2mdl->temp_sample = 2500 + (temp * 100) / 8;

	return 0;
}

static int lis2mdl_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		lis2mdl_sample_fetch_mag(dev);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		lis2mdl_sample_fetch_temp(dev);
		break;
	case SENSOR_CHAN_ALL:
		lis2mdl_sample_fetch_mag(dev);
		lis2mdl_sample_fetch_temp(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lis2mdl_driver_api = {
	.attr_set = lis2mdl_attr_set,
#if CONFIG_LIS2MDL_TRIGGER
	.trigger_set = lis2mdl_trigger_set,
#endif
	.sample_fetch = lis2mdl_sample_fetch,
	.channel_get = lis2mdl_channel_get,
};

static int lis2mdl_init_interface(struct device *dev)
{
	const struct lis2mdl_device_config *const config =
						dev->config->config_info;
	struct lis2mdl_data *lis2mdl = dev->driver_data;

	lis2mdl->i2c = device_get_binding(config->master_dev_name);
	if (!lis2mdl->i2c) {
		LOG_DBG("Could not get pointer to %s device",
			    config->master_dev_name);
		return -EINVAL;
	}

	lis2mdl->i2c_addr = config->i2c_addr_config;

	return 0;
}

static const struct lis2mdl_device_config lis2mdl_dev_config = {
	.master_dev_name = DT_ST_LIS2MDL_MAGN_0_BUS_NAME,
#ifdef CONFIG_LIS2MDL_TRIGGER
	.gpio_name = DT_ST_LIS2MDL_MAGN_0_IRQ_GPIOS_CONTROLLER,
	.gpio_pin = DT_ST_LIS2MDL_MAGN_0_IRQ_GPIOS_PIN,
#endif  /* CONFIG_LIS2MDL_TRIGGER */
	.i2c_addr_config = DT_ST_LIS2MDL_MAGN_0_BASE_ADDRESS,
};

static int lis2mdl_init(struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->driver_data;
	u8_t wai;

	if (lis2mdl_init_interface(dev)) {
		return -EINVAL;
	}

	/* check chip ID */
	if (i2c_reg_read_byte(lis2mdl->i2c, lis2mdl->i2c_addr,
			      LIS2MDL_WHO_AM_I_REG, &wai) < 0) {
		LOG_DBG("Failed to read chip ID");
		return -EIO;
	}

	if (wai != LIS2MDL_WHOAMI_VAL) {
		LOG_DBG("Invalid chip ID");
		return -EINVAL;
	}

	/* reset sensor configuration */
	if (i2c_reg_write_byte(lis2mdl->i2c, lis2mdl->i2c_addr,
			       LIS2MDL_CFG_REG_A, LIS2MDL_SOFT_RST)) {
		return -EIO;
	}

	k_busy_wait(100);

	/* enable BDU */
	if (i2c_reg_update_byte(lis2mdl->i2c, lis2mdl->i2c_addr,
				LIS2MDL_CFG_REG_C, LIS2MDL_BDU_MASK,
				LIS2MDL_BDU_BIT)) {
		return -EIO;
	}

	/* set continuous MODE in default ODR, enable temperature comp. */
	if (i2c_reg_write_byte(lis2mdl->i2c, lis2mdl->i2c_addr,
			       LIS2MDL_CFG_REG_A,
			       LIS2MDL_MD_CONT_MODE | LIS2MDL_DEFAULT_ODR |
			       LIS2MDL_COMP_TEMP_MASK)) {
		return -EIO;
	}

	lis2mdl->mag_fs_sensitivity = LIS2MDL_SENSITIVITY;

#ifdef CONFIG_LIS2MDL_TRIGGER
	if (lis2mdl_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupts");
		return -EIO;
	}
#endif

	return 0;
}

DEVICE_AND_API_INIT(lis2mdl, DT_ST_LIS2MDL_MAGN_0_LABEL, lis2mdl_init,
		     &lis2mdl_device_data, &lis2mdl_dev_config, POST_KERNEL,
		     CONFIG_SENSOR_INIT_PRIORITY, &lis2mdl_driver_api);
