/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */

#define DT_DRV_COMPAT st_lis2dw12

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lis2dw12.h"

LOG_MODULE_DECLARE(LIS2DW12, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lis2dw12_enable_int - enable selected int pin to generate interrupt
 */
static int lis2dw12_enable_int(const struct device *dev,
			       enum sensor_trigger_type type, int enable)
{
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dw12_reg_t int_route;

	switch (type) {
	case SENSOR_TRIG_DATA_READY:
		if (cfg->int_pin == 1) {
			/* set interrupt for pin INT1 */
			lis2dw12_pin_int1_route_get(ctx,
					&int_route.ctrl4_int1_pad_ctrl);
			int_route.ctrl4_int1_pad_ctrl.int1_drdy = enable;

			return lis2dw12_pin_int1_route_set(ctx,
					&int_route.ctrl4_int1_pad_ctrl);
		} else {
			/* set interrupt for pin INT2 */
			lis2dw12_pin_int2_route_get(ctx,
					&int_route.ctrl5_int2_pad_ctrl);
			int_route.ctrl5_int2_pad_ctrl.int2_drdy = enable;

			return lis2dw12_pin_int2_route_set(ctx,
					&int_route.ctrl5_int2_pad_ctrl);
		}
		break;
#ifdef CONFIG_LIS2DW12_TAP
	case SENSOR_TRIG_TAP:
		/* set interrupt for pin INT1 */
		lis2dw12_pin_int1_route_get(ctx,
				&int_route.ctrl4_int1_pad_ctrl);
		int_route.ctrl4_int1_pad_ctrl.int1_single_tap = enable;

		return lis2dw12_pin_int1_route_set(ctx,
				&int_route.ctrl4_int1_pad_ctrl);
	case SENSOR_TRIG_DOUBLE_TAP:
		/* set interrupt for pin INT1 */
		lis2dw12_pin_int1_route_get(ctx,
				&int_route.ctrl4_int1_pad_ctrl);
		int_route.ctrl4_int1_pad_ctrl.int1_tap = enable;

		return lis2dw12_pin_int1_route_set(ctx,
				&int_route.ctrl4_int1_pad_ctrl);
#endif /* CONFIG_LIS2DW12_TAP */
#ifdef CONFIG_LIS2DW12_WAKEUP
	/**
	 * Trigger fires when channel reading transitions configured
	 * thresholds.  The thresholds are configured via the @ref
	 * SENSOR_ATTR_LOWER_THRESH and @ref SENSOR_ATTR_UPPER_THRESH
	 * attributes.
	 */
	case SENSOR_TRIG_MOTION:
		LOG_DBG("Setting int1_wu: %d\n", enable);
		lis2dw12_pin_int1_route_get(ctx,
				&int_route.ctrl4_int1_pad_ctrl);
		int_route.ctrl4_int1_pad_ctrl.int1_wu = enable;
		return lis2dw12_pin_int1_route_set(ctx,
						   &int_route.ctrl4_int1_pad_ctrl);
#endif
#ifdef CONFIG_LIS2DW12_SLEEP
	/**
	 * Trigger fires when channel reading transitions configured
	 * thresholds for a certain time. The thresholds are configured
	 * via the @ref SENSOR_ATTR_LOWER_THRESH and @ref SENSOR_ATTR_UPPER_THRESH
	 * attributes.
	 */
	case SENSOR_TRIG_STATIONARY:
	LOG_DBG("Setting int2_sleep_chg: %d\n", enable);
		lis2dw12_pin_int2_route_get(ctx,
				&int_route.ctrl5_int2_pad_ctrl);
		int_route.ctrl5_int2_pad_ctrl.int2_sleep_chg = enable;
		return lis2dw12_pin_int2_route_set(ctx,
						   &int_route.ctrl5_int2_pad_ctrl);
#endif
#ifdef CONFIG_LIS2DW12_FREEFALL
	/**
	 * Trigger fires when the readings does not include Earth's
	 * gravitional force for configured duration and threshold.
	 * The duration and the threshold can be configured in the
	 * devicetree source of the accelerometer node.
	 */
	case SENSOR_TRIG_FREEFALL:
		LOG_DBG("Setting int1_ff: %d\n", enable);
		lis2dw12_pin_int1_route_get(ctx,
				&int_route.ctrl4_int1_pad_ctrl);
		int_route.ctrl4_int1_pad_ctrl.int1_ff = enable;
		return lis2dw12_pin_int1_route_set(ctx,
				&int_route.ctrl4_int1_pad_ctrl);
#endif /* CONFIG_LIS2DW12_FREEFALL */
	default:
		LOG_ERR("Unsupported trigger interrupt route %d", type);
		return -ENOTSUP;
	}
}

/**
 * lis2dw12_trigger_set - link external trigger to event data ready
 */
int lis2dw12_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lis2dw12_data *lis2dw12 = dev->data;
	int16_t raw[3];
	int state = (handler != NULL) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

	if (cfg->gpio_int.port == NULL) {
		LOG_ERR("trigger_set is not supported");
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		lis2dw12->drdy_handler = handler;
		lis2dw12->drdy_trig = trig;
		if (state) {
			/* dummy read: re-trigger interrupt */
			lis2dw12_acceleration_raw_get(ctx, raw);
		}
		return lis2dw12_enable_int(dev, SENSOR_TRIG_DATA_READY, state);
		break;
#ifdef CONFIG_LIS2DW12_TAP
	case SENSOR_TRIG_TAP:
	case SENSOR_TRIG_DOUBLE_TAP:
		/* check if tap detection is enabled  */
		if ((cfg->tap_threshold[0] == 0) &&
		    (cfg->tap_threshold[1] == 0) &&
		    (cfg->tap_threshold[2] == 0)) {
			LOG_ERR("Unsupported sensor trigger");
			return -ENOTSUP;
		}

		/* Set single TAP trigger  */
		if (trig->type == SENSOR_TRIG_TAP) {
			lis2dw12->tap_handler = handler;
			lis2dw12->tap_trig = trig;
			return lis2dw12_enable_int(dev, SENSOR_TRIG_TAP, state);
		}

		/* Set double TAP trigger  */
		lis2dw12->double_tap_handler = handler;
		lis2dw12->double_tap_trig = trig;
		return lis2dw12_enable_int(dev, SENSOR_TRIG_DOUBLE_TAP, state);
#endif /* CONFIG_LIS2DW12_TAP */
#ifdef CONFIG_LIS2DW12_WAKEUP
	case SENSOR_TRIG_MOTION:
	{
		LOG_DBG("Set trigger %d (handler: %p)\n", trig->type, handler);
		lis2dw12->motion_handler = handler;
		lis2dw12->motion_trig = trig;
		return lis2dw12_enable_int(dev, SENSOR_TRIG_MOTION, state);
	}
#endif
#ifdef CONFIG_LIS2DW12_SLEEP
	case SENSOR_TRIG_STATIONARY:
	{
		LOG_DBG("Set trigger %d (handler: %p)\n", trig->type, handler);
		lis2dw12->stationary_handler = handler;
		lis2dw12->stationary_trig = trig;
		return lis2dw12_enable_int(dev, SENSOR_TRIG_STATIONARY, state);
	}
#endif
#ifdef CONFIG_LIS2DW12_FREEFALL
	case SENSOR_TRIG_FREEFALL:
	LOG_DBG("Set freefall %d (handler: %p)\n", trig->type, handler);
		lis2dw12->freefall_handler = handler;
		lis2dw12->freefall_trig = trig;
		return lis2dw12_enable_int(dev, SENSOR_TRIG_FREEFALL, state);
	break;
#endif /* CONFIG_LIS2DW12_FREEFALL */
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}
}

