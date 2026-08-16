/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sensor API driver for the Excelitas CaliPile TPiS 1S 1385 infrared
 * thermopile. The device is accessed over I2C. It exposes the temperature of
 * the surrounding air as SENSOR_CHAN_AMBIENT_TEMP, and the contactless object
 * temperature and the raw presence and motion counters as the device specific
 * channels declared in zephyr/drivers/sensor/tpi1s1385.h. The calibration
 * constants required to convert the raw counts into Celsius are read once from
 * the on chip EEPROM at initialization.
 */

#include "tpi1s1385.h"

#include <math.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(TPI1S1385, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT excelitas_tpi1s1385

/* Command byte sent through the I2C General Call to reload the slave address. */
#define TPI1S1385_GENERAL_CALL_RELOAD		0x04

/* Time the device needs to copy its slave address from EEPROM into the address register. */
#define TPI1S1385_EEPROM_RELOAD_DELAY_US	350

/* Values written to EEPROM_CONTROL to enable or disable EEPROM read access. */
#define TPI1S1385_EEPROM_CTRL_ENABLE		0x80
#define TPI1S1385_EEPROM_CTRL_DISABLE		0x00

/* Exponent of the lookup function used to compute the object temperature. */
#define TPI1S1385_LOOKUP_EXP			3.8

/* Reference temperatures used by the calibration formulas. */
#define TPI1S1385_T_REF_KELVIN			298.15
#define TPI1S1385_ZERO_C_KELVIN			273.15

/* Same reference temperatures expressed as integers in milli Kelvin. */
#define TPI1S1385_T_REF_MILLIKELVIN		298150
#define TPI1S1385_ZERO_C_MILLIKELVIN		273150

/*
 * Reads one byte from a device register.
 * The whole driver goes through this helper instead of calling the I2C API
 * directly. The bus details stay in one place, and the trigger file can
 * reach the registers without knowing how the device is wired.
 */
int tpi1s1385_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct tpi1s1385_config *config = dev->config;

	return i2c_reg_read_byte_dt(&config->i2c, reg, val);
}

/*
 * Writes one byte to a device register.
 * Counterpart of the read helper and the only place where the driver puts
 * a value on the bus.
 */
int tpi1s1385_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct tpi1s1385_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, val);
}

/*
 * Changes only the bits selected by the mask and leaves the others alone.
 * Several device registers pack unrelated fields into the same byte, so a
 * plain write would silently clear a neighbour field. Reading first, then
 * merging, then writing back avoids that.
 * The sequence is not atomic by itself. A caller that can race with
 * another context must hold the driver mutex around it.
 */
int tpi1s1385_reg_update(const struct device *dev, uint8_t reg,
			 uint8_t mask, uint8_t val)
{
	uint8_t old_val;
	int ret;

	ret = tpi1s1385_reg_read(dev, reg, &old_val);
	if (ret < 0) {
		return ret;
	}

	old_val = (old_val & ~mask) | (val & mask);

	return tpi1s1385_reg_write(dev, reg, old_val);
}

/*
 * Sensor API entry point that refreshes the cached raw counts.
 * Nothing is converted here. The object channel needs floating point, so
 * the maths is deferred to channel_get and a caller that only wants
 * proximity never pays for it.
 * Register 0x03 is read once and feeds both the object and the ambient
 * value, because the device packs the two into the same byte.
 * The mutex is held for the whole sequence so a sample is never a mix of
 * two acquisitions, and a single exit path releases it.
 * INTERRUPT_STATUS is refreshed only when every channel is requested,
 * because reading it clears the latched flags the trigger path needs.
 */
static int tpi1s1385_sample_fetch(const struct device *dev,
				  enum sensor_channel chan)
{
	const struct tpi1s1385_config *config = dev->config;
	struct tpi1s1385_data *data = dev->data;
	uint8_t raw[4];
	int ret;

