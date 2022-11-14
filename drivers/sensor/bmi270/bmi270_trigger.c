/*
 * Copyright (c) 2023 Elektronikutvecklingsbyrån EUB AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bmi270);

#include "bmi270.h"

enum {
	INT_FLAGS_INT1,
	INT_FLAGS_INT2,
};

static void bmi270_raise_int_flag(const struct device *dev, int bit)
{
	struct bmi270_data *data = dev->data;

	atomic_set_bit(&data->int_flags, bit);

#if defined(CONFIG_BMI270_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_BMI270_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->trig_work);
#endif
}

static void bmi270_int1_callback(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct bmi270_data *data =
		CONTAINER_OF(cb, struct bmi270_data, int1_cb);
	bmi270_raise_int_flag(data->dev, INT_FLAGS_INT1);
}

static void bmi270_int2_callback(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct bmi270_data *data =
		CONTAINER_OF(cb, struct bmi270_data, int2_cb);
	bmi270_raise_int_flag(data->dev, INT_FLAGS_INT2);
}


static void bmi270_thread_cb(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	int ret;

	/* INT1 is used for feature interrupts */
	if (atomic_test_and_clear_bit(&data->int_flags, INT_FLAGS_INT1)) {
		uint16_t int_status;

		ret = bmi270_reg_read(dev, BMI270_REG_INT_STATUS_0,
			(uint8_t *)&int_status, sizeof(int_status));
		if (ret < 0) {
			LOG_ERR("read interrupt status returned %d", ret);
			return;
		}

		k_mutex_lock(&data->trigger_mutex, K_FOREVER);

		if (data->motion_handler != NULL) {
			if (int_status & BMI270_INT_STATUS_ANY_MOTION) {
				data->motion_handler(dev, data->motion_trigger);
			}
		}

		k_mutex_unlock(&data->trigger_mutex);
	}

	/* INT2 is used for data ready interrupts */
	if (atomic_test_and_clear_bit(&data->int_flags, INT_FLAGS_INT2)) {
		k_mutex_lock(&data->trigger_mutex, K_FOREVER);

		if (data->drdy_handler != NULL) {
			data->drdy_handler(dev, data->drdy_trigger);
		}

		k_mutex_unlock(&data->trigger_mutex);
	}
}

#ifdef CONFIG_BMI270_TRIGGER_OWN_THREAD
static void bmi270_thread(struct bmi270_data *data)
{
	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		bmi270_thread_cb(data->dev);
	}
}
#endif

#ifdef CONFIG_BMI270_TRIGGER_GLOBAL_THREAD
static void bmi270_trig_work_cb(struct k_work *work)
{
	struct bmi270_data *data =
		CONTAINER_OF(work, struct bmi270_data, trig_work);

	bmi270_thread_cb(data->dev);
}
#endif

static int bmi270_feature_reg_write(const struct device *dev,
			     const struct bmi270_feature_reg *reg,
			     uint16_t value)
{
	int ret;
	uint8_t feat_page = reg->page;

	ret = bmi270_reg_write(dev, BMI270_REG_FEAT_PAGE, &feat_page, 1);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_write (0x%02x) failed: %d", BMI270_REG_FEAT_PAGE, ret);
		return ret;
	}

	LOG_DBG("feature reg[0x%02x]@%d = 0x%04x", reg->addr, reg->page, value);

	ret = bmi270_reg_write(dev, reg->addr, (uint8_t *)&value, 2);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_write (0x%02x) failed: %d", reg->addr, ret);
		return ret;
	}

	return 0;
}