static int lis2dw12_handle_drdy_int(const struct device *dev)
{
	struct lis2dw12_data *data = dev->data;

	if (data->drdy_handler) {
		data->drdy_handler(dev, data->drdy_trig);
	}

	return 0;
}

#ifdef CONFIG_LIS2DW12_TAP
static int lis2dw12_handle_single_tap_int(const struct device *dev)
{
	struct lis2dw12_data *data = dev->data;
	sensor_trigger_handler_t handler = data->tap_handler;

	if (handler) {
		handler(dev, data->tap_trig);
	}

	return 0;
}

static int lis2dw12_handle_double_tap_int(const struct device *dev)
{
	struct lis2dw12_data *data = dev->data;
	sensor_trigger_handler_t handler = data->double_tap_handler;

	if (handler) {
		handler(dev, data->double_tap_trig);
	}

	return 0;
}
#endif /* CONFIG_LIS2DW12_TAP */

#ifdef CONFIG_LIS2DW12_WAKEUP
static int lis2dw12_handle_wu_ia_int(const struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	sensor_trigger_handler_t handler = lis2dw12->motion_handler;

	if (handler) {
		handler(dev, lis2dw12->motion_trig);
	}

	return 0;
}
#endif

#ifdef CONFIG_LIS2DW12_SLEEP
static int lis2dw12_handle_sleep_change_int(const struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	sensor_trigger_handler_t handler = lis2dw12->stationary_handler;

	if (handler) {
		handler(dev, lis2dw12->stationary_trig);
	}

	return 0;
}
#endif

