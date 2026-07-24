/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "versal_sysmon.h"

static const struct device *sysmon_dev = DEVICE_DT_GET(DT_NODELABEL(sysmon0));
static enum sensor_channel chan_to_use;
static struct k_sem irq_sem;
static atomic_t trigger_handler_called;

static void sysmon_test_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);
	TC_PRINT("%s fired\n", __func__);
	atomic_set(&trigger_handler_called, 1);
	k_sem_give(&irq_sem);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_true(device_is_ready(sysmon_dev), "Device %s is not ready.", sysmon_dev->name);
	chan_to_use = (enum sensor_channel)VERSAL_SYSMON_VCCINT;
	k_sem_init(&irq_sem, 0, 1);
	atomic_clear(&trigger_handler_called);

	while (k_sem_take(&irq_sem, K_NO_WAIT) == 0) {
	}
}

ZTEST(sysmon, test_read_supply_channels)
{
	int rc;
	struct sensor_value val;

	rc = sensor_sample_fetch(sysmon_dev);
	zassert_ok(rc, "sample_fetch failed: %d", rc);
	rc = sensor_channel_get(sysmon_dev, chan_to_use, &val);
	zassert_ok(rc, "Cannot read from channel %d: %d.", chan_to_use, rc);
	TC_PRINT("supply voltage: %d\n", val.val1);
	rc = sensor_attr_get(sysmon_dev, chan_to_use, SENSOR_ATTR_UPPER_THRESH, &val);
	TC_PRINT("SENSOR_ATTR_UPPER_THRESH: %d\n", val.val1);
	zassert_ok(rc, "Failed to GET threshold: %d", rc);
	rc = sensor_attr_get(sysmon_dev, chan_to_use, SENSOR_ATTR_LOWER_THRESH, &val);
	TC_PRINT("SENSOR_ATTR_LOWER_THRESH: %d\n", val.val1);
	zassert_ok(rc, "Failed to GET threshold: %d", rc);
}

ZTEST(sysmon, test_th_temp_interrupt)
{
	int rc;
	struct sensor_value cur;
	struct sensor_value upper;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = (enum sensor_channel)VERSAL_SYSMON_DIE_TEMP,
	};

	atomic_clear(&trigger_handler_called);
	k_sem_reset(&irq_sem);

	rc = sensor_sample_fetch(sysmon_dev);
	zassert_ok(rc, "sample_fetch failed: %d", rc);

	rc = sensor_channel_get(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, &cur);
	zassert_ok(rc, "channel_get failed: %d", rc);
	TC_PRINT("Current DIE TEMP (raw units): %d\n", cur.val1);

	rc = sensor_trigger_set(sysmon_dev, &trig, sysmon_test_handler);
	zassert_ok(rc, "Failed to set trigger: %d", rc);

	upper.val1 = cur.val1 - 1000;
	upper.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_UPPER_THRESH, &upper);
	zassert_ok(rc, "Failed to get higher threshold: %d", rc);
	TC_PRINT(" set upper threshold to current temp: %d\n", upper.val1);

	upper.val1 = 25000;
	upper.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_LOWER_THRESH, &upper);
	zassert_ok(rc, "Failed to set lower threshold: %d", rc);
	TC_PRINT(" set lower threshold to current temp: %d\n", upper.val1);

	rc = sensor_attr_get(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_UPPER_THRESH, &upper);
	zassert_ok(rc, "Failed to get higher threshold: %d", rc);
	TC_PRINT(" get upper threshold to current temp: %d\n", upper.val1);

	rc = sensor_attr_get(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_LOWER_THRESH, &upper);
	zassert_ok(rc, "Failed to get lower threshold: %d", rc);
	TC_PRINT(" get lower threshold to current temp: %d\n", upper.val1);

	struct sensor_value en = {.val1 = 1, .val2 = 0};

	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_ALERT, &en);
	zassert_ok(rc, "Failed to enable alert: %d", rc);

	rc = k_sem_take(&irq_sem, K_SECONDS(5));
	zassert_ok(rc, "Temperature interrupt did not arrive");

	zassert_true(atomic_get(&trigger_handler_called), "Temperature trigger handler not called");

	rc = sensor_trigger_set(sysmon_dev, &trig, NULL);
	zassert_equal(rc, 0, "trigger disable failed: %d", rc);

	struct sensor_value dis = {.val1 = 0, .val2 = 0};

	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_ALERT, &dis);
	zassert_ok(rc, "Failed to disable alert: %d", rc);

	rc = sensor_sample_fetch(sysmon_dev);
	zassert_ok(rc, "sample_fetch failed: %d", rc);

	rc = sensor_channel_get(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, &cur);
	zassert_ok(rc, "channel_get failed: %d", rc);
	TC_PRINT("Current DIE TEMP (raw units): %d\n", cur.val1);

	upper.val1 = cur.val1 + 10000;
	upper.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_UPPER_THRESH, &upper);
	zassert_ok(rc, "Failed to get higher threshold: %d", rc);
	TC_PRINT(" set upper threshold to current temp: %d\n", upper.val1);
}

