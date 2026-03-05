/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT qst_qmi8658a

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(QMI8658A, CONFIG_SENSOR_LOG_LEVEL);

#define QMI8658A_REG_WHO_AM_I 0x00
#define QMI8658A_REG_CTRL1    0x02
#define QMI8658A_REG_CTRL2    0x03
#define QMI8658A_REG_CTRL3    0x04
#define QMI8658A_REG_CTRL7    0x08
#define QMI8658A_REG_TEMP_L   0x33
#define QMI8658A_REG_RESET    0x60

#define QMI8658A_CHIP_ID   0x05
#define QMI8658A_RESET_CMD 0xB0

#define QMI8658A_CTRL1_ADDR_AI    BIT(6)
#define QMI8658A_CTRL1_INT2_EN    BIT(4)
#define QMI8658A_CTRL1_INT1_EN    BIT(3)
#define QMI8658A_CTRL7_GYR_ENABLE BIT(1)
#define QMI8658A_CTRL7_ACC_ENABLE BIT(0)

#define QMI8658A_RESET_DELAY_MS 15
#define QMI8658A_SAMPLE_BYTES   14

struct qmi8658a_odr_map {
	uint16_t hz;
	uint8_t reg_val;
};

/*
 * Effective ODR in 6-DoF mode from datasheet Table 3.7.
 * CTRL2/CTRL3 use the same encoding for normal ODR values.
 */
static const struct qmi8658a_odr_map qmi8658a_odr_map[] = {
	{7174, 0x0}, {3587, 0x1}, {1793, 0x2}, {896, 0x3}, {448, 0x4},
	{224, 0x5},  {112, 0x6},  {56, 0x7},   {28, 0x8},
};

struct qmi8658a_config {
	struct i2c_dt_spec i2c;
	uint16_t accel_fs;
	uint16_t gyro_fs;
	uint16_t accel_odr;
	uint16_t gyro_odr;
#ifdef CONFIG_QMI8658A_TRIGGER
	struct gpio_dt_spec int_gpio;
	uint8_t int_pin;
#endif
};

struct qmi8658a_data {
	int16_t temp_raw;
	int16_t accel_x_raw;
	int16_t accel_y_raw;
	int16_t accel_z_raw;
	int16_t gyro_x_raw;
	int16_t gyro_y_raw;
	int16_t gyro_z_raw;

	uint16_t accel_fs;
	uint16_t gyro_fs;
	uint16_t accel_odr;
	uint16_t gyro_odr;

#ifdef CONFIG_QMI8658A_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;

#if defined(CONFIG_QMI8658A_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_QMI8658A_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_QMI8658A_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif
};

static bool qmi8658a_is_accel_channel(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return true;
	default:
		return false;
	}
}

static bool qmi8658a_is_gyro_channel(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		return true;
	default:
		return false;
	}
}

static int qmi8658a_select_odr(uint16_t requested_hz, uint16_t *selected_hz, uint8_t *reg_val)
{
	uint32_t min_delta = UINT32_MAX;
	size_t min_index = 0;

	if (requested_hz == 0U) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(qmi8658a_odr_map); i++) {
		uint16_t candidate_hz = qmi8658a_odr_map[i].hz;
		uint32_t delta = (candidate_hz > requested_hz) ? (candidate_hz - requested_hz)
							       : (requested_hz - candidate_hz);

		if (delta < min_delta) {
			min_delta = delta;
			min_index = i;
		}
	}

	*selected_hz = qmi8658a_odr_map[min_index].hz;
	*reg_val = qmi8658a_odr_map[min_index].reg_val;

	return 0;
}

static int qmi8658a_accel_fs_to_reg(uint16_t accel_fs, uint8_t *reg_val)
{
	switch (accel_fs) {
	case 2:
		*reg_val = 0;
		return 0;
	case 4:
		*reg_val = 1;
		return 0;
	case 8:
		*reg_val = 2;
		return 0;
	case 16:
		*reg_val = 3;
		return 0;
	default:
		return -EINVAL;
	}
}

