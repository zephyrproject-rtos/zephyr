/*
 * Copyright (c) 2022 HAW Hamburg FTZ-DIWIP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "max31865.h"

static int max31865_spi_write(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct max31865_config *cfg = dev->config;

	const struct spi_buf bufs[] = {{
					       .buf = &reg,
					       .len = 1,
				       },
				       {.buf = data, .len = len}};

	const struct spi_buf_set tx = {.buffers = bufs, .count = 2};

	return spi_write_dt(&cfg->spi, &tx);
}

static int max31865_spi_read(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct max31865_config *cfg = dev->config;

	reg &= 0x7F;
	const struct spi_buf tx_buf = {.buf = &reg, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_buf[] = {{
					   .buf = &reg,
					   .len = 1,
				   },
				   {.buf = data, .len = len}};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = 2};

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

/**
 * @brief Set device configuration register
 *
 * @param device device instance
 * @return 0 if successful, or negative error code from SPI API
 */
static int configure_device(const struct device *dev)
{
	struct max31865_data *data = dev->data;
	uint8_t cmd[] = {data->config_control_bits};
	int err = max31865_spi_write(dev, WR(REG_CONFIG), cmd, 1);

	if (err < 0) {
		LOG_ERR("Error write SPI%d\n", err);
	}
	return err;
}

/**
 * @brief Set device fail threshold registers
 *
 * @param device device instance
 * @return 0 if successful, or negative error code from SPI API
 */
static int set_threshold_values(const struct device *dev)
{
	const struct max31865_config *config = dev->config;
	uint8_t cmd[] = {
		(config->high_threshold >> 7) & 0x00ff, (config->high_threshold << 1) & 0x00ff,
		(config->low_threshold >> 7) & 0x00ff, (config->low_threshold << 1) & 0x00ff};
	int err = max31865_spi_write(dev, WR(REG_HIGH_FAULT_THR_MSB), cmd, 4);

	if (err < 0) {
		LOG_ERR("Error write SPI%d\n", err);
	}
	return err;
}

#ifdef CONFIG_NEWLIB_LIBC

/**
 * Apply the Callendar-Van Dusen equation to convert the RTD resistance
 * to temperature:
 * Tr = (-A + SQRT(delta) ) / 2*B
 * delta = A^2 - 4B*(1-Rt/Ro)
 * For under zero, taken from
 * https://www.analog.com/media/en/technical-documentation/application-notes/AN709_0.pdf
 * @param resistance measured resistance
 * @param resistance_0 constant resistance at 0oC
 * @return calculated temperature
 */
static double calculate_temperature(double resistance, double resistance_0)
{
	double temperature;
	double delta = (RTD_A * RTD_A) - 4 * RTD_B * (1.0 - resistance / resistance_0);

	temperature = (-RTD_A + sqrt(delta)) / (2 * RTD_B);
	if (temperature > 0.0) {
		return temperature;
	}
	resistance /= resistance_0;
	resistance *= 100.0;
	temperature = RTD_C[0] + RTD_C[1] * resistance + RTD_C[2] * pow(resistance, 2) -
		      RTD_C[3] * pow(resistance, 3) - RTD_C[4] * pow(resistance, 4) +
		      RTD_C[5] * pow(resistance, 5);
	return temperature;
}

#else

/**
 * Apply a very good linear approximation of the Callendar-Van Dusen equation to convert the RTD
 * resistance to temperature:
 * @param resistance measured resistance
 * @param resistance_0 constant resistance at 0oC
 * @return calculated temperature
 */
static double calculate_temperature(double resistance, double resistance_0)
{
	double temperature;

	temperature = (resistance - resistance_0) / (resistance_0 * RTD_A);
	return temperature;
}

#endif

/**
 * @brief Enable/Disable Vbias for MAX31865
 *
 * @param device device instance
 * @param enable true, turn on vbias, false, turn off vbias
 * @return 0 if successful, or negative error code from SPI API
 */
static int max31865_set_vbias(const struct device *dev, bool enable)
{
	struct max31865_data *data = dev->data;

	WRITE_BIT(data->config_control_bits, 7, enable);
	return configure_device(dev);
}

static int max31865_start_conversion(const struct device *dev)
{
	struct max31865_data *data = dev->data;
	uint8_t cmd[] = {data->config_control_bits | MAX31865_CONFIG_ONE_SHOT};

	return max31865_spi_write(dev, WR(REG_CONFIG), cmd, 1);
}

