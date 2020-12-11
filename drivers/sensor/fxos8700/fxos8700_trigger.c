/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fxos8700.h"
#include <logging/log.h>

LOG_MODULE_DECLARE(FXOS8700, CONFIG_SENSOR_LOG_LEVEL);

static void fxos8700_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb,
				   uint32_t pin_mask)
{
	struct fxos8700_data *data =
		CONTAINER_OF(cb, struct fxos8700_data, gpio_cb);

	if ((pin_mask & BIT(data->gpio_pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure(data->gpio, data->gpio_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_FXOS8700_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_FXOS8700_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static int fxos8700_handle_drdy_int(const struct device *dev)
{
	struct fxos8700_data *data = dev->data;

	struct sensor_trigger drdy_trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (data->drdy_handler) {
		data->drdy_handler(dev, &drdy_trig);
	}

	return 0;
}

#ifdef CONFIG_FXOS8700_PULSE
static int fxos8700_handle_pulse_int(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	sensor_trigger_handler_t handler = NULL;
	uint8_t pulse_source;

	struct sensor_trigger pulse_trig = {
		.chan = SENSOR_CHAN_ALL,
	};

	k_sem_take(&data->sem, K_FOREVER);

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_PULSE_SRC,
			      &pulse_source)) {
		LOG_ERR("Could not read pulse source");
	}

	k_sem_give(&data->sem);

	if (pulse_source & FXOS8700_PULSE_SRC_DPE) {
		pulse_trig.type = SENSOR_TRIG_DOUBLE_TAP;
		handler = data->double_tap_handler;
	} else {
		pulse_trig.type = SENSOR_TRIG_TAP;
		handler = data->tap_handler;
	}

	if (handler) {
		handler(dev, &pulse_trig);
	}

	return 0;
}
#endif

#ifdef CONFIG_FXOS8700_MOTION
static int fxos8700_handle_motion_int(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	sensor_trigger_handler_t handler = data->motion_handler;
	uint8_t motion_source;

	struct sensor_trigger motion_trig = {
		.chan = SENSOR_CHAN_ALL,
	};

	k_sem_take(&data->sem, K_FOREVER);

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_FF_MT_SRC,
			      &motion_source)) {
		LOG_ERR("Could not read pulse source");
	}

	k_sem_give(&data->sem);

	if (handler) {
		LOG_DBG("FF_MT_SRC 0x%x", motion_source);
		handler(dev, &motion_trig);
	}

	return 0;
}
#endif

#ifdef CONFIG_FXOS8700_MAG_VECM
static int fxos8700_handle_m_vecm_int(const struct device *dev)
{
	struct fxos8700_data *data = dev->data;

	struct sensor_trigger m_vecm_trig = {
		.type = FXOS8700_TRIG_M_VECM,
		.chan = SENSOR_CHAN_MAGN_XYZ,
	};

	if (data->m_vecm_handler) {
		data->m_vecm_handler(dev, &m_vecm_trig);
	}

	return 0;
}
#endif

static void fxos8700_handle_int(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	uint8_t int_source;

	/* Interrupt status register */
	k_sem_take(&data->sem, K_FOREVER);

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_INT_SOURCE,
			      &int_source)) {
		LOG_ERR("Could not read interrupt source");
		int_source = 0U;
	}

	k_sem_give(&data->sem);

	if (int_source & FXOS8700_DRDY_MASK) {
		fxos8700_handle_drdy_int(dev);
	}
#ifdef CONFIG_FXOS8700_PULSE
	if (int_source & FXOS8700_PULSE_MASK) {
		fxos8700_handle_pulse_int(dev);
	}
#endif
#ifdef CONFIG_FXOS8700_MOTION
	if (int_source & FXOS8700_MOTION_MASK) {
		fxos8700_handle_motion_int(dev);
	}
