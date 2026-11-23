/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device_runtime.h>

/**
 * Structure grouping gpio pins used for QENC emulation connected with a QDEC device
 * with a loopback.
 */
struct qdec_qenc_loopback {
	struct gpio_dt_spec qenc_phase_a;
	struct gpio_dt_spec qenc_phase_b;

	const struct device *qdec;
	uint32_t qdec_config_step;
};

static K_SEM_DEFINE(sem, 0, 1);

#define GET_QDEC_QENC_LOOPBACK(x)								   \
	{											   \
		.qenc_phase_a = GPIO_DT_SPEC_GET(DT_PHANDLE_BY_IDX(x, qenc_emul_gpios, 0), gpios), \
		.qenc_phase_b = GPIO_DT_SPEC_GET(DT_PHANDLE_BY_IDX(x, qenc_emul_gpios, 1), gpios), \
		.qdec = DEVICE_DT_GET(DT_PHANDLE(x, qdec)),					   \
		.qdec_config_step = DT_PROP_BY_PHANDLE(x, qdec, steps)				   \
	}


struct qdec_qenc_loopback loopbacks[] = {
	DT_FOREACH_CHILD_SEP(DT_NODELABEL(qdec_loopbacks), GET_QDEC_QENC_LOOPBACK, (,))
};

#define TESTED_QDEC_COUNT ARRAY_SIZE(loopbacks)

static struct sensor_trigger qdec_trigger = {.type = SENSOR_TRIG_DATA_READY,
					     .chan = SENSOR_CHAN_ROTATION};

static bool toggle_a = true;
static struct qdec_qenc_loopback *loopback_currently_under_test;

static void qdec_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	zassert_not_null(dev);
	zassert_not_null(trigger);
	/* trigger should be stored as a pointer in a driver
	 * thus this address should match
	 */
	zassert_equal_ptr(trigger, &qdec_trigger);

	k_sem_give(&sem);
}

static void qenc_emulate_work_handler(struct k_work *work)
{
	/* Check if emulation has been stopped after submitting this work item. */
	if (loopback_currently_under_test == NULL) {
		return;
	}

	if (toggle_a) {
		gpio_pin_toggle_dt(&loopback_currently_under_test->qenc_phase_a);
	} else {
		gpio_pin_toggle_dt(&loopback_currently_under_test->qenc_phase_b);
	}
	toggle_a = !toggle_a;
}

static K_WORK_DEFINE(qenc_emulate_work, qenc_emulate_work_handler);

static void qenc_emulate_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&qenc_emulate_work);
}

static K_TIMER_DEFINE(qenc_emulate_timer, qenc_emulate_timer_handler, NULL);

static void qenc_emulate_reset_pin(const struct gpio_dt_spec *gpio_dt)
{
	int rc;

	rc = gpio_pin_set_dt(gpio_dt, 0);
	zassert_ok(rc, "%s: pin set failed: %d", gpio_dt->port->name, rc);
}

static void qenc_emulate_setup_pin(const struct gpio_dt_spec *gpio_dt)
{
	int rc;

	rc = gpio_is_ready_dt(gpio_dt);
	zassert_true(rc, "%s: device not ready: %d", gpio_dt->port->name, rc);

	rc = gpio_pin_configure_dt(gpio_dt, GPIO_OUTPUT);
	zassert_true(rc == 0, "%s: pin configure failed: %d", gpio_dt->port->name, rc);
}

static void qenc_emulate_start(struct qdec_qenc_loopback *loopback, k_timeout_t period,
		bool forward)
{
	qenc_emulate_reset_pin(&loopback->qenc_phase_a);
	qenc_emulate_reset_pin(&loopback->qenc_phase_b);

	toggle_a = !forward;
	loopback_currently_under_test = loopback;
	k_timer_start(&qenc_emulate_timer, period, period);
}

static void qenc_emulate_stop(void)
{
	if (loopback_currently_under_test) {
		k_timer_stop(&qenc_emulate_timer);
		qenc_emulate_reset_pin(&loopback_currently_under_test->qenc_phase_a);
		qenc_emulate_reset_pin(&loopback_currently_under_test->qenc_phase_b);

		loopback_currently_under_test = NULL;
	}
}