	if (chan != SENSOR_CHAN_ALL &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != (enum sensor_channel)SENSOR_CHAN_TPI1S1385_OBJECT_TEMP &&
	    chan != (enum sensor_channel)SENSOR_CHAN_TPI1S1385_PRESENCE &&
	    chan != (enum sensor_channel)SENSOR_CHAN_TPI1S1385_MOTION) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (chan == SENSOR_CHAN_ALL ||
	    chan == SENSOR_CHAN_AMBIENT_TEMP ||
	    chan == (enum sensor_channel)SENSOR_CHAN_TPI1S1385_OBJECT_TEMP) {
		uint8_t start_reg = TPI1S1385_REG_TP_OBJECT_MSB;

		/*
		 * Registers 0x01 to 0x04 are consecutive and hold one single
		 * measurement. The device increments its address pointer after
		 * every byte it sends, so one transfer collects them all and no
		 * new measurement can slip in between. Byte 0 and byte 1 and
		 * the top bit of byte 2 carry TPobject. The low bits of byte 2
		 * and byte 3 carry TPambient.
		 */
		ret = i2c_write_read_dt(&config->i2c, &start_reg,
					sizeof(start_reg), raw, sizeof(raw));
		if (ret < 0) {
			LOG_ERR("Failed to read sample regs (0x%02X): %d",
				TPI1S1385_REG_TP_OBJECT_MSB, ret);
			goto unlock;
		}

		data->tp_object =
			(int32_t)((raw[0] << TPI1S1385_TP_OBJECT_MSB_SHIFT) |
				  (raw[1] << TPI1S1385_TP_OBJECT_MID_SHIFT) |
				  ((raw[2] & TPI1S1385_TP_OBJECT_LSB_MASK) >>
					TPI1S1385_TP_OBJECT_LSB_SHIFT));

		data->tp_ambient =
			(int16_t)(((raw[2] & TPI1S1385_TP_AMBIENT_MSB_MASK) <<
					TPI1S1385_TP_AMBIENT_MSB_SHIFT) |
				  raw[3]);
	}

	if (chan == SENSOR_CHAN_ALL ||
	    chan == (enum sensor_channel)SENSOR_CHAN_TPI1S1385_PRESENCE ||
	    chan == (enum sensor_channel)SENSOR_CHAN_TPI1S1385_MOTION) {
		uint8_t start_reg = TPI1S1385_REG_TP_PRESENCE;
		uint8_t counters[2];

		/*
		 * Registers 0x0F and 0x10 are consecutive, so one transfer
		 * collects both counters and they cannot be recomputed by the
		 * device in between. Byte 0 carries TPpresence and byte 1
		 * carries TPmotion.
		 */
		ret = i2c_write_read_dt(&config->i2c, &start_reg,
					sizeof(start_reg), counters,
					sizeof(counters));
		if (ret < 0) {
			LOG_ERR("Failed to read counter regs (0x%02X): %d",
				TPI1S1385_REG_TP_PRESENCE, ret);
			goto unlock;
		}

		data->tp_presence = counters[0];
		data->tp_motion = counters[1];
	}

	if (chan == SENSOR_CHAN_ALL) {
		ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_INTERRUPT_STATUS,
					 &data->interrupt_status);
		if (ret < 0) {
			LOG_ERR("Failed to read INTERRUPT_STATUS (0x%02X): %d",
				TPI1S1385_REG_INTERRUPT_STATUS, ret);
			goto unlock;
		}
	}

	ret = 0;
unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/*
 * Converts the raw ambient counts into milli Kelvin.
 * The datasheet models the ambient sensor as a straight line through the
 * PTAT25 reference point, with the M constant as its slope.
 * It is computed with integers on purpose, so a board with no floating
 * point unit is not penalised just for reading an ambient temperature.
 * Scaling by one thousand keeps three decimals. The product is widened to
 * 64 bits first, because a full scale delta times that factor overflows a
 * signed 32 bit integer.
 */
static int32_t tpi1s1385_ambient_millikelvin(const struct tpi1s1385_data *data)
{
	int32_t delta = (int32_t)data->tp_ambient - (int32_t)data->eeprom.ptat25;

	return TPI1S1385_T_REF_MILLIKELVIN +
	       (int32_t)(((int64_t)delta * 100000) / data->eeprom.m);
}

