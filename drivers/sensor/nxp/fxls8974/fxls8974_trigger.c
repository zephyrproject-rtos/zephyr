/*
 * Copyright (c) 2024 NXP Semiconductor
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fxls8974

#include "fxls8974.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(FXLS8974, CONFIG_SENSOR_LOG_LEVEL);

static void fxls8974_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb,
				   uint32_t pin_mask)
{
	struct fxls8974_data *data =
		CONTAINER_OF(cb, struct fxls8974_data, gpio_cb);
	const struct fxls8974_config *config = data->dev->config;

	if ((pin_mask & BIT(config->int_gpio.pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_FXLS8974_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_FXLS8974_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static int fxls8974_handle_drdy_int(const struct device *dev)
{
	struct fxls8974_data *data = dev->data;

	if (data->drdy_handler) {
		data->drdy_handler(dev, data->drdy_trig);
	}

	return 0;
}

#ifdef CONFIG_FXLS8974_PULSE
static int fxls8974_handle_pulse_int(const struct device *dev)
{
	const struct fxls8974_config *config = dev->config;
	struct fxls8974_data *data = dev->data;
	sensor_trigger_handler_t handler = NULL;
	const struct sensor_trigger *trig = NULL;
	uint8_t pulse_source;

	k_sem_take(&data->sem, K_FOREVER);

	if (config->ops->byte_read(dev, FXLS8974_REG_PULSE_SRC,
				   &pulse_source)) {
		LOG_ERR("Could not read pulse source");
	}

	k_sem_give(&data->sem);

	if (pulse_source & FXLS8974_PULSE_SRC_DPE) {
		handler = data->double_tap_handler;
		trig = data->double_tap_trig;
	} else {
		handler = data->tap_handler;
		trig = data->tap_trig;
	}

	if (handler) {
		handler(dev, trig);
	}

	return 0;
}
#endif

#ifdef CONFIG_FXLS8974_MOTION
static int fxls8974_handle_motion_int(const struct device *dev)
{
	const struct fxls8974_config *config = dev->config;
	struct fxls8974_data *data = dev->data;
	sensor_trigger_handler_t handler = data->motion_handler;
	uint8_t motion_source;

	k_sem_take(&data->sem, K_FOREVER);

	if (config->ops->byte_read(dev, FXLS8974_REG_FF_MT_SRC,
				   &motion_source)) {
		LOG_ERR("Could not read pulse source");
	}

	k_sem_give(&data->sem);

	if (handler) {
		LOG_DBG("FF_MT_SRC 0x%x", motion_source);
		handler(dev, data->motion_trig);
	}

	return 0;
}
#endif

static void fxls8974_handle_int(const struct device *dev)
{
	const struct fxls8974_config *config = dev->config;
	struct fxls8974_data *data = dev->data;
	uint8_t int_source;

	/* Interrupt status register */
	k_sem_take(&data->sem, K_FOREVER);

	if (config->ops->byte_read(dev, FXLS8974_REG_INT_SOURCE,
				   &int_source)) {
		LOG_ERR("Could not read interrupt source");
		int_source = 0U;
	}

	k_sem_give(&data->sem);

	if (int_source & FXLS8974_DRDY_MASK) {
		fxls8974_handle_drdy_int(dev);
	}
#ifdef CONFIG_FXLS8974_PULSE
	if (int_source & FXLS8974_PULSE_MASK) {
		fxls8974_handle_pulse_int(dev);
	}
#endif
#ifdef CONFIG_FXLS8974_MOTION
	if (int_source & FXLS8974_MOTION_MASK) {
		fxls8974_handle_motion_int(dev);
	}
#endif

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_FXLS8974_TRIGGER_OWN_THREAD
static void fxls8974_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct fxls8974_data *data = p1;

	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		fxls8974_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_FXLS8974_TRIGGER_GLOBAL_THREAD
static void fxls8974_work_handler(struct k_work *work)
{
	struct fxls8974_data *data =
		CONTAINER_OF(work, struct fxls8974_data, work);

	fxls8974_handle_int(data->dev);
}
#endif

int fxls8974_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct fxls8974_config *config = dev->config;
	struct fxls8974_data *data = dev->data;
	enum fxls8974_power power = FXLS8974_POWER_STANDBY;
	uint8_t mask;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		mask = FXLS8974_DRDY_MASK;
		data->drdy_handler = handler;
		data->drdy_trig = trig;
		break;