#ifdef CONFIG_MAX31865_DRDY

static void max31865_drdy_callback(const struct device *port, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct max31865_data *data = CONTAINER_OF(cb, struct max31865_data, drdy_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->drdy_sem);
}

#endif /* CONFIG_MAX31865_DRDY */

static int max31865_set_three_wire(const struct device *dev, bool enable)
{
	struct max31865_data *data = dev->data;

	WRITE_BIT(data->config_control_bits, 4, enable);
	return configure_device(dev);
}

static char *max31865_error_to_string(uint8_t fault_register)
{
	switch (fault_register) {
	case 0:
		return "No error";

	case MAX31865_FAULT_VOLTAGE:
		return "Over/under voltage fault";

	case MAX31865_FAULT_RTDIN_FORCE:
		return "RTDIN- < 0.85*VBIAS (FORCE- open)";

	case MAX31865_FAULT_REFIN_FORCE:
		return "REFIN- < 0.85*VBIAS (FORCE- open)";

	case MAX31865_FAULT_REFIN:
		return "REFIN- > 0.85*VBIAS";

	case MAX31865_FAULT_LOW_THRESHOLD:
		return "RTD below low threshold";

	case MAX31865_FAULT_HIGH_THRESHOLD:
		return "RTD above high threshold";
	}
	return "";
}

static int max31865_fault_register(const struct device *dev)
{
	uint8_t fault_register;
	uint8_t saved_fault_bits;

	max31865_spi_read(dev, (REG_FAULT_STATUS), &fault_register, 1);
	struct max31865_data *data = dev->data;
	saved_fault_bits  = data->config_control_bits & FAULT_BITS_CLEAR_MASK;
	/*Clear fault register */
	WRITE_BIT(data->config_control_bits, 1, 1);
	data->config_control_bits &= ~FAULT_BITS_CLEAR_MASK;
	configure_device(dev);
	LOG_ERR("Fault Register: 0x%02x, %s", fault_register,
		max31865_error_to_string(fault_register));
	WRITE_BIT(data->config_control_bits, 1, 0);
	data->config_control_bits |= saved_fault_bits;

	return 0;
}

#ifdef CONFIG_MAX31865_DRDY

static int max31865_convert_drdy(const struct device *dev)
{
	const struct max31865_config *config = dev->config;
	struct max31865_data *data = dev->data;
	int err;

	k_sem_reset(&data->drdy_sem);
	err = gpio_pin_interrupt_configure_dt(&config->drdy, GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0) {
		LOG_ERR("Could not arm DRDY: %d", err);
		return err;
	}

	err = max31865_start_conversion(dev);
	if (err == 0 && k_sem_take(&data->drdy_sem, K_MSEC(MAX31865_DRDY_TIMEOUT_MS)) != 0) {
		LOG_ERR("DRDY stayed high for %d ms", MAX31865_DRDY_TIMEOUT_MS);
		err = -EIO;
	}

	gpio_pin_interrupt_configure_dt(&config->drdy, GPIO_INT_DISABLE);
	return err;
}

#endif /* CONFIG_MAX31865_DRDY */

static int max31865_convert(const struct device *dev)
{
	int err;

#ifdef CONFIG_MAX31865_DRDY
	const struct max31865_config *config = dev->config;

	if (config->drdy.port != NULL) {
		return max31865_convert_drdy(dev);
	}
#endif

	err = max31865_start_conversion(dev);
	if (err == 0) {
		k_sleep(K_MSEC(MAX31865_CONVERSION_TIME_MS));
	}
	return err;
}

/**
 * @brief Get temperature value in oC for device
 *
 * @param device device instance
 * @param temperature measured temperature
 * @return 0 if successful, or negative error code
 */
