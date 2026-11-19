/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * clock_monitor power-management tests.
 *
 * The cases drive the device through the public clock_monitor and pm_device
 * APIs only - no register access and no board knowledge - so any back-end that
 * implements PM actions runs them unchanged. A device the PM core does not know
 * is skipped (pm_device_action_run() returns -ENOSYS).
 *
 * Only MEASURE mode is exercised: every case needs an asynchronous completion
 * it can observe across a power transition.
 *
 * The instances under test are listed as children of a "test-clock-monitor-pm"
 * fixture node in the board overlay. That overlay also enables the monitor node
 * and declares the clocks the measurement needs in a "test-required-clocks"
 * node, which setup enables before the first case runs.
 *
 * The suite runs with system PM enabled, so the idle thread may suspend a
 * device between two statements of a case. The cases tolerate that (a manual
 * action that is already satisfied returns -EALREADY); what they verify is that
 * no measurement is lost and that the device is usable again after RESUME.
 *
 * The harness takes a device runtime PM reference for the duration of each
 * case, so the cases also work with "zephyr,pm-device-runtime-auto".
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

struct clkmon_fixture {
	const struct device *dev;
};

#if DT_HAS_COMPAT_STATUS_OKAY(test_clock_monitor_pm)

#define FIXTURE_ENTRY(node_id)                                                 \
	{                                                                      \
		.dev = DEVICE_DT_GET(DT_PHANDLE(node_id, monitor)),            \
	},

static const struct clkmon_fixture fixtures[] = {
	DT_FOREACH_CHILD(DT_COMPAT_GET_ANY_STATUS_OKAY(test_clock_monitor_pm),
			 FIXTURE_ENTRY)
};

#define NUM_CLOCK_DEVICES ARRAY_SIZE(fixtures)

#else

static const struct clkmon_fixture fixtures[] = { { NULL } };

#define NUM_CLOCK_DEVICES 0U

#endif

/* A measurement long enough to still be in flight when a case suspends. */
#define PM_TEST_WINDOW_NS CONFIG_TEST_CLOCK_MONITOR_PM_WINDOW_NS

/*
 * How long a case waits for a completion before asserting that none arrives.
 * Must cover a measurement that never completes: one window plus the back-end's
 * internal retry budget. The floor is 5 windows, so the assertion cannot pass
 * merely because the cycle had not finished yet.
 */
#define PM_TEST_SETTLE_MS CONFIG_TEST_CLOCK_MONITOR_PM_SETTLE_MS

BUILD_ASSERT((uint64_t)PM_TEST_SETTLE_MS * 1000000ULL >= 5ULL * PM_TEST_WINDOW_NS,
	     "CONFIG_TEST_CLOCK_MONITOR_PM_SETTLE_MS must cover a hung cycle's "
	     "retry budget (5x CONFIG_TEST_CLOCK_MONITOR_PM_WINDOW_NS)");

static const struct clock_monitor_config measure_cfg = {
	.mode = CLOCK_MONITOR_MODE_MEASURE,
	.measure = {
		.window_ns = CONFIG_TEST_CLOCK_MONITOR_MEASURE_NS,
	},
};

static const struct clock_monitor_config pm_test_long_cfg = {
	.mode = CLOCK_MONITOR_MODE_MEASURE,
	.measure = {
		.window_ns = PM_TEST_WINDOW_NS,
	},
};

/* Capture of the completion callback. */
struct cb_capture {
	atomic_t count;
	uint32_t last_events;
	uint32_t last_hz;
	struct k_sem sem;
};

static void measure_cb(const struct device *dev,
		       const struct clock_monitor_event_data *evt,
		       void *user_data)
{
	struct cb_capture *cap = user_data;

	ARG_UNUSED(dev);

	cap->last_events = evt->events;
	cap->last_hz = evt->measured_hz;

	atomic_inc(&cap->count);
	k_sem_give(&cap->sem);
}

static void cb_capture_init(struct cb_capture *cap)
{
	atomic_clear(&cap->count);
	cap->last_events = 0U;
	cap->last_hz = 0U;
	k_sem_init(&cap->sem, 0, 100);
}

/* The API has no teardown path: re-configuring re-inits the hardware. */
static void clean_slate(const struct device *dev)
{
	(void)clock_monitor_stop(dev);
}

#if DT_HAS_COMPAT_STATUS_OKAY(test_required_clocks)

