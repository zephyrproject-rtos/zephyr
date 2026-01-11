/*
 * Copyright (c) 2025 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3032_temp

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/mfd/rv3032.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rv3032_temp, CONFIG_SENSOR_LOG_LEVEL);

struct rv3032_temp_config {
	const struct device *parent;
};

struct rv3032_temp_data {
	struct sensor_value val;
	int8_t tlow;
	int8_t thigh;
	uint8_t temp_lsb;
	uint8_t temp_msb;
#ifdef CONFIG_RV3032_TEMP_TRIGGER
	sensor_trigger_handler_t trigger_handler;
	const struct sensor_trigger *trigger;
#endif /* CONFIG_RV3032_TEMP_TRIGGER */
};

static int rv3032_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct rv3032_temp_data *data = dev->data;
	const struct rv3032_temp_config *config = dev->config;
	uint8_t temp[2];
	int16_t tmp_val1;

	int ret = mfd_rv3032_read_regs(config->parent, RV3032_REG_TEMPERATURE_LSB, temp,
				       sizeof(temp));

	data->val.val2 = ((temp[0] & 0xf0) >> 4)  * 625;
	tmp_val1 = (temp[1] << 4) * 0.0625;
	data->val.val1 = tmp_val1 < 0 ? -(tmp_val1 & 0xef) : tmp_val1;

	return ret;
}

static int rv3032_temp_channel_get(const struct device *dev, enum sensor_channel chan,
					struct sensor_value *val)
{
	struct rv3032_temp_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = data->val.val1;
	val->val2 = data->val.val2;

	return 0;
}

static int rv3032_temp_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct rv3032_temp_config *config = dev->config;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	if ((val->val1 <= -128) || (val->val1 >= 127)) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		mfd_rv3032_write_reg8(config->parent, RV3032_REG_TEMP_LOW_THLD, val->val1);
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		mfd_rv3032_write_reg8(config->parent, RV3032_REG_TEMP_HIGH_THLD, val->val1);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rv3032_temp_attr_get(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val)
{
	struct rv3032_temp_data *data = dev->data;
	const struct rv3032_temp_config *config = dev->config;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	/* TODO: Add error checks */
	mfd_rv3032_read_reg8(config->parent, RV3032_REG_TEMP_LOW_THLD, &data->tlow);
	mfd_rv3032_read_reg8(config->parent, RV3032_REG_TEMP_HIGH_THLD, &data->thigh);

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		val->val1 = data->tlow;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		val->val1 = data->thigh;
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_RV3032_TEMP_TRIGGER
static void rv3032_temp_isr(const struct device *dev)
{
	struct rv3032_temp_data *data = dev->data;

	data->trigger_handler(dev, data->trigger);
}

static int rv3032_temp_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler)
{
	const struct rv3032_temp_config *config = dev->config;
	struct rv3032_temp_data *data = dev->data;
	uint8_t low, high, ctrl;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	if (trig->chan !=  SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	if (!handler) {
		return -EINVAL;
	}

	data->trigger_handler = handler;
	data->trigger = trig;

	/* setup mfd IRQ here !!! */
	mfd_rv3032_set_irq_handler(config->parent, dev, RV3032_DEV_SENSOR, rv3032_temp_isr);
	mfd_rv3032_update_reg8(config->parent, RV3032_REG_CONTROL3,
			       RV3032_CONTROL3_THE | RV3032_CONTROL3_THIE,
			       RV3032_CONTROL3_THE | RV3032_CONTROL3_THIE);
	mfd_rv3032_update_reg8(config->parent, RV3032_REG_CONTROL3,
			       RV3032_CONTROL3_TLE | RV3032_CONTROL3_TLIE,
			       RV3032_CONTROL3_TLE | RV3032_CONTROL3_TLIE);

	mfd_rv3032_read_reg8(config->parent, RV3032_REG_TEMP_LOW_THLD, &low);
	mfd_rv3032_read_reg8(config->parent, RV3032_REG_TEMP_HIGH_THLD, &high);
	mfd_rv3032_read_reg8(config->parent, RV3032_REG_CONTROL3, &ctrl);

	LOG_DBG("TLOW[%d] THIGH[%d] CRTL3[%x]", low, high, ctrl);

	return 0;
}
#endif /* CONFIG_RV3032_TEMP_TRIGGER */

static DEVICE_API(sensor, rv3032_temp_driver_api) = {
	.sample_fetch = rv3032_temp_sample_fetch,
	.channel_get = rv3032_temp_channel_get,
	.attr_set = rv3032_temp_attr_set,
	.attr_get = rv3032_temp_attr_get,
#ifdef CONFIG_RV3032_TEMP_TRIGGER
	.trigger_set = rv3032_temp_trigger_set,
#endif /* CONFIG_RV3032_TEMP_TRIGGER */

};

static int rv3032_temp_init(const struct device *dev)
{
	struct rv3032_temp_data *data = dev->data;
	const struct rv3032_temp_config *config = dev->config;

	LOG_DBG("Temp dev[%s] mdf-parent[%s]\n", dev->name, config->parent->name);

	/* Setup TLOW and THIGH default from device tree */
	mfd_rv3032_write_reg8(config->parent, RV3032_REG_TEMP_LOW_THLD, data->tlow);
	mfd_rv3032_write_reg8(config->parent, RV3032_REG_TEMP_HIGH_THLD, data->thigh);

	return 0;
}

#define RV3032_TEMP(inst)                                                                       \
	static const struct rv3032_temp_config rv3032_temp_dev_config_##inst = {                \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                  \
	};                                                                                      \
                                                                                                \
	static struct rv3032_temp_data rv3032_temp_dev_data_##inst = {                          \
		.tlow = DT_INST_PROP(inst, low_temp),                                           \
		.thigh = DT_INST_PROP(inst, high_temp)                                          \
	};                                                                                      \
                                                                                                \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, rv3032_temp_init, NULL,                              \
			      &rv3032_temp_dev_data_##inst, &rv3032_temp_dev_config_##inst,     \
			      POST_KERNEL, CONFIG_TEMP_MICROCRYSTAL_RV3032_INIT_PRIORITY,       \
			      &rv3032_temp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV3032_TEMP)
