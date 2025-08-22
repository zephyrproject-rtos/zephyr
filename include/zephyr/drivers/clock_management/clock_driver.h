/*
 * Copyright 2024 NXP
 * Copyright (c) 2025 Tenstorrent AI ULC
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
#include <zephyr/toolchain.h>

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
 * @brief Shared Clock driver API
 *
 * These clock APIs are shared between standard, multiplexer, and root clocks.
 */
struct clock_management_shared_api {
	/** Configure a clock with device specific data */
	int (*configure)(const struct clk *clk_hw, const void *data);
	/** Turn a clock on or off */
	int (*on_off)(const struct clk *clk_hw, bool on);
};

/**
 * @brief Get a clock's only parent.
 *
 * This macro gets a pointer to the clock's only parent, handling the difference
 * in access between when CONFIG_CLOCK_MANAGEMENT_RUNTIME is enabled or not.
 * It should only be used by "standard type" clocks.
 */
#define GET_CLK_PARENT(clk) (((struct clk_standard_subsys_data *)clk->hw_data)->parent)
/**
 * @brief Get a clock's parent array.
 *
 * This macro gets a pointer to the clock's parent array, handling the difference
 * in access between when CONFIG_CLOCK_MANAGEMENT_RUNTIME is enabled or not.
 * It should only be used by "multiplexer type" clocks.
 */
#define GET_CLK_PARENTS(clk) (((struct clk_mux_subsys_data *)clk->hw_data)->parents)

/**
 * @brief Get shared data for a clock
 *
 * Get the shared data structure for a clock. The contents of this structure
 * vary depending on the type of clock. This macro should only be used by
 * the clock subsystem.
 */
#define GET_CLK_SHARED_DATA(clk) ((struct clk_shared_subsys_data *)clk->hw_data)

/**
 * @brief Standard Clock Driver API
 *
 * This clock driver API is utilized for clocks that have a singular parent,
 * which are considered "standard clocks". A pointer to this structure should
 * be passed to "CLOCK_DT_DEFINE" when defining the @ref clk that has a singular
 * parent.
 */
struct clock_management_standard_api {
	/** Shared API. Must be first member of structure */
	struct clock_management_shared_api shared;
	/** Recalculate a clock rate given a parent's new clock rate */
	clock_freq_t (*recalc_rate)(const struct clk *clk_hw, clock_freq_t parent_rate);
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/** Recalculate a clock rate given device specific configuration data */
	clock_freq_t (*configure_recalc)(const struct clk *clk_hw, const void *data,
				clock_freq_t parent_rate);
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) || defined(__DOXYGEN__)
	/** Gets nearest rate clock can support given rate request */
	clock_freq_t (*round_rate)(const struct clk *clk_hw, clock_freq_t rate_req,
			  clock_freq_t parent_rate);
	/** Sets clock rate using rate request */
	clock_freq_t (*set_rate)(const struct clk *clk_hw, clock_freq_t rate_req,
			clock_freq_t parent_rate);
#endif
};

/**
 * @brief Multiplexer Clock Driver API
 *
 * This clock driver API is utilized for clocks that have multiple parents,
 * which are considered "multiplexer clocks". A pointer to this structure should
 * be passed to "CLOCK_DT_DEFINE_MUX" when defining the @ref clk that has
 * multiple parents. Note that multiplexer clocks may only switch between clock
 * outputs, they cannot modify the rate of the clock output they select.
 */
struct clock_management_mux_api {
	/** Shared API. Must be first member of structure */
	struct clock_management_shared_api shared;
	/** Get parent of clock */
	int (*get_parent)(const struct clk *clk_hw);
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/** Query new parent selection based on device specific configuration data */
	int (*mux_configure_recalc)(const struct clk *clk_hw, const void *data);
	/** Validate mux can accept a new parent */
	int (*mux_validate_parent)(const struct clk *clk_hw, clock_freq_t parent_freq,
				   uint8_t new_idx);
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) || defined(__DOXYGEN__)
	/** Sets parent clock */
	int (*set_parent)(const struct clk *clk_hw, uint8_t new_idx);
#endif
};

/**
 * @brief Root Clock Driver API
 *
 * This clock driver API is utilized for clocks that no parents, which are
 * considered "root clocks". A pointer to this structure should
 * be passed to "CLOCK_DT_DEFINE_ROOT" when defining the @ref clk that has
 * no parents.
 */
