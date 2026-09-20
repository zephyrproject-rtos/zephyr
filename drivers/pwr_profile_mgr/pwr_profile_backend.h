/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PWR_PROFILE_MGR_PWR_PROFILE_BACKEND_H_
#define ZEPHYR_DRIVERS_PWR_PROFILE_MGR_PWR_PROFILE_BACKEND_H_

#include <stddef.h>
#include <zephyr/kernel.h>

#include "pwr_profile_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Error contract for every backend callback:
 *   0          success
 *   -EIO       unrecoverable hardware fault; core skips rollback and latches
 *   any other  recoverable (e.g. -ETIMEDOUT, -EAGAIN); core rolls back
 * Each callback must return -ETIMEDOUT if it cannot finish within @p timeout.
 */

/** One integrated peripheral (LED, UART shell, accelerometer, ...). */
struct pwr_profile_drv {
	const char *name;
	int (*suspend)(k_timeout_t timeout);
	int (*resume)(k_timeout_t timeout);
	/*
	 * Undo a failed transition towards @p target, restoring the state
	 * before the attempt. Must be idempotent: it is called for every
	 * driver, including ones the failed transition never reached.
	 */
	int (*rollback)(enum pwr_profile_state target, k_timeout_t timeout);
};

/** Board-specific backend; exactly one instance, named pwr_profile_backend. */
struct pwr_profile_backend_ops {
	int (*enter_sleep)(k_timeout_t timeout);
	int (*exit_sleep)(k_timeout_t timeout);
	const struct pwr_profile_drv *drivers;
	size_t num_drivers;
};

extern const struct pwr_profile_backend_ops pwr_profile_backend;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_PWR_PROFILE_MGR_PWR_PROFILE_BACKEND_H_ */
