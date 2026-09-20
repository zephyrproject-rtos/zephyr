/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/ztest.h>

#include "pwr_profile_backend.h"

#define NUM_DRV 3

/* Fault injection, set per test. */
static int suspend_rc[NUM_DRV];
static int resume_rc[NUM_DRV];
static int rollback_fail_count[NUM_DRV]; /* fail this many rollback calls, then succeed */
static int rollback_rc[NUM_DRV];         /* error returned while failing */
static int enter_rc, exit_rc;

/* Observations. */
static int rollback_calls[NUM_DRV];
static int suspend_calls[NUM_DRV];

#define DRV_FN(n)                                                                                  \
	static int drv##n##_suspend(k_timeout_t t)                                                 \
	{                                                                                          \
		suspend_calls[n]++;                                                                \
		return suspend_rc[n];                                                              \
	}                                                                                          \
	static int drv##n##_resume(k_timeout_t t)                                                  \
	{                                                                                          \
		return resume_rc[n];                                                               \
	}                                                                                          \
	static int drv##n##_rollback(enum pwr_profile_state target, k_timeout_t t)                 \
	{                                                                                          \
		rollback_calls[n]++;                                                               \
		if (rollback_fail_count[n] > 0) {                                                  \
			rollback_fail_count[n]--;                                                  \
			return rollback_rc[n];                                                     \
		}                                                                                  \
		return 0;                                                                          \
	}

DRV_FN(0)
DRV_FN(1)
DRV_FN(2)

static int stub_enter(k_timeout_t t)
{
	return enter_rc;
}

static int stub_exit(k_timeout_t t)
{
	return exit_rc;
}

#define DRV_ENTRY(n, nm) {.name = nm, .suspend = drv##n##_suspend, .resume = drv##n##_resume, \
			  .rollback = drv##n##_rollback}

static const struct pwr_profile_drv drivers[NUM_DRV] = {
	DRV_ENTRY(0, "led"), DRV_ENTRY(1, "uart"), DRV_ENTRY(2, "accel"),
};

const struct pwr_profile_backend_ops pwr_profile_backend = {
	.enter_sleep = stub_enter,
	.exit_sleep = stub_exit,
	.drivers = drivers,
	.num_drivers = NUM_DRV,
};

static void reset_stub(void *unused)
{
	memset(suspend_rc, 0, sizeof(suspend_rc));
	memset(resume_rc, 0, sizeof(resume_rc));
	memset(rollback_fail_count, 0, sizeof(rollback_fail_count));
	memset(rollback_rc, 0, sizeof(rollback_rc));
	memset(rollback_calls, 0, sizeof(rollback_calls));
	memset(suspend_calls, 0, sizeof(suspend_calls));
	enter_rc = exit_rc = 0;
}

ZTEST_SUITE(pwr_profile_core, NULL, NULL, reset_stub, NULL, NULL);

ZTEST(pwr_profile_core, test_a_normal_cycle)
{
	zassert_equal(pwr_profile_get_state(), PWR_PROFILE_ACTIVE);
	zassert_equal(pwr_profile_suspend(), 0);
	zassert_equal(pwr_profile_get_state(), PWR_PROFILE_SLEEP);
	zassert_equal(pwr_profile_suspend(), -EALREADY);
	zassert_equal(pwr_profile_resume(), 0);
	zassert_equal(pwr_profile_get_state(), PWR_PROFILE_ACTIVE);
	zassert_equal(pwr_profile_resume(), -EALREADY);
}

ZTEST(pwr_profile_core, test_b_recoverable_failure_reverts)
{
	suspend_rc[1] = -ETIMEDOUT;

	zassert_equal(pwr_profile_suspend(), -EAGAIN);
	zassert_equal(pwr_profile_get_state(), PWR_PROFILE_ACTIVE);
	/* every driver is rolled back, including the one that never ran */
	for (int i = 0; i < NUM_DRV; i++) {
		zassert_equal(rollback_calls[i], 1, "driver %d", i);
	}
	zassert_equal(suspend_calls[2], 0, "forward path stops at first failure");

	suspend_rc[1] = 0;
	zassert_equal(pwr_profile_suspend(), 0, "module still usable after clean revert");
	zassert_equal(pwr_profile_resume(), 0);
}

ZTEST(pwr_profile_core, test_c_rollback_retry_succeeds)
{
	suspend_rc[0] = -ETIMEDOUT;
	rollback_fail_count[2] = 2;
	rollback_rc[2] = -ETIMEDOUT;

	zassert_equal(pwr_profile_suspend(), -EAGAIN);
	zassert_equal(rollback_calls[2], 3, "two failures + one success");
	zassert_equal(rollback_calls[0], 1, "retry is per-driver only");
	zassert_equal(pwr_profile_get_state(), PWR_PROFILE_ACTIVE);
}

ZTEST(pwr_profile_core, test_d_enter_sleep_failure_reverts)
{
	enter_rc = -EAGAIN;

	zassert_equal(pwr_profile_suspend(), -EAGAIN);
	zassert_equal(pwr_profile_get_state(), PWR_PROFILE_ACTIVE);
}

ZTEST(pwr_profile_core, test_e_rollback_exhausted_latches)
{
	suspend_rc[0] = -ETIMEDOUT;
	rollback_fail_count[0] = 100;
	rollback_rc[0] = -EAGAIN;
	rollback_fail_count[1] = 100;
	rollback_rc[1] = -EAGAIN;

	zassert_equal(pwr_profile_suspend(), -EIO);
	/* walk-all: both failing drivers retried max times, third still visited */
	zassert_equal(rollback_calls[0], 1 + CONFIG_PWR_PROFILE_MGR_MAX_RETRIES);
	zassert_equal(rollback_calls[1], 1 + CONFIG_PWR_PROFILE_MGR_MAX_RETRIES);
	zassert_equal(rollback_calls[2], 1);
	zassert_equal(pwr_profile_get_state(), PWR_PROFILE_ACTIVE,
		      "callers never see a transient state");

	/* latched: no further hardware operation */
	int calls = suspend_calls[0];

	zassert_equal(pwr_profile_suspend(), -EIO);
	zassert_equal(pwr_profile_resume(), -EIO);
	zassert_equal(suspend_calls[0], calls);
}

ZTEST(pwr_profile_core, test_f_unrecoverable_skips_rollback_and_latches)
{
	/* runs after test_e, which latched the module: only checks still-latched state */
	zassert_equal(pwr_profile_suspend(), -EIO);
	for (int i = 0; i < NUM_DRV; i++) {
		zassert_equal(rollback_calls[i], 0);
	}
}
