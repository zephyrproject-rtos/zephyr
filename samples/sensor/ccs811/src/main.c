/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 * Copyright (c) 2018 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <stdio.h>

static bool app_fw_2;

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}

static int do_fetch(const struct device *dev)
{
	struct sensor_value co2, tvoc, voltage, current;
	int rc = 0;
	int baseline = -1;

#ifdef CONFIG_APP_MONITOR_BASELINE
	rc = ccs811_baseline_fetch(dev);
	if (rc >= 0) {
		baseline = rc;
		rc = 0;
	}
#endif
	if (rc == 0) {
		rc = sensor_sample_fetch(dev);
	}
	if (rc == 0) {
		const struct ccs811_result_type *rp = ccs811_result(dev);

		sensor_channel_get(dev, SENSOR_CHAN_CO2, &co2);
		sensor_channel_get(dev, SENSOR_CHAN_VOC, &tvoc);
		sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &voltage);
		sensor_channel_get(dev, SENSOR_CHAN_CURRENT, &current);
		printk("\n[%s]: CCS811: %u ppm eCO2; %u ppb eTVOC\n",
		       now_str(), co2.val1, tvoc.val1);
		printk("Voltage: %d.%06dV; Current: %d.%06dA\n", voltage.val1,
		       voltage.val2, current.val1, current.val2);
#ifdef CONFIG_APP_MONITOR_BASELINE
		printk("BASELINE %04x\n", baseline);
#endif
		if (app_fw_2 && !(rp->status & CCS811_STATUS_DATA_READY)) {
			printk("STALE DATA\n");
		}

		if (rp->status & CCS811_STATUS_ERROR) {
			printk("ERROR: %02x\n", rp->error);
		}
	}
	return rc;
}

#ifndef CONFIG_CCS811_TRIGGER_NONE

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	int rc = do_fetch(dev);

	if (rc == 0) {
		printk("Triggered fetch got %d\n", rc);
	} else if (-EAGAIN == rc) {
		printk("Triggered fetch got stale data\n");
	} else {
		printk("Triggered fetch failed: %d\n", rc);
	}
}

#endif /* !CONFIG_CCS811_TRIGGER_NONE */

static void do_main(const struct device *dev)
{
	while (true) {
		int rc = do_fetch(dev);

		if (rc == 0) {
			printk("Timed fetch got %d\n", rc);
		} else if (-EAGAIN == rc) {
			printk("Timed fetch got stale data\n");
		} else {
			printk("Timed fetch failed: %d\n", rc);
			break;
		}
		k_msleep(1000);
	}
}

void main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(ams_ccs811);
	struct ccs811_configver_type cfgver;
	int rc;

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->name);

	rc = ccs811_configver_fetch(dev, &cfgver);
	if (rc == 0) {
		printk("HW %02x; FW Boot %04x App %04x ; mode %02x\n",
		       cfgver.hw_version, cfgver.fw_boot_version,
		       cfgver.fw_app_version, cfgver.mode);
		app_fw_2 = (cfgver.fw_app_version >> 8) > 0x11;
	}

#ifdef CONFIG_APP_USE_ENVDATA
	struct sensor_value temp = { CONFIG_APP_ENV_TEMPERATURE };
	struct sensor_value humidity = { CONFIG_APP_ENV_HUMIDITY };

	rc = ccs811_envdata_update(dev, &temp, &humidity);
	printk("ENV_DATA set for %d Cel, %d %%RH got %d\n",
	       temp.val1, humidity.val1, rc);
#endif

#ifdef CONFIG_CCS811_TRIGGER
	struct sensor_trigger trig = { 0 };

#ifdef CONFIG_APP_TRIGGER_ON_THRESHOLD
	printk("Triggering on threshold:\n");
	if (rc == 0) {
		struct sensor_value thr = {
			.val1 = CONFIG_APP_CO2_MEDIUM_PPM,
		};
		rc = sensor_attr_set(dev, SENSOR_CHAN_CO2,
				     SENSOR_ATTR_LOWER_THRESH,
				     &thr);
		printk("L/M threshold to %d got %d\n", thr.val1, rc);
	}
	if (rc == 0) {
		struct sensor_value thr = {
			.val1 = CONFIG_APP_CO2_HIGH_PPM,
		};
		rc = sensor_attr_set(dev, SENSOR_CHAN_CO2,
				     SENSOR_ATTR_UPPER_THRESH,
				     &thr);
		printk("M/H threshold to %d got %d\n", thr.val1, rc);
	}
	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_CO2;
#elif defined(CONFIG_APP_TRIGGER_ON_DATAREADY)
	printk("Triggering on data ready\n");
	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ALL;
#else
#error Unhandled trigger on
#endif
	if (rc == 0) {
		rc = sensor_trigger_set(dev, &trig, trigger_handler);
	}
	if (rc == 0) {
#ifdef CONFIG_APP_TRIGGER_ON_DATAREADY
		while (true) {
			k_sleep(K_FOREVER);
		}
#endif
	}
	printk("Trigger installation got: %d\n", rc);
#endif /* CONFIG_CCS811_TRIGGER */
	if (rc == 0) {
		do_main(dev);
	}
}