static int qmi8658a_gyro_fs_to_reg(uint16_t gyro_fs, uint8_t *reg_val)
{
	switch (gyro_fs) {
	case 16:
		*reg_val = 0;
		return 0;
	case 32:
		*reg_val = 1;
		return 0;
	case 64:
		*reg_val = 2;
		return 0;
	case 128:
		*reg_val = 3;
		return 0;
	case 256:
		*reg_val = 4;
		return 0;
	case 512:
		*reg_val = 5;
		return 0;
	case 1024:
		*reg_val = 6;
		return 0;
	case 2048:
		*reg_val = 7;
		return 0;
	default:
		return -EINVAL;
	}
}

static void qmi8658a_convert_accel(struct sensor_value *val, int16_t raw, uint16_t fs_g)
{
	int64_t micro_ms2 = ((int64_t)raw * SENSOR_G * fs_g) / 32768LL;

	(void)sensor_value_from_micro(val, micro_ms2);
}

static void qmi8658a_convert_gyro(struct sensor_value *val, int16_t raw, uint16_t fs_dps)
{
	int64_t micro_rads = ((int64_t)raw * fs_dps * SENSOR_PI) / (32768LL * 180LL);

	(void)sensor_value_from_micro(val, micro_rads);
}

static void qmi8658a_convert_temp(struct sensor_value *val, int16_t raw)
{
	int64_t micro_celsius = ((int64_t)raw * 1000000LL) / 256LL;

	(void)sensor_value_from_micro(val, micro_celsius);
}

static int qmi8658a_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct qmi8658a_config *cfg = dev->config;
	struct qmi8658a_data *data = dev->data;
	uint8_t sample[QMI8658A_SAMPLE_BYTES];
	int rc;

	if ((chan != SENSOR_CHAN_ALL) && !qmi8658a_is_accel_channel(chan) &&
	    !qmi8658a_is_gyro_channel(chan) && (chan != SENSOR_CHAN_DIE_TEMP)) {
		return -ENOTSUP;
	}

	rc = i2c_burst_read_dt(&cfg->i2c, QMI8658A_REG_TEMP_L, sample, sizeof(sample));
	if (rc < 0) {
		LOG_ERR("Failed to read sample data (%d)", rc);
		return rc;
	}

	data->temp_raw = (int16_t)sys_get_le16(&sample[0]);
	data->accel_x_raw = (int16_t)sys_get_le16(&sample[2]);
	data->accel_y_raw = (int16_t)sys_get_le16(&sample[4]);
	data->accel_z_raw = (int16_t)sys_get_le16(&sample[6]);
	data->gyro_x_raw = (int16_t)sys_get_le16(&sample[8]);
	data->gyro_y_raw = (int16_t)sys_get_le16(&sample[10]);
	data->gyro_z_raw = (int16_t)sys_get_le16(&sample[12]);

	return 0;
}

static int qmi8658a_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct qmi8658a_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		qmi8658a_convert_accel(val, data->accel_x_raw, data->accel_fs);
		return 0;
	case SENSOR_CHAN_ACCEL_Y:
		qmi8658a_convert_accel(val, data->accel_y_raw, data->accel_fs);
		return 0;
	case SENSOR_CHAN_ACCEL_Z:
		qmi8658a_convert_accel(val, data->accel_z_raw, data->accel_fs);
		return 0;
	case SENSOR_CHAN_ACCEL_XYZ:
		qmi8658a_convert_accel(&val[0], data->accel_x_raw, data->accel_fs);
		qmi8658a_convert_accel(&val[1], data->accel_y_raw, data->accel_fs);
		qmi8658a_convert_accel(&val[2], data->accel_z_raw, data->accel_fs);
		return 0;
	case SENSOR_CHAN_GYRO_X:
		qmi8658a_convert_gyro(val, data->gyro_x_raw, data->gyro_fs);
		return 0;
	case SENSOR_CHAN_GYRO_Y:
		qmi8658a_convert_gyro(val, data->gyro_y_raw, data->gyro_fs);
		return 0;
	case SENSOR_CHAN_GYRO_Z:
		qmi8658a_convert_gyro(val, data->gyro_z_raw, data->gyro_fs);
		return 0;
	case SENSOR_CHAN_GYRO_XYZ:
		qmi8658a_convert_gyro(&val[0], data->gyro_x_raw, data->gyro_fs);
		qmi8658a_convert_gyro(&val[1], data->gyro_y_raw, data->gyro_fs);
		qmi8658a_convert_gyro(&val[2], data->gyro_z_raw, data->gyro_fs);
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		qmi8658a_convert_temp(val, data->temp_raw);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qmi8658a_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct qmi8658a_data *data = dev->data;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (qmi8658a_is_accel_channel(chan)) {
			val->val1 = data->accel_odr;
			val->val2 = 0;
			return 0;
		}

		if (qmi8658a_is_gyro_channel(chan)) {
			val->val1 = data->gyro_odr;
			val->val2 = 0;
			return 0;
		}
		break;
	case SENSOR_ATTR_FULL_SCALE:
		if (qmi8658a_is_accel_channel(chan)) {
			sensor_g_to_ms2(data->accel_fs, val);
			return 0;
		}

		if (qmi8658a_is_gyro_channel(chan)) {
			sensor_degrees_to_rad(data->gyro_fs, val);
			return 0;
		}
		break;
	default:
		break;
	}

	return -ENOTSUP;
}

