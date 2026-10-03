/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mdf_interface_npm2100
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_NPM2100_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_NPM2100_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mdf_interface_npm2100 nPM2100
 * @ingroup mfd_interfaces
 * @since 4.1
 * @version 0.1.0
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

enum mfd_npm2100_event {
	NPM2100_EVENT_SYS_DIETEMP_WARN,
	NPM2100_EVENT_SYS_SHIPHOLD_FALL,
	NPM2100_EVENT_SYS_SHIPHOLD_RISE,
	NPM2100_EVENT_SYS_PGRESET_FALL,
	NPM2100_EVENT_SYS_PGRESET_RISE,
	NPM2100_EVENT_SYS_TIMER_EXPIRY,
	NPM2100_EVENT_ADC_VBAT_READY,
	NPM2100_EVENT_ADC_DIETEMP_READY,
	NPM2100_EVENT_ADC_DROOP_DETECT,
	NPM2100_EVENT_ADC_VOUT_READY,
	NPM2100_EVENT_GPIO0_FALL,
	NPM2100_EVENT_GPIO0_RISE,
	NPM2100_EVENT_GPIO1_FALL,
	NPM2100_EVENT_GPIO1_RISE,
	NPM2100_EVENT_BOOST_VBAT_WARN,
	NPM2100_EVENT_BOOST_VOUT_MIN,
	NPM2100_EVENT_BOOST_VOUT_WARN,
	NPM2100_EVENT_BOOST_VOUT_DPS,
	NPM2100_EVENT_BOOST_VOUT_OK,
	NPM2100_EVENT_LDOSW_OCP,
	NPM2100_EVENT_LDOSW_VINTFAIL,
	NPM2100_EVENT_MAX
};

enum mfd_npm2100_timer_mode {
	NPM2100_TIMER_MODE_GENERAL_PURPOSE,
	NPM2100_TIMER_MODE_WDT_RESET,
	NPM2100_TIMER_MODE_WDT_POWER_CYCLE,
	NPM2100_TIMER_MODE_WAKEUP,
};

/** Event bits encoded as BIT(NPM2100_EVENT_*). */
typedef uint32_t npm2100_event_t;

struct mfd_npm2100_event_callback;

/**
 * Handle one PMIC event.
 *
 * @param dev PMIC
 * @param cb Registered subscription
 * @param events Matching event bit
 */
typedef void (*npm2100_callback_handler_t)(const struct device *dev,
					   struct mfd_npm2100_event_callback *cb,
					   npm2100_event_t events);

/**
 * Caller-owned subscription.
 *
 * Handlers run on the system workqueue before event clearing.
 * Handlers may remove only themselves. Serialize external list changes with dispatch.
 * Keep this object intact until removal and dispatch complete.
 */
struct mfd_npm2100_event_callback {
	/** Driver-owned linkage. */
	sys_snode_t node;
	/** Subscribed events. */
	npm2100_event_t event_mask;
	/** Non-NULL event handler. */
	npm2100_callback_handler_t handler;
};

/**
 * Configure the timer without starting it.
 *
 * @param dev PMIC
 * @param time_ms Duration in ms, rounded to 1/64 s
 * @param mode Timer function
 * @retval 0 Success
 * @retval -EINVAL Duration outside timer range
 * @retval -EBUSY Timer running
 * @return Negative errno
 */
int mfd_npm2100_set_timer(const struct device *dev, uint32_t time_ms,
			  enum mfd_npm2100_timer_mode mode);

/**
 * Start the timer.
 *
 * @param dev PMIC
 * @retval 0 Success
 * @return Negative errno
 */
int mfd_npm2100_start_timer(const struct device *dev);

/**
 * Reset PMIC power.
 *
 * @param dev PMIC
 * @retval 0 Success
 * @return Negative errno
 */
int mfd_npm2100_reset(const struct device *dev);

/**
 * Hibernate until SHPHLD or timer wakeup.
 *
 * @param dev PMIC
 * @param time_ms Wake delay in ms; 0 skips timer setup
 * @param pass_through Bypass boost when battery voltage allows
 * @retval 0 Success
 * @retval -EINVAL Duration outside timer range
 * @retval -EBUSY Timer running
 * @return Negative errno
 */
int mfd_npm2100_hibernate(const struct device *dev, uint32_t time_ms, bool pass_through);

/**
 * Register an event callback.
 *
 * Thread context; delivery needs host-int-gpios.
 * Re-registering succeeds without duplication.
 *
 * @param dev PMIC
 * @param[in,out] callback Subscription for one device
 * @retval 0 Callback registered
 * @retval -EINVAL NULL callback or handler
 * @return Negative I2C errno
 */
int mfd_npm2100_add_callback(const struct device *dev, struct mfd_npm2100_event_callback *callback);

/**
 * Unlink an event callback.
 *
 * Thread context; no wait for dispatch. PMIC interrupts stay enabled.
 *
 * @param dev PMIC
 * @param[in,out] callback Registered subscription
 * @retval 0 Callback removed
 * @retval -EINVAL NULL or unregistered callback
 */
int mfd_npm2100_remove_callback(const struct device *dev,
				struct mfd_npm2100_event_callback *callback);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_NPM2100_H_ */
