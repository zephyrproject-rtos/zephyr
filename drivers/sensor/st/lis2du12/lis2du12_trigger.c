/* ST Microelectronics LIS2DU12 3-axis accelerometer sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2025 8tronix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2du12.pdf
 */

#define DT_DRV_COMPAT st_lis2du12

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lis2du12.h"

LOG_MODULE_DECLARE(LIS2DU12, CONFIG_SENSOR_LOG_LEVEL);

#define INT1_IDX 0
#define INT2_IDX 1

/**
 * lis2du12_enable_drdy_int - XL enable selected int pin to generate interrupt
 */
static int lis2du12_enable_drdy_int(const struct device *dev, int enable)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		lis2du12_md_t md;
		lis2du12_data_t xl_data;

		/* dummy read: re-trigger interrupt */
		md.fs = cfg->accel_range;
		lis2du12_data_get(ctx, &md, &xl_data);
	}

	/* set interrupt */
	if (cfg->drdy_pin == 1) {
		lis2du12_pin_int_route_t val;

		ret = lis2du12_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy_xl = enable ? PROPERTY_ENABLE : PROPERTY_DISABLE;

		ret = lis2du12_pin_int1_route_set(ctx, &val);
	} else {
		lis2du12_pin_int_route_t val;

		ret = lis2du12_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy_xl = enable ? PROPERTY_ENABLE : PROPERTY_DISABLE;

		ret = lis2du12_pin_int2_route_set(ctx, &val);
	}

	return ret;
}

int lis2du12_enable_delta_int(const struct device *dev, int enable)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (cfg->delta_pin == 1) {
		lis2du12_pin_int_route_t val;

		ret = lis2du12_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.wake_up = enable ? PROPERTY_ENABLE : PROPERTY_DISABLE;

		ret = lis2du12_pin_int1_route_set(ctx, &val);
	} else {
		lis2du12_pin_int_route_t val;

		ret = lis2du12_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.wake_up = enable ? PROPERTY_ENABLE : PROPERTY_DISABLE;

		ret = lis2du12_pin_int2_route_set(ctx, &val);
	}

	lis2du12_wkup_md_t wkup_md;

	if (lis2du12_wake_up_mode_get(ctx, &wkup_md) < 0) {
		LOG_ERR("failed reading wake up mode");
		return -EIO;
	}

	wkup_md.x_en = enable ? PROPERTY_ENABLE : PROPERTY_DISABLE;
	wkup_md.y_en = wkup_md.x_en;
	wkup_md.z_en = wkup_md.x_en;

	if (lis2du12_wake_up_mode_set(ctx, &wkup_md) < 0) {
		LOG_ERR("failed setting wake up mode");
		return -EIO;
	}

	lis2du12_int_mode_t int_mode;

	if (lis2du12_interrupt_mode_set(ctx, &int_mode) < 0) {
		LOG_ERR("failed reading int mode");
		return -EIO;
	}

	int_mode.enable = enable ? PROPERTY_ENABLE : PROPERTY_DISABLE;

	if (lis2du12_interrupt_mode_set(ctx, &int_mode) < 0) {
		LOG_ERR("failed setting int mode");
		return -EIO;
	}

	return ret;
}

/**
 * lis2du12_trigger_set - link external trigger to event data ready
 */
int lis2du12_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct lis2du12_config *cfg = dev->config;
	struct lis2du12_data *lis2du12 = dev->data;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		if (trig->chan != SENSOR_CHAN_ACCEL_XYZ) {
			return -ENOTSUP;
		}
		lis2du12->handler_drdy_acc = handler;
		lis2du12->trig_drdy_acc = trig;
		if (handler) {
			return lis2du12_enable_drdy_int(dev, LIS2DU12_EN_BIT);
		}

		return lis2du12_enable_drdy_int(dev, LIS2DU12_DIS_BIT);

	case SENSOR_TRIG_DELTA:
		if (trig->chan != SENSOR_CHAN_ACCEL_XYZ) {
			return -ENOTSUP;
		}
		lis2du12->handler_delta_xyz_acc = handler;
		lis2du12->trig_delta_xyz_acc = trig;
		if (handler) {
			return lis2du12_enable_delta_int(dev, LIS2DU12_EN_BIT);
		}

		return lis2du12_enable_delta_int(dev, LIS2DU12_DIS_BIT);

	default:
		return -ENOTSUP;
	}

}

