/*
 * Copyright (c) 2024 STMicroelectronics
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

#ifdef CONFIG_STTS22H_TRIGGER
static int stts22h_trig_cnt;

static void stts22h_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	stts22h_trig_cnt++;
}
#endif

#ifdef CONFIG_IIS2DLPC_TRIGGER
static int iis2dlpc_trig_cnt;

static void iis2dlpc_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	iis2dlpc_trig_cnt++;
}
#endif

#ifdef CONFIG_IIS2MDC_TRIGGER
static int iis2mdc_trig_cnt;

static void iis2mdc_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	iis2mdc_trig_cnt++;
}
#endif

#ifdef CONFIG_ISM330DHCX_TRIGGER
static int ism330dhcx_acc_trig_cnt;
static int ism330dhcx_gyr_trig_cnt;

static void ism330dhcx_acc_trigger_handler(const struct device *dev,
					   const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	ism330dhcx_acc_trig_cnt++;
}

static void ism330dhcx_gyr_trigger_handler(const struct device *dev,
					   const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	ism330dhcx_gyr_trig_cnt++;
}
#endif

#ifdef CONFIG_IIS2ICLX_TRIGGER
static int iis2iclx_trig_cnt;

static void iis2iclx_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	iis2iclx_trig_cnt++;
}
#endif

static void stts22h_config(const struct device *stts22h)
{
	struct sensor_value odr_attr;

	/* set STTS22H sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(stts22h, SENSOR_CHAN_AMBIENT_TEMP,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for STTS22H\n");
		return;
	}

#ifdef CONFIG_STTS22H_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_AMBIENT_TEMP;
	sensor_trigger_set(stts22h, &trig, stts22h_trigger_handler);
#endif
}

static void iis2dlpc_config(const struct device *iis2dlpc)
{
	struct sensor_value odr_attr, fs_attr;

	/* set IIS2DLPC accel/gyro sampling frequency to 100 Hz */
	odr_attr.val1 = 200;
	odr_attr.val2 = 0;

	if (sensor_attr_set(iis2dlpc, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for IIS2DLPC accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(iis2dlpc, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set full scale for IIS2DLPC accel\n");
		return;
	}

#ifdef CONFIG_IIS2DLPC_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(iis2dlpc, &trig, iis2dlpc_trigger_handler);
#endif
}

static void iis2iclx_config(const struct device *iis2iclx)
{
	struct sensor_value odr_attr, fs_attr;

	/* set IIS2ICLX accel/gyro sampling frequency to 200 Hz */
	odr_attr.val1 = 200;
	odr_attr.val2 = 0;

	if (sensor_attr_set(iis2iclx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for IIS2ICLX accel\n");
		return;
	}

	sensor_g_to_ms2(2, &fs_attr);

	if (sensor_attr_set(iis2iclx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set full scale for IIS2ICLX accel\n");
		return;
	}

#ifdef CONFIG_IIS2ICLX_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(iis2iclx, &trig, iis2iclx_trigger_handler);
#endif
}

static void iis2mdc_config(const struct device *iis2mdc)
{
	struct sensor_value odr_attr;

	/* set IIS2MDC sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(iis2mdc, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for IIS2MDC\n");
		return;
	}

#ifdef CONFIG_IIS2MDC_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_MAGN_XYZ;
	sensor_trigger_set(iis2mdc, &trig, iis2mdc_trigger_handler);
#endif
}

static void ism330dhcx_config(const struct device *ism330dhcx)
{
	struct sensor_value odr_attr, fs_attr;

	/* set ISM330DHCX sampling frequency to 416 Hz */
	odr_attr.val1 = 416;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for ISM330DHCX accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set sampling frequency for ISM330DHCX accel\n");
		return;
	}

	/* set ISM330DHCX gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for ISM330DHCX gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for ISM330DHCX gyro\n");
		return;
	}

#ifdef CONFIG_ISM330DHCX_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_acc_trigger_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_gyr_trigger_handler);
#endif
}

static void ilps22qs_config(const struct device *ilps22qs)
{
	struct sensor_value odr_attr;

	/* set ILPS22QS sampling frequency to 50 Hz */
	odr_attr.val1 = 50;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ilps22qs, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for ILPS22QS\n");
		return;
	}
}

static int led_pattern_out(void)
{
	const struct gpio_dt_spec led0_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
	const struct gpio_dt_spec led1_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
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

	/* output led alternate pattern */
	for (i = 0; i < 8; i++) {
		gpio_pin_set_dt(&led0_gpio, ((i % 2) == 0) ? 1 : 0);
		gpio_pin_set_dt(&led1_gpio, ((i % 2) == 1) ? 1 : 0);
		k_msleep(100);
	}

	/* output led in sync pattern */
	for (i = 0; i < 6; i++) {
		gpio_pin_set_dt(&led0_gpio, ((i % 2) == 0) ? 1 : 0);
		gpio_pin_set_dt(&led1_gpio, ((i % 2) == 0) ? 1 : 0);
		k_msleep(250);
	}

	/* turn all leds off */
	gpio_pin_set_dt(&led0_gpio, 0);
	gpio_pin_set_dt(&led1_gpio, 0);

	return 0;
}

int main(void)
{
	int cnt = 1;

	/* signal that sample is started */
	if (led_pattern_out() < 0) {
		return -1;
	}

	printk("STWIN.box board sensor test\n");

	const struct device *const stts22h = DEVICE_DT_GET_ONE(st_stts22h);
	const struct device *const iis2mdc = DEVICE_DT_GET_ONE(st_iis2mdc);
	const struct device *const ism330dhcx = DEVICE_DT_GET_ONE(st_ism330dhcx);
	const struct device *const iis2dlpc = DEVICE_DT_GET_ONE(st_iis2dlpc);
	const struct device *const iis2iclx = DEVICE_DT_GET_ONE(st_iis2iclx);
	const struct device *const ilps22qs = DEVICE_DT_GET_ONE(st_ilps22qs);

	if (!device_is_ready(stts22h)) {
		printk("%s: device not ready.\n", stts22h->name);
		return 0;
	}
	if (!device_is_ready(iis2mdc)) {
		printk("%s: device not ready.\n", iis2mdc->name);
		return 0;
	}
	if (!device_is_ready(ism330dhcx)) {
		printk("%s: device not ready.\n", ism330dhcx->name);
		return 0;
	}
	if (!device_is_ready(iis2dlpc)) {
		printk("%s: device not ready.\n", iis2dlpc->name);
		return 0;
	}
	if (!device_is_ready(iis2iclx)) {
		printk("%s: device not ready.\n", iis2iclx->name);
		return 0;
	}
	if (!device_is_ready(ilps22qs)) {
		printk("%s: device not ready.\n", ilps22qs->name);
		return 0;
	}

	stts22h_config(stts22h);
	iis2mdc_config(iis2mdc);
	ism330dhcx_config(ism330dhcx);
	iis2dlpc_config(iis2dlpc);
	iis2iclx_config(iis2iclx);
	ilps22qs_config(ilps22qs);

	while (1) {
		struct sensor_value stts22h_temp;
		struct sensor_value iis2dlpc_accel[3];
		struct sensor_value iis2mdc_magn[3];
		struct sensor_value iis2mdc_temp;
		struct sensor_value ism330dhcx_accel[3];
		struct sensor_value ism330dhcx_gyro[3];
		struct sensor_value iis2iclx_accel[2];
		struct sensor_value ilps22qs_press, ilps22qs_temp;

#ifndef CONFIG_STTS22H_TRIGGER
		if (sensor_sample_fetch(stts22h) < 0) {
			printf("STTS22H Sensor sample update error\n");
			return 0;
		}
#endif
#ifndef CONFIG_IIS2MDC_TRIGGER
		if (sensor_sample_fetch(iis2mdc) < 0) {
			printf("IIS2MDC Magn Sensor sample update error\n");
			return 0;
		}
#endif
#ifndef CONFIG_ISM330DHCX_TRIGGER
		if (sensor_sample_fetch(ism330dhcx) < 0) {
			printf("ISM330DHCX IMU Sensor sample update error\n");
			return 0;
		}
#endif
#ifndef CONFIG_IIS2DLPC_TRIGGER
		if (sensor_sample_fetch(iis2dlpc) < 0) {
			printf("IIS2DLPC Sensor sample update error\n");
			return 0;
		}
#endif
#ifndef CONFIG_IIS2ICLX_TRIGGER
		if (sensor_sample_fetch(iis2iclx) < 0) {
			printf("IIS2ICLX Sensor sample update error\n");
			return 0;
		}
#endif

		if (sensor_sample_fetch(ilps22qs) < 0) {
			printf("ILPS22QS Sensor sample update error\n");
			return 0;
		}

		sensor_channel_get(stts22h, SENSOR_CHAN_AMBIENT_TEMP, &stts22h_temp);
		sensor_channel_get(iis2dlpc, SENSOR_CHAN_ACCEL_XYZ, iis2dlpc_accel);
		sensor_channel_get(iis2mdc, SENSOR_CHAN_MAGN_XYZ, iis2mdc_magn);
		sensor_channel_get(iis2mdc, SENSOR_CHAN_DIE_TEMP, &iis2mdc_temp);
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ, ism330dhcx_accel);
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_GYRO_XYZ, ism330dhcx_gyro);
		sensor_channel_get(iis2iclx, SENSOR_CHAN_ACCEL_XYZ, iis2iclx_accel);
		sensor_channel_get(ilps22qs, SENSOR_CHAN_AMBIENT_TEMP, &ilps22qs_temp);
		sensor_channel_get(ilps22qs, SENSOR_CHAN_PRESS, &ilps22qs_press);

		/* Display sensor data */

		/*  Clear terminal (ANSI ESC-C) */
		printf("\0033\014");

		printf("STWIN.box dashboard\n\n");

		/* STTS22H temperature */
		printf("STTS22H: Temperature: %.1f C\n",
		       sensor_value_to_double(&stts22h_temp));

		/* iis2dlpc */
		printf("IIS2DLPC: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&iis2dlpc_accel[0]),
			sensor_value_to_double(&iis2dlpc_accel[1]),
			sensor_value_to_double(&iis2dlpc_accel[2]));

		/* iis2mdc */
		printf("IIS2MDC: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&iis2mdc_magn[0]),
		       sensor_value_to_double(&iis2mdc_magn[1]),
		       sensor_value_to_double(&iis2mdc_magn[2]));

		printf("IIS2MDC: Temperature: %.1f C\n",
		       sensor_value_to_double(&iis2mdc_temp));

		/* ism330dhcx */
		printf("ISM330DHCX: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&ism330dhcx_accel[0]),
			sensor_value_to_double(&ism330dhcx_accel[1]),
			sensor_value_to_double(&ism330dhcx_accel[2]));

		printf("ISM330DHCX: Gyro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&ism330dhcx_gyro[0]),
			sensor_value_to_double(&ism330dhcx_gyro[1]),
			sensor_value_to_double(&ism330dhcx_gyro[2]));

		/* iis2iclx */
		printf("IIS2ICLX: Accel (m.s-2): x: %.3f, y: %.3f\n",
			sensor_value_to_double(&iis2iclx_accel[0]),
			sensor_value_to_double(&iis2iclx_accel[1]));

		/* temperature */
		printf("ILPS22QS: Temperature: %.1f C\n",
		       sensor_value_to_double(&ilps22qs_temp));

		/* pressure */
		printf("ILPS22QS: Pressure: %.3f kpa\n",
		       sensor_value_to_double(&ilps22qs_press));

#ifdef CONFIG_STTS22H_TRIGGER
		printk("%d:: stts22h trig %d\n", cnt, stts22h_trig_cnt);
#endif

#ifdef CONFIG_IIS2DLPC_TRIGGER
		printk("%d:: iis2dlpc trig %d\n", cnt, iis2dlpc_trig_cnt);
#endif

#if defined(CONFIG_IIS2MDC_TRIGGER)
		printk("%d:: iis2mdc trig %d\n", cnt, iis2mdc_trig_cnt);
#endif

#ifdef CONFIG_ISM330DHCX_TRIGGER
		printk("%d:: ism330dhcx acc trig %d\n", cnt, ism330dhcx_acc_trig_cnt);
		printk("%d:: ism330dhcx gyr trig %d\n", cnt, ism330dhcx_gyr_trig_cnt);
#endif

#ifdef CONFIG_IIS2ICLX_TRIGGER
		printk("%d:: iis2iclx trig %d\n", cnt, iis2iclx_trig_cnt);
#endif

		k_msleep(2000);
	}
}