ZTEST(sysmon, test_ot_temp_interrupt)
{
	int rc;
	struct sensor_value cur;
	struct sensor_value ot_up_old, ot_lo_old;
	struct sensor_value ot_up_new, ot_lo_new;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = (enum sensor_channel)VERSAL_SYSMON_DIE_TEMP_OT,
	};

	atomic_clear(&trigger_handler_called);
	k_sem_reset(&irq_sem);

	rc = sensor_sample_fetch(sysmon_dev);
	zassert_ok(rc, "sample_fetch failed: %d", rc);

	rc = sensor_channel_get(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, &cur);
	zassert_ok(rc, "channel_get failed: %d", rc);
	TC_PRINT("Current DIE TEMP: %d\n", cur.val1);

	rc = sensor_trigger_set(sysmon_dev, &trig, sysmon_test_handler);
	zassert_ok(rc, "Failed to set trigger: %d", rc);

	ot_up_new.val1 = cur.val1 - 5000;
	ot_up_new.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_UPPER_THRESH_OT,
			     &ot_up_new);
	zassert_ok(rc, "Failed to set OT upper threshold: %d", rc);

	ot_lo_new.val1 = cur.val1 - 10000;
	ot_lo_new.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_LOWER_THRESH_OT,
			     &ot_lo_new);
	zassert_ok(rc, "Failed to set OT lower threshold: %d", rc);

	struct sensor_value en = {.val1 = 1, .val2 = 0};

	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_ALERT, &en);
	zassert_ok(rc, "Failed to enable alert: %d", rc);

	rc = sensor_attr_get(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_UPPER_THRESH_OT,
			     &ot_up_old);
	zassert_ok(rc, "Failed to get OT upper threshold: %d", rc);
	TC_PRINT("OT upper threshold: %d\n", ot_up_old.val1);

	rc = sensor_attr_get(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_LOWER_THRESH_OT,
			     &ot_lo_old);
	zassert_ok(rc, "Failed to get OT lower threshold: %d", rc);
	TC_PRINT("OT lower threshold: %d\n", ot_lo_old.val1);

	rc = k_sem_take(&irq_sem, K_SECONDS(5));
	zassert_ok(rc, "OT interrupt did not arrive");

	zassert_true(atomic_get(&trigger_handler_called), "OT trigger handler not called");

	rc = sensor_trigger_set(sysmon_dev, &trig, NULL);
	zassert_equal(rc, 0, "trigger disable failed: %d", rc);

	struct sensor_value dis = {.val1 = 0, .val2 = 0};

	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_ALERT, &dis);
	zassert_ok(rc, "Failed to disable alert: %d", rc);

	ot_up_new.val1 = cur.val1 + 10000;
	ot_up_new.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_UPPER_THRESH_OT,
			     &ot_up_new);
	zassert_ok(rc, "Failed to set OT upper threshold: %d", rc);

	ot_lo_new.val1 = cur.val1 + 5000;
	ot_lo_new.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, VERSAL_SYSMON_DIE_TEMP, SENSOR_ATTR_LOWER_THRESH_OT,
			     &ot_lo_new);
	zassert_ok(rc, "Failed to set OT lower threshold: %d", rc);
}