struct clock_management_root_api {
	/** Shared API. Must be first member of structure */
	struct clock_management_shared_api shared;
	/** Get rate of clock  */
	clock_freq_t (*get_rate)(const struct clk *clk_hw);
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/** Recalculate a clock rate given device specific configuration data */
	clock_freq_t (*root_configure_recalc)(const struct clk *clk_hw, const void *data);
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) || defined(__DOXYGEN__)
	/** Gets nearest rate clock can support given rate request */
	clock_freq_t (*root_round_rate)(const struct clk *clk_hw, clock_freq_t rate_req);
	/** Sets clock rate using rate request */
	clock_freq_t (*root_set_rate)(const struct clk *clk_hw, clock_freq_t rate_req);
#endif
};

/**
 * @brief Configure a clock
 *
 * Configure a clock device using hardware specific data. Called by the clock
 * management subsystem, not intended to be used directly by clock drivers
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
	const struct clock_management_shared_api *api = clk_hw->api;

	if (!(api) || !(api->configure)) {
		return -ENOSYS;
	}
	ret = api->configure(clk_hw, data);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s reconfigured with result %d", clk_hw->clk_name, ret);
#endif
	return ret;
}

/**
 * @brief Turn a clock on or off
 *
 * Turns a clock on or off. This may be used to gate a clock, or to power it
 * down. The specific behavior is implementation defined, and may vary by clock
 * driver.
 * @param clk_hw clock device to turn on or off
 * @param on true to turn the clock on, false to turn it off
 * @return -ENOSYS if clock does not implement on_off API
 * @return -EIO if clock could not be turned on or off
 * @return -EBUSY if clock cannot be modified at this time
 * @return negative errno for other error turning clock on or off
 * @return 0 on success
 */
static inline int clock_onoff(const struct clk *clk_hw, bool on)
{
	int ret;
	const struct clock_management_shared_api *api = clk_hw->api;

	if (!(api) || !(api->on_off)) {
		return -ENOSYS;
	}
	ret = api->on_off(clk_hw, on);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s %s", clk_hw->clk_name, on ? "enabled" : "disabled");
#endif
	return ret;
}

/**
 * @brief Get rate of a clock
 *
 * Gets the current rate of a clock. This function is used by the clock
 * management subsystem to determine the clock's operating frequency.
 * It is only relevant for clocks at the root of the tree. Other clocks
 * calculate their rate via clock_recalc_rate.
 *
 * @param clk_hw clock device to read rate from
 * @return -ENOSYS if clock does not implement get_rate API
 * @return -EIO if clock could not be read
 * @return -ENOTCONN if clock is not yet configured (will produce zero rate)
 * @return negative errno for other error reading clock rate
 * @return clock rate on success
 */
static inline clock_freq_t clock_get_rate(const struct clk *clk_hw)
{
	clock_freq_t ret;
	const struct clock_management_root_api *api = clk_hw->api;

	if (!(api) || !(api->get_rate)) {
		return -ENOSYS;
	}

	ret = api->get_rate(clk_hw);
	if (ret == -ENOTCONN) {
		/* Clock isn't configured, rate is 0 */
		return 0;
	}
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s returns rate %d", clk_hw->clk_name, ret);
#endif
	return ret;
}

/**
 * @brief Get parent of a clock
 *
 * Gets the current parent of a clock. This function is used by the clock
 * management subsystem to determine the current parent of a clock.
 *
 * @param clk_hw clock device to read rate from
 * @return -ENOSYS if clock does not implement get_parent API
 * @return -ENOTCONN if clock is not yet configured (will produce zero rate)
 * @return -EIO if clock could not be read
 * @return negative errno for other error reading clock parent
 * @return index of parent in parent array on success
 */
static inline int clock_get_parent(const struct clk *clk_hw)
{
	int ret;
	const struct clock_management_mux_api *api = clk_hw->api;

	if (!(api) || !(api->get_parent)) {
		return -ENOSYS;
	}

	ret = api->get_parent(clk_hw);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	if (ret >= 0) {
		LOG_DBG("Clock %s returns parent %s", clk_hw->clk_name,
			GET_CLK_PARENTS(clk_hw)[ret]->clk_name);
	}
#endif
	return ret;
}

