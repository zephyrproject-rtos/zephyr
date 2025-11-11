/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>

#include <stdio.h>

#define WHOAMI_REG      0x0F
#define WHOAMI_ALT_REG  0x4F

#ifdef CONFIG_LPS22HH_TRIGGER
static int lps22hh_trig_cnt;

static void lps22hh_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
	lps22hh_trig_cnt++;
}
#endif

#ifdef CONFIG_LIS2DW12_TRIGGER
static int lis2dw12_trig_cnt;

static void lis2dw12_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lis2dw12_trig_cnt++;
}
#endif

#ifdef CONFIG_LSM6DSO_TRIGGER
static int lsm6dso_acc_trig_cnt;
static int lsm6dso_gyr_trig_cnt;
static int lsm6dso_temp_trig_cnt;

static void lsm6dso_acc_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lsm6dso_acc_trig_cnt++;
}

static void lsm6dso_gyr_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	lsm6dso_gyr_trig_cnt++;
}

static void lsm6dso_temp_trig_handler(const struct device *dev,
				      const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP);
	lsm6dso_temp_trig_cnt++;
}
#endif

#ifdef CONFIG_STTS751_TRIGGER
static int stts751_trig_cnt;

static void stts751_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	stts751_trig_cnt++;
}
#endif

#ifdef CONFIG_IIS3DHHC_TRIGGER
static int iis3dhhc_trig_cnt;

static void iis3dhhc_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	iis3dhhc_trig_cnt++;
}
#endif

static void lps22hh_config(const struct device *lps22hh)
{
	struct sensor_value odr_attr;

	/* set LPS22HH sampling frequency to 50 Hz */
	odr_attr.val1 = 50;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lps22hh, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LPS22HH\n");
		return;
	}

#ifdef CONFIG_LPS22HH_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ALL;
	sensor_trigger_set(lps22hh, &trig, lps22hh_trigger_handler);
#endif
}

static void lis2dw12_config(const struct device *lis2dw12)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LIS2DW12 accel/gyro sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lis2dw12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2DW12 accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lis2dw12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2DW12 gyro\n");
		return;
	}

#ifdef CONFIG_LIS2DW12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lis2dw12, &trig, lis2dw12_trigger_handler);
#endif
}

static void lsm6dso_config(const struct device *lsm6dso)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LSM6DSO accel sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSO accel\n");
		return;
	}

	/* set LSM6DSO gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSO gyro\n");
		return;
	}

#ifdef CONFIG_LSM6DSO_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_acc_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_gyr_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_DIE_TEMP;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_temp_trig_handler);
#endif
}

static void stts751_config(const struct device *stts751)
{
	struct sensor_value odr_attr;

	/* set STTS751 conversion rate to 16 Hz */
	odr_attr.val1 = 16;
	odr_attr.val2 = 0;

	if (sensor_attr_set(stts751, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for STTS751\n");
		return;
	}

#ifdef CONFIG_STTS751_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_ALL;
	sensor_trigger_set(stts751, &trig, stts751_trigger_handler);
#endif
}