ZTEST(sysmon, test_supply_chan_interrupts)
{
	int rc;

	chan_to_use = (enum sensor_channel)VERSAL_SYSMON_GTY_AVTT_202;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = chan_to_use,
	};
	struct sensor_value cur;

	atomic_clear(&trigger_handler_called);
	k_sem_reset(&irq_sem);

	rc = sensor_sample_fetch(sysmon_dev);
	zassert_ok(rc, "sample_fetch failed: %d", rc);

	rc = sensor_channel_get(sysmon_dev, chan_to_use, &cur);
	zassert_ok(rc, "channel_get failed: %d", rc);
	TC_PRINT("Current  (raw/driver units): %d\n", cur.val1);

	struct sensor_value upper = {.val1 = cur.val1 + 300, .val2 = 0};

	rc = sensor_attr_set(sysmon_dev, chan_to_use, SENSOR_ATTR_UPPER_THRESH, &upper);
	zassert_ok(rc, "Failed to set upper threshold: %d", rc);

	cur.val1 = 100;
	cur.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, chan_to_use, SENSOR_ATTR_LOWER_THRESH, &cur);
	zassert_ok(rc, "Failed to set lower threshold: %d", rc);

	rc = sensor_attr_get(sysmon_dev, chan_to_use, SENSOR_ATTR_LOWER_THRESH, &cur);
	zassert_ok(rc, "Failed to GET threshold: %d", rc);
	TC_PRINT("SENSOR_ATTR_LOWER_THRESH: %d\n", cur.val1);

	rc = sensor_attr_get(sysmon_dev, chan_to_use, SENSOR_ATTR_UPPER_THRESH, &cur);
	zassert_ok(rc, "Failed to GET threshold: %d", rc);
	TC_PRINT("SENSOR_ATTR_UPPER_THRESH: %d\n", cur.val1);

	rc = sensor_trigger_set(sysmon_dev, &trig, sysmon_test_handler);
	zassert_ok(rc, "Failed to set trigger: %d", rc);

	struct sensor_value en = {.val1 = 1, .val2 = 0};

	rc = sensor_attr_set(sysmon_dev, chan_to_use, SENSOR_ATTR_ALERT, &en);
	zassert_ok(rc, "Failed to enable alert: %d", rc);

	upper.val1 = 0;
	upper.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, chan_to_use, SENSOR_ATTR_UPPER_THRESH, &upper);
	zassert_ok(rc, "Failed to set upper threshold: %d", rc);

	rc = k_sem_take(&irq_sem, K_SECONDS(1));
	zassert_ok(rc, "Threshold interrupt did not arrive");

	zassert_true(atomic_get(&trigger_handler_called) == 1, "Handler not called");

	rc = sensor_trigger_set(sysmon_dev, &trig, NULL);
	zassert_equal(rc, 0, "trigger disable failed: %d", rc);

	struct sensor_value dis = {.val1 = 0, .val2 = 0};

	rc = sensor_attr_set(sysmon_dev, chan_to_use, SENSOR_ATTR_ALERT, &dis);
	zassert_ok(rc, "Failed to disable alert: %d", rc);

	rc = sensor_sample_fetch(sysmon_dev);
	zassert_ok(rc, "sample_fetch failed: %d", rc);

	rc = sensor_channel_get(sysmon_dev, chan_to_use, &cur);
	zassert_ok(rc, "channel_get failed: %d", rc);
	TC_PRINT("Current  (raw/driver units): %d\n", cur.val1);

	upper.val1 = cur.val1 + 300;
	upper.val2 = 0;
	rc = sensor_attr_set(sysmon_dev, chan_to_use, SENSOR_ATTR_UPPER_THRESH, &upper);
	zassert_ok(rc, "Failed to set upper threshold: %d", rc);
}

ZTEST_SUITE(sysmon, NULL, NULL, before, NULL, NULL);