static int bmi270_init_int_pin(const struct gpio_dt_spec *pin,
			       struct gpio_callback *pin_cb,
			       gpio_callback_handler_t handler)
{
	int ret;

	if (!pin->port) {
		return 0;
	}

	if (!device_is_ready(pin->port)) {
		LOG_DBG("%s not ready", pin->port->name);
		return -ENODEV;
	}

	gpio_init_callback(pin_cb, handler, BIT(pin->pin));

	ret = gpio_pin_configure_dt(pin, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(pin, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		return ret;
	}

	ret = gpio_add_callback(pin->port, pin_cb);
	if (ret) {
		return ret;
	}

	return 0;
}


int bmi270_init_interrupts(const struct device *dev)
{
	const struct bmi270_config *cfg = dev->config;
	struct bmi270_data *data = dev->data;
	int ret;

#if CONFIG_BMI270_TRIGGER_OWN_THREAD
	k_sem_init(&data->trig_sem, 0, 1);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_BMI270_THREAD_STACK_SIZE,
			(k_thread_entry_t)bmi270_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_BMI270_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif CONFIG_BMI270_TRIGGER_GLOBAL_THREAD
	k_work_init(&data->trig_work, bmi270_trig_work_cb)
#endif

	ret = bmi270_init_int_pin(&cfg->int1, &data->int1_cb,
				  bmi270_int1_callback);
	if (ret) {
		LOG_ERR("Failed to initialize INT1");
		return -EINVAL;
	}

	ret = bmi270_init_int_pin(&cfg->int2, &data->int2_cb,
				  bmi270_int2_callback);
	if (ret) {
		LOG_ERR("Failed to initialize INT2");
		return -EINVAL;
	}

	if (cfg->int1.port) {
		uint8_t int1_io_ctrl = BMI270_INT_IO_CTRL_OUTPUT_EN;

		ret = bmi270_reg_write(dev, BMI270_REG_INT1_IO_CTRL, &int1_io_ctrl, 1);
		if (ret < 0) {
			LOG_ERR("failed configuring INT1_IO_CTRL (%d)", ret);
			return ret;
		}
	}

	if (cfg->int2.port) {
		uint8_t int2_io_ctrl = BMI270_INT_IO_CTRL_OUTPUT_EN;

		ret = bmi270_reg_write(dev, BMI270_REG_INT2_IO_CTRL, &int2_io_ctrl, 1);
		if (ret < 0) {
			LOG_ERR("failed configuring INT2_IO_CTRL (%d)", ret);
			return ret;
		}
	}

	return 0;
}

static int bmi270_anymo_config(const struct device *dev, bool enable)
{
	const struct bmi270_config *cfg = dev->config;
	struct bmi270_data *data = dev->data;
	uint16_t anymo_2;
	int ret;

	if (enable) {
		ret = bmi270_feature_reg_write(dev, cfg->feature->anymo_1,
					       data->anymo_1);
		if (ret < 0) {
			return ret;
		}
	}

	anymo_2 = data->anymo_2;
	if (enable) {
		anymo_2 |= BMI270_ANYMO_2_ENABLE;
	}

	ret = bmi270_feature_reg_write(dev, cfg->feature->anymo_2, anymo_2);
	if (ret < 0) {
		return ret;
	}

	uint8_t int1_map_feat = 0;

	if (enable) {
		int1_map_feat |= BMI270_INT_MAP_ANY_MOTION;
	}

	ret = bmi270_reg_write(dev, BMI270_REG_INT1_MAP_FEAT, &int1_map_feat, 1);
	if (ret < 0) {
		LOG_ERR("failed configuring INT1_MAP_FEAT (%d)", ret);
		return ret;
	}

	return 0;
}

static int bmi270_drdy_config(const struct device *dev, bool enable)
{
	int ret;

	uint8_t int_map_data = 0;

	if (enable) {
		int_map_data |= BMI270_INT_MAP_DATA_DRDY_INT2;
	}

	ret = bmi270_reg_write(dev, BMI270_REG_INT_MAP_DATA, &int_map_data, 1);
	if (ret < 0) {
		LOG_ERR("failed configuring INT_MAP_DATA (%d)", ret);
		return ret;
	}

	return 0;
}

int bmi270_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bmi270_data *data = dev->data;
	const struct bmi270_config *cfg = dev->config;

	switch (trig->type) {
	case SENSOR_TRIG_MOTION:
		if (!cfg->int1.port) {
			return -ENOTSUP;
		}

		k_mutex_lock(&data->trigger_mutex, K_FOREVER);
		data->motion_handler = handler;
		data->motion_trigger = trig;
		k_mutex_unlock(&data->trigger_mutex);
		return bmi270_anymo_config(dev, handler != NULL);

	case SENSOR_TRIG_DATA_READY:
		if (!cfg->int2.port) {
			return -ENOTSUP;
		}

		k_mutex_lock(&data->trigger_mutex, K_FOREVER);
		data->drdy_handler = handler;
		data->drdy_trigger = trig;
		k_mutex_unlock(&data->trigger_mutex);
		return bmi270_drdy_config(dev, handler != NULL);
	default:
		return -ENOTSUP;
	}

	return 0;
}
