/*
 * Copyright (c) 2022 Pat Deegan, https://psychogenic.com
 *
 * Zephyr driver for ADP8866
 * Charge Pump Driven 9-Channel LED Driver with
 * Automated LED Lighting Effects
 * https://www.analog.com/media/en/technical-documentation/data-sheets/adp8866.pdf
 *
 * On, off and set brightness are implemented for each of the outputs.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <drivers/led.h>
#include <device.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <logging/log.h>

LOG_MODULE_REGISTER(adi_adp8866);


#define DT_DRV_COMPAT adi_adp8866


#define APD8866_I2C_ADDR DT_INST_REG_ADDR(0)

#define APD8866_NUM_LED_DRIVERS		9
#define APD8866_MIN_LED_BRIGHTNESS	0
#define APD8866_MAX_LED_BRIGHTNESS	0x7F

#define APD8866_REG_MFDVID		0x00
#define APD8866_REG_MDCR		0x01
#define APD8866_REG_INT_STAT	0x02
#define APD8866_REG_INT_EN		0x03
#define APD8866_REG_ISCOFF_SEL1	0x04
#define APD8866_REG_ISCOFF_SEL2	0x05
#define APD8866_REG_GAIN_SEL	0x06
#define APD8866_REG_LVL_SEL1	0x07
#define APD8866_REG_LVL_SEL2	0x08
#define APD8866_REG_PWR_SEL1	0x09
#define APD8866_REG_PWR_SEL2	0x0A
/* 0x0b - 0x0f -- reserved */
#define APD8866_REG_CFGR		0x10
#define APD8866_REG_BLSEL		0x11
#define APD8866_REG_BLFR		0x12
#define APD8866_REG_BLMX		0x13
#define APD8866_REG_to			0x14
#define APD8866_REG_ISCC1		0x1A
#define APD8866_REG_ISCC2		0x1B
#define APD8866_REG_ISCT1		0x1C
#define APD8866_REG_ISCT2		0x1D
#define APD8866_REG_OFFTIMER6	0x1E
#define APD8866_REG_OFFTIMER7	0x1F
#define APD8866_REG_OFFTIMER8	0x20
#define APD8866_REG_OFFTIMER9	0x21
#define APD8866_REG_ISCF		0x22
#define APD8866_REG_ISC1		0x23
#define APD8866_REG_ISC2		0x24
#define APD8866_REG_ISC3		0x25
#define APD8866_REG_ISC4		0x26
#define APD8866_REG_ISC5		0x27
#define APD8866_REG_ISC6		0x28
#define APD8866_REG_ISC7		0x29
#define APD8866_REG_ISC8		0x2A
#define APD8866_REG_ISC9		0x2B
#define APD8866_REG_HB_SEL		0x2C
#define APD8866_REG_ISC6_HB		0x2D
#define APD8866_REG_ISC7_HB		0x2E
#define APD8866_REG_ISC8_HB		0x2F
#define APD8866_REG_ISC9_HB		0x30
#define APD8866_REG_OFFTIMER6_HB	0x31
#define APD8866_REG_OFFTIMER7_HB	0x32
#define APD8866_REG_OFFTIMER8_HB	0x33
#define APD8866_REG_OFFTIMER9_HB	0x34
#define APD8866_REG_ISCT_HB		0x35
/* 0x36 - 0x3B -- reserved */
#define APD8866_REG_DELAY6		0x3C
#define APD8866_REG_DELAY7		0x3D
#define APD8866_REG_DELAY8		0x3E
#define APD8866_REG_DELAY9		0x3F




struct adp8866_data {
	const struct device *i2c;
	unsigned char manufacturerID;
	unsigned char deviceID;
	unsigned char int_stat;
	unsigned char mdcr;
	unsigned char setup_is_done;
};


static int adp8866_lowlevel_setup(const struct device *dev);

static int adp8866_led_set_brightness(
		const struct device *dev,
		uint32_t led, /* [0-8] */
		unsigned char value)
{
	struct adp8866_data *data = dev->data;

	if (led >= APD8866_NUM_LED_DRIVERS) {

		LOG_ERR("LED idx beyond max: %d", led);
		return -EINVAL;
	}

	if (! data->setup_is_done) {
		LOG_INF("Re-trig low level setup...");
		if (adp8866_lowlevel_setup(dev)) {
			/* didn't work...; */

			LOG_ERR("Low level setup failed");
			return -EIO;
		}
	}





	if ((value > APD8866_MAX_LED_BRIGHTNESS)) {
		LOG_ERR("Value beyond max");
		return -EINVAL;
	}

	if (led >= APD8866_NUM_LED_DRIVERS) {

		LOG_ERR("LED idx beyond max: %d", led);
		return -EINVAL;
	}
	if (i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR, (APD8866_REG_ISC1+led),
			value))
	{

		LOG_ERR("Writing LED value failed.");
		return -EIO;
	}

	return 0;
}