static void qenc_emulate_verify_reading(struct qdec_qenc_loopback *loopback,
		int emulator_period_ms, int emulation_duration_ms, bool forward,
		bool overflow_expected)
{
	int rc;
	struct sensor_value val = {0};
	int32_t expected_steps = emulation_duration_ms / emulator_period_ms;
	int32_t expected_reading = 360 * expected_steps / loopback->qdec_config_step;
	int32_t delta = expected_reading / 4;

	if (!forward) {
		expected_reading *= -1;
	}

	qenc_emulate_start(loopback, K_MSEC(emulator_period_ms), forward);

	/* wait for some readings */
	k_msleep(emulation_duration_ms);

	rc = sensor_sample_fetch(loopback->qdec);

	if (!overflow_expected) {
		zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);
	} else {
		zassert_true(rc == -EOVERFLOW, "Failed to detect overflow");
	}

	rc = sensor_channel_get(loopback->qdec, SENSOR_CHAN_ROTATION, &val);
	zassert_true(rc == 0, "Failed to get sample (%d)", rc);

	TC_PRINT("Expected reading: %d, actual value: %d, delta: %d\n",
			expected_reading, val.val1, delta);
	if (!overflow_expected) {
		zassert_within(val.val1, expected_reading, delta,
			       "Expected reading: %d,  but got: %d", expected_reading, val.val1);
	}

	qenc_emulate_stop();

	/* wait and get readings to clear state */
	k_msleep(100);

	rc = sensor_sample_fetch(loopback->qdec);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_channel_get(loopback->qdec, SENSOR_CHAN_ROTATION, &val);
	zassert_true(rc == 0, "Failed to get sample (%d)", rc);
}

static void sensor_trigger_set_and_disable(struct qdec_qenc_loopback *loopback)
{
	int rc;

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(loopback->qdec);
	}

	k_sem_give(&sem);

	qdec_trigger.type = SENSOR_TRIG_DATA_READY;
	qdec_trigger.chan = SENSOR_CHAN_ALL;
	rc = sensor_trigger_set(loopback->qdec, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc != -ENOSYS, "sensor_trigger_set not supported");

	zassert_true(rc == 0, "sensor_trigger_set failed: %d", rc);

	qenc_emulate_start(loopback, K_MSEC(10), true);

	/* emulation working, handler should be called */
	rc = k_sem_take(&sem, K_MSEC(200));
	zassert_true(rc == 0, "qdec handler should be triggered (%d)", rc);

	qenc_emulate_stop();

	/* emulation not working, but there maybe old trigger, ignore */
	rc = k_sem_take(&sem, K_MSEC(200));

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(loopback->qdec);
	}

	/* there should be no triggers now*/
	rc = k_sem_take(&sem, K_MSEC(200));
	zassert_true(rc == -EAGAIN, "qdec handler should not be triggered (%d)", rc);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(loopback->qdec);
	}

	/* register empty trigger - disable trigger */
	rc = sensor_trigger_set(loopback->qdec, &qdec_trigger, NULL);
	zassert_true(rc == 0, "sensor_trigger_set failed: %d", rc);

	qenc_emulate_start(loopback, K_MSEC(10), true);

	/* emulation working, but handler not set, thus should not be called */
	rc = k_sem_take(&sem, K_MSEC(200));
	zassert_true(rc == -EAGAIN, "qdec handler should not be triggered (%d)", rc);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(loopback->qdec);
	}

	qenc_emulate_stop();
	k_sem_reset(&sem);
}

/**
 * @brief sensor_trigger_set test disable trigger
 *
 * Confirm trigger happens after set and stops after being disabled
 *
 */
ZTEST(qdec_sensor, test_sensor_trigger_set_and_disable)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		TC_PRINT("Testing QDEC index %d, address: %p\n", i, loopbacks[i].qdec);
		sensor_trigger_set_and_disable(&loopbacks[i]);
	}
}

