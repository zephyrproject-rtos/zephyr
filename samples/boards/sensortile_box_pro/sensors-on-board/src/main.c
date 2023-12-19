/*
 * Copyright (c) 2023 STMicroelectronics
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

#include <stdio.h>

#ifdef CONFIG_LPS2XDF_TRIGGER
static int lps22df_trig_cnt;

static void lps22df_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
	lps22df_trig_cnt++;
}
#endif

#ifdef CONFIG_LSM6DSV16X_TRIGGER
static int lsm6dsv16x_acc_trig_cnt;
static int lsm6dsv16x_gyr_trig_cnt;
static int lsm6dsv16x_temp_trig_cnt;

static void lsm6dsv16x_acc_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lsm6dsv16x_acc_trig_cnt++;
}

static void lsm6dsv16x_gyr_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	lsm6dsv16x_gyr_trig_cnt++;
}

static void lsm6dsv16x_temp_trig_handler(const struct device *dev,
				      const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP);
	lsm6dsv16x_temp_trig_cnt++;
}
#endif

#ifdef CONFIG_LIS2MDL_TRIGGER
static int lis2mdl_trig_cnt;

static void lis2mdl_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lis2mdl_trig_cnt++;
}
#endif

#ifdef CONFIG_LIS2DU12_TRIGGER
static int lis2du12_trig_cnt;

static void lis2du12_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lis2du12_trig_cnt++;
}
#endif

static void lps22df_config(const struct device *lps22df)
{
	struct sensor_value odr_attr;

	/* set LPS22DF sampling frequency to 50 Hz */
	odr_attr.val1 = 50;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lps22df, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LPS22DF\n");
		return;
	}

#ifdef CONFIG_LPS2XDF_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ALL;
	sensor_trigger_set(lps22df, &trig, lps22df_trigger_handler);
#endif
}

static void lsm6dsv16x_config(const struct device *lsm6dsv16x)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LSM6DSV16X accel sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSV16X accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSV16X accel\n");
		return;
	}

	/* set LSM6DSV16X gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSV16X gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSV16X gyro\n");
		return;
	}

#ifdef CONFIG_LSM6DSV16X_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dsv16x, &trig, lsm6dsv16x_acc_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(lsm6dsv16x, &trig, lsm6dsv16x_gyr_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_DIE_TEMP;
	sensor_trigger_set(lsm6dsv16x, &trig, lsm6dsv16x_temp_trig_handler);
#endif
}

static void lis2mdl_config(const struct device *lis2mdl)
{
	struct sensor_value odr_attr;

	/* set LIS2MDL sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lis2mdl, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2MDL\n");
		return;
	}

#ifdef CONFIG_LIS2MDL_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_MAGN_XYZ;
	sensor_trigger_set(lis2mdl, &trig, lis2mdl_trigger_handler);
#endif
}

static void lis2du12_config(const struct device *lis2du12)
{
	struct sensor_value odr_attr;

	/* set LIS2DU12 sampling frequency to 400 Hz */
	odr_attr.val1 = 400;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lis2du12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2DU12\n");
		return;
	}

#ifdef CONFIG_LIS2DU12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lis2du12, &trig, lis2du12_trigger_handler);
#endif
}

