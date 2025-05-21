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

static K_SEM_DEFINE(sem, 0, 1);
static const struct gpio_dt_spec phase_a = GPIO_DT_SPEC_GET(DT_ALIAS(qenca), gpios);
static const struct gpio_dt_spec phase_b = GPIO_DT_SPEC_GET(DT_ALIAS(qencb), gpios);
static const struct device *const qdec_dev = DEVICE_DT_GET(DT_ALIAS(qdec0));
static const uint32_t qdec_config_step = DT_PROP(DT_ALIAS(qdec0), steps);

#if DT_NODE_EXISTS(DT_ALIAS(qdec1))
#define SECOND_QDEC_INSTANCE

static const struct gpio_dt_spec phase_a1 = GPIO_DT_SPEC_GET(DT_ALIAS(qenca1), gpios);
static const struct gpio_dt_spec phase_b1 = GPIO_DT_SPEC_GET(DT_ALIAS(qencb1), gpios);
static const struct device *const qdec_1_dev = DEVICE_DT_GET(DT_ALIAS(qdec1));
static const uint32_t qdec_1_config_step = DT_PROP(DT_ALIAS(qdec1), steps);
#endif
static struct sensor_trigger qdec_trigger = {.type = SENSOR_TRIG_DATA_READY,
					     .chan = SENSOR_CHAN_ROTATION};
static bool toggle_a = true;

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
	if (toggle_a) {
		gpio_pin_toggle_dt(&phase_a);
#if defined(SECOND_QDEC_INSTANCE)
		gpio_pin_toggle_dt(&phase_a1);
#endif
	} else {
		gpio_pin_toggle_dt(&phase_b);
#if defined(SECOND_QDEC_INSTANCE)
		gpio_pin_toggle_dt(&phase_b1);
#endif
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

static void qenc_emulate_start(k_timeout_t period, bool forward)
{
	qenc_emulate_reset_pin(&phase_a);
	qenc_emulate_reset_pin(&phase_b);
#if defined(SECOND_QDEC_INSTANCE)
	qenc_emulate_reset_pin(&phase_a1);
	qenc_emulate_reset_pin(&phase_b1);
#endif

	toggle_a = !forward;

	k_timer_start(&qenc_emulate_timer, period, period);
}

static void qenc_emulate_stop(void)
{
	k_timer_stop(&qenc_emulate_timer);

	qenc_emulate_reset_pin(&phase_a);
	qenc_emulate_reset_pin(&phase_b);
#if defined(SECOND_QDEC_INSTANCE)
	qenc_emulate_reset_pin(&phase_a1);
	qenc_emulate_reset_pin(&phase_b1);
#endif
}

static void qenc_emulate_verify_reading(const struct device *const dev, int emulator_period_ms,
		int emulation_duration_ms, bool forward, bool overflow_expected,
		const uint32_t config_step)
{
	int rc;
	struct sensor_value val = {0};
	int32_t expected_steps = emulation_duration_ms / emulator_period_ms;
	int32_t expected_reading = 360 * expected_steps / config_step;
	int32_t delta = expected_reading / 5;

	if (!forward) {
		expected_reading *= -1;
	}

	qenc_emulate_start(K_MSEC(emulator_period_ms), forward);

	/* wait for some readings */
	k_msleep(emulation_duration_ms);

	rc = sensor_sample_fetch(dev);

	if (!overflow_expected) {
		zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);
	} else {
		zassert_true(rc == -EOVERFLOW, "Failed to detect overflow");
	}

	rc = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
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

	rc = sensor_sample_fetch(dev);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
	zassert_true(rc == 0, "Failed to get sample (%d)", rc);
}

static void sensor_trigger_set_and_disable(const struct device *const dev)
{
	int rc;

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	}

	k_sem_give(&sem);

	qdec_trigger.type = SENSOR_TRIG_DATA_READY;
	qdec_trigger.chan = SENSOR_CHAN_ALL;
	rc = sensor_trigger_set(dev, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc != -ENOSYS, "sensor_trigger_set not supported");

	zassert_true(rc == 0, "sensor_trigger_set failed: %d", rc);

	qenc_emulate_start(K_MSEC(10), true);

	/* emulation working, handler should be called */
	rc = k_sem_take(&sem, K_MSEC(200));
	zassert_true(rc == 0, "qdec handler should be triggered (%d)", rc);

	qenc_emulate_stop();

	/* emulation not working, but there maybe old trigger, ignore */
	rc = k_sem_take(&sem, K_MSEC(200));

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
	}

	/* there should be no triggers now*/
	rc = k_sem_take(&sem, K_MSEC(200));
	zassert_true(rc == -EAGAIN, "qdec handler should not be triggered (%d)", rc);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	}

	/* register empty trigger - disable trigger */
	rc = sensor_trigger_set(dev, &qdec_trigger, NULL);
	zassert_true(rc == 0, "sensor_trigger_set failed: %d", rc);

	qenc_emulate_start(K_MSEC(10), true);

	/* emulation working, but handler not set, thus should not be called */
	rc = k_sem_take(&sem, K_MSEC(200));
	zassert_true(rc == -EAGAIN, "qdec handler should not be triggered (%d)", rc);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
	}
}