static void sensor_trigger_set_test(struct qdec_qenc_loopback *loopback)
{
	int rc;
	struct sensor_value val = {0};

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(loopback->qdec);
	}

	qdec_trigger.type = SENSOR_TRIG_DATA_READY;
	qdec_trigger.chan = SENSOR_CHAN_ROTATION;
	rc = sensor_trigger_set(loopback->qdec, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc != -ENOSYS, "sensor_trigger_set not supported");

	zassert_true(rc == 0, "sensor_trigger_set failed: %d", rc);

	qenc_emulate_start(loopback, K_MSEC(10), true);

	/* emulation working now */
	rc = k_sem_take(&sem, K_MSEC(200));
	zassert_true(rc == 0, "qdec handler should be triggered (%d)", rc);

	rc = sensor_sample_fetch(loopback->qdec);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_channel_get(loopback->qdec, SENSOR_CHAN_ROTATION, &val);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	TC_PRINT("QDEC reading: %d\n", val.val1);
	zassert_true(val.val1 != 0, "No readings from QDEC");

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(loopback->qdec);
	}

	qenc_emulate_stop();
	k_sem_reset(&sem);
}

/**
 * @brief sensor_trigger_set test
 *
 * Confirm trigger happens after set
 *
 */
ZTEST(qdec_sensor, test_sensor_trigger_set)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		TC_PRINT("Testing QDEC index %d, address: %p\n", i, loopbacks[i].qdec);
		sensor_trigger_set_test(&loopbacks[i]);
	}
}

static void sensor_trigger_set_negative(struct qdec_qenc_loopback *loopback)
{
	int rc;

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(loopback->qdec);
	}

	rc = sensor_trigger_set(loopback->qdec, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc != -ENOSYS, "sensor_trigger_set not supported");

	qdec_trigger.type = SENSOR_TRIG_MAX;
	qdec_trigger.chan = SENSOR_CHAN_ROTATION;

	rc = sensor_trigger_set(loopback->qdec, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc < 0, "sensor_trigger_set should fail due to invalid trigger type");

	qdec_trigger.type = SENSOR_TRIG_DATA_READY;
	qdec_trigger.chan = SENSOR_CHAN_MAX;

	rc = sensor_trigger_set(loopback->qdec, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc < 0, "sensor_trigger_set should fail due to invalid channel");

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(loopback->qdec);
	}
}

/**
 * @brief sensor_trigger_set test negative
 *
 * Confirm setting trigger with invalid data does not work
 *
 */
ZTEST(qdec_sensor, test_sensor_trigger_set_negative)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		TC_PRINT("Testing QDEC index %d, address: %p\n", i, loopbacks[i].qdec);
		sensor_trigger_set_negative(&loopbacks[i]);
	}
}

/**
 * @brief QDEC readings tests
 *
 * Valid reading from QDEC base on simulated signal
 *
 */
ZTEST(qdec_sensor, test_qdec_readings)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
			pm_device_runtime_get(loopbacks[i].qdec);
		}

		TC_PRINT("Testing QDEC index %d, address: %p\n", i, loopbacks[i].qdec);
		qenc_emulate_verify_reading(&loopbacks[i], 10,  100,  true, false);
		qenc_emulate_verify_reading(&loopbacks[i],  2,  500,  true, false);
		qenc_emulate_verify_reading(&loopbacks[i], 10,  200, false, false);
		qenc_emulate_verify_reading(&loopbacks[i],  1, 1000, false,  true);
		qenc_emulate_verify_reading(&loopbacks[i],  1, 1000,  true,  true);

		if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
			pm_device_runtime_put(loopbacks[i].qdec);
		}
	}
}

static void sensor_channel_get_empty(const struct device *const dev)
{
	int rc;
	struct sensor_value val = {0};

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	}

	/* wait for potential new readings */
	k_msleep(100);

	rc = sensor_sample_fetch(dev);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	/* get readings but ignore them, as they may include reading from time
	 * when emulation was still working (i.e. during previous test)
	 */
	rc = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
	zassert_true(rc == 0, "Failed to get sample (%d)", rc);

	/* wait for potential new readings */
	k_msleep(100);

	rc = sensor_sample_fetch(dev);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	/* emulation was not working, expect no readings */
	rc = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
	zassert_true(rc == 0, "Failed to get sample (%d)", rc);
	zassert_true(val.val1 == 0, "Expected no readings but got: %d", val.val1);
	zassert_true(val.val2 == 0, "Expected no readings but got: %d", val.val2);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
	}
}

/**
 * @brief sensor_channel_get test with no emulation
 *
 * Confirm getting empty reading from QDEC
 *
 */
ZTEST(qdec_sensor, test_sensor_channel_get_empty)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		TC_PRINT("Testing QDEC index %d, address: %p\n", i, loopbacks[i].qdec);
		sensor_channel_get_empty(loopbacks[i].qdec);
	}
}

