/*
 * Copyright (c) 2024 Plentify
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ade9153a.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ade9153a_sample, CONFIG_SENSOR_LOG_LEVEL);

static const struct device *get_ade9153a_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(adi_ade9153a);

	if (dev == NULL) {
		LOG_ERR("\nError: no device found.");

		return NULL;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("\nError: Device \"%s\" is not ready; "
			"check the driver initialization logs for errors.",
			dev->name);

		return NULL;
	}

	LOG_DBG("Found device \"%s\", getting sensor data", dev->name);

	return dev;
}

void ade9153a_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	if (trigger->type == (int16_t)SENSOR_TRIG_ADE9153A_IRQ) {
		LOG_DBG("Triggered IRQ");
	} else if (trigger->type == (int16_t)SENSOR_TRIG_ADE9153A_CF) {
		LOG_DBG("Triggered CF");
	} else {
		LOG_DBG("Invalid trigger type %d", trigger->type);
	}
}

static void get_reg(const struct device *dev, uint16_t addr, uint16_t size)
{
	union ade9153a_register reg = {.addr = addr, .size = size};

	sensor_attr_get(dev, SENSOR_CHAN_ALL, (int16_t)SENSOR_ATTR_ADE9153A_REGISTER,
			&reg.as_sensor_value);

	LOG_DBG(" == REG%d[0x%X]:0x%X", reg.size * 8, reg.addr, reg.as_sensor_value.val1);
}

static inline void set_reg(const struct device *dev, uint16_t addr, uint16_t size, void *data)
{
	union ade9153a_register reg = {.addr = addr, .size = size};

	if (size == sizeof(uint16_t)) {
		reg.as_sensor_value.val1 = *(uint16_t *)data;
	} else {
		reg.as_sensor_value.val1 = *(int32_t *)data;
	}

	sensor_attr_set(dev, SENSOR_CHAN_ALL, (int16_t)SENSOR_ATTR_ADE9153A_REGISTER,
			&reg.as_sensor_value);

	LOG_DBG(" >> REG%d[0x%X]:0x%X", reg.size * 8, reg.addr, reg.value);

	LOG_DBG("Checking the last command:");
	get_reg(dev, ADE9153A_REG_LAST_CMD, sizeof(uint16_t));

	if (size == sizeof(uint16_t)) {
		get_reg(dev, ADE9153A_REG_LAST_DATA_16, sizeof(uint16_t));
	} else {
		get_reg(dev, ADE9153A_REG_LAST_DATA_32, sizeof(uint32_t));
	}
	LOG_DBG("--------------------------");
}

#if defined(CONFIG_ADE9153A_TRIGGER)

static const struct sensor_trigger cf_trigger = {.chan = SENSOR_CHAN_ALL,
						 .type = (int16_t)SENSOR_TRIG_ADE9153A_CF};

static const struct sensor_trigger irq_trigger = {.chan = SENSOR_CHAN_ALL,
						  .type = (int16_t)SENSOR_TRIG_ADE9153A_IRQ};

#endif /* CONFIG_ADE9153A_TRIGGER */

int main(void)
{
	const struct device *dev = get_ade9153a_device();

	if (dev == NULL) {
		LOG_ERR("Error %d", -ENODEV);
		return 0;
	}

#if defined(CONFIG_ADE9153A_TRIGGER)
	sensor_trigger_set(dev, &cf_trigger, ade9153a_trigger_handler);
	sensor_trigger_set(dev, &irq_trigger, ade9153a_trigger_handler);
#endif /* CONFIG_ADE9153A_TRIGGER */

	while (1) {
		sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP);

		struct sensor_value sval;

		LOG_DBG("------------------------------");
		sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &sval);
		LOG_DBG("Temperature: %d.%06d C", sval.val1, sval.val2);

		sensor_sample_fetch_chan(dev, SENSOR_CHAN_AC_CURRENT_RMS);

		sensor_channel_get(dev, SENSOR_CHAN_AC_VOLTAGE_RMS, &sval);
		LOG_DBG("Voltage:     %d.%06d mV", sval.val1, sval.val2);

		sensor_channel_get(dev, SENSOR_CHAN_AC_CURRENT_RMS, &sval);
		LOG_DBG("Current:     %d.%06d mA", sval.val1, sval.val2);

		k_sleep(K_MSEC(1000));
	}

	return 0;
}