#define REQUIRED_CLOCK_ON(node_id, prop, idx)                                  \
	do {                                                                   \
		const struct device *_clk =                                    \
			DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(node_id, idx));    \
		if (device_is_ready(_clk)) {                                   \
			(void)clock_control_on(_clk,                           \
				(clock_control_subsys_t)(uintptr_t)            \
				DT_CLOCKS_CELL_BY_IDX(node_id, idx, name));    \
		}                                                              \
	} while (0)

static void enable_required_clocks(void)
{
	DT_FOREACH_PROP_ELEM_SEP(DT_COMPAT_GET_ANY_STATUS_OKAY(test_required_clocks),
				 clocks, REQUIRED_CLOCK_ON, (;));
}

#else

static void enable_required_clocks(void) {}

#endif

/*
 * Probe whether the back-end implements MEASURE mode. -ENOTSUP means it does
 * not; any other status (including a state error) means the cases are relevant.
 */
static bool dev_supports_measure(const struct device *dev)
{
	int ret = clock_monitor_configure(dev, &measure_cfg);

	clean_slate(dev);

	return ret != -ENOTSUP;
}

typedef void (*case_body_t)(const struct device *dev);

/* Run `body` on every fixture device that supports MEASURE; skip if none. */
static void foreach_measure_device(case_body_t body)
{
	bool ran = false;

	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		const struct device *dev = fixtures[i].dev;

		zassert_true(device_is_ready(dev), "%s not ready", dev->name);

		if (!dev_supports_measure(dev)) {
			continue;
		}
		clean_slate(dev);

		/* The consumer owns the runtime reference, so take it for the body.
		 * A back-end without runtime PM treats both calls as no-ops, and the
		 * runtime case below releases this reference itself.
		 */
		zassert_ok(pm_device_runtime_get(dev),
			   "runtime get failed on %s", dev->name);
		body(dev);
		(void)pm_device_runtime_put(dev);

		ran = true;
	}
	if (!ran) {
		ztest_test_skip();
	}
}

static void *clkmon_setup(void)
{
	/* Power the measured clocks once, before any case configures. */
	enable_required_clocks();
	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	for (size_t i = 0; i < NUM_CLOCK_DEVICES; i++) {
		clean_slate(fixtures[i].dev);
	}
}

ZTEST_SUITE(clock_monitor_pm, NULL, clkmon_setup, before_each, NULL, NULL);

/* Devices without PM actions return -ENOSYS; -ENOTSUP means the state cannot
 * be driven from here. Both skip the case.
 */
static bool pm_test_supported(const struct device *dev)
{
	int ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);

	if (ret == -ENOSYS || ret == -ENOTSUP) {
		return false;
	}
	zassert_true(ret == 0 || ret == -EALREADY,
		     "probe SUSPEND failed on %s (%d)", dev->name, ret);

	ret = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
	zassert_true(ret == 0 || ret == -EALREADY,
		     "probe RESUME failed on %s (%d)", dev->name, ret);

	return true;
}

static void pm_test_suspend(const struct device *dev)
{
	int ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);

	zassert_true(ret == 0 || ret == -EALREADY,
		     "SUSPEND failed on %s (%d)", dev->name, ret);
}

static void pm_test_resume(const struct device *dev)
{
	int ret = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);

	zassert_true(ret == 0 || ret == -EALREADY,
		     "RESUME failed on %s (%d)", dev->name, ret);
}

/*
 * SUSPEND aborts the cycle: no completion arrives while suspended, the device
 * rejects work instead of silently dropping it, and the monitor is usable again
 * after RESUME without re-initialising the device.
 */
static void body_pm_suspend_aborts_measure(const struct device *dev)
{
	struct clock_monitor_config cfg = pm_test_long_cfg;
	struct cb_capture cap;
	int ret;

	if (!pm_test_supported(dev)) {
		ztest_test_skip();
	}

	cb_capture_init(&cap);
	cfg.callback = measure_cb;
	cfg.user_data = &cap;

	zassert_ok(clock_monitor_configure(dev, &cfg),
		   "long-window configure failed on %s", dev->name);
	zassert_ok(clock_monitor_start(dev), "start failed on %s", dev->name);

	pm_test_suspend(dev);

	zassert_not_equal(k_sem_take(&cap.sem, K_MSEC(PM_TEST_SETTLE_MS)), 0,
			  "%s delivered a completion while suspended", dev->name);
	clean_slate(dev);

	/* A suspended device must not silently accept work: configure() and start()
	 * have to report the failure instead of returning 0 and dropping the
	 * request. The exact errno is left to the back-end.
	 */
	ret = clock_monitor_configure(dev, &cfg);
	zassert_true(ret != 0,
		     "configure() while suspended returned 0 on %s", dev->name);
	ret = clock_monitor_start(dev);
	zassert_true(ret != 0,
		     "start() while suspended returned 0 on %s", dev->name);

	pm_test_resume(dev);

	/* Usable again, with the normal window. */
	cb_capture_init(&cap);
	cfg = measure_cfg;
	cfg.callback = measure_cb;
	cfg.user_data = &cap;

	zassert_ok(clock_monitor_configure(dev, &cfg),
		   "configure() after RESUME failed on %s", dev->name);
	zassert_ok(clock_monitor_start(dev),
		   "start() after RESUME failed on %s", dev->name);
	zassert_ok(k_sem_take(&cap.sem, K_MSEC(100)),
		   "no completion after RESUME on %s", dev->name);
	zassert_true((cap.last_events & CLOCK_MONITOR_EVT_MEASURE_DONE) != 0U,
		     "no MEASURE_DONE after RESUME on %s", dev->name);
}
ZTEST(clock_monitor_pm, test_pm_suspend_aborts_measure)
{
	foreach_measure_device(body_pm_suspend_aborts_measure);
}

