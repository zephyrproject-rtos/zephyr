/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Internal APIs for clock management drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_DRIVER_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_DRIVER_H_

#include <zephyr/sys/slist.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_management/clock.h>
#include <errno.h>
#include <zephyr/logging/log.h>

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
 * @brief Clock management event types
 *
 * Types of events the clock management framework can generate for consumers.
 */
enum clock_management_event_type {
	/**
	 * Clock is about to change from frequency given by
	 * `old_rate` to `new_rate`
	 */
	CLOCK_MANAGEMENT_PRE_RATE_CHANGE,
	/**
	 * Clock has just changed from frequency given by
	 * `old_rate` to `new_rate`
	 */
	CLOCK_MANAGEMENT_POST_RATE_CHANGE,
	/**
	 * Used internally by the clock framework to check if
	 * a clock can accept a frequency given by `new_rate`
	 */
	CLOCK_MANAGEMENT_QUERY_RATE_CHANGE
};

/**
 * @brief Clock notification event structure
 *
 * Notification of clock rate change event. Consumers may examine this
 * structure to determine what rate a clock will change to, as
 * well as to determine if a clock is about to change rate or has already
 */
struct clock_management_event {
	/** Type of event */
	enum clock_management_event_type type;
	/** Old clock rate */
	uint32_t old_rate;
	/** New clock rate */
	uint32_t new_rate;
};

/**
 * @brief Return code to indicate clock has no children actively using its
 * output
 */
#define CLK_NO_CHILDREN (1)


#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
/**
 * @brief Helper to issue a clock callback to all children nodes
 *
 * Helper function to issue a callback to all children of a given clock, using
 * the provided notification event. This function will call clock_notify on
 * all children of the given clock.
 *
 * @param clk_hw Clock object to issue callbacks for
 * @param event Clock reconfiguration event
 * @return 0 on success
 * @return CLK_NO_CHILDREN to indicate clock has no children actively using it,
 *         and may safely shut down.
 * @return -errno from @ref clock_notify on any other failure
 */
int clock_notify_children(const struct clk *clk_hw,
			  const struct clock_management_event *event);
#endif

/**
 * @brief Helper to query children nodes if they can support a rate
 *
 * Helper function to send a notification event to all children nodes via
 * clock_notify, which queries the nodes to determine if they can accept
 * a given rate.
 *
 * @param clk_hw Clock object to issue callbacks for
 * @param rate Rate to query children with
 * @return 0 on success, indicating children can accept rate
 * @return CLK_NO_CHILDREN to indicate clock has no children actively using it,
 *         and may safely shut down.
 * @return -errno from @ref clock_notify on any other failure
 */
static inline int clock_children_check_rate(const struct clk *clk_hw,
					    uint32_t rate)
{
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	const struct clock_management_event event = {
		.type = CLOCK_MANAGEMENT_QUERY_RATE_CHANGE,
		.old_rate = rate,
		.new_rate = rate,
	};
	return clock_notify_children(clk_hw, &event);
#else
	return 0;
#endif
}

/**
 * @brief Helper to notify children nodes a new rate is about to be applied
 *
 * Helper function to send a notification event to all children nodes via
 * clock_notify, which informs the children nodes a new rate is about to
 * be applied.
 *
 * @param clk_hw Clock object to issue callbacks for
 * @param old_rate Current rate of clock
 * @param new_rate Rate clock will change to
 * @return 0 on success, indicating children can accept rate
 * @return CLK_NO_CHILDREN to indicate clock has no children actively using it,
 *         and may safely shut down.
 * @return -errno from @ref clock_notify on any other failure
 */
static inline int clock_children_notify_pre_change(const struct clk *clk_hw,
						   uint32_t old_rate,
						   uint32_t new_rate)
{
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	const struct clock_management_event event = {
		.type = CLOCK_MANAGEMENT_PRE_RATE_CHANGE,
		.old_rate = old_rate,
		.new_rate = new_rate,
	};
	return clock_notify_children(clk_hw, &event);
#else
	return 0;
#endif
}

/**
 * @brief Helper to notify children nodes a new rate has been applied
 *
 * Helper function to send a notification event to all children nodes via
 * clock_notify, which informs the children nodes a new rate has been applied
 *
 * @param clk_hw Clock object to issue callbacks for
 * @param old_rate Old rate of clock
 * @param new_rate Rate clock has changed to
 * @return 0 on success, indicating children accept rate
 * @return CLK_NO_CHILDREN to indicate clock has no children actively using it,
 *         and may safely shut down.
 * @return -errno from @ref clock_notify on any other failure
 */
static inline int clock_children_notify_post_change(const struct clk *clk_hw,
						    uint32_t old_rate,
						    uint32_t new_rate)
{
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	const struct clock_management_event event = {
		.type = CLOCK_MANAGEMENT_POST_RATE_CHANGE,
		.old_rate = old_rate,
		.new_rate = new_rate,
	};
	return clock_notify_children(clk_hw, &event);
#else
	return 0;
#endif
}

/**
 * @brief Clock Driver API
 *
 * Clock driver API function prototypes. A pointer to a structure of this
 * type should be passed to "CLOCK_DT_DEFINE" when defining the @ref clk
 */
