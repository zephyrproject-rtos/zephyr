/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Vendor-neutral clock_monitor API tests.
 *
 * The board overlay lists all clock_monitor instances to exercise under
 *   /zephyr,user { clock-monitors = <&dev0> [, <&dev1> ...]; };
 *
 * Each case iterates the array and, for every device whose runtime
 * caps advertise the mode the case needs, runs the body. A case that
 * finds no matching device ztest_test_skip()'s so the suite is
 * portable across back-ends that only support a subset of modes.
 *
 * A single device that supports both WINDOW and METER (e.g. Renesas
 * CAC) is listed once; caps-filtering makes every case run on it.
 * A multi-device board (e.g. NXP MCXE31 with cmu_0 FC + cmu_1 FM)
 * lists both and each case runs on whichever device supports it.
 */

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define CLKMON_NODE DT_PATH(zephyr_user)

#if DT_NODE_HAS_PROP(CLKMON_NODE, clock_monitors)

#define TO_DEV(node_id, prop, idx) \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

static const struct device *const clock_devices[] = {
	DT_FOREACH_PROP_ELEM(CLKMON_NODE, clock_monitors, TO_DEV)
};

#define NUM_CLOCK_DEVICES ARRAY_SIZE(clock_devices)

#else

static const struct device *const clock_devices[] = {NULL};

#define NUM_CLOCK_DEVICES 0U

#endif

/* ------------------------------------------------------------------ */
/* Config builders                                                     */
/* ------------------------------------------------------------------ */

static struct clock_monitor_config make_window(uint32_t event_mask)
{
	struct clock_monitor_config cfg = {
		.mode = CLOCK_MONITOR_MODE_WINDOW,
		.window = {
			.expected_hz   = 0U,             /* 0 = auto via clock_control */
			.tolerance_ppm = 50000U,         /* +/- 5 % */
			.window_ns     = 1U * NSEC_PER_MSEC,
			.event_mask    = event_mask,
		},
	};
	return cfg;
}

static struct clock_monitor_config make_meter(void)
{
	struct clock_monitor_config cfg = {
		.mode = CLOCK_MONITOR_MODE_METER,
		.meter = {
			.window_ns = 1U * NSEC_PER_MSEC,
		},
	};
	return cfg;
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/*
 * Bring a device back to IDLE. stop() alone only undoes RUNNING ->
 * CONFIGURED; configure(DISABLED) is what tears the HW down and resets
 * the state machine so the next case's preconditions hold.
 */
static void clean_slate(const struct device *dev)
{
	(void)clock_monitor_stop(dev);

	struct clock_monitor_config disabled = {
		.mode = CLOCK_MONITOR_MODE_DISABLED,
	};
	(void)clock_monitor_configure(dev, &disabled);

	uint32_t evts;
	(void)clock_monitor_get_events(dev, &evts);
}

static bool dev_supports_mode(const struct device *dev,
			      enum clock_monitor_mode mode)
{
	const struct clock_monitor_capabilities *caps = clock_monitor_caps(dev);

	return (caps->supported_modes & BIT(mode)) != 0U;
}

typedef void (*case_body_t)(const struct device *dev);

/*
 * Run `body` on every DT-listed device that supports `mode`. Skip the
 * whole case if no device matches.
 */
static void foreach_device_with_mode(enum clock_monitor_mode mode,
				     case_body_t body)
{
	bool ran = false;

	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = clock_devices[i];

		zassert_true(device_is_ready(dev), "%s not ready", dev->name);

		if (!dev_supports_mode(dev, mode)) {
			continue;
		}
		clean_slate(dev);
		body(dev);
		ran = true;
	}
	if (!ran) {
		ztest_test_skip();
	}
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		clean_slate(clock_devices[i]);
	}
}

ZTEST_SUITE(clock_monitor_api, NULL, NULL, before_each, NULL, NULL);

/* ------------------------------------------------------------------ */
/* Caps sanity                                                         */
/* ------------------------------------------------------------------ */