#ifdef CONFIG_LIS2DW12_FREEFALL
static int lis2dw12_handle_ff_ia_int(const struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	sensor_trigger_handler_t handler = lis2dw12->freefall_handler;

	if (handler) {
		handler(dev, lis2dw12->freefall_trig);
	}

	return 0;
}
#endif /* CONFIG_LIS2DW12_FREEFALL */

/**
 * lis2dw12_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lis2dw12_handle_interrupt(const struct device *dev)
{
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dw12_all_sources_t sources;

	lis2dw12_all_sources_get(ctx, &sources);

	if (sources.status_dup.drdy) {
		lis2dw12_handle_drdy_int(dev);
	}
#ifdef CONFIG_LIS2DW12_TAP
	if (sources.status_dup.single_tap) {
		lis2dw12_handle_single_tap_int(dev);
	}
	if (sources.status_dup.double_tap) {
		lis2dw12_handle_double_tap_int(dev);
	}
#endif /* CONFIG_LIS2DW12_TAP */
#ifdef CONFIG_LIS2DW12_WAKEUP
	if (sources.all_int_src.wu_ia) {
		lis2dw12_handle_wu_ia_int(dev);
	}
#endif
#ifdef CONFIG_LIS2DW12_SLEEP
	if (sources.all_int_src.sleep_change_ia) {
		lis2dw12_handle_sleep_change_int(dev);
	}
#endif
#ifdef CONFIG_LIS2DW12_FREEFALL
	if (sources.all_int_src.ff_ia) {
		lis2dw12_handle_ff_ia_int(dev);
	}
#endif /* CONFIG_LIS2DW12_FREEFALL */

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void lis2dw12_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lis2dw12_data *lis2dw12 =
		CONTAINER_OF(cb, struct lis2dw12_data, gpio_cb);
	const struct lis2dw12_device_config *cfg = lis2dw12->dev->config;

	if ((pins & BIT(cfg->gpio_int.pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dw12->gpio_sem);
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dw12->work);
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LIS2DW12_TRIGGER_OWN_THREAD
static void lis2dw12_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct lis2dw12_data *lis2dw12 = p1;

	while (1) {
		k_sem_take(&lis2dw12->gpio_sem, K_FOREVER);
		lis2dw12_handle_interrupt(lis2dw12->dev);
	}
}
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD
static void lis2dw12_work_cb(struct k_work *work)
{
	struct lis2dw12_data *lis2dw12 =
		CONTAINER_OF(work, struct lis2dw12_data, work);

	lis2dw12_handle_interrupt(lis2dw12->dev);
}
#endif /* CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD */

#ifdef CONFIG_LIS2DW12_TAP
static int lis2dw12_tap_init(const struct device *dev)
{
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	LOG_DBG("TAP: tap mode is %d", cfg->tap_mode);
	if (lis2dw12_tap_mode_set(ctx, cfg->tap_mode) < 0) {
		LOG_ERR("Failed to select tap trigger mode");
		return -EIO;
	}

	LOG_DBG("TAP: ths_x is %02x", cfg->tap_threshold[0]);
	if (lis2dw12_tap_threshold_x_set(ctx, cfg->tap_threshold[0]) < 0) {
		LOG_ERR("Failed to set tap X axis threshold");
		return -EIO;
	}

	LOG_DBG("TAP: ths_y is %02x", cfg->tap_threshold[1]);
	if (lis2dw12_tap_threshold_y_set(ctx, cfg->tap_threshold[1]) < 0) {
		LOG_ERR("Failed to set tap Y axis threshold");
		return -EIO;
	}

	LOG_DBG("TAP: ths_z is %02x", cfg->tap_threshold[2]);
	if (lis2dw12_tap_threshold_z_set(ctx, cfg->tap_threshold[2]) < 0) {
		LOG_ERR("Failed to set tap Z axis threshold");
		return -EIO;
	}

	if (cfg->tap_threshold[0] > 0) {
		LOG_DBG("TAP: tap_x enabled");
		if (lis2dw12_tap_detection_on_x_set(ctx, 1) < 0) {
			LOG_ERR("Failed to set tap detection on X axis");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[1] > 0) {
		LOG_DBG("TAP: tap_y enabled");
		if (lis2dw12_tap_detection_on_y_set(ctx, 1) < 0) {
			LOG_ERR("Failed to set tap detection on Y axis");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[2] > 0) {
		LOG_DBG("TAP: tap_z enabled");
		if (lis2dw12_tap_detection_on_z_set(ctx, 1) < 0) {
			LOG_ERR("Failed to set tap detection on Z axis");
			return -EIO;
		}
	}

	LOG_DBG("TAP: shock is %02x", cfg->tap_shock);
	if (lis2dw12_tap_shock_set(ctx, cfg->tap_shock) < 0) {
		LOG_ERR("Failed to set tap shock duration");
		return -EIO;
	}

	LOG_DBG("TAP: latency is %02x", cfg->tap_latency);
	if (lis2dw12_tap_dur_set(ctx, cfg->tap_latency) < 0) {
		LOG_ERR("Failed to set tap latency");
		return -EIO;
	}

	LOG_DBG("TAP: quiet time is %02x", cfg->tap_quiet);
	if (lis2dw12_tap_quiet_set(ctx, cfg->tap_quiet) < 0) {
		LOG_ERR("Failed to set tap quiet time");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_LIS2DW12_TAP */

#ifdef CONFIG_LIS2DW12_FREEFALL
static int lis2dw12_ff_init(const struct device *dev)
{
	int rc;
	const struct lis2dw12_device_config *cfg = dev->config;
	struct lis2dw12_data *lis2dw12 = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint16_t duration;

	duration = (lis2dw12->odr * cfg->freefall_duration) / 1000;

	LOG_DBG("FREEFALL: duration is %d ms", cfg->freefall_duration);
	rc = lis2dw12_ff_dur_set(ctx, duration);
	if (rc != 0) {
		LOG_ERR("Failed to set freefall duration");
		return -EIO;
	}

	LOG_DBG("FREEFALL: threshold is %02x", cfg->freefall_threshold);
	rc = lis2dw12_ff_threshold_set(ctx, cfg->freefall_threshold);
	if (rc != 0) {
		LOG_ERR("Failed to set freefall thrshold");
		return -EIO;
	}
	return 0;
}
#endif /* CONFIG_LIS2DW12_FREEFALL */

int lis2dw12_init_interrupt(const struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		if (cfg->gpio_int.port) {
			LOG_ERR("%s: device %s is not ready", dev->name,
						cfg->gpio_int.port->name);
			return -ENODEV;
		}

		LOG_DBG("%s: gpio_int not defined in DT", dev->name);
		return 0;
	}

	lis2dw12->dev = dev;

	LOG_INF("%s: int-pin is on INT%d", dev->name, cfg->int_pin);
#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2dw12->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lis2dw12->thread, lis2dw12->thread_stack,
		       CONFIG_LIS2DW12_THREAD_STACK_SIZE,
		       lis2dw12_thread, lis2dw12,
		       NULL, NULL, K_PRIO_COOP(CONFIG_LIS2DW12_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	lis2dw12->work.handler = lis2dw12_work_cb;
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	LOG_INF("%s: int on %s.%02u", dev->name, cfg->gpio_int.port->name,
				      cfg->gpio_int.pin);

	gpio_init_callback(&lis2dw12->gpio_cb,
			   lis2dw12_gpio_callback,
			   BIT(cfg->gpio_int.pin));

	if (gpio_add_callback(cfg->gpio_int.port, &lis2dw12->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* set data ready mode on int1/int2 */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	lis2dw12_drdy_pulsed_t mode = cfg->drdy_pulsed ? LIS2DW12_DRDY_PULSED :
							 LIS2DW12_DRDY_LATCHED;

	ret = lis2dw12_data_ready_mode_set(ctx, mode);
	if (ret < 0) {
		LOG_ERR("drdy_pulsed config error %d", (int)cfg->drdy_pulsed);
		return ret;
	}

#ifdef CONFIG_LIS2DW12_TAP
	ret = lis2dw12_tap_init(dev);
	if (ret < 0) {
		return ret;
	}
#endif /* CONFIG_LIS2DW12_TAP */

#ifdef CONFIG_LIS2DW12_FREEFALL
	ret = lis2dw12_ff_init(dev);
		if (ret < 0) {
			return ret;
		}
#endif /* CONFIG_LIS2DW12_FREEFALL */

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