struct clock_management_driver_api {
	/**
	 * Notify a clock that a parent has been reconfigured.
	 * Note that this MUST remain the first field in the API structure
	 * to support clock management callbacks
	 */
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	int (*notify)(const struct clk *clk_hw, const struct clk *parent,
		      const struct clock_management_event *event);
#endif
	/** Gets clock rate in Hz */
	int (*get_rate)(const struct clk *clk_hw);
	/** Configure a clock with device specific data */
	int (*configure)(const struct clk *clk_hw, const void *data);
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) || defined(__DOXYGEN__)
	/** Gets nearest rate clock can support given rate request */
	int (*round_rate)(const struct clk *clk_hw, uint32_t rate_req);
	/** Sets clock rate using rate request */
	int (*set_rate)(const struct clk *clk_hw, uint32_t rate_req);
#endif
};

/**
 * @brief Notify clock of reconfiguration
 *
 * Notifies a clock that a reconfiguration event has occurred. Parent clocks
 * should use @ref clock_notify_children to send notifications to all child
 * clocks, and should not use this API directly. Clocks may return an error
 * to reject a rate change.
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
 * @param event Clock reconfiguration event
 * @return -ENOSYS if clock does not implement notify_children API
 * @return -ENOTSUP if clock child cannot support new rate
 * @return -ENOTCONN to indicate that clock is not using this parent. This can
 *         be useful to multiplexers to indicate to parents that they may safely
 *         shutdown
 * @return negative errno for other error notifying clock
 * @return 0 on success
 */
static inline int clock_notify(const struct clk *clk_hw,
			       const struct clk *parent,
			       const struct clock_management_event *event)
{
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (!(clk_hw->api) || !(clk_hw->api->notify)) {
		return -ENOSYS;
	}

	return clk_hw->api->notify(clk_hw, parent, event);
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
	int ret;

	if (!(clk_hw->api) || !(clk_hw->api->configure)) {
		return -ENOSYS;
	}

	ret = clk_hw->api->configure(clk_hw, data);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s reconfigured with result %d", clk_hw->clk_name, ret);
#endif
	return ret;
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
	int ret;

	if (!(clk_hw->api) || !(clk_hw->api->get_rate)) {
		return -ENOSYS;
	}

	ret = clk_hw->api->get_rate(clk_hw);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s returns rate %d", clk_hw->clk_name, ret);
#endif
	return ret;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) || defined(__DOXYGEN__)

/**
 * @brief Get nearest rate a clock can support given constraints
 *
 * Returns the actual rate that this clock would produce if `clock_set_rate`
 * was called with the requested constraints. Clocks should return the highest
 * frequency possible within the requested parameters.
 * @param clk_hw clock device to query
 * @param rate_req requested rate
 * @return -ENOTSUP if API is not supported
 * @return -ENOENT if clock cannot satisfy request
 * @return -ENOSYS if clock does not implement round_rate API
 * @return -EINVAL if arguments are invalid
 * @return -EIO if clock could not be queried
 * @return negative errno for other error calculating rate
 * @return rate clock would produce (in Hz) on success
 */
static inline int clock_round_rate(const struct clk *clk_hw, uint32_t rate_req)
{
	int ret;

	if (!(clk_hw->api) || !(clk_hw->api->round_rate)) {
		return -ENOSYS;
	}

	ret = clk_hw->api->round_rate(clk_hw, rate_req);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s reports rate %d for rate %u",
		clk_hw->clk_name, ret, rate_req);
#endif
	return ret;
}

/**
 * @brief Set a clock rate
 *
 * Sets a clock to the best frequency given the parameters provided in @p req.
 * Clocks should set the highest frequency possible within the requested
 * parameters.
 * @param clk_hw clock device to set rate for
 * @param rate_req requested rate
 * @return -ENOTSUP if API is not supported
 * @return -ENOENT if clock cannot satisfy request
 * @return -ENOSYS if clock does not implement set_rate API
 * @return -EPERM if clock cannot be reconfigured
 * @return -EINVAL if arguments are invalid
 * @return -EIO if clock rate could not be set
 * @return negative errno for other error setting rate
 * @return rate clock is set to produce (in Hz) on success
 */
static inline int clock_set_rate(const struct clk *clk_hw, uint32_t rate_req)
{
	int ret;

	if (!(clk_hw->api) || !(clk_hw->api->set_rate)) {
		return -ENOSYS;
	}

	ret = clk_hw->api->set_rate(clk_hw, rate_req);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	if (ret > 0) {
		LOG_DBG("Clock %s set to rate %d for request %u",
			clk_hw->clk_name, ret, rate_req);
	}
#endif
	return ret;
}

#else /* if !defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) */

/* Stub functions to indicate set_rate and round_rate aren't supported */

static inline int clock_round_rate(const struct clk *clk_hw, uint32_t req_rate)
{
	return -ENOTSUP;
}

static inline int clock_set_rate(const struct clk *clk_hw, uint32_t req_rate)
{
	return -ENOTSUP;
}

#endif /* defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) || defined(__DOXYGEN__) */


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_DRIVER_H_ */