ZTEST(clock_monitor_api, test_caps_non_null)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = clock_devices[i];
		const struct clock_monitor_capabilities *c =
			clock_monitor_caps(dev);

		zassert_not_null(c, "caps NULL on %s", dev->name);
		zassert_not_equal(c->supported_modes, 0,
				  "no modes advertised on %s", dev->name);
	}
}

/* ------------------------------------------------------------------ */
/* WINDOW path                                                         */
/* ------------------------------------------------------------------ */

static void body_window_configure_ok(const struct device *dev)
{
	struct clock_monitor_config cfg = make_window(
		CLOCK_MONITOR_EVT_FREQ_HIGH | CLOCK_MONITOR_EVT_FREQ_LOW);

	zassert_ok(clock_monitor_configure(dev, &cfg),
		   "WINDOW configure failed on %s", dev->name);
}
ZTEST(clock_monitor_api, test_window_configure_ok)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_WINDOW,
				 body_window_configure_ok);
}

static void body_window_check_stop(const struct device *dev)
{
	struct clock_monitor_config cfg = make_window(0U);

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);
	zassert_ok(clock_monitor_check(dev), "check failed on %s", dev->name);
	zassert_ok(clock_monitor_stop(dev), "stop failed on %s", dev->name);
}
ZTEST(clock_monitor_api, test_window_check_stop)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_WINDOW,
				 body_window_check_stop);
}

static void body_window_configure_while_running(const struct device *dev)
{
	struct clock_monitor_config cfg = make_window(0U);

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);
	zassert_ok(clock_monitor_check(dev), NULL);

	zassert_equal(clock_monitor_configure(dev, &cfg), -EBUSY,
		      "configure while running must return -EBUSY on %s",
		      dev->name);
	zassert_ok(clock_monitor_stop(dev), NULL);
}
ZTEST(clock_monitor_api, test_window_configure_while_running)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_WINDOW,
				 body_window_configure_while_running);
}

/* ------------------------------------------------------------------ */
/* METER path                                                          */
/* ------------------------------------------------------------------ */

static void body_meter_configure_ok(const struct device *dev)
{
	struct clock_monitor_config cfg = make_meter();

	zassert_ok(clock_monitor_configure(dev, &cfg),
		   "METER configure failed on %s", dev->name);
}
ZTEST(clock_monitor_api, test_meter_configure_ok)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_METER,
				 body_meter_configure_ok);
}

static void body_meter_measure_oneshot(const struct device *dev)
{
	struct clock_monitor_config cfg = make_meter();

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);

	uint32_t hz = 0U;

	zassert_ok(clock_monitor_measure(dev, &hz, K_MSEC(50)),
		   "measure() failed on %s", dev->name);
	zassert_true(hz > 0U, "measured hz must be > 0 on %s (got %u)",
		     dev->name, hz);
}
ZTEST(clock_monitor_api, test_meter_measure_oneshot)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_METER,
				 body_meter_measure_oneshot);
}

static void body_meter_measure_tight_timeout(const struct device *dev)
{
	struct clock_monitor_config cfg = make_meter();

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);

	uint32_t hz;
	int ret = clock_monitor_measure(dev, &hz, K_NO_WAIT);

	zassert_equal(ret, -EAGAIN,
		      "measure() with no wait must return -EAGAIN on %s "
		      "(got %d)", dev->name, ret);
}
ZTEST(clock_monitor_api, test_meter_measure_tight_timeout)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_METER,
				 body_meter_measure_tight_timeout);
}

static void body_meter_measure_without_configure(const struct device *dev)
{
	uint32_t hz;
	/* before_each + foreach's clean_slate both ran, so state is IDLE. */
	zassert_equal(clock_monitor_measure(dev, &hz, K_MSEC(1)), -EINVAL,
		      "measure() on unconfigured %s must return -EINVAL",
		      dev->name);
}
ZTEST(clock_monitor_api, test_meter_measure_without_configure)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_METER,
				 body_meter_measure_without_configure);
}

