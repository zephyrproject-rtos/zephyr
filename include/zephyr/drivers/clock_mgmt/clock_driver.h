/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Internal APIs for clock management drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_CLOCK_DRIVER_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_CLOCK_DRIVER_H_

#include <zephyr/sys/slist.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_mgmt/clock.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clock Driver Interface
 * @defgroup clock_driver_interface Clock Driver Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Return code to indicate clock has no children actively using its
 * output
 */
#define CLK_NO_CHILDREN (1)

/**
 * @brief Helper to issue a clock callback to all children nodes
 *
 * Helper function to issue a callback to all children of a given clock, with
 * a new clock rate. This function will call clock_notify on all children of
 * the given clock, with the provided rate as the parent rate
 *
 * @param clk_hw Clock object to issue callbacks for
 * @param clk_rate Rate clock will be reconfigured to
 * @return 0 on success
 * @return CLK_NO_CHILDREN to indicate clock has no children actively using it,
 *         and may safely shut down.
 * @return -errno from @ref clock_notify on any other failure
 */
#ifdef CONFIG_CLOCK_MGMT_NOTIFY
int clock_notify_children(const struct clk *clk_hw, uint32_t clk_rate);
#else
static inline int clock_notify_children(const struct clk *clk_hw, uint32_t clk_rate)
{
	return 0;
}
#endif

/**
 * @brief Clock Driver API
 *
 * Clock driver API function prototypes. A pointer to a structure of this
 * type should be passed to "CLOCK_DT_DEFINE" when defining the @ref clk
 */
struct clock_driver_api {
	/**
	 * Notify a clock that a parent has been reconfigured.
	 * Note that this must remain the first field in the API structure
	 * to support clock management callbacks
	 */
#if defined(CONFIG_CLOCK_MGMT_NOTIFY) || defined(__DOXYGEN__)
	int (*notify)(const struct clk *clk_hw, const struct clk *parent,
		      uint32_t parent_rate);
#endif
	/** Gets clock rate in Hz */
	int (*get_rate)(const struct clk *clk_hw);
	/** Configure a clock with device specific data */
	int (*configure)(const struct clk *clk_hw, const void *data);
#if defined(CONFIG_CLOCK_MGMT_SET_RATE) || defined(__DOXYGEN__)
	/** Gets nearest rate clock can support, in Hz */
	int (*round_rate)(const struct clk *clk_hw, uint32_t rate);
	/** Sets clock rate in Hz */
	int (*set_rate)(const struct clk *clk_hw, uint32_t rate);
#endif
};

/**
 * @brief Notify clock of reconfiguration
 *
 * Notifies a clock that a reconfiguration event has occurred. Parent clocks
 * should use @ref clock_notify_children to send notifications to all child
 * clocks, and should not use this API directly.
 *
 * This API may also be called by the clock management subsystem directly to
 * notify the clock node that it should attempt to power itself down if is not
 * used.
 *
 * Clocks should forward this notification to their children clocks with
 * @ref clock_notify_children, and if the return code of that call is
 * ``CLK_NO_CHILDREN`` the clock may safely power itself down.
 * @param clk_hw Clock object to notify of reconfiguration
 * @param parent Parent clock device that was reconfigured
 * @param parent_rate Rate of parent clock
 * @return -ENOSYS if clock does not implement notify_children API
 * @return -ENOTSUP if clock child cannot support new rate
 * @return -ENOTCONN to indicate that clock is not using this parent. This can
 *         be useful to multiplexers to indicate to parents that they may safely
 *         shutdown
 * @return negative errno for other error notifying clock
 * @return 0 on success
 */
static inline int clock_notify(const struct clk *clk_hw, const struct clk *parent,
			       uint32_t parent_rate)
{
#ifdef CONFIG_CLOCK_MGMT_NOTIFY
	if (!(clk_hw->api) || !(clk_hw->api->notify)) {
		return -ENOSYS;
	}

	return clk_hw->api->notify(clk_hw, parent, parent_rate);
#else
	return -ENOTSUP;
#endif
}

/**
 * @brief Configure a clock
 *
 * Configure a clock device using hardware specific data. This must also
 * trigger a reconfiguration notification for any consumers of the clock.
 * Called by the clock management subsystem, not intended to be used directly
 * by clock drivers
 * @param clk_hw clock device to configure
 * @param data hardware specific clock configuration data
 * @return -ENOSYS if clock does not implement configure API
 * @return -EIO if clock could not be configured
 * @return -EBUSY if clock cannot be modified at this time
 * @return negative errno for other error configuring clock
 * @return 0 on successful clock configuration
 */
static inline int clock_configure(const struct clk *clk_hw, const void *data)
{
	if (!(clk_hw->api) || !(clk_hw->api->configure)) {
		return -ENOSYS;
	}

	return clk_hw->api->configure(clk_hw, data);
}

/**
 * @brief Get rate of a clock
 *
 * Gets the rate of a clock, in Hz. A rate of zero indicates the clock is
 * active or powered down.
 * @param clk_hw clock device to read rate from
 * @return -ENOSYS if clock does not implement get_rate API
 * @return -EIO if clock could not be read
 * @return negative errno for other error reading clock rate
 * @return frequency of clock output in HZ
 */
