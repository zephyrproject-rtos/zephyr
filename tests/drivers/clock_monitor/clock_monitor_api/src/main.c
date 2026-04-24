/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Vendor-neutral clock_monitor API tests.
 *
 * The board overlay lists every clock_monitor instance to exercise as a
 * child of a `test-clock-monitor` fixture node (see
 * dts/bindings/test-clock-monitor.yaml), optionally with per-instance
 * expectations:
 *
 *   / {
 *       clock-monitor-test {
 *           compatible = "test-clock-monitor";
 *           fm {
 *               monitor = <&cmu_1>;
 *               measure-expected-hz = <48000000>;
 *           };
 *       };
 *   };
 *
 * Each case iterates the fixtures and, for every device that accepts
 * the mode the case needs (configure() does not return -ENOTSUP), runs
 * the body. A case that finds no matching device ztest_test_skip()'s so
 * the suite is portable across back-ends that only support a subset of
 * modes.
 *
 * A single device that supports both WINDOW and MEASURE (e.g. Renesas
 * CAC) is listed once; mode-probing makes every case run on it. A
 * multi-device board (e.g. NXP MCXE31 with cmu_0 FC + cmu_1 FM) lists
 * both and each case runs on whichever device supports it.
 */

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

struct clkmon_fixture {
	const struct device *dev;
	/* 0 = no accuracy check for this instance. */
	uint32_t measure_expected_hz;
	uint32_t measure_tolerance_ppm;
};

#if DT_HAS_COMPAT_STATUS_OKAY(test_clock_monitor)

#define FIXTURE_ENTRY(node_id)                                                 \
	{                                                                      \
		.dev = DEVICE_DT_GET(DT_PHANDLE(node_id, monitor)),            \
		.measure_expected_hz = DT_PROP(node_id, measure_expected_hz),  \
		.measure_tolerance_ppm =                                       \
			DT_PROP(node_id, measure_tolerance_ppm),               \
	},

static const struct clkmon_fixture fixtures[] = {
	DT_FOREACH_CHILD(DT_COMPAT_GET_ANY_STATUS_OKAY(test_clock_monitor),
			 FIXTURE_ENTRY)
};

#define NUM_CLOCK_DEVICES ARRAY_SIZE(fixtures)

#else

static const struct clkmon_fixture fixtures[] = { {NULL, 0U, 0U} };

#define NUM_CLOCK_DEVICES 0U

#endif

static const struct clkmon_fixture *fixture_of(const struct device *dev)
{
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		if (fixtures[i].dev == dev) {
			return &fixtures[i];
		}
	}
	return NULL;
}

/* Comfortably more than 10 measurement windows (window = 1 ms). */
#define SETTLE_SLEEP K_MSEC(50)

/* Shared configurations */
static const struct clock_monitor_config window_cfg = {
	.mode = CLOCK_MONITOR_MODE_WINDOW,
	.window = {
		.expected_hz   = CONFIG_TEST_CLOCK_MONITOR_EXPECTED_HZ,
		.tolerance_ppm = CONFIG_TEST_CLOCK_MONITOR_TOLERANCE_PPM,
		.window_ns     = CONFIG_TEST_CLOCK_MONITOR_WINDOW_NS,
	},
};

static const struct clock_monitor_config measure_cfg = {
	.mode = CLOCK_MONITOR_MODE_MEASURE,
	.measure = {
		.window_ns = CONFIG_TEST_CLOCK_MONITOR_MEASURE_NS,
	},
};

/* Callback capture */
struct cb_capture {
	atomic_t count;
	uint32_t last_events;
	uint32_t last_hz;
	struct k_sem sem;
	int last_stop_ret;
	/* When true, the callback calls clock_monitor_stop() from its
	 * (possibly ISR) context on each event.
	 */
	bool stop_in_cb;
	/* When > 0, the callback re-starts the device from its (possibly
	 * ISR) context until this many completions have been counted —
	 * the documented continuous-sampling pattern.
	 */
	int restart_until;
};