/**
 * @brief sensor_trigger_set test disable trigger
 *
 * Confirm trigger happens after set and stops after being disabled
 *
 */
ZTEST(qdec_sensor, test_sensor_trigger_set_and_disable)
{
	TC_PRINT("Testing QDEC-0, address: %p\n", qdec_dev);
	sensor_trigger_set_and_disable(qdec_dev);
#if defined(SECOND_QDEC_INSTANCE)
	TC_PRINT("Testing QDEC-1, address: %p\n", qdec_1_dev);
	sensor_trigger_set_and_disable(qdec_1_dev);
#endif

}

static void sensor_trigger_set_test(const struct device *const dev)
{
	int rc;
	struct sensor_value val = {0};

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	}

	qdec_trigger.type = SENSOR_TRIG_DATA_READY;
	qdec_trigger.chan = SENSOR_CHAN_ROTATION;
	rc = sensor_trigger_set(dev, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc != -ENOSYS, "sensor_trigger_set not supported");

	zassert_true(rc == 0, "sensor_trigger_set failed: %d", rc);

	qenc_emulate_start(K_MSEC(10), true);

	/* emulation working now */
	rc = k_sem_take(&sem, K_MSEC(200));
	zassert_true(rc == 0, "qdec handler should be triggered (%d)", rc);

	rc = sensor_sample_fetch(dev);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	TC_PRINT("QDEC reading: %d\n", val.val1);
	zassert_true(val.val1 != 0, "No readings from QDEC");

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
	}
}

/**
 * @brief sensor_trigger_set test
 *
 * Confirm trigger happens after set
 *
 */
ZTEST(qdec_sensor, test_sensor_trigger_set)
{
	TC_PRINT("Testing QDEC-0, address: %p\n", qdec_dev);
	sensor_trigger_set_test(qdec_dev);
#if defined(SECOND_QDEC_INSTANCE)
	TC_PRINT("Testing QDEC-1, address: %p\n", qdec_1_dev);
	sensor_trigger_set_test(qdec_1_dev);
#endif
}

static void sensor_trigger_set_negative(const struct device *const dev)
{
	int rc;

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	}

	rc = sensor_trigger_set(dev, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc != -ENOSYS, "sensor_trigger_set not supported");

	qdec_trigger.type = SENSOR_TRIG_MAX;
	qdec_trigger.chan = SENSOR_CHAN_ROTATION;

	rc = sensor_trigger_set(dev, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc < 0, "sensor_trigger_set should fail due to invalid trigger type");

	qdec_trigger.type = SENSOR_TRIG_DATA_READY;
	qdec_trigger.chan = SENSOR_CHAN_MAX;

	rc = sensor_trigger_set(dev, &qdec_trigger, qdec_trigger_handler);
	zassume_true(rc < 0, "sensor_trigger_set should fail due to invalid channel");

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
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
	TC_PRINT("Testing QDEC-0, address: %p\n", qdec_dev);
	sensor_trigger_set_negative(qdec_dev);
#if defined(SECOND_QDEC_INSTANCE)
	TC_PRINT("Testing QDEC-1, address: %p\n", qdec_1_dev);
	sensor_trigger_set_negative(qdec_1_dev);
#endif
}

/**
 * @brief QDEC readings tests
 *
 * Valid reading from QDEC base on simulated signal
 *
 */
ZTEST(qdec_sensor, test_qdec_readings)
{
	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(qdec_dev);
	}

	TC_PRINT("Testing QDEC-0, address: %p\n", qdec_dev);
	qenc_emulate_verify_reading(qdec_dev, 10, 100, true, false, qdec_config_step);
	qenc_emulate_verify_reading(qdec_dev, 2, 500, true, false, qdec_config_step);
	qenc_emulate_verify_reading(qdec_dev, 10, 200, false, false, qdec_config_step);
	qenc_emulate_verify_reading(qdec_dev, 1, 1000, false, true, qdec_config_step);
	qenc_emulate_verify_reading(qdec_dev, 1, 1000, true, true, qdec_config_step);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(qdec_dev);
	}