/**
 * @brief Recalculate a clock frequency given a new parent frequency
 *
 * Calculate the frequency that a clock would generate if its parent were
 * reconfigured to the frequency @p parent_rate. This call does not indicate
 * that the clock has been reconfigured, and is simply a query.Called by the
 * clock management subsystem, not intended for use directly within drivers.
 * @param clk_hw clock to recalculate rate for
 * @param parent_rate new frequency parent would update to
 * @return -ENOSYS if API is not supported by this clock
 * @return -EINVAL if clock cannot accept rate
 * @return -EIO if calculation is not possible
 * @return -ENOTCONN if clock is not yet configured (will produce zero rate)
 * @return negative errno for other error calculating rate
 * @return rate clock would produce with @p parent_rate on success
 */
static inline clock_freq_t clock_recalc_rate(const struct clk *clk_hw,
				    clock_freq_t parent_rate)
{
	clock_freq_t ret;
	const struct clock_management_standard_api *api = clk_hw->api;

	if (!(api) || !(api->recalc_rate)) {
		return -ENOSYS;
	}
	ret = api->recalc_rate(clk_hw, parent_rate);
	if (ret == -ENOTCONN) {
		/* Clock isn't configured, rate is 0 */
		return 0;
	}
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s would produce frequency %d from parent rate %d",
		clk_hw->clk_name, ret, parent_rate);
#endif
	return ret;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
/**
 * @brief Recalculate a clock frequency prior to configuration
 *
 * Calculate the new frequency a clock device would generate prior to
 * applying a hardware specific configuration blob. The clock should not
 * apply the setting when this function is called, simply calculate what
 * the new frequency would be. Called by the clock management subsystem, not
 * intended for use directly within drivers.
 * @param clk_hw clock device to query
 * @param data hardware specific clock configuration data
 * @param parent_rate clock rate of this clock's parent
 * @return -ENOSYS if clock does not implement configure_recalc API
 * @return -EBUSY if clock cannot be modified at this time
 * @return -EINVAL if the configuration data in invalid
 * @return -EIO for I/O error configuring clock
 * @return negative errno for other error configuring clock
 * @return rate clock would produce with @p data configuration on success
 */
static inline clock_freq_t clock_configure_recalc(const struct clk *clk_hw,
					const void *data, clock_freq_t parent_rate)
{
	clock_freq_t ret;
	const struct clock_management_standard_api *api = clk_hw->api;

	if (!(api) || !(api->configure_recalc)) {
		return -ENOSYS;
	}
	ret = api->configure_recalc(clk_hw, data, parent_rate);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s would produce frequency %d after configuration",
		clk_hw->clk_name, ret);
#endif
	return ret;
}

/**
 * @brief Recalculate a root clock frequency prior to configuration
 *
 * Calculate the new frequency a root clock device would generate prior to
 * applying a hardware specific configuration blob. The clock should not
 * apply the setting when this function is called, simply calculate what
 * the new frequency would be. Called by the clock management subsystem, not
 * intended for use directly within drivers.
 * @param clk_hw clock device to query
 * @param data hardware specific clock configuration data
 * @return -ENOSYS if clock does not implement root_configure_recalc API
 * @return -EBUSY if clock cannot be modified at this time
 * @return -EINVAL if the configuration data in invalid
 * @return -EIO for I/O error configuring clock
 * @return negative errno for other error configuring clock
 * @return rate clock would produce with @p data configuration on success
 */
static inline clock_freq_t clock_root_configure_recalc(const struct clk *clk_hw,
					      const void *data)
{
	clock_freq_t ret;
	const struct clock_management_root_api *api = clk_hw->api;

	if (!(api) || !(api->root_configure_recalc)) {
		return -ENOSYS;
	}
	ret = api->root_configure_recalc(clk_hw, data);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s would produce frequency %d after configuration",
		clk_hw->clk_name, ret);
#endif
	return ret;
}

/**
 * @brief Recalculate the parent in use for a mux after reconfiguration
 *
 * Calculate the parent clock a mux will use after reconfiguration. The
 * clock should not select a new parent clock when this function is called,
 * it should simply report which parent the mux would use.
 * @param clk_hw clock device to query
 * @param data hardware specific clock configuration data
 * @return -ENOSYS if clock does not implement mux_configure_recalc API
 * @return -EBUSY if clock cannot be modified at this time
 * @return -EINVAL if the configuration data in invalid
 * @return -EIO for I/O error configuring clock
 * @return negative errno for other error configuring clock
 * @return index of new parent in parent array when using @p data on success
 */