/*
 * Converts the raw object counts into Kelvin.
 * The thermopile response is not linear, so the datasheet models it with a
 * power law whose exponent comes from the lookup table stored in EEPROM.
 * The factory calibration point gives the sensitivity k, which scales the
 * raw counts before the response is inverted.
 * This one keeps floating point. A power with a fractional exponent has no
 * cheap integer equivalent and the accuracy would suffer.
 * The ambient temperature is an input because the sensor measures the
 * difference between the object and its own body.
 */
static double tpi1s1385_object_kelvin(const struct tpi1s1385_data *data,
				      double tamb_k)
{
	double u0 = (double)data->eeprom.u0;
	double uout1 = (double)data->eeprom.uout1;
	double tobj1_k = (double)data->eeprom.tobj1 + TPI1S1385_ZERO_C_KELVIN;

	double f_tobj1 = pow(tobj1_k, TPI1S1385_LOOKUP_EXP);
	double f_tref = pow(TPI1S1385_T_REF_KELVIN, TPI1S1385_LOOKUP_EXP);
	double k = (uout1 - u0) / (f_tobj1 - f_tref);

	double f_tamb = pow(tamb_k, TPI1S1385_LOOKUP_EXP);
	double f_tobj = ((double)data->tp_object - u0) / k + f_tamb;

	return pow(f_tobj, 1.0 / TPI1S1385_LOOKUP_EXP);
}

/*
 * Sensor API entry point that hands the cached values to the application.
 * It converts but never touches the bus, so it stays cheap and can be
 * called several times after a single fetch.
 * The ambient case fills val1 and val2 by hand. The integer result is
 * already in the units the structure expects, and going through a double
 * would only lose precision.
 * Proximity is not a temperature. The raw presence and motion counters are
 * passed as they are, presence in val1 and motion in val2.
 */
static int tpi1s1385_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct tpi1s1385_data *data = dev->data;
	int32_t tamb_mc;
	double tobj_c;

	switch ((int)chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		tamb_mc = tpi1s1385_ambient_millikelvin(data) -
			  TPI1S1385_ZERO_C_MILLIKELVIN;
		val->val1 = tamb_mc / 1000;
		val->val2 = (tamb_mc % 1000) * 1000;
		return 0;

	case SENSOR_CHAN_TPI1S1385_OBJECT_TEMP:
		tobj_c = tpi1s1385_object_kelvin(
				data,
				tpi1s1385_ambient_millikelvin(data) / 1000.0) -
			 TPI1S1385_ZERO_C_KELVIN;
		return sensor_value_from_double(val, tobj_c);

	case SENSOR_CHAN_TPI1S1385_PRESENCE:
		val->val1 = (int32_t)data->tp_presence;
		val->val2 = 0;
		return 0;

	case SENSOR_CHAN_TPI1S1385_MOTION:
		val->val1 = (int32_t)data->tp_motion;
		val->val2 = 0;
		return 0;

	default:
		return -ENOTSUP;
	}
}

/*
 * Reads the factory calibration constants once at init.
 * These values never change, so they are cached in the driver data and the
 * conversion helpers then run without any bus traffic.
 * Access must be opened through EEPROM_CONTROL first. That mode raises the
 * supply current, so the function always closes it again on the way out,
 * including after a failure. That is what the single exit path is for.
 * The individual reads are not logged one by one. A failure here means the
 * bus is broken, and one message at the end says it.
 */