static void measure_cb(const struct device *dev,
		       const struct clock_monitor_event_data *evt,
		       void *user_data)
{
	struct cb_capture *cap = user_data;

	if (cap->stop_in_cb) {
		cap->last_stop_ret = clock_monitor_stop(dev);
	}
	cap->last_events = evt->events;
	cap->last_hz = evt->measured_hz;

	atomic_val_t n = atomic_inc(&cap->count) + 1;

	if (n < cap->restart_until) {
		(void)clock_monitor_start(dev);
	}
	k_sem_give(&cap->sem);
}

static void cb_capture_init(struct cb_capture *cap, bool stop_in_cb,
			    int restart_until)
{
	atomic_clear(&cap->count);
	cap->last_events = 0U;
	cap->last_hz = 0U;
	cap->last_stop_ret = -1;
	cap->stop_in_cb = stop_in_cb;
	cap->restart_until = restart_until;
	k_sem_init(&cap->sem, 0, 100);
}

/*
 * Bring a device back to a stopped state. The API has no teardown path
 * back to the unconfigured state; each case installs its own
 * configuration, which fully re-inits the hardware.
 */
static void clean_slate(const struct device *dev)
{
	(void)clock_monitor_stop(dev);
}

/*
 * Probe whether the back-end accepts `mode` by issuing a minimal valid
 * configure(); leaves the device stopped.
 *
 * -ENOTSUP from configure() means the back-end does not implement the
 * mode; any other status (including 0) means it does.
 */
static bool dev_supports_mode(const struct device *dev,
			      enum clock_monitor_mode mode)
{
	struct clock_monitor_config cfg;
	int ret;

	switch (mode) {
	case CLOCK_MONITOR_MODE_WINDOW:
		cfg = window_cfg;
		break;
	case CLOCK_MONITOR_MODE_MEASURE:
		cfg = measure_cfg;
		break;
	default:
		return false;
	}

	ret = clock_monitor_configure(dev, &cfg);
	clean_slate(dev);
	return ret != -ENOTSUP;
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
		const struct device *dev = fixtures[i].dev;

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
		clean_slate(fixtures[i].dev);
	}
}

ZTEST_SUITE(clock_monitor_api, NULL, NULL, before_each, NULL, NULL);

/* WINDOW path */
static void body_window_check_stop(const struct device *dev)
{
	struct clock_monitor_config cfg = window_cfg;

	zassert_ok(clock_monitor_configure(dev, &cfg),
		   "WINDOW configure failed on %s", dev->name);
	zassert_ok(clock_monitor_start(dev), "start failed on %s", dev->name);
	zassert_ok(clock_monitor_stop(dev), "stop failed on %s", dev->name);
}
ZTEST(clock_monitor_api, test_window_check_stop)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_WINDOW,
				 body_window_check_stop);
}

static void body_window_configure_while_running(const struct device *dev)
{
	struct clock_monitor_config cfg = window_cfg;

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);
	zassert_ok(clock_monitor_start(dev), NULL);

	zassert_equal(clock_monitor_configure(dev, &cfg), -EBUSY,
		      "configure while running must return -EBUSY on %s",
		      dev->name);
	zassert_equal(clock_monitor_start(dev), -EBUSY,
		      "start while running must return -EBUSY on %s",
		      dev->name);
	zassert_ok(clock_monitor_stop(dev), NULL);
}
ZTEST(clock_monitor_api, test_window_configure_while_running)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_WINDOW,
				 body_window_configure_while_running);
}

/* MEASURE path */
static void body_measure_configure_ok(const struct device *dev)
{
	struct clock_monitor_config cfg = measure_cfg;

	zassert_ok(clock_monitor_configure(dev, &cfg),
		   "MEASURE configure failed on %s", dev->name);
}
ZTEST(clock_monitor_api, test_measure_configure_ok)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_MEASURE,
				 body_measure_configure_ok);
}

/*
 * MEASURE delivers exactly one completion per start() through the
 * configure-time callback and auto-disarms.
 */
static struct cb_capture single_cap;