static inline int clock_mux_configure_recalc(const struct clk *clk_hw,
					      const void *data)
{
	int ret;
	const struct clock_management_mux_api *api = clk_hw->api;

	if (!(api) || !(api->mux_configure_recalc)) {
		return -ENOSYS;
	}
	ret = api->mux_configure_recalc(clk_hw, data);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	if (ret >= 0) {
		LOG_DBG("Clock %s would use parent %s after reconfiguration",
			clk_hw->clk_name, GET_CLK_PARENTS(clk_hw)[ret]->clk_name);
	}
#endif
	return ret;
}

/**
 * @brief Validate that a mux can accept a new parent
 *
 * Validate that a mux can accept a new parent clock. This will be called
 * before the mux is reconfigured to a new parent clock, and allows the mux
 * to indicate it is unable to accept the new parent.
 * @param clk_hw clock device to query
 * @param parent_freq frequency of proposed parent
 * @param new_idx New parent index
 * @return -ENOSYS if clock does not implement mux_validate_parent API
 * @return -EBUSY if clock cannot be modified at this time
 * @return -EINVAL if the mux index is invalid
 * @return -ENOTSUP if api is not supported
 * @return negative errno for other error validating parent
 * @return 0 on success
 */
static inline int clock_mux_validate_parent(const struct clk *clk_hw,
					   clock_freq_t parent_freq,
					   uint8_t new_idx)
{
	int ret;
	const struct clock_management_mux_api *api = clk_hw->api;

	if (!(api) || !(api->mux_validate_parent)) {
		return -ENOSYS;
	}
	ret = api->mux_validate_parent(clk_hw, parent_freq, new_idx);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s %s parent %s",
		clk_hw->clk_name, (ret == 0) ? "accepts" : "rejects",
		GET_CLK_PARENTS(clk_hw)[new_idx]->clk_name);
#endif
	return ret;
}

#else
/* Stub functions to indicate recalc isn't supported */

static inline clock_freq_t clock_configure_recalc(const struct clk *clk_hw, const void *data,
					 clock_freq_t parent_rate)
{
	return -ENOTSUP;
}

static inline clock_freq_t clock_root_configure_recalc(const struct clk *clk_hw, const void *data)
{
	return -ENOTSUP;
}

static inline int clock_mux_configure_recalc(const struct clk *clk_hw, const void *data)
{
	return -ENOTSUP;
}

static inline int clock_mux_validate_parent(const struct clk *clk_hw,
					    clock_freq_t parent_freq, uint8_t new_idx)
{
	return -ENOTSUP;
}

#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) || defined(__DOXYGEN__)

/**
 * @brief Get nearest rate a clock can support
 *
 * Returns the actual rate that this clock would produce if `clock_set_rate`
 * was called with the requested frequency.
 * @param clk_hw clock device to query
 * @param rate_req requested rate
 * @param parent_rate best parent clock rate offered based on request
 * @return -ENOTSUP if API is not supported
 * @return -ENOENT if clock cannot satisfy request
 * @return -ENOSYS if clock does not implement round_rate API
 * @return -EINVAL if arguments are invalid
 * @return -EBUSY if clock can't be reconfigured
 * @return -EIO if clock could not be queried
 * @return negative errno for other error calculating rate
 * @return best rate clock could produce on success
 */
