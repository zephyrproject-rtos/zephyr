/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_vtm.h>
#include <zephyr/sys/printk.h>

static const struct device *const vtm_temp = DEVICE_DT_GET(DT_ALIAS(vtm_temp0));

static const struct sensor_trigger th0_trig = {
	.type = TI_VTM_TRIG_TH0,
	.chan = SENSOR_CHAN_DIE_TEMP,
};

static const struct sensor_trigger th1_trig = {
	.type = TI_VTM_TRIG_TH1,
	.chan = SENSOR_CHAN_DIE_TEMP,
};

static const struct sensor_trigger th2_trig = {
	.type = TI_VTM_TRIG_TH2,
	.chan = SENSOR_CHAN_DIE_TEMP,
};

static void th0_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);
	printk("[TH0] cold threshold crossed\n");
}

static void th1_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);
	printk("[TH1] hot threshold crossed\n");
}

static void th2_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);
	printk("[TH2] critical threshold crossed\n");
}

static struct sensor_value mc_to_val(int32_t temp_mc)
{
	struct sensor_value val = {
		.val1 = temp_mc / 1000,
		.val2 = (temp_mc % 1000) * 1000,
	};

	return val;
}

static int read_temperature(int32_t *temp_mc)
{
	struct sensor_value val;
	int ret;

	ret = sensor_sample_fetch(vtm_temp);
	if (ret < 0) {
		printk("Failed to fetch sample (%d)\n", ret);
		return ret;
	}

	ret = sensor_channel_get(vtm_temp, SENSOR_CHAN_DIE_TEMP, &val);
	if (ret < 0) {
		printk("Failed to get channel (%d)\n", ret);
		return ret;
	}

	*temp_mc = val.val1 * 1000 + val.val2 / 1000;
	printk("VTM temperature: %.2f C\n", sensor_value_to_double(&val));

	return 0;
}

static int arm_threshold(enum sensor_attribute attr, const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler, int32_t temp_mc)
{
	struct sensor_value val = mc_to_val(temp_mc);
	int ret;

	ret = sensor_attr_set(vtm_temp, SENSOR_CHAN_DIE_TEMP, attr, &val);
	if (ret < 0) {
		printk("Failed to set threshold attr %d (%d)\n", attr, ret);
		return ret;
	}

	ret = sensor_trigger_set(vtm_temp, trig, handler);
	if (ret < 0) {
		printk("Failed to set trigger %d (%d)\n", trig->type, ret);
		return ret;
	}

	printk("Armed threshold at %.2f C\n", sensor_value_to_double(&val));
	return 0;
}

int main(void)
{
	int32_t temp_mc;

	if (!device_is_ready(vtm_temp)) {
		printk("VTM sensor device not ready\n");
		return 0;
	}

	if (read_temperature(&temp_mc) < 0) {
		return 0;
	}

	arm_threshold(TI_VTM_ATTR_TH0_THRESH, &th0_trig, th0_handler, temp_mc - 5000);
	arm_threshold(TI_VTM_ATTR_TH1_THRESH, &th1_trig, th1_handler, temp_mc + 3000);
	arm_threshold(TI_VTM_ATTR_TH2_THRESH, &th2_trig, th2_handler, temp_mc + 8000);

	while (1) {
		read_temperature(&temp_mc);
		k_msleep(1000);
	}

	return 0;
}
