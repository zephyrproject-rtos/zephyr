/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PWR_PROFILE_MGR_PWR_PROFILE_MGR_H_
#define ZEPHYR_DRIVERS_PWR_PROFILE_MGR_PWR_PROFILE_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Power profile states.
 *
 * PWR_PROFILE_SLEEP maps to STM32 Stop mode (SRAM retained), not to the
 * STM32 "Sleep mode" described in RM0090.
 * SLEEPING and WAKING are internal transient states; they double as the
 * fault-latch value when a transition cannot be rolled back.
 */
enum pwr_profile_state {
	PWR_PROFILE_ACTIVE,
	PWR_PROFILE_SLEEPING,
	PWR_PROFILE_SLEEP,
	PWR_PROFILE_WAKING,
};

/** Trigger commands; every trigger source reduces to one of these. */
enum pwr_profile_cmd {
	PWR_PROFILE_CMD_SUSPEND,
	PWR_PROFILE_CMD_RESUME,
};

/**
 * @brief Get the current stable profile state.
 *
 * Never returns a transient state: while a transition is in flight, or
 * after a fault latch, the last stable state is returned.
 *
 * @retval PWR_PROFILE_ACTIVE or PWR_PROFILE_SLEEP
 */
enum pwr_profile_state pwr_profile_get_state(void);

/**
 * @brief Request the Active -> Sleep transition. Thread context only.
 *
 * @retval 0          Suspended.
 * @retval -EALREADY  Already in PWR_PROFILE_SLEEP.
 * @retval -EBUSY     Another transition is in flight.
 * @retval -EAGAIN    Recoverable failure, rolled back to ACTIVE.
 * @retval -EIO       Unrecoverable failure or fault latched; reset target.
 */
int pwr_profile_suspend(void);

/**
 * @brief Request the Sleep -> Active transition. Thread context only.
 *
 * @retval 0          Resumed.
 * @retval -EALREADY  Already in PWR_PROFILE_ACTIVE.
 * @retval -EBUSY     Another transition is in flight.
 * @retval -EAGAIN    Recoverable failure, rolled back to SLEEP.
 * @retval -EIO       Unrecoverable failure or fault latched; reset target.
 */
int pwr_profile_resume(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_PWR_PROFILE_MGR_PWR_PROFILE_MGR_H_ */
