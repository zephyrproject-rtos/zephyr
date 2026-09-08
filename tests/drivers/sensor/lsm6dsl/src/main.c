/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * Copyright (c) 2026 Arkadiusz Grzelka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "lsm6dsl_emul.h"

#define LSM6DSL_NODE DT_ALIAS(lsm6dsl)

/*
 * Every wait below carries a timeout and every one is asserted on, so a
 * regression that costs a callback fails the case instead of stalling it. One
 * regression escapes that: a driver that captures the cooperative system
 * workqueue stops the simulated clock with it, and no timeout can expire.
 * test_no_handler_does_not_spin is the case that relies on it, and says so.
 */
#define WAIT_TIMEOUT  K_MSEC(500)
#define QUIET_TIMEOUT K_MSEC(50)
#define SETTLE_TIME   K_MSEC(20)

DEFINE_FFF_GLOBALS;

static const struct device *const lsm6dsl_dev = DEVICE_DT_GET(LSM6DSL_NODE);
static const struct emul *const lsm6dsl_emul = EMUL_DT_GET(LSM6DSL_NODE);
static const struct gpio_dt_spec int_gpio = GPIO_DT_SPEC_GET(LSM6DSL_NODE, irq_gpios);

static const struct sensor_trigger drdy_trigger = {
	.type = SENSOR_TRIG_DATA_READY,
	.chan = SENSOR_CHAN_ALL,
};

static K_SEM_DEFINE(handler_sem, 0, K_SEM_MAX_LIMIT);
static K_SEM_DEFINE(probe_sem, 0, 1);

/* Result of the sensor_trigger_set() a handler performs on itself. */
static int remove_trigger_result;

FAKE_VOID_FUNC(drdy_handler, const struct device *, const struct sensor_trigger *);

/*
 * Drive the emulated INT1 pin and keep STATUS_REG in step with it, so the
 * register file the driver can read never contradicts the line it sees.
 */
static void drive_int_line(bool asserted)
{
	(void)gpio_emul_input_set(int_gpio.port, int_gpio.pin, asserted ? 1 : 0);
	lsm6dsl_emul_set_data_ready(lsm6dsl_emul, asserted);
}

/* A handler that reads the sample: data ready is consumed and the line goes idle. */
static void handler_reads_sample(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);

	drive_int_line(false);
	k_sem_give(&handler_sem);
}

/*
 * A read that clears data ready with the next sample completing right behind it.
 * The line ends up asserted again while the driver still has the interrupt
 * masked, so the rising edge is never presented to the interrupt controller and
 * only a re-read of the line can recover the sample.
 */
static void handler_reads_sample_and_races(const struct device *dev,
					   const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);

	drive_int_line(false);

	if (drdy_handler_fake.call_count == 1) {
		drive_int_line(true);
	}

	k_sem_give(&handler_sem);
}

/* A handler that drops the trigger while the line is still asserted. */
static void handler_removes_trigger(const struct device *dev, const struct sensor_trigger *trig)
{
	remove_trigger_result = sensor_trigger_set(dev, trig, NULL);
	k_sem_give(&handler_sem);
}

static void probe_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&probe_sem);
}

static K_WORK_DEFINE(probe_work, probe_work_handler);

static void *lsm6dsl_suite_setup(void)
{
	zassert_not_null(lsm6dsl_emul, "no emulator registered for the lsm6dsl node");
	zassert_true(device_is_ready(lsm6dsl_dev), "lsm6dsl device is not ready");
	zassert_true(gpio_is_ready_dt(&int_gpio), "the emulated interrupt line is not ready");
	zassert_ok(gpio_emul_input_set(int_gpio.port, int_gpio.pin, 0),
		   "the interrupt line cannot be driven");

	return NULL;
}

static void lsm6dsl_before(void *unused)
{
	ARG_UNUSED(unused);

	RESET_FAKE(drdy_handler);
	k_sem_reset(&handler_sem);
	k_sem_reset(&probe_sem);
	remove_trigger_result = -EAGAIN;
	drive_int_line(false);
}