static inline int adp8866_led_on(const struct device *dev,
		uint32_t led)
{

	return adp8866_led_set_brightness(dev, led, APD8866_MAX_LED_BRIGHTNESS);
}


static inline int adp8866_led_off(const struct device *dev, uint32_t led)
{
	return adp8866_led_set_brightness(dev, led, APD8866_MIN_LED_BRIGHTNESS);
}


static int adp8866_lowlevel_setup(const struct device *dev)
{

	struct adp8866_data *data = dev->data;
	data->setup_is_done = 0;
	/* struct led_data *dev_data = &data->dev_data; */

	if (i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_BLSEL, 0xff)) {
		LOG_ERR("Setting all to ICS failed.");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_BLFR, 0xAA)) {
		LOG_ERR("Setting backlight fade rate failed.");
		return -EIO;
	}

	LOG_INF("backlight set");

	/*
	 * Fade speed
	 * high nibble: fade out
	 * low nibble: fade in
	 * nibble values
	 *  0001 = 0.05 sec.
		0010 = 0.10 sec.
		0011 = 0.15 sec.
		0100 = 0.20 sec.
		0101 = 0.25 sec.
		0110 = 0.30 sec.
		0111 = 0.35 sec.
		1000 = 0.40 sec.
		1001 = 0.45 sec.
		1010 = 0.50 sec.
		1011 = 0.75 sec.
		1100 = 1.00 sec.
		1101 = 1.25 sec.
		1110 = 1.50 sec.
		1111 = 1.75 s
	 */
	if (i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_ISCF, 0x42)) {

		LOG_ERR("Setting ICS fade rate failed.");
		return -EIO;
	}

	LOG_INF("ICS fade rate set");


	data->mdcr = 0x65;

	if (i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_MDCR,data->mdcr)) {
		LOG_ERR("Setting backlight fade rate failed.");
		return -EIO;
	}


	LOG_INF("backlight fade rate set");

	if (i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_ISCC1, 0x6)) {
		LOG_ERR("Setting ISCC1 failed.");
		return -EIO;
	}

	LOG_INF("ISCC1 set");
	if (i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_ISCC2, 0xff)) {
		LOG_ERR("Setting ISCC2 (Independent Sink Current Control) failed.");
		return -EIO;
	}

	LOG_DBG("ISCC2 set");

	// set ISC for led 9 as well
	if (i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR, APD8866_REG_CFGR, 0x14)) {

		LOG_ERR("Setting ISC for LED9 failed.");
		// return 0;
		return -EIO;
	}

	// dunno if this is required...
	i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_PWR_SEL1, 0);
	i2c_reg_write_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_PWR_SEL2, 0);



	if (i2c_reg_read_byte(data->i2c, APD8866_I2C_ADDR,
			APD8866_REG_INT_STAT, &(data->int_stat))) {

		LOG_ERR("Reading ADP8866 chip failed.");
		return -EIO;
	}
	LOG_INF("GOT stat %i", data->int_stat);
	if (data->int_stat & (0x1C)) {

		LOG_ERR("Error condition reported--check int_stat");
		return -EIO;
	}


	LOG_DBG("LED driver setup done");
	data->setup_is_done = 1;
	return 0;
}


static int adp8866_led_init(const struct device *dev)
{

	LOG_INF("LED INIT CALLED");
	struct adp8866_data *data = dev->data;
	data->setup_is_done = 0;
	/* struct led_data *dev_data = &data->dev_data; */
	unsigned char tmp;

		if (! data->i2c) {

			data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
			if (data->i2c == NULL) {
				LOG_ERR("Failed to get I2C device");
				return -EINVAL;
			}
			LOG_INF("Got I2C device, addr is %i", APD8866_I2C_ADDR);
		}

		if (i2c_reg_read_byte(data->i2c, APD8866_I2C_ADDR,
				APD8866_REG_MFDVID, &(tmp))) {

			LOG_ERR("Reading ADP8866 chip failed.");
			return -EIO;
		}

		if (! tmp) {
			LOG_ERR("Could not get device ID");
			return -EIO;
		}
		LOG_DBG("DEV ID is %i", tmp);
		data->deviceID = tmp & 0x0f;
		data->manufacturerID = tmp >> 4;

	return 0;

}

static struct adp8866_data adp8866_led_data;

static const struct led_driver_api adp8866_led_api = {
	.set_brightness = adp8866_led_set_brightness,
	.on = adp8866_led_on,
	.off = adp8866_led_off,
};



DEVICE_DT_INST_DEFINE(0, &adp8866_led_init, NULL,
		&adp8866_led_data,
		NULL, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,
		&adp8866_led_api);