static int tpi1s1385_read_eeprom(const struct device *dev)
{
	struct tpi1s1385_data *data = dev->data;
	uint8_t msb, lsb;
	int ret;

	ret = tpi1s1385_reg_write(dev, TPI1S1385_REG_EEPROM_CONTROL,
				  TPI1S1385_EEPROM_CTRL_ENABLE);
	if (ret < 0) {
		LOG_ERR("Failed to enable EEPROM access: %d", ret);
		return ret;
	}

	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_PTAT25_MSB, &msb);
	if (ret < 0) {
		goto done;
	}
	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_PTAT25_LSB, &lsb);
	if (ret < 0) {
		goto done;
	}
	data->eeprom.ptat25 =
		((uint16_t)(msb & TPI1S1385_EEPROM_PTAT25_MSB_MASK) << 8) | lsb;

	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_M_MSB, &msb);
	if (ret < 0) {
		goto done;
	}
	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_M_LSB, &lsb);
	if (ret < 0) {
		goto done;
	}
	data->eeprom.m = ((uint16_t)msb << 8) | lsb;

	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_U0_MSB, &msb);
	if (ret < 0) {
		goto done;
	}
	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_U0_LSB, &lsb);
	if (ret < 0) {
		goto done;
	}
	data->eeprom.u0 = ((uint16_t)msb << 8) | lsb;

	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_UOUT1_MSB, &msb);
	if (ret < 0) {
		goto done;
	}
	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_UOUT1_LSB, &lsb);
	if (ret < 0) {
		goto done;
	}
	data->eeprom.uout1 = ((uint16_t)msb << 8) | lsb;

	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_TOBJ1,
				 &data->eeprom.tobj1);
	if (ret < 0) {
		goto done;
	}

	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_EEPROM_LOOKUP,
				 &data->eeprom.lookup);

done:
	(void)tpi1s1385_reg_write(dev, TPI1S1385_REG_EEPROM_CONTROL,
				  TPI1S1385_EEPROM_CTRL_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to read EEPROM calibration: %d", ret);
	}
	return ret;
}

/*
 * Brings the device up and registers it with the sensor subsystem.
 * The order of the steps matters.
 * After power up the device answers on the General Call address 0x00 only,
 * because it does not know its own slave address yet. Writing the reload
 * command to that address makes it copy the address held in EEPROM into
 * its address register. Only then does it answer where the devicetree says
 * it should.
 * That copy takes time, so the driver waits before addressing the device.
 * Skipping the wait would make the very first access fail.
 * A failure on the General Call is only a warning, because a warm reboot
 * leaves a device that is already addressable.
 * A read follows to confirm the device is really present before anything
 * is written to it.
 * Control registers hold undefined values after power up, so every one of
 * them is programmed from the devicetree instead of being left alone.
 * INTERRUPT_STATUS is then read to clear the over temperature flag the
 * device raises at power up, otherwise the first trigger would fire for
 * nothing.
 * Calibration is read last among the device accesses, and the trigger is
 * armed only once everything else is ready.
 */