static int max31865_get_temperature(const struct device *dev)
{
	union read_reg_u {
		uint8_t u8[2];
		uint16_t u16;
	} read_reg;
	int err;

	max31865_set_vbias(dev, true);

	err = max31865_convert(dev);
	if (err < 0) {
		max31865_set_vbias(dev, false);
		return err;
	}

	read_reg.u16 = 0;
	/* Read resistance measured value */
	err = max31865_spi_read(dev, (REG_RTD_MSB), read_reg.u8, 2);

	max31865_set_vbias(dev, false);

	if (err < 0) {
		LOG_ERR("SPI read %d\n", err);
		return -EIO;
	}
	read_reg.u16 = sys_be16_to_cpu(read_reg.u16);

	LOG_DBG("RAW: %02X %02X , %04X", read_reg.u8[0], read_reg.u8[1], read_reg.u16);
	if (TESTBIT(read_reg.u16, 0)) {
		max31865_fault_register(dev);
		return -EIO;
	}

	const struct max31865_config *config = dev->config;
	struct max31865_data *data = dev->data;

	read_reg.u16 = read_reg.u16 >> 1;
	double resistance = (double)read_reg.u16;

	resistance /= 32768;
	resistance *= config->resistance_reference;
	data->temperature = calculate_temperature(resistance, config->resistance_at_zero);
	return 0;
}

#ifdef CONFIG_MAX31865_DRDY

static int max31865_clear_drdy(const struct device *dev)
{
	uint8_t rtd[2];

	return max31865_spi_read(dev, REG_RTD_MSB, rtd, sizeof(rtd));
}

static int max31865_init_drdy(const struct device *dev)
{
	const struct max31865_config *config = dev->config;
	struct max31865_data *data = dev->data;
	int err;

	k_sem_init(&data->drdy_sem, 0, 1);

	if (config->drdy.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&config->drdy)) {
		LOG_ERR_DEVICE_NOT_READY(config->drdy.port);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->drdy, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	gpio_init_callback(&data->drdy_cb, max31865_drdy_callback, BIT(config->drdy.pin));

	return gpio_add_callback_dt(&config->drdy, &data->drdy_cb);
}

#endif /* CONFIG_MAX31865_DRDY */

static int max31865_init(const struct device *dev)
{
	const struct max31865_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR_DEVICE_NOT_READY(config->spi.bus);
		return -ENODEV;
	}
	struct max31865_data *data = dev->data;

#ifdef CONFIG_MAX31865_DRDY
	int err = max31865_init_drdy(dev);

	if (err < 0) {
		return err;
	}
#endif

	/* Set the configuration register */
	data->config_control_bits = 0;

	WRITE_BIT(data->config_control_bits, 6, config->conversion_mode);
	data->config_control_bits |= config->fault_cycle & 0b00001100;
	WRITE_BIT(data->config_control_bits, 0, config->filter_50hz);

	configure_device(dev);
	set_threshold_values(dev);
	max31865_set_vbias(dev, false);
	max31865_set_three_wire(dev, config->three_wire);

#ifdef CONFIG_MAX31865_DRDY
	return max31865_clear_drdy(dev);
#else
	return 0;
#endif
}

static int max31865_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Invalid channel provided");
		return -ENOTSUP;
	}
	return max31865_get_temperature(dev);
}

static int max31865_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max31865_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		return sensor_value_from_double(val, data->temperature);
	default:
		return -ENOTSUP;
	}
}

static int max31865_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Invalid channel provided");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_MAX31865_THREE_WIRE:
		return max31865_set_three_wire(dev, val->val1);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, max31865_api_funcs) = {
	.sample_fetch = max31865_sample_fetch,
	.channel_get = max31865_channel_get,
	.attr_set = max31865_attr_set,
};

#define MAX31865_DEFINE(inst)                                                                      \
                                                                                                   \
	static struct max31865_data max31865_data_##inst;                                          \
                                                                                                   \
	static const struct max31865_config max31865_config_##inst = {                             \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_MODE_CPHA | SPI_WORD_SET(8)),                \
		.resistance_at_zero = DT_INST_PROP(inst, resistance_at_zero),                      \
		.resistance_reference = DT_INST_PROP(inst, resistance_reference),                  \
		.conversion_mode = false,                                                          \
		.three_wire = DT_INST_PROP(inst, maxim_3_wire),                                    \
		.fault_cycle = MAX31865_FAULT_DETECTION_NONE,                                      \
		.filter_50hz = DT_INST_PROP(inst, filter_50hz),                                    \
		.low_threshold = DT_INST_PROP(inst, low_threshold),                                \
		.high_threshold = DT_INST_PROP(inst, high_threshold),                              \
		IF_ENABLED(CONFIG_MAX31865_DRDY,                                                   \
			   (.drdy = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, {0}),))};          \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, max31865_init, NULL, &max31865_data_##inst,             \
				     &max31865_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &max31865_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(MAX31865_DEFINE)