#endif
#ifdef CONFIG_FXOS8700_MAG_VECM
	/* Magnetometer interrupt source register */
	k_sem_take(&data->sem, K_FOREVER);

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_M_INT_SRC,
			      &int_source)) {
		LOG_ERR("Could not read magnetometer interrupt source");
		int_source = 0U;
	}

	k_sem_give(&data->sem);

	if (int_source & FXOS8700_VECM_MASK) {
		fxos8700_handle_m_vecm_int(dev);
	}
#endif

	gpio_pin_interrupt_configure(data->gpio, config->gpio_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_FXOS8700_TRIGGER_OWN_THREAD
static void fxos8700_thread_main(struct fxos8700_data *data)
{
	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		fxos8700_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_FXOS8700_TRIGGER_GLOBAL_THREAD
static void fxos8700_work_handler(struct k_work *work)
{
	struct fxos8700_data *data =
		CONTAINER_OF(work, struct fxos8700_data, work);

	fxos8700_handle_int(data->dev);
}
#endif

int fxos8700_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	enum fxos8700_power power = FXOS8700_POWER_STANDBY;
	uint8_t mask;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		mask = FXOS8700_DRDY_MASK;
		data->drdy_handler = handler;
		break;
#ifdef CONFIG_FXOS8700_PULSE
	case SENSOR_TRIG_TAP:
		mask = FXOS8700_PULSE_MASK;
		data->tap_handler = handler;
		break;
	case SENSOR_TRIG_DOUBLE_TAP:
		mask = FXOS8700_PULSE_MASK;
		data->double_tap_handler = handler;
		break;
#endif
#ifdef CONFIG_FXOS8700_MOTION
	case SENSOR_TRIG_DELTA:
		mask = FXOS8700_MOTION_MASK;
		data->motion_handler = handler;
		break;
#endif
#ifdef CONFIG_FXOS8700_MAG_VECM
	case FXOS8700_TRIG_M_VECM:
		mask = FXOS8700_VECM_MASK;
		data->m_vecm_handler = handler;
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
	if (fxos8700_get_power(dev, &power)) {
		LOG_ERR("Could not get power mode");
		ret = -EIO;
		goto exit;
	}

	/* Put the sensor in standby mode */
	if (fxos8700_set_power(dev, FXOS8700_POWER_STANDBY)) {
		LOG_ERR("Could not set standby mode");
		ret = -EIO;
		goto exit;
	}

	/* Configure the sensor interrupt */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_CTRLREG4,
				mask,
				handler ? mask : 0)) {
		LOG_ERR("Could not configure interrupt");
		ret = -EIO;
		goto exit;
	}

	/* Restore the previous power mode */
	if (fxos8700_set_power(dev, power)) {
		LOG_ERR("Could not restore power mode");
		ret = -EIO;
		goto exit;
	}

exit:
	k_sem_give(&data->sem);

	return ret;
}

#ifdef CONFIG_FXOS8700_PULSE
static int fxos8700_pulse_init(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_PULSE_CFG, config->pulse_cfg)) {
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_PULSE_THSX, config->pulse_ths[0])) {
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_PULSE_THSY, config->pulse_ths[1])) {
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_PULSE_THSZ, config->pulse_ths[2])) {
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_PULSE_TMLT, config->pulse_tmlt)) {
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_PULSE_LTCY, config->pulse_ltcy)) {
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_PULSE_WIND, config->pulse_wind)) {
		return -EIO;
	}

	return 0;
}
#endif

#ifdef CONFIG_FXOS8700_MOTION
static int fxos8700_motion_init(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;

	/* Set Mode 4, Motion detection with ELE = 1, OAE = 1 */
	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_FF_MT_CFG,
			       FXOS8700_FF_MT_CFG_ELE |
			       FXOS8700_FF_MT_CFG_OAE |
			       FXOS8700_FF_MT_CFG_ZEFE |
			       FXOS8700_FF_MT_CFG_YEFE |
			       FXOS8700_FF_MT_CFG_XEFE)) {
		return -EIO;
	}

	/* Set motion threshold to maximimum */
	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_FF_MT_THS,
			       FXOS8700_REG_FF_MT_THS)) {
		return -EIO;
	}

	return 0;
}
#endif