static int tpi1s1385_init(const struct device *dev)
{
	const struct tpi1s1385_config *config = dev->config;
	struct tpi1s1385_data *data = dev->data;
	uint8_t slave_addr;
	uint8_t reload_cmd = TPI1S1385_GENERAL_CALL_RELOAD;
	uint8_t int_status;
	uint8_t slp12;
	uint8_t src_reg;
	int ret;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	ret = i2c_write(config->i2c.bus, &reload_cmd, sizeof(reload_cmd), 0x00);
	if (ret < 0) {
		LOG_WRN("General Call failed (ret=%d), device may already be initialized", ret);
	}

	k_usleep(TPI1S1385_EEPROM_RELOAD_DELAY_US);

	ret = i2c_reg_read_byte_dt(&config->i2c,
				   TPI1S1385_REG_GENERAL_CALL,
				   &slave_addr);
	if (ret < 0) {
		LOG_ERR("Device did not respond at address 0x%02X: %d",
			config->i2c.addr, ret);
		return -EIO;
	}

	slp12 = ((config->slp2 & TPI1S1385_SLP_FIELD_MASK) << TPI1S1385_SLP2_SHIFT) |
		(config->slp1 & TPI1S1385_SLP_FIELD_MASK);
	ret = tpi1s1385_reg_write(dev, TPI1S1385_REG_SLP12, slp12);
	if (ret < 0) {
		LOG_ERR("Failed to write SLP12: %d", ret);
		return ret;
	}

	ret = tpi1s1385_reg_write(dev, TPI1S1385_REG_SLP3,
				  config->slp3 & TPI1S1385_SLP_FIELD_MASK);
	if (ret < 0) {
		LOG_ERR("Failed to write SLP3: %d", ret);
		return ret;
	}

	ret = tpi1s1385_reg_write(dev, TPI1S1385_REG_PRESENCE_THRESHOLD,
				  config->presence_threshold);
	if (ret < 0) {
		LOG_ERR("Failed to write presence threshold: %d", ret);
		return ret;
	}

	ret = tpi1s1385_reg_write(dev, TPI1S1385_REG_MOTION_THRESHOLD,
				  config->motion_threshold);
	if (ret < 0) {
		LOG_ERR("Failed to write motion threshold: %d", ret);
		return ret;
	}

	ret = tpi1s1385_reg_write(dev, TPI1S1385_REG_AMB_SHOCK_THRESHOLD,
				  config->amb_shock_threshold);
	if (ret < 0) {
		LOG_ERR("Failed to write ambient shock threshold: %d", ret);
		return ret;
	}

	src_reg = ((config->src_select & TPI1S1385_SRC_SELECT_MASK) <<
			TPI1S1385_SRC_SELECT_SHIFT) |
		  (config->cycle_time & TPI1S1385_CYCLE_TIME_MASK);

	if (!config->tpot_direction_falls_below) {
		src_reg |= TPI1S1385_TPOT_DIRECTION_EXCEEDS;
	}

	ret = tpi1s1385_reg_write(dev, TPI1S1385_REG_SRC_SELECT, src_reg);
	if (ret < 0) {
		LOG_ERR("Failed to write SRC/cycle/direction: %d", ret);
		return ret;
	}

	ret = tpi1s1385_reg_read(dev, TPI1S1385_REG_INTERRUPT_STATUS,
				 &int_status);
	if (ret < 0) {
		LOG_ERR("Failed to read INTERRUPT_STATUS: %d", ret);
		return ret;
	}

	ret = tpi1s1385_read_eeprom(dev);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("Excelitas TPiS 1S 1385 initialized at address 0x%02X "
		"(PTAT25=%u, M=%u, LOOKUP=%u)",
		config->i2c.addr, data->eeprom.ptat25, data->eeprom.m,
		data->eeprom.lookup);

#ifdef CONFIG_TPI1S1385_TRIGGER
	ret = tpi1s1385_trigger_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize trigger: %d", ret);
		return ret;
	}
#endif

	return 0;
}

static DEVICE_API(sensor, tpi1s1385_api) = {
	.sample_fetch = tpi1s1385_sample_fetch,
	.channel_get = tpi1s1385_channel_get,
#ifdef CONFIG_TPI1S1385_TRIGGER
	.trigger_set = tpi1s1385_trigger_set,
	.attr_set = tpi1s1385_attr_set,
#endif
};

#define TPI1S1385_INT_GPIO_INIT(inst)						\
	IF_ENABLED(CONFIG_TPI1S1385_TRIGGER,					\
		(.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))

#define TPI1S1385_DEFINE(inst)							\
	static struct tpi1s1385_data tpi1s1385_data_##inst;			\
										\
	static const struct tpi1s1385_config tpi1s1385_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
		TPI1S1385_INT_GPIO_INIT(inst)					\
		.slp1 = DT_INST_PROP(inst, slp1),				\
		.slp2 = DT_INST_PROP(inst, slp2),				\
		.slp3 = DT_INST_PROP(inst, slp3),				\
		.src_select = DT_INST_PROP(inst, src_select),			\
		.cycle_time = DT_INST_PROP(inst, cycle_time),			\
		.tpot_direction_falls_below =					\
			DT_INST_PROP(inst, tpot_direction_falls_below),		\
		.presence_threshold = DT_INST_PROP(inst, presence_threshold),	\
		.motion_threshold = DT_INST_PROP(inst, motion_threshold),	\
		.amb_shock_threshold = DT_INST_PROP(inst, amb_shock_threshold),	\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,					\
				     tpi1s1385_init,				\
				     NULL,					\
				     &tpi1s1385_data_##inst,			\
				     &tpi1s1385_config_##inst,			\
				     POST_KERNEL,				\
				     CONFIG_SENSOR_INIT_PRIORITY,		\
				     &tpi1s1385_api);

DT_INST_FOREACH_STATUS_OKAY(TPI1S1385_DEFINE)