#if defined(SECOND_QDEC_INSTANCE)
	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(qdec_1_dev);
	}

	TC_PRINT("Testing QDEC-1, address: %p\n", qdec_1_dev);
	qenc_emulate_verify_reading(qdec_1_dev, 10, 100, true, false, qdec_1_config_step);
	qenc_emulate_verify_reading(qdec_1_dev, 2, 500, true, false, qdec_1_config_step);
	qenc_emulate_verify_reading(qdec_1_dev, 10, 200, false, false, qdec_1_config_step);
	qenc_emulate_verify_reading(qdec_1_dev, 1, 1000, false, true, qdec_1_config_step);
	qenc_emulate_verify_reading(qdec_1_dev, 1, 1000, true, true, qdec_1_config_step);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(qdec_1_dev);
	}
#endif
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
	TC_PRINT("Testing QDEC-0, address: %p\n", qdec_dev);
	sensor_channel_get_empty(qdec_dev);
#if defined(SECOND_QDEC_INSTANCE)
	TC_PRINT("Testing QDEC-1, address: %p\n", qdec_1_dev);
	sensor_channel_get_empty(qdec_1_dev);
#endif
}

static void sensor_channel_get_test(const struct device *const dev)
{
	int rc;
	struct sensor_value val_first = {0};
	struct sensor_value val_second = {0};

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(qdec_dev);
	}

	qenc_emulate_start(K_MSEC(10), true);

	/* wait for some readings*/
	k_msleep(100);

	rc = sensor_sample_fetch(qdec_dev);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val_first);
	zassert_true(rc == 0, "Failed to get sample (%d)", rc);
	zassert_true(val_first.val1 != 0, "No readings from QDEC");

	/* wait for more readings*/
	k_msleep(200);

	rc = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val_second);
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
		pm_device_runtime_put(qdec_dev);
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
	TC_PRINT("Testing QDEC-0, address: %p\n", qdec_dev);
	sensor_channel_get_test(qdec_dev);
#if defined(SECOND_QDEC_INSTANCE)
	TC_PRINT("Testing QDEC-1, address: %p\n", qdec_1_dev);
	sensor_channel_get_test(qdec_1_dev);
#endif
}

static void sensor_channel_get_negative(const struct device *const dev)
{
	int rc;
	struct sensor_value val = {0};

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	}

	qenc_emulate_start(K_MSEC(10), true);

	/* wait for some readings*/
	k_msleep(100);

	rc = sensor_sample_fetch(dev);
	zassert_true(rc == 0, "Failed to fetch sample (%d)", rc);

	rc = sensor_channel_get(dev, SENSOR_CHAN_MAX, &val);
	zassert_true(rc < 0, "Should failed to get sample (%d)", rc);
	zassert_true(val.val1 == 0, "Some readings from QDEC: %d", val.val1);
	zassert_true(val.val2 == 0, "Some readings from QDEC: %d", val.val2);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
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
	TC_PRINT("Testing QDEC-0, address: %p\n", qdec_dev);
	sensor_channel_get_negative(qdec_dev);
#if defined(SECOND_QDEC_INSTANCE)
	TC_PRINT("Testing QDEC-1, address: %p\n", qdec_1_dev);
	sensor_channel_get_negative(qdec_1_dev);
#endif
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
	TC_PRINT("Testing QDEC-0, address: %p\n", qdec_dev);
	sensor_sample_fetch_test(qdec_dev);
#if defined(SECOND_QDEC_INSTANCE)
	TC_PRINT("Testing QDEC-1, address: %p\n", qdec_1_dev);
	sensor_sample_fetch_test(qdec_1_dev);
#endif
}

static void *setup(void)
{
	int rc;

	rc = device_is_ready(qdec_dev);
	zassert_true(rc, "QDEC device not ready: %d", rc);

	qenc_emulate_setup_pin(&phase_a);
	qenc_emulate_setup_pin(&phase_b);

#if defined(SECOND_QDEC_INSTANCE)
	rc = device_is_ready(qdec_1_dev);
	zassert_true(rc, "QDEC 1 device not ready: %d", rc);

	qenc_emulate_setup_pin(&phase_a1);
	qenc_emulate_setup_pin(&phase_b1);
#endif

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
}

ZTEST_SUITE(qdec_sensor, NULL, setup, before, after, NULL);
