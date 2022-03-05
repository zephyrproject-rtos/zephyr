/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2022 Grzegorz Blach
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/sensor.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <init.h>
#include <kernel.h>
#include <string.h>
#include <logging/log.h>

#include "bh1745.h"

static struct k_work_delayable bh1745_init_work;
static const struct device *bh1745_dev;

LOG_MODULE_REGISTER(BH1745, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT rohm_bh1745

static const int32_t async_init_delay[ASYNC_INIT_STEP_COUNT] = {
	[ASYNC_INIT_STEP_RESET_CHECK] = 2,
	[ASYNC_INIT_RGB_ENABLE] = 0,
	[ASYNC_INIT_STEP_CONFIGURE] = 0
};

static int bh1745_async_init_reset_check(struct bh1745_data *data);
static int bh1745_async_init_rgb_enable(struct bh1745_data *data);
static int bh1745_async_init_configure(struct bh1745_data *data);

static int(*const async_init_fn[ASYNC_INIT_STEP_COUNT])(
	struct bh1745_data *data) = {
	[ASYNC_INIT_STEP_RESET_CHECK] = bh1745_async_init_reset_check,
	[ASYNC_INIT_RGB_ENABLE] = bh1745_async_init_rgb_enable,
	[ASYNC_INIT_STEP_CONFIGURE] = bh1745_async_init_configure,
};

static int bh1745_sample_fetch(const struct device *dev, enum sensor_channel chan)

{
	struct bh1745_data *data = dev->data;
	uint8_t status;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	if (unlikely(!data->ready)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	LOG_DBG("Fetching sample...\n");

	if (i2c_reg_read_byte(data->i2c, DT_REG_ADDR(DT_DRV_INST(0)),
			      BH1745_MODE_CONTROL2, &status)) {
		LOG_ERR("Could not read status register CONTROL2");
		return -EIO;
	}

	LOG_DBG("MODE_CONTROL_2 %x", status);

	if ((status & (BH1745_MODE_CONTROL2_VALID_Msk)) == 0) {
		LOG_ERR("No valid data to fetch.");
		return -EIO;
	}

	if (i2c_burst_read(data->i2c, DT_REG_ADDR(DT_DRV_INST(0)),
			   BH1745_RED_DATA_LSB,
			   (uint8_t *)data->sample_rgb_light,
			   BH1745_SAMPLES_TO_FETCH * sizeof(uint16_t))) {
		LOG_ERR("Could not read sensor samples");
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_BH1745_TRIGGER)) {
		/* Clearing interrupt by reading INTERRUPT register */
		uint8_t dummy;

		if (i2c_reg_read_byte(data->i2c,
				      DT_REG_ADDR(DT_DRV_INST(0)),
				      BH1745_INTERRUPT, &dummy)) {
			LOG_ERR("Could not disable sensor interrupt.");
			return -EIO;
		}

		if (gpio_pin_interrupt_configure(data->gpio,
					DT_INST_GPIO_PIN(0, int_gpios),
					GPIO_INT_LEVEL_LOW)) {
			LOG_ERR("Could not enable pin callback");
			return -EIO;
		}
	}

	return 0;
}

static int bh1745_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bh1745_data *data = dev->data;

	if (unlikely(!data->ready)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	switch (chan) {
	case SENSOR_CHAN_RED:
		val->val1 = sys_le16_to_cpu(
			data->sample_rgb_light[BH1745_SAMPLE_POS_RED]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = sys_le16_to_cpu(
			data->sample_rgb_light[BH1745_SAMPLE_POS_GREEN]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = sys_le16_to_cpu(
			data->sample_rgb_light[BH1745_SAMPLE_POS_BLUE]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_LIGHT:
		val->val1 = sys_le16_to_cpu(
			data->sample_rgb_light[BH1745_SAMPLE_POS_LIGHT]);
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bh1745_check(struct bh1745_data *data)
{
	uint8_t manufacturer_id;

	if (i2c_reg_read_byte(data->i2c, DT_REG_ADDR(DT_DRV_INST(0)),
			      BH1745_MANUFACTURER_ID, &manufacturer_id)) {
		LOG_ERR("Failed when reading manufacturer ID");
		return -EIO;
	}

	LOG_DBG("Manufacturer ID: 0x%02x", manufacturer_id);

	if (manufacturer_id != BH1745_MANUFACTURER_ID_DEFAULT) {
		LOG_ERR("Invalid manufacturer ID: 0x%02x", manufacturer_id);
		return -EIO;
	}

	uint8_t part_id;

	if (i2c_reg_read_byte(data->i2c, DT_REG_ADDR(DT_DRV_INST(0)),
			      BH1745_SYSTEM_CONTROL, &part_id)) {
		LOG_ERR("Failed when reading part ID");
		return -EIO;
	}

	if ((part_id & BH1745_SYSTEM_CONTROL_PART_ID_Msk) !=
	    BH1745_SYSTEM_CONTROL_PART_ID) {
		LOG_ERR("Invalid part ID: 0x%02x", part_id);
		return -EIO;
	}

	LOG_DBG("Part ID: 0x%02x", part_id);

	return 0;
}

static int bh1745_sw_reset(const struct device *dev)
{
	return i2c_reg_update_byte(dev, DT_REG_ADDR(DT_DRV_INST(0)),
				   BH1745_SYSTEM_CONTROL,
				   BH1745_SYSTEM_CONTROL_SW_RESET_Msk,
				   BH1745_SYSTEM_CONTROL_SW_RESET);
}

static int bh1745_rgb_measurement_enable(struct bh1745_data *data, bool enable)
{
	uint8_t en = enable ?
		  BH1745_MODE_CONTROL2_RGB_EN_ENABLE :
		  BH1745_MODE_CONTROL2_RGB_EN_DISABLE;

	return i2c_reg_update_byte(data->i2c,
				   DT_REG_ADDR(DT_DRV_INST(0)),
				   BH1745_MODE_CONTROL2,
				   BH1745_MODE_CONTROL2_RGB_EN_Msk,
				   en);
}


static void bh1745_async_init(struct k_work *work)
{
	struct bh1745_data *data = bh1745_dev->data;

	ARG_UNUSED(work);

	LOG_DBG("BH1745 async init step %d", data->async_init_step);

	data->err = async_init_fn[data->async_init_step](data);

	if (data->err) {
		LOG_ERR("BH1745 initialization failed");
	} else {
		data->async_init_step++;

		if (data->async_init_step == ASYNC_INIT_STEP_COUNT) {
			data->ready = true;
			LOG_INF("BH1745 initialized");
		} else {
			k_work_schedule(&bh1745_init_work,
					K_MSEC(async_init_delay[data->async_init_step]));
		}
	}
}

static int bh1745_async_init_reset_check(struct bh1745_data *data)
{
	(void)memset(data->sample_rgb_light, 0, sizeof(data->sample_rgb_light));
	int err;

	err = bh1745_sw_reset(data->i2c);

	if (err) {
		LOG_ERR("Could not apply software reset.");
		return -EIO;
	}
	err = bh1745_check(data);
	if (err) {
		LOG_ERR("Communication with BH1745 failed with error %d", err);
		return -EIO;
	}
	return 0;
}

static int bh1745_async_init_rgb_enable(struct bh1745_data *data)
{
	int err;

	err = bh1745_rgb_measurement_enable(data, true);
	if (err) {
		LOG_ERR("Could not set measurement mode.");
		return -EIO;
	}
	return 0;
}

static int bh1745_async_init_configure(struct bh1745_data *data)
{
	if (i2c_reg_write_byte(data->i2c, DT_REG_ADDR(DT_DRV_INST(0)),
			       BH1745_MODE_CONTROL1,
			       BH1745_MODE_CONTROL1_DEFAULTS)) {
		LOG_ERR(
			"Could not set gain and measurement mode configuration.");
		return -EIO;
	}

#ifdef CONFIG_BH1745_TRIGGER
	int err = bh1745_gpio_interrupt_init(bh1745_dev);
	if (err) {
		LOG_ERR("Failed to initialize interrupt with error %d", err);
		return -EIO;
	}

	LOG_DBG("GPIO Sense Interrupts initialized");
#endif /* CONFIG_BH1745_TRIGGER */

	return 0;
}

static int bh1745_init(const struct device *dev)
{
	bh1745_dev = dev;

	struct bh1745_data *data = bh1745_dev->data;
	data->i2c = device_get_binding(DT_BUS_LABEL(DT_DRV_INST(0)));

	if (data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DT_BUS_LABEL(DT_DRV_INST(0)));

		return -EINVAL;
	}
	k_work_init_delayable(&bh1745_init_work, bh1745_async_init);
	k_work_schedule(&bh1745_init_work, K_MSEC(async_init_delay[data->async_init_step]));
	return 0;
};

static const struct sensor_driver_api bh1745_driver_api = {
	.sample_fetch = &bh1745_sample_fetch,
	.channel_get = &bh1745_channel_get,
#ifdef CONFIG_BH1745_TRIGGER
	.attr_set = &bh1745_attr_set,
	.trigger_set = &bh1745_trigger_set,
#endif
};

static struct bh1745_data bh1745_data;

DEVICE_DEFINE(bh1745, DT_INST_LABEL(0),
	      bh1745_init, NULL, &bh1745_data, NULL,
	      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &bh1745_driver_api);