#ifdef CONFIG_QMI8658A_TRIGGER
static int qmi8658a_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	struct qmi8658a_data *data = dev->data;
	const struct qmi8658a_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if ((trig->chan != SENSOR_CHAN_ALL) && (trig->chan != SENSOR_CHAN_ACCEL_XYZ) &&
	    (trig->chan != SENSOR_CHAN_GYRO_XYZ)) {
		return -ENOTSUP;
	}

	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

	data->drdy_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	data->drdy_trigger = trig;

	return gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static void qmi8658a_thread_cb(const struct device *dev)
{
	struct qmi8658a_data *data = dev->data;
	const struct qmi8658a_config *cfg = dev->config;

	if (data->drdy_handler != NULL) {
		data->drdy_handler(dev, data->drdy_trigger);
	}

	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static void qmi8658a_gpio_callback(const struct device *port, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct qmi8658a_data *data = CONTAINER_OF(cb, struct qmi8658a_data, gpio_cb);
	const struct qmi8658a_config *cfg = data->dev->config;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_QMI8658A_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_QMI8658A_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#ifdef CONFIG_QMI8658A_TRIGGER_OWN_THREAD
static void qmi8658a_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct qmi8658a_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		qmi8658a_thread_cb(data->dev);
	}
}
#endif

#ifdef CONFIG_QMI8658A_TRIGGER_GLOBAL_THREAD
static void qmi8658a_work_cb(struct k_work *work)
{
	struct qmi8658a_data *data = CONTAINER_OF(work, struct qmi8658a_data, work);

	qmi8658a_thread_cb(data->dev);
}
#endif

static int qmi8658a_init_interrupt(const struct device *dev)
{
	struct qmi8658a_data *data = dev->data;
	const struct qmi8658a_config *cfg = dev->config;
	int rc;

	if (!cfg->int_gpio.port) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("Interrupt GPIO device is not ready");
		return -ENODEV;
	}

	data->dev = dev;

	rc = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Failed to configure interrupt GPIO (%d)", rc);
		return rc;
	}

	gpio_init_callback(&data->gpio_cb, qmi8658a_gpio_callback, BIT(cfg->int_gpio.pin));

	rc = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	if (rc < 0) {
		LOG_ERR("Failed to add interrupt callback (%d)", rc);
		return rc;
	}

#if defined(CONFIG_QMI8658A_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_QMI8658A_THREAD_STACK_SIZE,
			qmi8658a_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_QMI8658A_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, "qmi8658a_trigger");
#elif defined(CONFIG_QMI8658A_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, qmi8658a_work_cb);
#endif

	return 0;
}
#endif /* CONFIG_QMI8658A_TRIGGER */

static DEVICE_API(sensor, qmi8658a_api) = {
#ifdef CONFIG_QMI8658A_TRIGGER
	.trigger_set = qmi8658a_trigger_set,
#endif
	.attr_get = qmi8658a_attr_get,
	.sample_fetch = qmi8658a_sample_fetch,
	.channel_get = qmi8658a_channel_get,
};