static void body_measure_single(const struct device *dev)
{
	struct clock_monitor_config cfg = measure_cfg;

	cb_capture_init(&single_cap, false, 0);
	cfg.callback = measure_cb;
	cfg.user_data = &single_cap;

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);
	zassert_ok(clock_monitor_start(dev), "start failed on %s", dev->name);

	zassert_ok(k_sem_take(&single_cap.sem, K_MSEC(100)),
		   "no completion callback on %s", dev->name);
	zassert_true((single_cap.last_events &
		      CLOCK_MONITOR_EVT_MEASURE_DONE) != 0U,
		     "MEASURE_DONE missing on %s (events 0x%x)",
		     dev->name, single_cap.last_events);
	zassert_true(single_cap.last_hz > 0U,
		     "measured hz must be > 0 on %s", dev->name);

	/* Accuracy: with an overlay-provided nominal rate, the measured
	 * value must lie within the per-instance tolerance.
	 */
	const struct clkmon_fixture *fx = fixture_of(dev);

	if (fx != NULL && fx->measure_expected_hz != 0U) {
		uint32_t tol = (uint32_t)(((uint64_t)fx->measure_expected_hz *
			fx->measure_tolerance_ppm) / 1000000U);

		zassert_within(single_cap.last_hz, fx->measure_expected_hz,
			       tol, "measured %u Hz outside %u +/- %u Hz on %s",
			       single_cap.last_hz, fx->measure_expected_hz,
			       tol, dev->name);
	}

	/* Auto-disarm: no further callbacks across many window periods. */
	k_sleep(SETTLE_SLEEP);
	zassert_equal(atomic_get(&single_cap.count), 1,
		      "MEASURE must deliver exactly one callback on %s (got %ld)",
		      dev->name, atomic_get(&single_cap.count));

	/* Device is back in CONFIGURED: another start() must succeed. */
	zassert_ok(clock_monitor_start(dev),
		   "restart after auto-disarm failed on %s", dev->name);
	zassert_ok(k_sem_take(&single_cap.sem, K_MSEC(100)), NULL);
	(void)clock_monitor_stop(dev);
}
ZTEST(clock_monitor_api, test_measure_single)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_MEASURE,
				 body_measure_single);
}

/*
 * Continuous sampling pattern: the callback calls clock_monitor_start()
 * again (in ISR context), which also exercises start()'s ISR-safety.
 */
static struct cb_capture restart_cap;

static void body_measure_restart_from_callback(const struct device *dev)
{
	struct clock_monitor_config cfg = measure_cfg;

	cb_capture_init(&restart_cap, false, 3);
	cfg.callback = measure_cb;
	cfg.user_data = &restart_cap;

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);
	zassert_ok(clock_monitor_start(dev), NULL);

	for (int i = 0; i < 3; i++) {
		zassert_ok(k_sem_take(&restart_cap.sem, K_MSEC(100)),
			   "completion %d missing on %s", i, dev->name);
	}
	zassert_true(atomic_get(&restart_cap.count) >= 3,
		     "restart-from-callback must keep measuring on %s",
		     dev->name);
	zassert_true((restart_cap.last_events &
		      CLOCK_MONITOR_EVT_MEASURE_DONE) != 0U, NULL);
	zassert_true(restart_cap.last_hz > 0U, NULL);

	/* The callback stops re-starting after 3: count must settle. */
	atomic_t settled_count = atomic_get(&restart_cap.count);

	k_sleep(SETTLE_SLEEP);
	zassert_equal(atomic_get(&restart_cap.count), settled_count,
		      "callbacks after the callback stopped re-starting "
		      "on %s", dev->name);
}
ZTEST(clock_monitor_api, test_measure_restart_from_callback)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_MEASURE,
				 body_measure_restart_from_callback);
}

/*
 * stop() aborts an in-flight measurement: no callback may be delivered
 * after stop() returns, and no result is latched.
 */
static struct cb_capture abort_cap;