#ifdef CONFIG_FXOS8700_MAG_VECM
static int fxos8700_m_vecm_init(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	uint8_t m_vecm_cfg = config->mag_vecm_cfg;

	/* Route the interrupt to INT1 pin */
#if CONFIG_FXOS8700_MAG_VECM_INT1
	m_vecm_cfg |= FXOS8700_MAG_VECM_INT1_MASK;
#endif

	/* Set magnetic vector-magnitude function */
	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_M_VECM_CFG, m_vecm_cfg)) {
		LOG_ERR("Could not set magnetic vector-magnitude function");
		return -EIO;
	}

	/* Set magnetic vector-magnitude function threshold values:
	 * handle both MSB and LSB registers
	 */
	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_M_VECM_THS_MSB, config->mag_vecm_ths[0])) {
		LOG_ERR("Could not set magnetic vector-magnitude function threshold MSB value");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_M_VECM_THS_LSB, config->mag_vecm_ths[1])) {
		LOG_ERR("Could not set magnetic vector-magnitude function threshold LSB value");
		return -EIO;
	}

	return 0;
}
#endif

int fxos8700_trigger_init(const struct device *dev)
{
	const struct fxos8700_config *config = dev->config;
	struct fxos8700_data *data = dev->data;
	uint8_t ctrl_reg5;

	data->dev = dev;

#if defined(CONFIG_FXOS8700_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, UINT_MAX);
	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_FXOS8700_THREAD_STACK_SIZE,
			(k_thread_entry_t)fxos8700_thread_main,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_FXOS8700_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_FXOS8700_TRIGGER_GLOBAL_THREAD)
	data->work.handler = fxos8700_work_handler;
#endif

	/* Route the interrupts to INT1/INT2 pins */
	ctrl_reg5 = 0U;
#if CONFIG_FXOS8700_DRDY_INT1
	ctrl_reg5 |= FXOS8700_DRDY_MASK;
#endif
#if CONFIG_FXOS8700_PULSE_INT1
	ctrl_reg5 |= FXOS8700_PULSE_MASK;
#endif
#if CONFIG_FXOS8700_MOTION_INT1
	ctrl_reg5 |= FXOS8700_MOTION_MASK;
#endif

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_CTRLREG5, ctrl_reg5)) {
		LOG_ERR("Could not configure interrupt pin routing");
		return -EIO;
	}

#ifdef CONFIG_FXOS8700_PULSE
	if (fxos8700_pulse_init(dev)) {
		LOG_ERR("Could not configure pulse");
		return -EIO;
	}
#endif
#ifdef CONFIG_FXOS8700_MOTION
	if (fxos8700_motion_init(dev)) {
		LOG_ERR("Could not configure motion");
		return -EIO;
	}
#endif
#ifdef CONFIG_FXOS8700_MAG_VECM
	if (fxos8700_m_vecm_init(dev)) {
		LOG_ERR("Could not configure magnetic vector-magnitude");
		return -EIO;
	}
#endif

	/* Get the GPIO device */
	data->gpio = device_get_binding(config->gpio_name);
	if (data->gpio == NULL) {
		LOG_ERR("Could not find GPIO device");
		return -EINVAL;
	}

	data->gpio_pin = config->gpio_pin;

	gpio_pin_configure(data->gpio, config->gpio_pin,
			   GPIO_INPUT | config->gpio_flags);

	gpio_init_callback(&data->gpio_cb, fxos8700_gpio_callback,
			   BIT(config->gpio_pin));

	gpio_add_callback(data->gpio, &data->gpio_cb);

	gpio_pin_interrupt_configure(data->gpio, config->gpio_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
