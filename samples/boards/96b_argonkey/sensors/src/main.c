/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>

#include <stdio.h>

/* #define ARGONKEY_TEST_LOG 1 */

#define WHOAMI_REG      0x0F
#define WHOAMI_ALT_REG  0x4F

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}

static int lsm6dsl_trig_cnt;
#ifdef CONFIG_LSM6DSL_TRIGGER
static void lsm6dsl_trigger_handler(const struct device *dev,
				    struct sensor_trigger *trig)
{
#ifdef ARGONKEY_TEST_LOG
	char out_str[64];
#endif
	struct sensor_value accel_x, accel_y, accel_z;
	struct sensor_value gyro_x, gyro_y, gyro_z;

#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
	struct sensor_value magn_x, magn_y, magn_z;
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
	struct sensor_value press, temp;
#endif

	lsm6dsl_trig_cnt++;

	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel_x);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel_z);
#ifdef ARGONKEY_TEST_LOG
	sprintf(out_str, "accel (%f %f %f) m/s2", out_ev(&accel_x),
						out_ev(&accel_y),
						out_ev(&accel_z));
	printk("TRIG %s\n", out_str);
#endif

	/* lsm6dsl gyro */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &gyro_x);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &gyro_z);
#ifdef ARGONKEY_TEST_LOG
	sprintf(out_str, "gyro (%f %f %f) dps", out_ev(&gyro_x),
						out_ev(&gyro_y),
						out_ev(&gyro_z));
	printk("TRIG %s\n", out_str);
#endif

#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
	/* lsm6dsl magn */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_MAGN_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &magn_x);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &magn_y);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &magn_z);
#ifdef ARGONKEY_TEST_LOG
	sprintf(out_str, "magn (%f %f %f) gauss", out_ev(&magn_x),
						 out_ev(&magn_y),
						 out_ev(&magn_z));
	printk("TRIG %s\n", out_str);
#endif

#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
	/* lsm6dsl press/temp */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
	sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);

	sensor_sample_fetch_chan(dev, SENSOR_CHAN_AMBIENT_TEMP);
	sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);

#ifdef ARGONKEY_TEST_LOG
	sprintf(out_str, "press (%f) kPa - temp (%f) deg", out_ev(&press),
							   out_ev(&temp));
	printk("%s\n", out_str);
#endif

#endif
}
#endif

#define NUM_LEDS 12
#define DELAY_TIME K_MSEC(50)

void main(void)
{
	int cnt = 0;
	char out_str[64];
	static const struct device *led0, *led1;
	int i, on = 1;

#ifdef CONFIG_LP3943
	static const struct device *ledc;

	ledc = device_get_binding(DT_LABEL(DT_INST(0, ti_lp3943)));
	if (!ledc) {
		printk("Could not get pointer to %s sensor\n",
			DT_LABEL(DT_INST(0, ti_lp3943)));
		return;
	}

	/* turn all leds on */
	for (i = 0; i < NUM_LEDS; i++) {
		led_on(ledc, i);
		k_sleep(DELAY_TIME);
	}

	/* turn all leds off */
	for (i = 0; i < NUM_LEDS; i++) {
		led_off(ledc, i);
		k_sleep(DELAY_TIME);
	}
#endif

	led0 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	gpio_pin_configure(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
			   GPIO_OUTPUT_ACTIVE |
			   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));

	led1 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led1), gpios));
	gpio_pin_configure(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios),
			   GPIO_OUTPUT_INACTIVE |
			   DT_GPIO_FLAGS(DT_ALIAS(led1), gpios));

	for (i = 0; i < 5; i++) {
		gpio_pin_set(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios), on);
		k_sleep(K_MSEC(200));
		on = (on == 1) ? 0 : 1;
	}

	printk("ArgonKey test!!\n");

#ifdef CONFIG_LPS22HB
	const struct device *baro_dev =
			device_get_binding(DT_LABEL(DT_INST(0, st_lps22hb_press)));

	if (!baro_dev) {
		printk("Could not get pointer to %s sensor\n",
			DT_LABEL(DT_INST(0, st_lps22hb_press)));
		return;
	}
#endif

#ifdef CONFIG_HTS221
	const struct device *hum_dev = device_get_binding(DT_LABEL(DT_INST(0, st_hts221)));

	if (!hum_dev) {
		printk("Could not get pointer to %s sensor\n",
			DT_LABEL(DT_INST(0, st_hts221)));
		return;
	}
#endif

#ifdef CONFIG_LSM6DSL
	const struct device *accel_dev = device_get_binding(DT_LABEL(DT_INST(0, st_lsm6dsl)));

	if (!accel_dev) {
		printk("Could not get pointer to %s sensor\n",
			DT_LABEL(DT_INST(0, st_lsm6dsl)));
		return;
	}

#if defined(CONFIG_LSM6DSL_ACCEL_ODR) && (CONFIG_LSM6DSL_ACCEL_ODR == 0)
	struct sensor_value a_odr_attr;

	/* set sampling frequency to 104Hz for accel */
	a_odr_attr.val1 = 104;
	a_odr_attr.val2 = 0;

	if (sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &a_odr_attr) < 0) {
		printk("Cannot set sampling frequency for accelerometer.\n");
		return;
	}
#endif

#if defined(CONFIG_LSM6DSL_ACCEL_FS) && (CONFIG_LSM6DSL_ACCEL_FS == 0)
	struct sensor_value a_fs_attr;

	/* set full scale to 16g for accel */
	sensor_g_to_ms2(16, &a_fs_attr);

	if (sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &a_fs_attr) < 0) {
		printk("Cannot set fs for accelerometer.\n");
		return;
	}
#endif