static int led_pattern_out(void)
{
	const struct gpio_dt_spec led0_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
	const struct gpio_dt_spec led1_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
	const struct gpio_dt_spec led2_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
	const struct gpio_dt_spec led3_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);
	int i;

	/* led 0 */
	if (!gpio_is_ready_dt(&led0_gpio)) {
		printk("%s: device not ready.\n", led0_gpio.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&led0_gpio, GPIO_OUTPUT_INACTIVE);

	/* led 1 */
	if (!gpio_is_ready_dt(&led1_gpio)) {
		printk("%s: device not ready.\n", led1_gpio.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&led1_gpio, GPIO_OUTPUT_INACTIVE);

	/* led 2 */
	if (!gpio_is_ready_dt(&led2_gpio)) {
		printk("%s: device not ready.\n", led2_gpio.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&led2_gpio, GPIO_OUTPUT_INACTIVE);

	/* led 3 */
	if (!gpio_is_ready_dt(&led3_gpio)) {
		printk("%s: device not ready.\n", led3_gpio.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&led3_gpio, GPIO_OUTPUT_INACTIVE);

	/* output led pattern */
	for (i = 0; i < 12; i++) {
		gpio_pin_set_dt(&led0_gpio, ((i % 4) == 0) ? 1 : 0);
		gpio_pin_set_dt(&led1_gpio, ((i % 4) == 1) ? 1 : 0);
		gpio_pin_set_dt(&led2_gpio, ((i % 4) == 2) ? 1 : 0);
		gpio_pin_set_dt(&led3_gpio, ((i % 4) == 3) ? 1 : 0);
		k_sleep(K_MSEC(100));
	}

	/* turn all leds off */
	gpio_pin_set_dt(&led0_gpio, 0);
	gpio_pin_set_dt(&led1_gpio, 0);
	gpio_pin_set_dt(&led2_gpio, 0);
	gpio_pin_set_dt(&led3_gpio, 0);

	return 0;
}

int main(void)
{
	int cnt = 1;

	/* signal that sample is started */
	if (led_pattern_out() < 0) {
		return -1;
	}

	printk("SensorTile.box Pro sensor test\n");

	const struct device *const hts221 = DEVICE_DT_GET_ONE(st_hts221);
	const struct device *const lps22df = DEVICE_DT_GET_ONE(st_lps22df);
	const struct device *const lsm6dsv16x = DEVICE_DT_GET_ONE(st_lsm6dsv16x);
	const struct device *const lis2mdl = DEVICE_DT_GET_ONE(st_lis2mdl);
	const struct device *const lis2du12 = DEVICE_DT_GET_ONE(st_lis2du12);

	if (!device_is_ready(hts221)) {
		printk("%s: device not ready.\n", hts221->name);
		return 0;
	}
	if (!device_is_ready(lps22df)) {
		printk("%s: device not ready.\n", lps22df->name);
		return 0;
	}
	if (!device_is_ready(lsm6dsv16x)) {
		printk("%s: device not ready.\n", lsm6dsv16x->name);
		return 0;
	}
	if (!device_is_ready(lis2mdl)) {
		printk("%s: device not ready.\n", lis2mdl->name);
		return 0;
	}
	if (!device_is_ready(lis2du12)) {
		printk("%s: device not ready.\n", lis2du12->name);
		return 0;
	}

	lis2du12_config(lis2du12);
	lis2mdl_config(lis2mdl);
	lps22df_config(lps22df);
	lsm6dsv16x_config(lsm6dsv16x);

	while (1) {
		struct sensor_value hts221_hum, hts221_temp;
		struct sensor_value lps22df_press, lps22df_temp;
		struct sensor_value lsm6dsv16x_accel[3], lsm6dsv16x_gyro[3];
		struct sensor_value lis2mdl_magn[3];
		struct sensor_value lis2mdl_temp;
		struct sensor_value lis2du12_accel[3];

		/* handle HTS221 sensor */
		if (sensor_sample_fetch(hts221) < 0) {
			printf("HTS221 Sensor sample update error\n");
			return 0;
		}

#ifndef CONFIG_LSM6DSV16X_TRIGGER
		if (sensor_sample_fetch(lsm6dsv16x) < 0) {
			printf("LSM6DSV16X Sensor sample update error\n");
			return 0;
		}
#endif

#ifndef CONFIG_LPS2XDF_TRIGGER
		if (sensor_sample_fetch(lps22df) < 0) {
			printf("LPS22DF Sensor sample update error\n");
			return 0;
		}
#endif

#ifndef CONFIG_LIS2MDL_TRIGGER
		if (sensor_sample_fetch(lis2mdl) < 0) {
			printf("LIS2MDL Magn Sensor sample update error\n");
			return 0;
		}
#endif

#ifndef CONFIG_LIS2DU12_TRIGGER
		if (sensor_sample_fetch(lis2du12) < 0) {
			printf("LIS2DU12 xl Sensor sample update error\n");
			return 0;
		}
#endif

		sensor_channel_get(hts221, SENSOR_CHAN_HUMIDITY, &hts221_hum);
		sensor_channel_get(hts221, SENSOR_CHAN_AMBIENT_TEMP, &hts221_temp);
		sensor_channel_get(lps22df, SENSOR_CHAN_AMBIENT_TEMP, &lps22df_temp);
		sensor_channel_get(lps22df, SENSOR_CHAN_PRESS, &lps22df_press);
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_ACCEL_XYZ, lsm6dsv16x_accel);
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_GYRO_XYZ, lsm6dsv16x_gyro);
		sensor_channel_get(lis2mdl, SENSOR_CHAN_MAGN_XYZ, lis2mdl_magn);
		sensor_channel_get(lis2mdl, SENSOR_CHAN_DIE_TEMP, &lis2mdl_temp);
		sensor_channel_get(lis2du12, SENSOR_CHAN_ACCEL_XYZ, lis2du12_accel);

		/* Display sensor data */

		/*  Clear terminal (ANSI ESC-C) */
		printf("\0033\014");

		printf("SensorTile.box dashboard\n\n");

		/* HTS221 temperature */
		printf("HTS221: Temperature: %.1f C\n",
		       sensor_value_to_double(&hts221_temp));

		/* HTS221 humidity */
		printf("HTS221: Relative Humidity: %.1f%%\n",
		       sensor_value_to_double(&hts221_hum));

		/* temperature */
		printf("LPS22DF: Temperature: %.1f C\n",
		       sensor_value_to_double(&lps22df_temp));

		/* pressure */
		printf("LPS22DF: Pressure: %.3f kpa\n",
		       sensor_value_to_double(&lps22df_press));

		printf("LSM6DSV16X: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dsv16x_accel[0]),
			sensor_value_to_double(&lsm6dsv16x_accel[1]),
			sensor_value_to_double(&lsm6dsv16x_accel[2]));

		printf("LSM6DSV16X: GYro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dsv16x_gyro[0]),
			sensor_value_to_double(&lsm6dsv16x_gyro[1]),
			sensor_value_to_double(&lsm6dsv16x_gyro[2]));

		/* lis2mdl */
		printf("LIS2MDL: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&lis2mdl_magn[0]),
		       sensor_value_to_double(&lis2mdl_magn[1]),
		       sensor_value_to_double(&lis2mdl_magn[2]));

		printf("LIS2MDL: Temperature: %.1f C\n",
		       sensor_value_to_double(&lis2mdl_temp));

		printf("LIS2DU12: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lis2du12_accel[0]),
			sensor_value_to_double(&lis2du12_accel[1]),
			sensor_value_to_double(&lis2du12_accel[2]));

#ifdef CONFIG_LPS2XDF_TRIGGER
		printk("%d:: lps22df trig %d\n", cnt, lps22df_trig_cnt);
#endif

#ifdef CONFIG_LSM6DSV16X_TRIGGER
		printk("%d:: lsm6dsv16x acc trig %d\n", cnt, lsm6dsv16x_acc_trig_cnt);
		printk("%d:: lsm6dsv16x gyr trig %d\n", cnt, lsm6dsv16x_gyr_trig_cnt);
#endif

#ifdef CONFIG_LIS2MDL_TRIGGER
		printk("%d:: lis2mdl trig %d\n", cnt, lis2mdl_trig_cnt);
#endif

#ifdef CONFIG_LIS2DU12_TRIGGER
		printk("%d:: lis2du12 trig %d\n", cnt, lis2du12_trig_cnt);
#endif

		k_sleep(K_MSEC(2000));
	}
}