static void body_measure_stop_aborts_in_flight(const struct device *dev)
{
	struct clock_monitor_config cfg = measure_cfg;
	uint32_t hz;

	cb_capture_init(&abort_cap, false, 0);
	cfg.callback = measure_cb;
	cfg.user_data = &abort_cap;

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);
	zassert_ok(clock_monitor_start(dev), NULL);
	/* Well inside the measurement window: abort immediately. */
	zassert_ok(clock_monitor_stop(dev), NULL);

	k_sleep(SETTLE_SLEEP);
	zassert_equal(atomic_get(&abort_cap.count), 0,
		      "no callback may arrive after stop() on %s (got %ld)",
		      dev->name, atomic_get(&abort_cap.count));
	zassert_equal(clock_monitor_get_rate(dev, &hz), -EAGAIN,
		      "aborted measurement must not latch a result on %s",
		      dev->name);

	/* Device is CONFIGURED: a fresh measurement must work. */
	zassert_ok(clock_monitor_start(dev),
		   "restart after abort failed on %s", dev->name);
	zassert_ok(k_sem_take(&abort_cap.sem, K_MSEC(100)),
		   "no completion after restart on %s", dev->name);
	(void)clock_monitor_stop(dev);
}
ZTEST(clock_monitor_api, test_measure_stop_aborts_in_flight)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_MEASURE,
				 body_measure_stop_aborts_in_flight);
}

/*
 * stop() is ISR-safe and callable from the event callback — a benign
 * no-op there in MEASURE mode (auto-disarm already stopped the device).
 */
static struct cb_capture stopcb_cap;

static void body_measure_stop_from_callback(const struct device *dev)
{
	struct clock_monitor_config cfg = measure_cfg;

	cb_capture_init(&stopcb_cap, true, 0);
	cfg.callback = measure_cb;
	cfg.user_data = &stopcb_cap;

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);
	zassert_ok(clock_monitor_start(dev), NULL);

	zassert_ok(k_sem_take(&stopcb_cap.sem, K_MSEC(100)),
		   "no completion callback on %s", dev->name);

	k_sleep(SETTLE_SLEEP);
	zassert_equal(stopcb_cap.last_stop_ret, 0,
		      "stop() from callback must return 0 on %s (got %d)",
		      dev->name, stopcb_cap.last_stop_ret);
	zassert_equal(atomic_get(&stopcb_cap.count), 1,
		      "exactly one event expected on %s (got %ld)",
		      dev->name, atomic_get(&stopcb_cap.count));

	/* Device is CONFIGURED: start() must succeed. */
	zassert_ok(clock_monitor_start(dev),
		   "restart after stop-from-callback failed on %s",
		   dev->name);
	(void)clock_monitor_stop(dev);
}
ZTEST(clock_monitor_api, test_measure_stop_from_callback)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_MEASURE,
				 body_measure_stop_from_callback);
}

/*
 * get_rate() returns -EAGAIN before the first completion, then 0 with
 * a retained value (not cleared by reads).
 */
static struct cb_capture rate_cap;

static void body_get_rate_contract(const struct device *dev)
{
	struct clock_monitor_config cfg = measure_cfg;
	uint32_t hz1 = 0U;
	uint32_t hz2 = 0U;

	cb_capture_init(&rate_cap, false, 0);
	cfg.callback = measure_cb;
	cfg.user_data = &rate_cap;

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);

	/* No measurement completed since configure(). */
	zassert_equal(clock_monitor_get_rate(dev, &hz1), -EAGAIN,
		      "get_rate before first completion must be -EAGAIN "
		      "on %s", dev->name);

	zassert_ok(clock_monitor_start(dev), NULL);
	zassert_ok(k_sem_take(&rate_cap.sem, K_MSEC(100)), NULL);

	zassert_ok(clock_monitor_get_rate(dev, &hz1),
		   "get_rate after completion failed on %s", dev->name);
	zassert_true(hz1 > 0U, "rate must be > 0 on %s", dev->name);

	/* Value is retained: a second read returns the same value. */
	zassert_ok(clock_monitor_get_rate(dev, &hz2), NULL);
	zassert_equal(hz1, hz2,
		      "get_rate must retain the value across reads on %s "
		      "(%u != %u)", dev->name, hz1, hz2);
	zassert_equal(hz1, rate_cap.last_hz,
		      "get_rate and callback must agree on %s", dev->name);
}
ZTEST(clock_monitor_api, test_get_rate_contract)
{
	foreach_device_with_mode(CLOCK_MONITOR_MODE_MEASURE,
				 body_get_rate_contract);
}