#if defined(CONFIG_LSM6DSL_GYRO_ODR) && (CONFIG_LSM6DSL_GYRO_ODR == 0)
	struct sensor_value g_odr_attr;

	/* set sampling frequency to 104Hz for accel */
	g_odr_attr.val1 = 104;
	g_odr_attr.val2 = 0;

	if (sensor_attr_set(accel_dev, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &g_odr_attr) < 0) {
		printk("Cannot set sampling frequency for gyro.\n");
		return;
	}
#endif

#if defined(CONFIG_LSM6DSL_GYRO_FS) && (CONFIG_LSM6DSL_GYRO_FS == 0)
	struct sensor_value g_fs_attr;

	/* set full scale to 245dps for accel */
	sensor_degrees_to_rad(245, &g_fs_attr);

	if (sensor_attr_set(accel_dev, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &g_fs_attr) < 0) {
		printk("Cannot set fs for gyroscope.\n");
		return;
	}
#endif

#endif

#ifdef CONFIG_VL53L0X
	const struct device *tof_dev = device_get_binding(DT_LABEL(DT_INST(0, st_vl53l0x)));

	if (!tof_dev) {
		printk("Could not get pointer to %s sensor\n",
			DT_LABEL(DT_INST(0, st_vl53l0x)));
		return;
	}
#endif

#ifdef CONFIG_LSM6DSL_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	if (sensor_trigger_set(accel_dev, &trig,
			       lsm6dsl_trigger_handler) != 0) {
		printk("Could not set sensor type and channel\n");
		return;
	}
#endif

	while (1) {
#ifdef CONFIG_LPS22HB
		struct sensor_value temp, press;
#endif
#ifdef CONFIG_HTS221
		struct sensor_value humidity;
#endif
#ifdef CONFIG_LSM6DSL
		struct sensor_value accel_x, accel_y, accel_z;
		struct sensor_value gyro_x, gyro_y, gyro_z;
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
		struct sensor_value magn_x, magn_y, magn_z;
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
		struct sensor_value press, temp;
#endif
#endif
#ifdef CONFIG_VL53L0X
		struct sensor_value prox;
#endif

#ifdef CONFIG_VL53L0X
		sensor_sample_fetch(tof_dev);
		sensor_channel_get(tof_dev, SENSOR_CHAN_PROX, &prox);
		printk("proxy: %d  ;\n", prox.val1);
		sensor_channel_get(tof_dev, SENSOR_CHAN_DISTANCE, &prox);
		printk("distance: %d m -- %02d cm;\n", prox.val1,
						       prox.val2/10000);
#endif

#ifdef CONFIG_LPS22HB
		sensor_sample_fetch(baro_dev);
		sensor_channel_get(baro_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(baro_dev, SENSOR_CHAN_PRESS, &press);

		printk("temp: %d.%02d C; press: %d.%06d\n",
		       temp.val1, temp.val2, press.val1, press.val2);
#endif

#ifdef CONFIG_HTS221
		sensor_sample_fetch(hum_dev);
		sensor_channel_get(hum_dev, SENSOR_CHAN_HUMIDITY, &humidity);

		printk("humidity: %d.%06d\n",
		       humidity.val1, humidity.val2);
#endif

#ifdef CONFIG_LSM6DSL
		/* lsm6dsl accel */
		sensor_sample_fetch_chan(accel_dev, SENSOR_CHAN_ACCEL_XYZ);
		sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_X, &accel_x);
		sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
		sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_Z, &accel_z);
		sprintf(out_str, "accel (%f %f %f) m/s2", out_ev(&accel_x),
							out_ev(&accel_y),
							out_ev(&accel_z));
		printk("%s\n", out_str);

		/* lsm6dsl gyro */
		sensor_sample_fetch_chan(accel_dev, SENSOR_CHAN_GYRO_XYZ);
		sensor_channel_get(accel_dev, SENSOR_CHAN_GYRO_X, &gyro_x);
		sensor_channel_get(accel_dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
		sensor_channel_get(accel_dev, SENSOR_CHAN_GYRO_Z, &gyro_z);
		sprintf(out_str, "gyro (%f %f %f) dps", out_ev(&gyro_x),
							out_ev(&gyro_y),
							out_ev(&gyro_z));
		printk("%s\n", out_str);
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
		/* lsm6dsl magn */
		sensor_sample_fetch_chan(accel_dev, SENSOR_CHAN_MAGN_XYZ);
		sensor_channel_get(accel_dev, SENSOR_CHAN_MAGN_X, &magn_x);
		sensor_channel_get(accel_dev, SENSOR_CHAN_MAGN_Y, &magn_y);
		sensor_channel_get(accel_dev, SENSOR_CHAN_MAGN_Z, &magn_z);
		sprintf(out_str, "magn (%f %f %f) gauss", out_ev(&magn_x),
							 out_ev(&magn_y),
							 out_ev(&magn_z));
		printk("%s\n", out_str);
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
		/* lsm6dsl press/temp */
		sensor_sample_fetch_chan(accel_dev, SENSOR_CHAN_PRESS);
		sensor_channel_get(accel_dev, SENSOR_CHAN_PRESS, &press);

		sensor_sample_fetch_chan(accel_dev, SENSOR_CHAN_AMBIENT_TEMP);
		sensor_channel_get(accel_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);

		sprintf(out_str, "press (%f) kPa - temp (%f) deg",
			out_ev(&press), out_ev(&temp));
		printk("%s\n", out_str);
#endif

#endif /* CONFIG_LSM6DSL */

		printk("- (%d) (trig_cnt: %d)\n\n", ++cnt, lsm6dsl_trig_cnt);
		k_sleep(K_MSEC(2000));
	}
}