static void body_meter_get_events_after_measure(const struct device *dev)
{
	struct clock_monitor_config cfg = make_meter();

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);

	uint32_t hz;

	zassert_ok(clock_monitor_measure(dev, &hz, K_MSEC(50)), NULL);

	uint32_t evts = 0U;

	zassert_ok(clock_monitor_get_events(dev, &evts), NULL);
	zassert_true((evts & CLOCK_MONITOR_EVT_MEASURE_DONE) != 0U,
		     "MEASURE_DONE not latched on %s (got 0x%x)",
		     dev->name, evts);

	zassert_ok(clock_monitor_get_events(dev, &evts), NULL);
	zassert_equal(evts, 0U, "second get_events must be 0 on %s (got 0x%x)",
		      dev->name, evts);
}
ZTEST(clock_monitor_api, test_meter_get_events_after_measure)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_METER,
				 body_meter_get_events_after_measure);
}

/* ------------------------------------------------------------------ */
/* Cross-mode rejection: a device that supports only one of the modes  */
/* must reject the other.                                              */
/* ------------------------------------------------------------------ */

ZTEST(clock_monitor_api, test_rejects_unsupported_mode)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	bool ran = false;

	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = clock_devices[i];
		const struct clock_monitor_capabilities *c =
			clock_monitor_caps(dev);
		bool sup_window = (c->supported_modes &
				   BIT(CLOCK_MONITOR_MODE_WINDOW)) != 0U;
		bool sup_meter  = (c->supported_modes &
				   BIT(CLOCK_MONITOR_MODE_METER)) != 0U;

		clean_slate(dev);

		if (!sup_window) {
			struct clock_monitor_config cfg =
				make_window(0U);
			zassert_equal(clock_monitor_configure(dev, &cfg),
				      -EINVAL,
				      "WINDOW on non-WINDOW-capable %s "
				      "must be -EINVAL", dev->name);
			ran = true;
		}
		if (!sup_meter) {
			struct clock_monitor_config cfg = make_meter();

			zassert_equal(clock_monitor_configure(dev, &cfg),
				      -EINVAL,
				      "METER on non-METER-capable %s "
				      "must be -EINVAL", dev->name);
			ran = true;
		}
	}
	if (!ran) {
		/* Every listed device supports both modes; nothing to reject. */
		ztest_test_skip();
	}
}

/* ------------------------------------------------------------------ */
/* Shared API contracts                                                */
/* ------------------------------------------------------------------ */

ZTEST(clock_monitor_api, test_stop_while_idle)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = clock_devices[i];

		zassert_ok(clock_monitor_stop(dev),
			   "idle stop() must be a no-op on %s", dev->name);
	}
}

ZTEST(clock_monitor_api, test_get_events_initial)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = clock_devices[i];
		uint32_t evts = 0xFFFFFFFFU;

		zassert_ok(clock_monitor_get_events(dev, &evts), NULL);
		zassert_equal(evts, 0U,
			      "fresh get_events must be 0 on %s (got 0x%x)",
			      dev->name, evts);
	}
}

static void body_measure_enosys_on_window_only(const struct device *dev)
{
	/* Only call this for devices that are WINDOW-capable but NOT
	 * METER-capable — test_rejects_unsupported_mode guarantees we
	 * only dispatch here when that's true.
	 */
	uint32_t hz;

	zassert_equal(clock_monitor_measure(dev, &hz, K_MSEC(1)), -ENOSYS,
		      "measure() on WINDOW-only %s must return -ENOSYS",
		      dev->name);
}
ZTEST(clock_monitor_api, test_measure_enosys_on_window_only)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	bool ran = false;

	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = clock_devices[i];
		const struct clock_monitor_capabilities *c =
			clock_monitor_caps(dev);

		if ((c->supported_modes & BIT(CLOCK_MONITOR_MODE_WINDOW)) &&
		    !(c->supported_modes & BIT(CLOCK_MONITOR_MODE_METER))) {
			clean_slate(dev);
			body_measure_enosys_on_window_only(dev);
			ran = true;
		}
	}
	if (!ran) {
		ztest_test_skip();
	}
}