#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
/* Both states the node declares must be locked while a cycle runs. */
static bool pm_test_state_locked(void)
{
	return pm_policy_state_lock_is_active(PM_STATE_SUSPEND_TO_IDLE,
					     PM_ALL_SUBSTATES) ||
	       pm_policy_state_lock_is_active(PM_STATE_STANDBY,
					     PM_ALL_SUBSTATES);
}

static void body_pm_power_lock(const struct device *dev)
{
	struct clock_monitor_config cfg = pm_test_long_cfg;
	struct cb_capture cap;

	if (!pm_test_supported(dev)) {
		ztest_test_skip();
	}
	if (pm_test_state_locked()) {
		/* Another consumer already holds one of these states. */
		ztest_test_skip();
	}

	cb_capture_init(&cap);
	cfg.callback = measure_cb;
	cfg.user_data = &cap;

	zassert_ok(clock_monitor_configure(dev, &cfg), NULL);
	zassert_ok(clock_monitor_start(dev), NULL);

	zassert_true(pm_test_state_locked(),
		     "%s holds no power-state lock while measuring", dev->name);

	zassert_ok(clock_monitor_stop(dev), NULL);
	zassert_false(pm_test_state_locked(),
		      "%s leaked its power-state lock on stop()", dev->name);

	/* stop() aborted the cycle, so no completion may arrive. */
	zassert_not_equal(k_sem_take(&cap.sem, K_MSEC(PM_TEST_SETTLE_MS)), 0,
			  "%s completed a cycle that stop() aborted", dev->name);
}
ZTEST(clock_monitor_pm, test_pm_power_lock_while_measuring)
{
	foreach_measure_device(body_pm_power_lock);
}
#endif /* CONFIG_PM_POLICY_DEVICE_CONSTRAINTS */

#if defined(CONFIG_PM_DEVICE_RUNTIME)
/*
 * With device runtime PM the consumer owns the reference: once the device is
 * released it is suspended, and configure()/start() must refuse to touch it
 * until a reference is taken again.
 */
static void body_pm_runtime_reference(const struct device *dev)
{
	struct clock_monitor_config cfg = measure_cfg;
	int ret;

	if (!pm_device_runtime_is_enabled(dev)) {
		ztest_test_skip();
	}

	/* Drop the runtime reference: the device is suspended synchronously.
	 * -EALREADY means the usage count is already 0.
	 */
	ret = pm_device_runtime_put(dev);
	zassert_true(ret == 0 || ret == -EALREADY,
		     "runtime put failed on %s (%d)", dev->name, ret);

	ret = clock_monitor_configure(dev, &cfg);
	zassert_equal(ret, -EBUSY,
		      "configure() without a runtime reference must return "
		      "-EBUSY on %s (%d)", dev->name, ret);
	ret = clock_monitor_start(dev);
	zassert_equal(ret, -EBUSY,
		      "start() without a runtime reference must return -EBUSY "
		      "on %s (%d)", dev->name, ret);

	zassert_ok(pm_device_runtime_get(dev),
		   "runtime get failed on %s", dev->name);
	zassert_ok(clock_monitor_configure(dev, &cfg),
		   "configure() after runtime get failed on %s", dev->name);
	clean_slate(dev);
}
ZTEST(clock_monitor_pm, test_pm_runtime_reference_required)
{
	foreach_measure_device(body_pm_runtime_reference);
}
#endif /* CONFIG_PM_DEVICE_RUNTIME */
