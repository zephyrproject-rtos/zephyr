/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl345

#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "adxl345.h"

LOG_MODULE_REGISTER(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
static bool adxl345_bus_is_ready_i2c(const union adxl345_bus *bus)
{
	return device_is_ready(bus->i2c.bus);
}

static int adxl345_reg_access_i2c(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
				  uint8_t *data, size_t length)
{
	const struct adxl345_dev_config *cfg = dev->config;

	if (cmd == ADXL345_READ_CMD) {
		return i2c_burst_read_dt(&cfg->bus.i2c, reg_addr, data, length);
	} else {
		return i2c_burst_write_dt(&cfg->bus.i2c, reg_addr, data, length);
	}
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
static bool adxl345_bus_is_ready_spi(const union adxl345_bus *bus)
{
	return spi_is_ready_dt(&bus->spi);
}

static int adxl345_reg_access_spi(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
				  uint8_t *data, size_t length)
{
	const struct adxl345_dev_config *cfg = dev->config;
	uint8_t access = reg_addr | cmd | (length == 1 ? 0 : ADXL345_MULTIBYTE_FLAG);
	const struct spi_buf buf[2] = {{.buf = &access, .len = 1}, {.buf = data, .len = length}};
	const struct spi_buf_set rx = {.buffers = buf, .count = ARRAY_SIZE(buf)};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	if (cmd == ADXL345_READ_CMD) {
		tx.count = 1;
		return spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	} else {
		return spi_write_dt(&cfg->bus.spi, &tx);
	}
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

static inline int adxl345_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr,
				     uint8_t *data, size_t len)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, cmd, addr, data, len);
}

static inline int adxl345_reg_write(const struct device *dev, uint8_t addr, uint8_t *data,
				    uint8_t len)
{

	return adxl345_reg_access(dev, ADXL345_WRITE_CMD, addr, data, len);
}

static inline int adxl345_reg_read(const struct device *dev, uint8_t addr, uint8_t *data,
				   uint8_t len)
{

	return adxl345_reg_access(dev, ADXL345_READ_CMD, addr, data, len);
}

static inline int adxl345_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val)
{
	return adxl345_reg_write(dev, addr, &val, 1);
}

static inline int adxl345_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf)
{
	return adxl345_reg_read(dev, addr, buf, 1);
}

static inline bool adxl345_bus_is_ready(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

static int adxl345_read_sample(const struct device *dev, struct adxl345_sample *sample)
{
	int16_t raw_x, raw_y, raw_z;
	uint8_t axis_data[6];

	int rc = adxl345_reg_read(dev, ADXL345_X_AXIS_DATA_0_REG, axis_data, 6);

	if (rc < 0) {
		LOG_ERR("Samples read failed with rc=%d\n", rc);
		return rc;
	}

	raw_x = axis_data[0] | (axis_data[1] << 8);
	raw_y = axis_data[2] | (axis_data[3] << 8);
	raw_z = axis_data[4] | (axis_data[5] << 8);

	sample->x = raw_x;
	sample->y = raw_y;
	sample->z = raw_z;

	return 0;
}

static void adxl345_accel_convert(struct sensor_value *val, int16_t sample)
{
	if (sample & BIT(9)) {
		sample |= ADXL345_COMPLEMENT;
	}

	val->val1 = ((sample * SENSOR_G) / 32) / 1000000;
	val->val2 = ((sample * SENSOR_G) / 32) % 1000000;
}

static int adxl345_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adxl345_dev_data *data = dev->data;
	struct adxl345_sample sample;
	uint8_t samples_count;
	int rc;

	data->sample_number = 0;
	rc = adxl345_reg_read_byte(dev, ADXL345_FIFO_STATUS_REG, &samples_count);
	if (rc < 0) {
		LOG_ERR("Failed to read FIFO status rc = %d\n", rc);
		return rc;
	}

	__ASSERT_NO_MSG(samples_count <= ARRAY_SIZE(data->bufx));

	for (uint8_t s = 0; s < samples_count; s++) {
		rc = adxl345_read_sample(dev, &sample);
		if (rc < 0) {
			LOG_ERR("Failed to fetch sample rc=%d\n", rc);
			return rc;
		}
		data->bufx[s] = sample.x;
		data->bufy[s] = sample.y;
		data->bufz[s] = sample.z;
	}

	return samples_count;
}