static int qmi8658a_init(const struct device *dev)
{
	const struct qmi8658a_config *cfg = dev->config;
	struct qmi8658a_data *data = dev->data;
	uint8_t chip_id;
	uint8_t accel_fs_reg;
	uint8_t gyro_fs_reg;
	uint8_t accel_odr_reg;
	uint8_t gyro_odr_reg;
	uint8_t ctrl1;
	uint16_t accel_odr;
	uint16_t gyro_odr;
	int rc;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	rc = i2c_reg_read_byte_dt(&cfg->i2c, QMI8658A_REG_WHO_AM_I, &chip_id);
	if (rc < 0) {
		LOG_ERR("Failed to read chip ID (%d)", rc);
		return rc;
	}

	if (chip_id != QMI8658A_CHIP_ID) {
		LOG_ERR("Unexpected chip ID 0x%02x", chip_id);
		return -ENODEV;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658A_REG_RESET, QMI8658A_RESET_CMD);
	if (rc < 0) {
		LOG_ERR("Failed to issue reset command (%d)", rc);
		return rc;
	}

	k_msleep(QMI8658A_RESET_DELAY_MS);

	rc = qmi8658a_accel_fs_to_reg(cfg->accel_fs, &accel_fs_reg);
	if (rc < 0) {
		LOG_ERR("Unsupported accel-fs value: %u", cfg->accel_fs);
		return rc;
	}

	rc = qmi8658a_gyro_fs_to_reg(cfg->gyro_fs, &gyro_fs_reg);
	if (rc < 0) {
		LOG_ERR("Unsupported gyro-fs value: %u", cfg->gyro_fs);
		return rc;
	}

	rc = qmi8658a_select_odr(cfg->accel_odr, &accel_odr, &accel_odr_reg);
	if (rc < 0) {
		LOG_ERR("Invalid accel-odr value: %u", cfg->accel_odr);
		return rc;
	}

	rc = qmi8658a_select_odr(cfg->gyro_odr, &gyro_odr, &gyro_odr_reg);
	if (rc < 0) {
		LOG_ERR("Invalid gyro-odr value: %u", cfg->gyro_odr);
		return rc;
	}

	ctrl1 = QMI8658A_CTRL1_ADDR_AI;
#ifdef CONFIG_QMI8658A_TRIGGER
	if (cfg->int_gpio.port) {
		if (cfg->int_pin == 1) {
			ctrl1 |= QMI8658A_CTRL1_INT1_EN;
		} else {
			ctrl1 |= QMI8658A_CTRL1_INT2_EN;
		}
	}
#endif

	rc = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658A_REG_CTRL1, ctrl1);
	if (rc < 0) {
		LOG_ERR("Failed to configure CTRL1 (%d)", rc);
		return rc;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658A_REG_CTRL2,
				   (accel_fs_reg << 4) | accel_odr_reg);
	if (rc < 0) {
		LOG_ERR("Failed to configure CTRL2 (%d)", rc);
		return rc;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658A_REG_CTRL3,
				   (gyro_fs_reg << 4) | gyro_odr_reg);
	if (rc < 0) {
		LOG_ERR("Failed to configure CTRL3 (%d)", rc);
		return rc;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658A_REG_CTRL7,
				   QMI8658A_CTRL7_ACC_ENABLE | QMI8658A_CTRL7_GYR_ENABLE);
	if (rc < 0) {
		LOG_ERR("Failed to configure CTRL7 (%d)", rc);
		return rc;
	}

	data->accel_fs = cfg->accel_fs;
	data->gyro_fs = cfg->gyro_fs;
	data->accel_odr = accel_odr;
	data->gyro_odr = gyro_odr;

#ifdef CONFIG_QMI8658A_TRIGGER
	rc = qmi8658a_init_interrupt(dev);
	if (rc < 0) {
		return rc;
	}
#endif

	return 0;
}

#define QMI8658A_DEFINE(inst)                                                                      \
	static struct qmi8658a_data qmi8658a_data_##inst;                                          \
                                                                                                   \
	static const struct qmi8658a_config qmi8658a_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.accel_fs = DT_INST_PROP(inst, accel_fs),                                          \
		.gyro_fs = DT_INST_PROP(inst, gyro_fs),                                            \
		.accel_odr = DT_INST_PROP(inst, accel_odr),                                        \
		.gyro_odr = DT_INST_PROP(inst, gyro_odr),                                          \
		IF_ENABLED(CONFIG_QMI8658A_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),	\
			    .int_pin = DT_INST_PROP(inst, int_pin),)) }; \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, qmi8658a_init, NULL, &qmi8658a_data_##inst,             \
				     &qmi8658a_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &qmi8658a_api);

DT_INST_FOREACH_STATUS_OKAY(QMI8658A_DEFINE)
