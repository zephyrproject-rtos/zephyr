/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "pwr_profile_mgr.h"
#include "pwr_profile_backend.h"

LOG_MODULE_REGISTER(pwr_profile_mgr, CONFIG_PWR_PROFILE_MGR_LOG_LEVEL);

#define STEP_TIMEOUT K_MSEC(CONFIG_PWR_PROFILE_MGR_STEP_TIMEOUT_MS)

struct pwr_profile_ctx {
	enum pwr_profile_state state;
	enum pwr_profile_state stable_state;
	bool transition_failed;
	uint8_t retry_count;
};

static struct pwr_profile_ctx ctx = {
	.state = PWR_PROFILE_ACTIVE,
	.stable_state = PWR_PROFILE_ACTIVE,
};

/* ctx fields are guarded by ctx_lock; transitions are serialized by transition_lock. */
static struct k_spinlock ctx_lock;
static K_MUTEX_DEFINE(transition_lock);

static inline size_t drv_index(size_t i, bool ascending)
{
	return ascending ? i : pwr_profile_backend.num_drivers - 1U - i;
}

/* Run the forward path towards @p target; returns the first failure. */
static int forward(enum pwr_profile_state target)
{
	const struct pwr_profile_backend_ops *be = &pwr_profile_backend;
	bool suspending = (target == PWR_PROFILE_SLEEP);
	int rc;

	if (!suspending) {
		rc = be->exit_sleep(STEP_TIMEOUT);
		if (rc != 0) {
			LOG_ERR("exit_sleep failed (%d)", rc);
			return rc;
		}
	}

	for (size_t i = 0; i < be->num_drivers; i++) {
		const struct pwr_profile_drv *d = &be->drivers[drv_index(i, suspending)];

		rc = suspending ? d->suspend(STEP_TIMEOUT) : d->resume(STEP_TIMEOUT);
		if (rc != 0) {
			LOG_ERR("%s: %s failed (%d)", d->name,
				suspending ? "suspend" : "resume", rc);
			return rc;
		}
	}

	if (suspending) {
		rc = be->enter_sleep(STEP_TIMEOUT);
		if (rc != 0) {
			LOG_ERR("enter_sleep failed (%d)", rc);
			return rc;
		}
	}

	return 0;
}

/*
 * Roll back every driver, in the reverse of the forward order. A failing
 * driver does not stop the walk (full per-driver diagnostics); its own
 * rollback is retried up to CONFIG_PWR_PROFILE_MGR_MAX_RETRIES times.
 * Returns the number of drivers whose rollback never succeeded.
 */
static size_t rollback_all(enum pwr_profile_state target, uint8_t *retries)
{
	const struct pwr_profile_backend_ops *be = &pwr_profile_backend;
	bool forward_ascending = (target == PWR_PROFILE_SLEEP);
	size_t failed = 0;

	for (size_t i = 0; i < be->num_drivers; i++) {
		const struct pwr_profile_drv *d = &be->drivers[drv_index(i, !forward_ascending)];
		int rc = d->rollback(target, STEP_TIMEOUT);
		unsigned int attempt = 0;

		while (rc != 0 && attempt < CONFIG_PWR_PROFILE_MGR_MAX_RETRIES) {
			attempt++;
			LOG_WRN("%s: rollback failed (%d), retry %u/%d", d->name, rc, attempt,
				CONFIG_PWR_PROFILE_MGR_MAX_RETRIES);
			rc = d->rollback(target, STEP_TIMEOUT);
		}

		if (*retries + attempt > UINT8_MAX) {
			*retries = UINT8_MAX;
		} else {
			*retries += attempt;
		}

		if (rc != 0) {
			LOG_ERR("%s: rollback failed after %u retries (%d)", d->name, attempt, rc);
			failed++;
		}
	}

	return failed;
}

/*
 * Single implementation of every transition. Fail-stuck design: if the
 * forward path fails and rollback cannot restore every driver, the state
 * stays latched at the transient value (SLEEPING/WAKING) with
 * transition_failed set, and no further hardware operation is attempted.
 * Reverting silently would hide an untrustworthy hardware state.
 */
static int pwr_profile_set_state(enum pwr_profile_state target)
{
	enum pwr_profile_state transient = PWR_PROFILE_SLEEPING;
	uint8_t retries = 0;
	int rc;

	if (target != PWR_PROFILE_ACTIVE && target != PWR_PROFILE_SLEEP) {
		return -EINVAL;
	}

	if (k_mutex_lock(&transition_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	K_SPINLOCK(&ctx_lock) {
		if (ctx.transition_failed) {
			rc = -EIO;
		} else if (ctx.stable_state == target) {
			rc = -EALREADY;
		} else {
			rc = 0;
			transient = (target == PWR_PROFILE_SLEEP) ? PWR_PROFILE_SLEEPING
								  : PWR_PROFILE_WAKING;
			ctx.state = transient;
			ctx.retry_count = 0;
		}
	}

	if (rc != 0) {
		goto out;
	}

	rc = forward(target);
	if (rc == 0) {
		K_SPINLOCK(&ctx_lock) {
			ctx.state = target;
			ctx.stable_state = target;
		}
		goto out;
	}

	/* -EIO is unrecoverable: rollback itself may be unsafe, so skip it. */
	if (rc == -EIO || rollback_all(target, &retries) != 0) {
		K_SPINLOCK(&ctx_lock) {
			ctx.transition_failed = true;
			ctx.retry_count = retries;
		}
		LOG_ERR("fault latched in %s, target must be reset",
			(transient == PWR_PROFILE_SLEEPING) ? "SLEEPING" : "WAKING");
		rc = -EIO;
	} else {
		K_SPINLOCK(&ctx_lock) {
			ctx.state = ctx.stable_state;
			ctx.retry_count = retries;
		}
		rc = -EAGAIN;
	}

out:
	k_mutex_unlock(&transition_lock);
	return rc;
}

enum pwr_profile_state pwr_profile_get_state(void)
{
	enum pwr_profile_state s;

	K_SPINLOCK(&ctx_lock) {
		s = ctx.stable_state;
	}

	return s;
}

int pwr_profile_suspend(void)
{
	return pwr_profile_set_state(PWR_PROFILE_SLEEP);
}

int pwr_profile_resume(void)
{
	return pwr_profile_set_state(PWR_PROFILE_ACTIVE);
}