#ifdef CONFIG_FXLS8974_PULSE
	case SENSOR_TRIG_TAP:
		mask = FXLS8974_PULSE_MASK;
		data->tap_handler = handler;
		data->tap_trig = trig;
		break;
	case SENSOR_TRIG_DOUBLE_TAP:
		mask = FXLS8974_PULSE_MASK;
		data->double_tap_handler = handler;
		data->double_tap_trig = trig;
		break;
#endif
#ifdef CONFIG_FXLS8974_MOTION
	case SENSOR_TRIG_DELTA:
		mask = FXLS8974_MOTION_MASK;
		data->motion_handler = handler;
		data->motion_trig = trig;
		break;
#endif
	default:
		LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		goto exit;
	}

	/* The sensor must be in standby mode when writing the configuration
	 * registers, therefore get the current power mode so we can restore it
	 * later.
	 */
	if (fxls8974_get_power(dev, &power)) {
		LOG_ERR("Could not get power mode");
		ret = -EIO;
		goto exit;
	}

	/* Put the sensor in standby mode */
	if (fxls8974_set_power(dev, FXLS8974_POWER_STANDBY)) {
		LOG_ERR("Could not set standby mode");
		ret = -EIO;
		goto exit;
	}

	/* Configure the sensor interrupt */
	if (config->ops->reg_field_update(dev, FXLS8974_REG_CTRLREG4, mask,
					  handler ? mask : 0)) {
		LOG_ERR("Could not configure interrupt");
		ret = -EIO;
		goto exit;
	}

	/* Restore the previous power mode */
	if (fxls8974_set_power(dev, power)) {
		LOG_ERR("Could not restore power mode");
		ret = -EIO;
		goto exit;
	}

exit:
	k_sem_give(&data->sem);

	return ret;
}

#ifdef CONFIG_FXLS8974_PULSE
static int fxls8974_pulse_init(const struct device *dev)
{
	const struct fxls8974_config *config = dev->config;

	if (config->ops->byte_write(dev, FXLS8974_REG_PULSE_CFG,
				    config->pulse_cfg)) {
		return -EIO;
	}

	if (config->ops->byte_write(dev, FXLS8974_REG_PULSE_THSX,
				    config->pulse_ths[0])) {
		return -EIO;
	}

	if (config->ops->byte_write(dev, FXLS8974_REG_PULSE_THSY,
				    config->pulse_ths[1])) {
		return -EIO;
	}

	if (config->ops->byte_write(dev, FXLS8974_REG_PULSE_THSZ,
				    config->pulse_ths[2])) {
		return -EIO;
	}

	if (config->ops->byte_write(dev, FXLS8974_REG_PULSE_TMLT,
				    config->pulse_tmlt)) {
		return -EIO;
	}

	if (config->ops->byte_write(dev, FXLS8974_REG_PULSE_LTCY,
				    config->pulse_ltcy)) {
		return -EIO;
	}

	if (config->ops->byte_write(dev, FXLS8974_REG_PULSE_WIND,
				    config->pulse_wind)) {
		return -EIO;
	}

	return 0;
}
#endif


int fxls8974_trigger_init(const struct device *dev)
{
	const struct fxls8974_config *config = dev->config;
	struct fxls8974_data *data = dev->data;
	uint8_t ctrl_reg5;
	int ret;

	data->dev = dev;

#if defined(CONFIG_FXLS8974_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_FXLS8974_THREAD_STACK_SIZE,
			fxls8974_thread_main,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_FXLS8974_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_FXLS8974_TRIGGER_GLOBAL_THREAD)
	data->work.handler = fxls8974_work_handler;
#endif

	/* Route the interrupts to INT1/INT2 pins */
	ctrl_reg5 = 0U;
#if CONFIG_FXLS8974_DRDY_INT1
	ctrl_reg5 |= FXLS8974_DRDY_MASK;
#endif
#if CONFIG_FXLS8974_PULSE_INT1
	ctrl_reg5 |= FXLS8974_PULSE_MASK;
#endif
#if CONFIG_FXLS8974_MOTION_INT1
	ctrl_reg5 |= FXLS8974_MOTION_MASK;
#endif

	if (config->ops->byte_write(dev, FXLS8974_REG_CTRLREG5,
				    ctrl_reg5)) {
		LOG_ERR("Could not configure interrupt pin routing");
		return -EIO;
	}

#ifdef CONFIG_FXLS8974_PULSE
	if (fxls8974_pulse_init(dev)) {
		LOG_ERR("Could not configure pulse");
		return -EIO;
	}
#endif
#ifdef CONFIG_FXLS8974_MOTION
	if (fxls8974_motion_init(dev)) {
		LOG_ERR("Could not configure motion");
		return -EIO;
	}
#endif

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, fxls8974_gpio_callback,
			   BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