static int adxl345_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl345_dev_data *data = dev->data;

	if (data->sample_number >= ARRAY_SIZE(data->bufx)) {
		data->sample_number = 0;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl345_accel_convert(val, data->bufx[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl345_accel_convert(val, data->bufy[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl345_accel_convert(val, data->bufz[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl345_accel_convert(val++, data->bufx[data->sample_number]);
		adxl345_accel_convert(val++, data->bufy[data->sample_number]);
		adxl345_accel_convert(val, data->bufz[data->sample_number]);
		data->sample_number++;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int adxl345_attr_set_thresh(const struct device *dev, enum sensor_channel chan,
				   enum sensor_attribute attr, const struct sensor_value *val)
{
	uint8_t reg;
	uint16_t threshold = val->val1;
	size_t rc;

	if (chan != SENSOR_CHAN_ACCEL_X && chan != SENSOR_CHAN_ACCEL_Y &&
	    chan != SENSOR_CHAN_ACCEL_Z) {
		return -EINVAL;
	}

	if (threshold > 2047) {
		return -EINVAL;
	}

	/* Configure motion threshold. */
	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		reg = ADXL345_REG_THRESH_ACT;
	} else {
		reg = ADXL345_REG_THRESH_INACT;
	}

	threshold &= 0x7FF;

	rc = adxl345_reg_write(dev, reg, (uint8_t *)&threshold, 2);

	return rc;
}

static int adxl345_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_LOWER_THRESH:
		return adxl345_attr_set_thresh(dev, chan, attr, val);
	default:
		LOG_DBG("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
}

#if defined(CONFIG_ADXL345_TRIGGER)

static int adxl345_reg_write_mask(const struct device *dev, uint8_t register_address, uint8_t mask,
				  uint8_t data)
{
	int rc;
	uint8_t tmp;

	rc = adxl345_reg_read(dev, register_address, &tmp, 1);

	if (rc) {
		return rc;
	}

	tmp &= ~mask;
	tmp |= data;

	return adxl345_reg_write(dev, register_address, &tmp, 1);
}

static int adxl345_interrupt_config(const struct device *dev, uint8_t int_map, uint8_t int_enable)
{
	int rc;

	rc = adxl345_reg_write_byte(dev, ADXL345_REG_INTMAP, int_map);
	if (rc) {
		return rc;
	}
	return adxl345_reg_write_byte(dev, ADXL345_REG_INTENABLE, int_enable);
}

static int adxl345_get_int_source(const struct device *dev, uint8_t *status)
{
	return adxl345_reg_read_byte(dev, ADXL345_REG_INTSOURCE, status);
}

static int adxl345_clear_data_ready(const struct device *dev)
{
	uint8_t buf[6];
	/* Reading any data register clears the data ready interrupt */
	return adxl345_reg_read(dev, ADXL345_X_AXIS_DATA_0_REG, buf, 6);
}

static void adxl345_thread_cb(const struct device *dev)
{
	struct adxl345_dev_data *drv_data = dev->data;
	const struct adxl345_dev_config *config = dev->config;
	int ret;
	uint8_t status_buf;

	/* Clears activity and inactivity interrupt */
	if (adxl345_get_int_source(dev, &status_buf)) {
		LOG_ERR("Unable to get status.");
		return;
	}

	k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
	if (drv_data->inact_handler != NULL) {
		if (ADXL345_STATUS_CHECK_INACT(status_buf)) {
			drv_data->inact_handler(dev, drv_data->inact_trigger);
		}
	}

	if (drv_data->act_handler != NULL) {
		if (ADXL345_STATUS_CHECK_ACTIVITY(status_buf)) {
			drv_data->act_handler(dev, drv_data->act_trigger);
		}
	}

	if (drv_data->drdy_handler != NULL && ADXL345_STATUS_CHECK_DATA_READY(status_buf)) {
		drv_data->drdy_handler(dev, drv_data->drdy_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	__ASSERT(ret == 0, "Interrupt configuration failed");

	k_mutex_unlock(&drv_data->trigger_mutex);
}

static void adxl345_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct adxl345_dev_data *drv_data = CONTAINER_OF(cb, struct adxl345_dev_data, gpio_cb);
	const struct adxl345_dev_config *config = drv_data->dev->config;

	gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_DISABLE);

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
static void adxl345_thread(struct adxl345_dev_data *drv_data)
{
	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adxl345_thread_cb(drv_data->dev);
	}
}
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
static void adxl345_work_cb(struct k_work *work)
{
	struct adxl345_dev_data *drv_data = CONTAINER_OF(work, struct adxl345_dev_data, work);

	adxl345_thread_cb(drv_data->dev);
}
#endif

int adxl345_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adxl345_dev_data *drv_data = dev->data;
	const struct adxl345_dev_config *config = dev->config;
	uint8_t int_mask, int_en, status_buf;
	int rc;

	if (!config->interrupt.port) {
		return -ENOTSUP;
	}

	rc = gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_DISABLE);
	if (rc < 0) {
		return -EIO;
	}

	switch (trig->type) {
	case SENSOR_TRIG_MOTION:
		k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
		drv_data->act_handler = handler;
		drv_data->act_trigger = trig;
		k_mutex_unlock(&drv_data->trigger_mutex);
		int_mask = ADXL345_INTMAP_ACT;
		/* Clear activity and inactivity interrupts */
		adxl345_get_int_source(dev, &status_buf);
		break;
	case SENSOR_TRIG_STATIONARY:
		k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
		drv_data->inact_handler = handler;
		drv_data->inact_trigger = trig;
		k_mutex_unlock(&drv_data->trigger_mutex);
		int_mask = ADXL345_INTMAP_INACT;
		/* Clear activity and inactivity interrupts */
		adxl345_get_int_source(dev, &status_buf);
		break;
	case SENSOR_TRIG_DATA_READY:
		k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = trig;
		k_mutex_unlock(&drv_data->trigger_mutex);
		int_mask = ADXL345_INTMAP_DATA_READY;
		adxl345_clear_data_ready(dev);
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (handler) {
		int_en = int_mask;
	} else {
		int_en = 0U;
	}

	/* TODO int1 and int2 choice */

	rc = adxl345_reg_write_mask(dev, ADXL345_REG_INTMAP, int_mask, int_en);
	if (rc < 0) {
		return rc;
	}
	rc = adxl345_reg_write_mask(dev, ADXL345_REG_INTENABLE, int_mask, int_en);
	if (rc < 0) {
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		return -EIO;
	}

	return 0;
}

static uint8_t adxl345_convert_threshold(int threshold)
{
	unsigned int res = ((float)threshold) / 62.5;

	return MIN(255, res);
}

static int adxl345_convert_axes(char *axes_str, uint8_t *axes)
{
	*axes = 0;

	for (; *axes_str != '\0'; axes_str++) {
		switch (*axes_str) {
		case 'x':
			*axes |= 4;
			break;
		case 'y':
			*axes |= 2;
			break;
		case 'z':
			*axes |= 1;
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

static int adxl345_set_activity_and_inactivity(const struct device *dev)
{
	int rc;
	uint8_t reg = 0;
	uint8_t axes = 0;

	rc = adxl345_reg_write_byte(dev, ADXL345_REG_THRESH_ACT,
				    adxl345_convert_threshold(CONFIG_ADXL345_ACTIVITY_THRESHOLD));
	if (rc < 0) {
		return rc;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_REG_THRESH_INACT,
				    adxl345_convert_threshold(CONFIG_ADXL345_INACTIVITY_THRESHOLD));
	if (rc < 0) {
		return rc;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_REG_TIME_INACT,
				    CONFIG_ADXL345_INACTIVITY_TIME & 0xff);
	if (rc < 0) {
		return rc;
	}

	if (CONFIG_ADXL345_ABS_REF_ACTIVITY) {
		reg |= BIT(7);
	}

	if (CONFIG_ADXL345_ABS_REF_INACTIVITY) {
		reg |= BIT(3);
	}

	rc = adxl345_convert_axes(CONFIG_ADXL345_INACTIVITY_AXES, &axes);
	if (rc < 0) {
		return rc;
	}
	reg |= axes;
	rc = adxl345_convert_axes(CONFIG_ADXL345_ACTIVITY_AXES, &axes);
	if (rc < 0) {
		return rc;
	}
	reg |= axes << 4;

	return adxl345_reg_write_byte(dev, ADXL345_REG_ACT_INACT_CTL, reg);
}

static int adxl345_init_interrupt(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;
	struct adxl345_dev_data *drv_data = dev->data;
	int rc;

	k_mutex_init(&drv_data->trigger_mutex);

	if (!device_is_ready(cfg->interrupt.port)) {
		LOG_ERR("GPIO port %s not ready", cfg->interrupt.port->name);
		return -ENODEV;
	}

	rc = adxl345_set_activity_and_inactivity(dev);
	if (rc < 0) {
		return rc;
	}

	rc = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (rc < 0) {
		return rc;
	}

	gpio_init_callback(&drv_data->gpio_cb, adxl345_gpio_callback, BIT(cfg->interrupt.pin));

	rc = gpio_add_callback(cfg->interrupt.port, &drv_data->gpio_cb);
	if (rc < 0) {
		return rc;
	}

	drv_data->dev = dev;

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack, CONFIG_ADXL345_THREAD_STACK_SIZE,
			(k_thread_entry_t)adxl345_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADXL345_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adxl345_work_cb;
#endif

	rc = gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

#endif

static int adxl345_set_powerctl(const struct device *dev)
{
	uint8_t reg = 0;

	if (CONFIG_ADXL345_AUTO_SLEEP) {
		reg |= ADXL345_ENABLE_AUTO_SLEEP_BIT;
	}

	if (CONFIG_ADXL345_LINK_MODE) {
		reg |= ADXL345_ENABLE_LINK_BIT;
	}

	reg |= ADXL345_ENABLE_MEASURE_BIT;

	if (CONFIG_ADXL345_SLEEP) {
		reg |= ADXL345_ENABLE_SLEEP_BIT;
	}

	reg |= (CONFIG_ADXL345_WAKEUP_POLL_FREQ & 0x3);

	return adxl345_reg_write_byte(dev, ADXL345_POWER_CTL_REG, reg);
}

static const struct sensor_driver_api adxl345_api_funcs = {
	.attr_set = adxl345_attr_set,
	.sample_fetch = adxl345_sample_fetch,
	.channel_get = adxl345_channel_get,
#ifdef CONFIG_ADXL345_TRIGGER
	.trigger_set = adxl345_trigger_set,
#endif
};

static int adxl345_init(const struct device *dev)
{
	int rc;
	struct adxl345_dev_data *data = dev->data;
	const struct adxl345_dev_config *config = dev->config;
	uint8_t dev_id;

	data->sample_number = 0;

	if (!adxl345_bus_is_ready(dev)) {
		LOG_ERR("bus not ready");
		return -ENODEV;
	}

	rc = adxl345_reg_read_byte(dev, ADXL345_DEVICE_ID_REG, &dev_id);
	if (rc < 0 || dev_id != ADXL345_PART_ID) {
		LOG_ERR("Read PART ID failed: 0x%x %x\n", rc, dev_id);
		return -ENODEV;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_FIFO_CTL_REG, ADXL345_FIFO_STREAM_MODE);
	if (rc < 0) {
		LOG_ERR("FIFO enable failed\n");
		return -EIO;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_DATA_FORMAT_REG, ADXL345_RANGE_16G);
	if (rc < 0) {
		LOG_ERR("Data format set failed\n");
		return -EIO;
	}

	rc = adxl345_reg_write_byte(dev, ADXL345_RATE_REG, CONFIG_ADXL345_ODR & 0xf);
	if (rc < 0) {
		LOG_ERR("Rate setting failed\n");
		return -EIO;
	}

	rc = adxl345_set_powerctl(dev);
	if (rc < 0) {
		LOG_ERR("Setting powerctl failed\n");
		return -EIO;
	}

#if defined(CONFIG_ADXL345_TRIGGER)
	if (config->interrupt.port) {
		if (adxl345_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupt!");
			return -EIO;
		}

		if (adxl345_interrupt_config(dev, config->int_map, config->int_map) < 0) {
			LOG_ERR("Failed to configure interrupt");
			return -EIO;
		}
	}
#else
	(void)config;
#endif
	return 0;
}

#define ADXL345_CONFIG_INTERRUPT_PART(inst)                                                        \
	IF_ENABLED(CONFIG_ADXL345_TRIGGER,                                                         \
		   (.interrupt = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}), .int_map = 0))

#define ADXL345_CONFIG_SPI(inst)                                                                   \
	{                                                                                          \
		.bus = {.spi = SPI_DT_SPEC_INST_GET(inst,                                          \
						    SPI_WORD_SET(8) | SPI_TRANSFER_MSB |           \
							    SPI_MODE_CPOL | SPI_MODE_CPHA,         \
						    0)},                                           \
		.bus_is_ready = adxl345_bus_is_ready_spi, .reg_access = adxl345_reg_access_spi,    \
		ADXL345_CONFIG_INTERRUPT_PART(inst)                                                \
	}

#define ADXL345_CONFIG_I2C(inst)                                                                   \
	{                                                                                          \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)},                                        \
		.bus_is_ready = adxl345_bus_is_ready_i2c, .reg_access = adxl345_reg_access_i2c,    \
		ADXL345_CONFIG_INTERRUPT_PART(inst)                                                \
	}

#define ADXL345_DEFINE(inst)                                                                       \
	static struct adxl345_dev_data adxl345_data_##inst;                                        \
                                                                                                   \
	static const struct adxl345_dev_config adxl345_config_##inst =                             \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (ADXL345_CONFIG_SPI(inst)),                 \
			    (ADXL345_CONFIG_I2C(inst)));                                           \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl345_init, NULL, &adxl345_data_##inst,               \
				     &adxl345_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &adxl345_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(ADXL345_DEFINE)