static void sensor_channel_get_test(struct qdec_qenc_loopback *loopback)
{
	int rc;
	struct sensor_value val_first = {0};
	struct sensor_value val_second = {0};

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(loopback->qdec);
	}

	qenc_emulate_start(loopback, K_MSEC(10), true);

	/* wait for some readings*/
	k_msleep(100);

	rc = sensor_sample_fetch(loopback->qdec);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_channel_get(loopback->qdec, SENSOR_CHAN_ROTATION, &val_first);
	zassert_true(rc == 0, "Failed to get sample (%d)", rc);
	zassert_true(val_first.val1 != 0, "No readings from QDEC");

	/* wait for more readings*/
	k_msleep(200);

	rc = sensor_channel_get(loopback->qdec, SENSOR_CHAN_ROTATION, &val_second);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);
	zassert_true(val_second.val1 != 0, "No readings from QDEC");

	/* subsequent calls of sensor_channel_get without calling sensor_sample_fetch
	 * should yield the same value
	 */
	zassert_true(val_first.val1 == val_second.val1,
		     "Expected the same readings: %d vs %d",
		     val_first.val1,
		     val_second.val1);

	zassert_true(val_first.val2 == val_second.val2,
		     "Expected the same readings: %d vs %d",
		     val_first.val2,
		     val_second.val2);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(loopback->qdec);
	}
}

/**
 * @brief sensor_channel_get test with emulation
 *
 * Confirm getting readings from QDEC
 *
 */
ZTEST(qdec_sensor, test_sensor_channel_get)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		TC_PRINT("Testing QDEC index %d, address: %p\n", i, loopbacks[i].qdec);
		sensor_channel_get_test(&loopbacks[i]);
	}
}

static void sensor_channel_get_negative(struct qdec_qenc_loopback *loopback)
{
	int rc;
	struct sensor_value val = {0};

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(loopback->qdec);
	}

	qenc_emulate_start(loopback, K_MSEC(10), true);

	/* wait for some readings*/
	k_msleep(100);

	rc = sensor_sample_fetch(loopback->qdec);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_channel_get(loopback->qdec, SENSOR_CHAN_MAX, &val);
	zassert_true(rc < 0, "Should failed to get sample (%d)", rc);
	zassert_true(val.val1 == 0, "Some readings from QDEC: %d", val.val1);
	zassert_true(val.val2 == 0, "Some readings from QDEC: %d", val.val2);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(loopback->qdec);
	}
}

/**
 * @brief sensor_channel_get test negative
 *
 * Confirm getting readings from QDEC with invalid channel
 *
 */
ZTEST(qdec_sensor, test_sensor_channel_get_negative)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		TC_PRINT("Testing QDEC index %d, address: %p\n", i, loopbacks[i].qdec);
		sensor_channel_get_negative(&loopbacks[i]);
	}
}

static void sensor_sample_fetch_test(const struct device *const dev)
{
	int rc;

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	}

	rc = sensor_sample_fetch(dev);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ROTATION);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_sample_fetch_chan(dev, SENSOR_CHAN_MAX);
	zassert_true(rc < 0, "Should fail to fetch sample from invalid channel (%d)", rc);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
	}
}

/**
 * @brief sensor_sample_fetch(_chan) test
 *
 * Confirm fetching work with QDEC specific channel - rotation
 *
 */
ZTEST(qdec_sensor, test_sensor_sample_fetch)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		TC_PRINT("Testing QDEC index %d, address: %p\n", i, loopbacks[i].qdec);
		sensor_sample_fetch_test(loopbacks[i].qdec);
	}
}

static void *setup(void)
{
	for (size_t i = 0; i < TESTED_QDEC_COUNT; i++) {
		int rc = device_is_ready(loopbacks[i].qdec);

		zassert_true(rc, "QDEC index %d not ready: %d", i, rc);

		qenc_emulate_setup_pin(&loopbacks[i].qenc_phase_a);
		qenc_emulate_setup_pin(&loopbacks[i].qenc_phase_b);
	}
	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	qenc_emulate_stop();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	qenc_emulate_stop();
	k_sem_reset(&sem);
}

ZTEST_SUITE(qdec_sensor, NULL, setup, before, after, NULL);