static void lsm6dsl_after(void *unused)
{
	ARG_UNUSED(unused);

	/*
	 * Idle the line and let anything already queued drain before disarming,
	 * so a work item still in flight cannot re-enable the interrupt behind
	 * the next case.
	 */
	drive_int_line(false);
	k_sleep(SETTLE_TIME);
	(void)sensor_trigger_set(lsm6dsl_dev, &drdy_trigger, NULL);
}

ZTEST_SUITE(lsm6dsl_trigger, NULL, lsm6dsl_suite_setup, lsm6dsl_before, lsm6dsl_after, NULL);

/* Baseline: one edge on the interrupt line produces exactly one callback. */
ZTEST(lsm6dsl_trigger, test_trigger_fires_on_edge)
{
	drdy_handler_fake.custom_fake = handler_reads_sample;

	zassert_ok(sensor_trigger_set(lsm6dsl_dev, &drdy_trigger, drdy_handler));

	drive_int_line(true);

	zassert_ok(k_sem_take(&handler_sem, WAIT_TIMEOUT),
		   "the data ready handler never ran for the first edge");
	zassert_equal(k_sem_take(&handler_sem, QUIET_TIMEOUT), -EAGAIN,
		      "the handler ran more than once for a single edge");
	zassert_equal(drdy_handler_fake.call_count, 1, "expected 1 callback, got %d",
		      drdy_handler_fake.call_count);
}

/*
 * Regression anchor. A sample completes while the driver has the interrupt
 * masked, so its edge is lost. Data ready is latched on the part, which means
 * the line stays asserted and no further edge will ever arrive: unless the
 * driver re-reads the line after re-arming, acquisition stops for good.
 */
ZTEST(lsm6dsl_trigger, test_sample_landing_in_the_masked_window_is_not_lost)
{
	drdy_handler_fake.custom_fake = handler_reads_sample_and_races;

	zassert_ok(sensor_trigger_set(lsm6dsl_dev, &drdy_trigger, drdy_handler));

	drive_int_line(true);

	zassert_ok(k_sem_take(&handler_sem, WAIT_TIMEOUT),
		   "the data ready handler never ran for the first edge");
	zassert_ok(k_sem_take(&handler_sem, WAIT_TIMEOUT),
		   "the sample that completed while the interrupt was masked was lost: "
		   "the driver re-armed the edge interrupt without re-reading the line");
	zassert_equal(drdy_handler_fake.call_count, 2, "expected 2 callbacks, got %d",
		      drdy_handler_fake.call_count);
}

/*
 * The trigger is removed from inside the handler, leaving the line asserted.
 * The driver re-arms after the handler returns and must not chase an interrupt
 * it no longer has a handler for: doing so would resubmit work forever and
 * capture the cooperative system workqueue.
 *
 * The probe below is the assertion, and it reports the regression as a hang
 * rather than a failure. That is not an oversight: the simulated clock only
 * advances when the CPU goes idle, so a cooperative work item that never
 * yields also stops time, and no timeout placed here could expire. Twister
 * reports the case as a timeout. Do not replace the probe with a bounded wait
 * expecting a tidier failure - there is none to be had.
 */
ZTEST(lsm6dsl_trigger, test_no_handler_does_not_spin)
{
	drdy_handler_fake.custom_fake = handler_removes_trigger;

	zassert_ok(sensor_trigger_set(lsm6dsl_dev, &drdy_trigger, drdy_handler));

	drive_int_line(true);

	zassert_ok(k_sem_take(&handler_sem, WAIT_TIMEOUT),
		   "the data ready handler never ran for the first edge");
	zassert_ok(remove_trigger_result, "removing the trigger from the handler failed");
	zassert_equal(gpio_pin_get_dt(&int_gpio), 1, "the interrupt line should still be asserted");

	zassert_true(k_work_submit(&probe_work) >= 0, "could not queue the probe work item");
	zassert_ok(k_sem_take(&probe_sem, WAIT_TIMEOUT),
		   "the system workqueue is captured: the driver kept servicing a trigger "
		   "that has no handler");
	zassert_equal(drdy_handler_fake.call_count, 1, "expected 1 callback, got %d",
		      drdy_handler_fake.call_count);
}