static void iis3dhhc_config(const struct device *iis3dhhc)
{
	struct sensor_value odr_attr;

	/* enable IIS3DHHC conversion */
	odr_attr.val1 = 1000; /* enable sensor at 1KHz */
	odr_attr.val2 = 0;

	if (sensor_attr_set(iis3dhhc, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for IIS3DHHC\n");
		return;
	}

#ifdef CONFIG_IIS3DHHC_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(iis3dhhc, &trig, iis3dhhc_trigger_handler);
#endif
}

int main(void)
{
	static const struct gpio_dt_spec led0_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
	static const struct gpio_dt_spec led1_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	int i, on = 1;
	int cnt = 1;
	uint32_t dtr = 0;

	if (!device_is_ready(dev)) {
		return 0;
	}

	/* Poll if the DTR flag was set, optional */
	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
	}

	if (!gpio_is_ready_dt(&led0_gpio)) {
		printk("%s: device not ready.\n", led0_gpio.port->name);
		return 0;
	}
	gpio_pin_configure_dt(&led0_gpio, GPIO_OUTPUT_ACTIVE);

	if (!gpio_is_ready_dt(&led1_gpio)) {
		printk("%s: device not ready.\n", led1_gpio.port->name);
		return 0;
	}
	gpio_pin_configure_dt(&led1_gpio, GPIO_OUTPUT_INACTIVE);

	for (i = 0; i < 6; i++) {
		gpio_pin_set_dt(&led0_gpio, on);
		gpio_pin_set_dt(&led1_gpio, !on);
		k_sleep(K_MSEC(100));
		on = (on == 1) ? 0 : 1;
	}

	gpio_pin_set_dt(&led0_gpio, 0);
	gpio_pin_set_dt(&led1_gpio, 1);

	printk("SensorTile.box test!!\n");

	const struct device *const hts221 = DEVICE_DT_GET_ONE(st_hts221);
	const struct device *const lis2dw12 = DEVICE_DT_GET_ONE(st_lis2dw12);
	const struct device *const lps22hh = DEVICE_DT_GET_ONE(st_lps22hh);
	const struct device *const lsm6dso = DEVICE_DT_GET_ONE(st_lsm6dso);
	const struct device *const stts751 = DEVICE_DT_GET_ONE(st_stts751);
	const struct device *const iis3dhhc = DEVICE_DT_GET_ONE(st_iis3dhhc);
	const struct device *const lis2mdl = DEVICE_DT_GET_ONE(st_lis2mdl);

	if (!device_is_ready(hts221)) {
		printk("%s: device not ready.\n", hts221->name);
		return 0;
	}
	if (!device_is_ready(lis2dw12)) {
		printk("%s: device not ready.\n", lis2dw12->name);
		return 0;
	}
	if (!device_is_ready(lps22hh)) {
		printk("%s: device not ready.\n", lps22hh->name);
		return 0;
	}
	if (!device_is_ready(lsm6dso)) {
		printk("%s: device not ready.\n", lsm6dso->name);
		return 0;
	}
	if (!device_is_ready(stts751)) {
		printk("%s: device not ready.\n", stts751->name);
		return 0;
	}
	if (!device_is_ready(iis3dhhc)) {
		printk("%s: device not ready.\n", iis3dhhc->name);
		return 0;
	}
	if (!device_is_ready(lis2mdl)) {
		printk("%s: device not ready.\n", lis2mdl->name);
		return 0;
	}

	lis2dw12_config(lis2dw12);
	lps22hh_config(lps22hh);
	lsm6dso_config(lsm6dso);
	stts751_config(stts751);
	iis3dhhc_config(iis3dhhc);

	while (1) {
		struct sensor_value hts221_hum, hts221_temp;
		struct sensor_value lps22hh_press, lps22hh_temp;
		struct sensor_value lis2dw12_accel[3];
		struct sensor_value iis3dhhc_accel[3];
		struct sensor_value lsm6dso_accel[3], lsm6dso_gyro[3];
		struct sensor_value stts751_temp;
		struct sensor_value magn[3];

		/* handle HTS221 sensor */
		if (sensor_sample_fetch(hts221) < 0) {
			printf("HTS221 Sensor sample update error\n");
			return 0;
		}

#ifndef CONFIG_LIS2DW12_TRIGGER
		/* handle LIS2DW12 sensor */
		if (sensor_sample_fetch(lis2dw12) < 0) {
			printf("LIS2DW12 Sensor sample update error\n");
			return 0;
		}
#endif

#ifndef CONFIG_LSM6DSO_TRIGGER
		if (sensor_sample_fetch(lsm6dso) < 0) {
			printf("LSM6DSO Sensor sample update error\n");
			return 0;
		}
#endif

#ifndef CONFIG_LPS22HH_TRIGGER
		if (sensor_sample_fetch(lps22hh) < 0) {
			printf("LPS22HH Sensor sample update error\n");
			return 0;
		}
#endif

#ifndef CONFIG_STTS751_TRIGGER
		if (sensor_sample_fetch(stts751) < 0) {
			printf("STTS751 Sensor sample update error\n");
			return 0;
		}
#endif

#ifndef CONFIG_IIS3DHHC_TRIGGER
		if (sensor_sample_fetch(iis3dhhc) < 0) {
			printf("IIS3DHHC Sensor sample update error\n");
			return 0;
		}
#endif

		if (sensor_sample_fetch(lis2mdl) < 0) {
			printf("LIS2MDL Sensor sample update error\n");
			return 0;
		}

		sensor_channel_get(hts221, SENSOR_CHAN_HUMIDITY, &hts221_hum);
		sensor_channel_get(hts221, SENSOR_CHAN_AMBIENT_TEMP, &hts221_temp);
		sensor_channel_get(lis2dw12, SENSOR_CHAN_ACCEL_XYZ, lis2dw12_accel);
		sensor_channel_get(lps22hh, SENSOR_CHAN_AMBIENT_TEMP, &lps22hh_temp);
		sensor_channel_get(lps22hh, SENSOR_CHAN_PRESS, &lps22hh_press);
		sensor_channel_get(lsm6dso, SENSOR_CHAN_ACCEL_XYZ, lsm6dso_accel);
		sensor_channel_get(lsm6dso, SENSOR_CHAN_GYRO_XYZ, lsm6dso_gyro);
		sensor_channel_get(stts751, SENSOR_CHAN_AMBIENT_TEMP, &stts751_temp);
		sensor_channel_get(iis3dhhc, SENSOR_CHAN_ACCEL_XYZ, iis3dhhc_accel);
		sensor_channel_get(lis2mdl, SENSOR_CHAN_MAGN_XYZ, magn);

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("SensorTile.box dashboard\n\n");

		/* HTS221 temperature */
		printf("HTS221: Temperature: %.1f C\n",
		       sensor_value_to_double(&hts221_temp));

		/* HTS221 humidity */
		printf("HTS221: Relative Humidity: %.1f%%\n",
		       sensor_value_to_double(&hts221_hum));

		/* temperature */
		printf("LPS22HH: Temperature: %.1f C\n",
		       sensor_value_to_double(&lps22hh_temp));

		/* pressure */
		printf("LPS22HH: Pressure:%.3f kpa\n",
		       sensor_value_to_double(&lps22hh_press));

		printf("LIS2DW12: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lis2dw12_accel[0]),
			sensor_value_to_double(&lis2dw12_accel[1]),
			sensor_value_to_double(&lis2dw12_accel[2]));

		printf("IIS3DHHC: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&iis3dhhc_accel[0]),
			sensor_value_to_double(&iis3dhhc_accel[1]),
			sensor_value_to_double(&iis3dhhc_accel[2]));

		printf("LSM6DSOX: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dso_accel[0]),
			sensor_value_to_double(&lsm6dso_accel[1]),
			sensor_value_to_double(&lsm6dso_accel[2]));

		printf("LSM6DSOX: GYro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dso_gyro[0]),
			sensor_value_to_double(&lsm6dso_gyro[1]),
			sensor_value_to_double(&lsm6dso_gyro[2]));

		/* temperature */
		printf("STTS751: Temperature: %.1f C\n",
		       sensor_value_to_double(&stts751_temp));

		printf("LIS2MDL: Magn (Gauss): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&magn[0]),
			sensor_value_to_double(&magn[1]),
			sensor_value_to_double(&magn[2]));

#if defined(CONFIG_LPS22HH_TRIGGER)
		printk("%d:: lps22hh trig %d\n", cnt, lps22hh_trig_cnt);
#endif

#ifdef CONFIG_LIS2DW12_TRIGGER
		printk("%d:: lis2dw12 trig %d\n", cnt, lis2dw12_trig_cnt);
#endif

#ifdef CONFIG_LSM6DSO_TRIGGER
		printk("%d:: lsm6dsox acc trig %d\n", cnt, lsm6dso_acc_trig_cnt);
		printk("%d:: lsm6dsox gyr trig %d\n", cnt, lsm6dso_gyr_trig_cnt);
#endif

#if defined(CONFIG_STTS751_TRIGGER)
		printk("%d:: stts751 trig %d\n", cnt, stts751_trig_cnt);
#endif

#if defined(CONFIG_IIS3DHHC_TRIGGER)
		printk("%d:: iis3dhhc trig %d\n", cnt, iis3dhhc_trig_cnt);
#endif

		k_sleep(K_MSEC(2000));
	}
}