static inline clock_freq_t clock_round_rate(const struct clk *clk_hw, clock_freq_t rate_req,
				   clock_freq_t parent_rate)
{
	clock_freq_t ret;
	const struct clock_management_standard_api *api = clk_hw->api;

	if (!(api) || !(api->round_rate)) {
		return -ENOSYS;
	}

	ret = api->round_rate(clk_hw, rate_req, parent_rate);
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
 * Sets a clock to the closest frequency possible given the requested rate.
 * @param clk_hw clock device to set rate for
 * @param rate_req requested rate
 * @param parent_rate best parent clock rate offered based on request
 * @return -ENOTSUP if API is not supported
 * @return -ENOENT if clock cannot satisfy request
 * @return -ENOSYS if clock does not implement set_rate API
 * @return -EPERM if clock cannot be reconfigured
 * @return -EINVAL if arguments are invalid
 * @return -EIO if clock rate could not be set
 * @return -EBUSY if clock can't be reconfigured
 * @return negative errno for other error setting rate
 * @return rate clock now produces on success
 */
static inline clock_freq_t clock_set_rate(const struct clk *clk_hw, clock_freq_t rate_req,
				clock_freq_t parent_rate)
{
	clock_freq_t ret;
	const struct clock_management_standard_api *api = clk_hw->api;

	if (!(api) || !(api->set_rate)) {
		return -ENOSYS;
	}

	ret = api->set_rate(clk_hw, rate_req, parent_rate);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	if (ret > 0) {
		LOG_DBG("Clock %s set to rate %d for request %u",
			clk_hw->clk_name, ret, rate_req);
	}
#endif
	return ret;
}

/**
 * @brief Get nearest rate a root clock can support
 *
 * Returns the actual rate that this clock would produce if `clock_root_set_rate`
 * was called with the requested frequency.
 * @param clk_hw clock device to query
 * @param rate_req requested rate
 * @return -ENOTSUP if API is not supported
 * @return -ENOENT if clock cannot satisfy request
 * @return -ENOSYS if clock does not implement round_rate API
 * @return -EINVAL if arguments are invalid
 * @return -EIO if clock could not be queried
 * @return -EBUSY if clock can't be reconfigured
 * @return negative errno for other error calculating rate
 * @return best rate clock could produce on success
 */
static inline clock_freq_t clock_root_round_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{
	clock_freq_t ret;
	const struct clock_management_root_api *api = clk_hw->api;

	if (!(api) || !(api->root_round_rate)) {
		return -ENOSYS;
	}

	ret = api->root_round_rate(clk_hw, rate_req);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s reports rate %d for rate %u",
		clk_hw->clk_name, ret, rate_req);
#endif
	return ret;
}

/**
 * @brief Set a root clock rate
 *
 * Sets a clock to the closest frequency possible given the requested rate.
 * @param clk_hw clock device to set rate for
 * @param rate_req requested rate
 * @return -ENOTSUP if API is not supported
 * @return -ENOENT if clock cannot satisfy request
 * @return -ENOSYS if clock does not implement set_rate API
 * @return -EPERM if clock cannot be reconfigured
 * @return -EINVAL if arguments are invalid
 * @return -EIO if clock rate could not be set
 * @return -EBUSY if clock can't be reconfigured
 * @return negative errno for other error setting rate
 * @return rate clock now produces on success
 */
static inline clock_freq_t clock_root_set_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{
	clock_freq_t ret;
	const struct clock_management_root_api *api = clk_hw->api;

	if (!(api) || !(api->root_set_rate)) {
		return -ENOSYS;
	}

	ret = api->root_set_rate(clk_hw, rate_req);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	if (ret > 0) {
		LOG_DBG("Clock %s set to rate %d for request %u",
			clk_hw->clk_name, ret, rate_req);
	}
#endif
	return ret;
}

/**
 * @brief Set the parent clock for a mux clock
 *
 * Sets the parent clock for a multiplexer clock device.
 * @param clk_hw clock device to set parent for
 * @param new_idx new parent index
 * @return -ENOSYS if clock does not implement set_parent API
 * @return -EIO if clock parent could not be set
 * @return -EINVAL if arguments are invalid
 * @return -EBUSY if clock can't be reconfigured
 * @return negative errno for other error setting parent
 * @return 0 on success
 */
static inline int clock_set_parent(const struct clk *clk_hw, uint8_t new_idx)
{
	int ret;
	const struct clock_management_mux_api *api = clk_hw->api;

	if (!(api) || !(api->set_parent)) {
		return -ENOSYS;
	}

	ret = api->set_parent(clk_hw, new_idx);
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_MODULE_DECLARE(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);
	LOG_DBG("Clock %s set parent to %s", clk_hw->clk_name,
		GET_CLK_PARENTS(clk_hw)[new_idx]->clk_name);
#endif
	return ret;
}

#else /* if !defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE) */

/* Stub functions to indicate SET_RATE APIs aren't supported */

static inline clock_freq_t clock_round_rate(const struct clk *clk_hw, clock_freq_t req_rate,
				   clock_freq_t parent_rate)
{
	return -ENOTSUP;
}

static inline clock_freq_t clock_set_rate(const struct clk *clk_hw, clock_freq_t req_rate,
				  clock_freq_t parent_rate)
{
	return -ENOTSUP;
}

static inline clock_freq_t clock_root_round_rate(const struct clk *clk_hw, clock_freq_t req_rate)
{
	return -ENOTSUP;
}

static inline clock_freq_t clock_root_set_rate(const struct clk *clk_hw, clock_freq_t req_rate)
{
	return -ENOTSUP;
}

static inline int clock_set_parent(const struct clk *clk_hw, uint8_t new_idx)
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