/*
 * start() without a prior configure() must fail. The API has no
 * teardown path back to the unconfigured state, so this is only
 * observable before the first configure() since boot: the "00" name
 * makes this case run first (ztest executes cases in alphabetical
 * order; CONFIG_ZTEST_SHUFFLE must stay off for this suite), and it
 * deliberately avoids the mode-probing helper, which configures.
 */
ZTEST(clock_monitor_api, test_00_start_before_first_configure)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = fixtures[i].dev;

		zassert_true(device_is_ready(dev), "%s not ready", dev->name);
		zassert_equal(clock_monitor_start(dev), -EINVAL,
			      "start() on unconfigured %s must return -EINVAL",
			      dev->name);
	}
}

/*
 * Mode enum value 0 is intentionally unnamed; a zero-initialized mode
 * must be rejected with -ENOTSUP by every back-end.
 */
ZTEST(clock_monitor_api, test_configure_zero_init_mode_rejected)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = fixtures[i].dev;
		struct clock_monitor_config cfg = {0};

		zassert_equal(clock_monitor_configure(dev, &cfg), -ENOTSUP,
			      "zero-init mode on %s must return -ENOTSUP",
			      dev->name);
	}
}

/*
 * Cross-mode rejection: a device that supports only one of the modes
 * must reject the other with -ENOTSUP.
 */
ZTEST(clock_monitor_api, test_rejects_unsupported_mode)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	bool ran = false;

	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = fixtures[i].dev;
		bool sup_window = dev_supports_mode(dev,
						    CLOCK_MONITOR_MODE_WINDOW);
		bool sup_measure = dev_supports_mode(dev,
					     CLOCK_MONITOR_MODE_MEASURE);

		clean_slate(dev);

		if (!sup_window) {
			struct clock_monitor_config cfg = window_cfg;

			zassert_equal(clock_monitor_configure(dev, &cfg),
				      -ENOTSUP,
				      "WINDOW on non-WINDOW-capable %s "
				      "must be -ENOTSUP", dev->name);
			ran = true;
		}
		if (!sup_measure) {
			struct clock_monitor_config cfg = measure_cfg;

			zassert_equal(clock_monitor_configure(dev, &cfg),
				      -ENOTSUP,
				      "MEASURE on non-MEASURE-capable %s "
				      "must be -ENOTSUP", dev->name);
			ran = true;
		}
	}
	if (!ran) {
		/* Every listed device supports both modes; nothing to reject. */
		ztest_test_skip();
	}
}

/* Shared API contracts */
ZTEST(clock_monitor_api, test_stop_while_idle)
{
	if (NUM_CLOCK_DEVICES == 0U) {
		ztest_test_skip();
	}
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = fixtures[i].dev;

		zassert_ok(clock_monitor_stop(dev),
			   "idle stop() must be a no-op on %s", dev->name);
	}
}

/*
 * Back-ends without measurement hardware leave the get_rate vtable
 * slot NULL, which the API maps to -ENOSYS.
 */
ZTEST(clock_monitor_api, test_get_rate_enosys_without_measure_hw)
{
	bool ran = false;

	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = fixtures[i].dev;
		uint32_t hz;

		if (dev_supports_mode(dev, CLOCK_MONITOR_MODE_MEASURE)) {
			continue;
		}
		zassert_equal(clock_monitor_get_rate(dev, &hz), -ENOSYS,
			      "get_rate on measurement-less %s must be "
			      "-ENOSYS", dev->name);
		ran = true;
	}
	if (!ran) {
		ztest_test_skip();
	}
}