/**
 * lis2du12_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lis2du12_handle_interrupt(const struct device *dev)
{
	struct lis2du12_data *lis2du12 = dev->data;
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	bool int1_triggered = atomic_test_and_clear_bit(&lis2du12->trig_flags, INT1_IDX);
	bool int2_triggered = atomic_test_and_clear_bit(&lis2du12->trig_flags, INT2_IDX);

	bool drdy = (int1_triggered && cfg->drdy_pin == 1)
		    || (int2_triggered && cfg->drdy_pin == 2);

	bool delta = (int1_triggered && cfg->delta_pin == 1)
		     || (int2_triggered && cfg->delta_pin == 2);

	/* This has to be repeated till the MSB of one of the output
	 * registers is read by the user
	 */
	while (drdy) {
		lis2du12_status_t status;

		if (lis2du12_status_get(ctx, &status) < 0) {
			LOG_ERR("failed reading status reg");
			return;
		}

		if (status.drdy_xl == 0) {
			break;
		}

		if ((status.drdy_xl) && (lis2du12->handler_drdy_acc != NULL)) {
			lis2du12->handler_drdy_acc(dev, lis2du12->trig_drdy_acc);
		}
	}

	if (delta) {
		lis2du12_all_sources_t all_src;

		if (lis2du12_all_sources_get(ctx, &all_src) < 0) {
			LOG_ERR("failed reading all interrupt sources");
			return;
		}

		if (all_src.wake_up && lis2du12->handler_delta_xyz_acc != NULL) {
			lis2du12->handler_delta_xyz_acc(dev, lis2du12->trig_delta_xyz_acc);
		}
	}

	if (int1_triggered) {
		gpio_pin_interrupt_configure_dt(&cfg->int1_gpio,
						GPIO_INT_EDGE_TO_ACTIVE);
	}

	if (int2_triggered) {
		gpio_pin_interrupt_configure_dt(&cfg->int2_gpio,
						GPIO_INT_EDGE_TO_ACTIVE);
	}
}

static void lis2du12_gpio_int1_callback(const struct device *dev,
					struct gpio_callback *cb, uint32_t pins)
{
	struct lis2du12_data *lis2du12 =
		CONTAINER_OF(cb, struct lis2du12_data, gpio_cb[INT1_IDX]);

	const struct lis2du12_config *cfg = lis2du12->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->int1_gpio, GPIO_INT_DISABLE);

	atomic_set_bit(&lis2du12->trig_flags, INT1_IDX);

#if defined(CONFIG_LIS2DU12_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2du12->gpio_sem);
#elif defined(CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2du12->work);
#endif /* CONFIG_LIS2DU12_TRIGGER_OWN_THREAD */
}

static void lis2du12_gpio_int2_callback(const struct device *dev,
					struct gpio_callback *cb, uint32_t pins)
{
	struct lis2du12_data *lis2du12 =
		CONTAINER_OF(cb, struct lis2du12_data, gpio_cb[INT2_IDX]);

	const struct lis2du12_config *cfg = lis2du12->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->int2_gpio, GPIO_INT_DISABLE);

	atomic_set_bit(&lis2du12->trig_flags, INT2_IDX);

#if defined(CONFIG_LIS2DU12_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2du12->gpio_sem);
#elif defined(CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2du12->work);
#endif /* CONFIG_LIS2DU12_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LIS2DU12_TRIGGER_OWN_THREAD
static void lis2du12_thread(struct lis2du12_data *lis2du12)
{
	while (1) {
		k_sem_take(&lis2du12->gpio_sem, K_FOREVER);
		lis2du12_handle_interrupt(lis2du12->dev);
	}
}
#endif /* CONFIG_LIS2DU12_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD
static void lis2du12_work_cb(struct k_work *work)
{
	struct lis2du12_data *lis2du12 =
		CONTAINER_OF(work, struct lis2du12_data, work);

	lis2du12_handle_interrupt(lis2du12->dev);
}
#endif /* CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD */

int lis2du12_init_interrupt(const struct device *dev)
{
	struct lis2du12_data *lis2du12 = dev->data;
	const struct lis2du12_config *cfg = dev->config;
	int ret;

	const struct gpio_dt_spec *int_gpios[] = {
		&cfg->int1_gpio,
		&cfg->int2_gpio
	};

	gpio_callback_handler_t gpio_callbacks[] = {
		lis2du12_gpio_int1_callback,
		lis2du12_gpio_int2_callback
	};

	for (int i = 0; i < ARRAY_SIZE(int_gpios); i++) {
		if (int_gpios[i]->port == NULL) {
			continue;
		}

		/* setup data ready gpio interrupt (INT1 or INT2) */
		if (!gpio_is_ready_dt(int_gpios[i])) {
			LOG_ERR("Cannot get pointer to int%d_gpio device (%p)",
				i + 1, int_gpios[i]);
			return -EINVAL;
		}
	}

#if defined(CONFIG_LIS2DU12_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2du12->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lis2du12->thread, lis2du12->thread_stack,
			CONFIG_LIS2DU12_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2du12_thread, lis2du12,
			NULL, NULL, K_PRIO_COOP(CONFIG_LIS2DU12_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lis2du12->thread, dev->name);
#elif defined(CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD)
	lis2du12->work.handler = lis2du12_work_cb;
#endif /* CONFIG_LIS2DU12_TRIGGER_OWN_THREAD */

	for (int i = 0; i < ARRAY_SIZE(int_gpios); i++) {
		if (int_gpios[i]->port == NULL) {
			continue;
		}

		ret = gpio_pin_configure_dt(int_gpios[i], GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure gpio: %d", ret);
			return ret;
		}

		gpio_init_callback(&lis2du12->gpio_cb[i],
				   gpio_callbacks[i],
				   BIT(int_gpios[i]->pin));

		if (gpio_add_callback(int_gpios[i]->port, &lis2du12->gpio_cb[i]) < 0) {
			LOG_ERR("Could not set gpio callback");
			return -EIO;
		}

		ret = gpio_pin_interrupt_configure_dt(int_gpios[i],
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure gpio interrupt: %d", ret);
			return ret;
		}
	}

	return 0;
}

int lis2du12_accel_set_wake_th(const struct device *dev, const struct sensor_value *val)
{
	struct lis2du12_data *lis2du12 = dev->data;
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_wkup_md_t wakeup_mode;
	int ret;
	int32_t fs_mg;
	int32_t threshold_mg;

	threshold_mg = sensor_ms2_to_mg(val);
	fs_mg = lis2du12->accel_fs * 1000;

	if ((threshold_mg < 0) || (threshold_mg > fs_mg)) {
		LOG_WRN("Invalid threshold");
		return -EINVAL;
	}

	ret = lis2du12_wake_up_mode_get(ctx, &wakeup_mode);
	if (ret < 0) {
		LOG_ERR("Failed to get wake-up mode");
		return ret;
	}

	wakeup_mode.threshold = (threshold_mg * (256) / fs_mg); /* 1 LSB = FS/2^8 */

	return lis2du12_wake_up_mode_set(ctx, &wakeup_mode);
}

int lis2du12_accel_set_wake_dur(const struct device *dev, const struct sensor_value *val)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_wkup_md_t wakeup_mode;
	int ret;

	if (val->val1 < 0 || val->val1 > LIS2DU12_WAKUP_DUR_SAMPLES_MAX) {
		LOG_WRN("Unsupported number of samples for wake-up duration");
		return -ENOTSUP;
	}

	ret = lis2du12_wake_up_mode_get(ctx, &wakeup_mode);
	if (ret < 0) {
		LOG_ERR("Failed to get wake-up mode");
		return ret;
	}

	wakeup_mode.duration = (uint8_t)val->val1;

	return lis2du12_wake_up_mode_set(ctx, &wakeup_mode);
}