static inline int clock_get_rate(const struct clk *clk_hw)
{
	if (!(clk_hw->api) || !(clk_hw->api->get_rate)) {
		return -ENOSYS;
	}

	return clk_hw->api->get_rate(clk_hw);
}

#if defined(CONFIG_CLOCK_MGMT_SET_RATE) || defined(__DOXYGEN__)

/**
 * @brief Take lock on clock configuration
 *
 * Takes a lock on a clock, preventing it from being reconfigured until
 * @ref clock_unlock is called on the same clock by the current lock owner.
 * @param clk_hw clock device to lock configuration of
 * @param owner clock device that will own the lock on this clock.
 * @return 0 on successful lock of the clock
 * @return -EPERM clock could not be locked
 * @return -ENOTSUP if API is not supported
 */
static inline int clock_lock(const struct clk *clk_hw, const struct clk *owner)
{
	if (*clk_hw->owner == clk_handle_get(owner)) {
		/* Clock is already locked by this owner */
		return 0;
	}

	if (*clk_hw->owner != CLOCK_HANDLE_NULL) {
		return -EPERM;
	}

	*clk_hw->owner = clk_handle_get(owner);
	return 0;
}

/**
 * @brief Release lock on clock configuration
 *
 * Releases a lock on a clock, allowing other clock children to reconfigure
 * it. This may only be called by the current owner of the clock.
 * @param clk_hw clock device to unlock configuration of
 * @param owner current owner of the clock
 * @return 0 on successful unlock of the clock
 * @return -EPERM clock could not be unlocked
 * @return -EALREADY clock was already unlocked
 * @return -ENOTSUP if API is not supported
 */
static inline int clock_unlock(const struct clk *clk_hw, const struct clk *owner)
{
	if (*clk_hw->owner == CLOCK_HANDLE_NULL) {
		return -EALREADY;
	}

	if (*clk_hw->owner != clk_handle_get(owner)) {
		return -EPERM;
	}

	*clk_hw->owner = CLOCK_HANDLE_NULL;
	return 0;
}

/**
 * @brief Get nearest rate a clock can support
 *
 * Returns the actual rate that this clock would produce if `clock_set_rate`
 * was called with the requested frequency.
 * @param clk_hw clock device to query
 * @param req_rate Requested clock rate, in Hz
 * @param consumer Clock consumer requesting this rate. Used to determine
 *        if consumer is allowed to reconfigure clock.
 * @return -ENOTSUP if API is not supported
 * @return -ENOSYS if clock does not implement round_rate API
 * @return -EIO if clock could not be queried
 * @return negative errno for other error calculating rate
 * @return rate clock would produce (in Hz) on success
 */
static inline int clock_round_rate(const struct clk *clk_hw, uint32_t req_rate,
				   const struct clk *consumer)
{
	if (!(clk_hw->api) || !(clk_hw->api->round_rate)) {
		return -ENOSYS;
	}

	if ((*clk_hw->owner != CLOCK_HANDLE_NULL) &&
	    (*clk_hw->owner != clk_handle_get(consumer))) {
		/* Clock cannot be reconfigured, just read current rate */
		return clock_get_rate(clk_hw);
	}

	return clk_hw->api->round_rate(clk_hw, req_rate);
}

/**
 * @brief Set a clock rate
 *
 * Sets a clock to the nearest frequency to the requested rate, and locks
 * clock configuration to the clock owner provided by @p owner . The
 * clock may be unlocked by calling @ref clock_unlock with the same owner.
 * @param clk_hw clock device to set rate for
 * @param rate rate to configure clock for, in Hz
 * @param owner clock device that will take ownership of this clock's
 *               configuration
 * @return -ENOTSUP if API is not supported
 * @return -ENOSYS if clock does not implement set_rate API
 * @return -EPERM if clock cannot be reconfigured
 * @return -EIO if clock rate could not be set
 * @return negative errno for other error setting rate
 * @return rate clock is set to produce (in Hz) on success
 */
static inline int clock_set_rate(const struct clk *clk_hw, uint32_t rate,
				 const struct clk *owner)
{
	int ret;

	if (!(clk_hw->api) || !(clk_hw->api->set_rate)) {
		return -ENOSYS;
	}

	ret = clock_lock(clk_hw, owner);
	if (ret < 0) {
		/* Clock is already locked, just read current rate */
		return clock_get_rate(clk_hw);
	}

	return clk_hw->api->set_rate(clk_hw, rate);
}

#else /* if !defined(CONFIG_CLOCK_MGMT_SET_RATE) */

/* Stub functions to indicate set_rate, round_rate, lock, and unlock
 * aren't supported
 */

static inline int clock_lock(const struct clk *clk_hw, const struct clk *owner)
{
	return -ENOTSUP;
}

static inline int clock_unlock(const struct clk *clk_hw, const struct clk *owner)
{
	return -ENOTSUP;
}

static inline int clock_round_rate(const struct clk *clk_hw, uint32_t req_rate,
				   const struct clk *consumer)
{
	return -ENOTSUP;
}

static inline int clock_set_rate(const struct clk *clk_hw, uint32_t rate,
				 const struct clk *owner)
{
	return -ENOTSUP;
}

#endif /* defined(CONFIG_CLOCK_MGMT_SET_RATE) || defined(__DOXYGEN__) */


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_CLOCK_DRIVER_H_ */
