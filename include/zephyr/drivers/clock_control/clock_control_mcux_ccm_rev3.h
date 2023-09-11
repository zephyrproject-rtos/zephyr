/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public definitions required by CCM Rev3 and its SoC implementations.
 *
 * Please note that some ideas below are marked with the ">" symbol. This means
 * that said ideas are especially important and should be taken note of.
 *
 * 1) Design philosophy
 *	The CCM Rev3 driver was designed with the following goals in mind:
 *		a) Scalability
 *			- the driver should accommodate all CCM
 *			variants found on the i.MX SoCs with as
 *			little modifications as possible.
 *
 *		b) Flexibility
 *			- the driver should support as many use cases as
 *			possible (e.g: clocks that are already configured,
 *			clocks used by drivers not yet implemented in
 *			Zephyr that need to be enabled, virtualized
 *			environments etc...)
 *
 *		c) Ease of use
 *			- the algorithms and complexity of the driver
 *			should be found in the upper layer (the
 *			clock_control_mcux_ccm_rev3.c module) instead
 *			of the SoC layer such that adding a new SoC
 *			implementation should be as simple as implementing
 *			some basic operations.
 *
 * 2) Behaviour specification
 *	a) Gating and ungating a clock.
 *		- there are 3 main functions used to perform these
 *		operations:
 *			I) mcux_ccm_on_off()
 *				> ungating a clock which hasn't been
 *				configured (its frequency is 0) is
 *				forbidden. Doing so will result in
 *				an error.
 *
 *				- trying to gate an already gated clock
 *				or ungate an already ungated clock shall
 *				be ignored. Return code is 0.
 *
 *			II) mcux_ccm_on()
 *				> this function ungates all relatives
 *				of the clock passed as argument. The
 *				reason for this behaviour is because,
 *				generally clocks on lower levels
 *				depend on the clocks from higher levels.
 *
 *			III) mcux_ccm_off()
 *				> this function gates only the clock
 *				passed as argument. The reason for this
 *				behaviour is because, generally clocks
 *				from higher levels are depended upon by
 *				multiple clocks from higer levels. As
 *				such, attempting to gate higher level
 *				clocks would surely affect multiple
 *				subsystems.
 *
 *				- this function should be used with care
 *				as it might affect multiple subsystems
 *				if the specified clock is used by
 *				other clocks.
 *
 *	b) Querying a clock's rate.
 *		- this operation is performed by mcux_ccm_get_rate().
 *		This function shall fail if the clock hasn't been
 *		configured (its frequency is 0).
 *
 *	c) Setting a clock's rate.
 *		- this operation is performed by mcux_ccm_set_rate().
 *
 *		> since one may need to also update the rates
 *		of a clock's relatives whenever setting its
 *		rate, this function first updates the rates
 *		of the clock's relatives. The rate setting
 *		is performed from higher levels to higher
 *		levels. For example, assuming we're dealing with
 *		the following clock tree:
 *
 *			----A----
 *			|       |
 *			|       |
 *			B       C
 *                      |       |
 *                      |       |
 *                      D       E
 *
 *              setting D's rate will result in attempting
 *              to also set B and A's rates in the following
 *              order:
 *			1) set_rate(A)
 *			2) set_rate(B)
 *			3) set_rate(D)
 *
 *		This is because it's assumed that D's rate
 *		depends on B's rate, which depends on A's
 *		rate.
 *
 *		Of course, this could be problematic during
 *		runtime as updating A's rate will also affect
 *		C and E. As such, if the SoC layer forbids it,
 *		the set_rate() function may stop earlier (this
 *		is signaled by the fact that imx_ccm_get_parent_rate()
 *		will return -EPERM). This function may also stop
 *		earlier if imx_ccm_get_parent_rate() returns
 *		-EALREADY, meaning the current clock's parent is
 *		already set to the requested rate.
 *
 *		> this function maintains the states of the clocks
 *		during the tree traversal. For instance, if clocks
 *		A, B, and D were initially gated, they will remain
 *		gated after set_rate() is called.
 *
 *		> before setting a clock's rate, this function shall
 *		first gate the clock.
 *
 *	d) Initializing clocks
 *		- clocks specified through the assign-clock* properties
 *		are initialized during the driver's initialization function.
 *		This is done by mcux_ccm_clock_init().
 *
 *		> because clocks from lower levels depend on clocks
 *		from higher levels, the array of clocks is first sorted
 *		by the clock levels. As such, clocks from higher levels
 *		shall be initialized before clocks from lower levels.
 *
 *		> this function gates all clocks.
 *
 *		- clocks specified through the "assigned-clocks" property
 *		are assigned the parents specified through the
 *		"assigned-clock-parents" property (if existing) and are
 *		set to the rates specified through the "assigned-clock-rates"
 *		property.
 *
 *		- the "assigned-clocks", "assigned-clock-parents", and
 *		"assigned-clock-rates" properties are scattered throughout
 *		the DTS nodes. To gather the arrays specified through
 *		this properties, the CCM Rev3 driver iterates through
 *		all DTS nodes, looking for nodes that employ these properties.
 *		If a node does indeed have these properties, the CCM Rev3
 *		driver will append the arrays to its internal arrays.
 *			* notes:
 *				>1) No iteration order shall be assumed.
 *
 *				>2) The properties needs to be consistent.
 *				If one DTS node specifies the
 *				"assigned-clock-parents", which is optional
 *				relative to the other 2 properties (meaning
 *				one may specify "assigned-clocks" and
 *				"assigned-clock-rates" but not
 *				"assigned-clock-parents") then all other
 *				DTS nodes shall also specify it. Not doing
 *				so will result in a build error.
 *
 *				>3) At the moment, the CCM Rev3 driver does
 *				not check if the node's status is "okay".
 *				As such, these properties should be used
 *				with care.
 *
 *	e) Initializing clocks assumed to be on.
 *		- to specify clocks that you know are already
 *		configured and turned on by another instance
 *		(e.g: the ROM code or another OS running in
 *		parallel), one can use the "clocks-assume-on"
 *		property.
 *
 *		- the initialization code will simply take
 *		the clocks specified through the aforementioned
 *		property and their rates and directly set
 *		the required fields in the associated
 *		struct imx_ccm_clock.
 *
 *		- as clocks specified through the assigned-clock*
 *		properties may depend on these clocks, it's required
 *		that these clocks be initialized before the other
 *		ones.
 *
 *	f) Ungating clocks upon initialization
 *		- to ungate clocks during CCM Rev3's initialization,
 *		one can make use of the "clocks-init-on" property
 *		which will ungate all of the clocks passed via this
 *		property.
 *
 * 3) Assumptions
 *	a) The clock tree
 *		- the CCM Rev3 makes no assumptions regarding
 *		the clock tree. It may contain as many levels
 *		as the SoC layer needs.
 *
 *		- it's assumed that the peripherals use the
 *		clocks from lower levels (although this
 *		is in no way enforced and one could get around
 *		this by specifying clocks from higher levels
 *		using the "clocks" property).
 *
 * 4) Misc information
 *	a) Computing a clock's level in the clock tree
 *		- a clock's level is computed as the
 *		number of parents one needs to traverse
 *		to reach the NULL parent.
 *
 *		- e.g:
 *			------- A ------
 *			|       |      |
 *			|       |      |
 *			B       C      F
 *			|       |
 *			|       |
 *			D       E
 *
 *		level(A) = 2
 *		level(B) = level(C) = level(F) = 1
 *		level(D) = level(E) = 0
 *
 *		- usually, peripherals use clocks like F,
 *		D, or E (terminal nodes), but this may not
 *		always be the case.
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCUX_CCM_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCUX_CCM_H_

#include <zephyr/device.h>
#include <errno.h>

/** @brief Clock state.
 *
 * This structure is used to represent the states
 * a CCM clock may be found in.
 */
enum imx_ccm_clock_state {
	/** Clock is currently gated. */
	IMX_CCM_CLOCK_STATE_GATED = 0,
	/** Clock is currently ungated. */
	IMX_CCM_CLOCK_STATE_UNGATED,
};

/** @brief Clock structure.
 *
 * This is the most important structure, used to represent a clock
 * in the CCM. The CCM Rev3 driver only knows how to operate with this
 * structure.
 *
 * If you ever need to store more data about your clock then just
 * create your new clock structure, which will contain a struct
 * imx_ccm_clock as its first member. Using this strategy, you
 * can just cast your clock data to a generic struct imx_ccm_clock *
 * which can be safely used by CCM Rev3 driver. An example of this
 * can be seen in imx93_ccm.c.
 */
struct imx_ccm_clock {
	/** Name of the clock. */
	char *name;
	/** NXP HAL clock encoding. */
	uint32_t id;
	/** Clock frequency. */
	uint32_t freq;
	/** Clock state. */
	enum imx_ccm_clock_state state;
	/** Clock parent. */
	struct imx_ccm_clock *parent;
};

/** @brief Clock operations.
 *
 * Since many clock operations are SoC-dependent, this structure
 * provides a set of operations each SoC needs to define to assure
 * the functionality of CCM Rev3.
 */
struct imx_ccm_clock_api {
	int (*on_off)(const struct device *dev,
		      struct imx_ccm_clock *clk, bool on);
	int (*set_clock_rate)(const struct device *dev,
			      struct imx_ccm_clock *clk, uint32_t rate);
	int (*get_clock)(uint32_t clk_id, struct imx_ccm_clock **clk);
	int (*assign_parent)(const struct device *dev,
			     struct imx_ccm_clock *clk,
			     struct imx_ccm_clock *parent);
	bool (*rate_is_valid)(const struct device *dev,
			      struct imx_ccm_clock *clk, uint32_t rate);
	int (*get_parent_rate)(struct imx_ccm_clock *clk,
			       struct imx_ccm_clock *parent,
			       uint32_t rate, uint32_t *parent_rate);
};

struct imx_ccm_config {
	uint32_t regmap_phys;
	uint32_t pll_regmap_phys;

	uint32_t regmap_size;
	uint32_t pll_regmap_size;
};

struct imx_ccm_data {
	mm_reg_t regmap;
	mm_reg_t pll_regmap;
	struct imx_ccm_clock_api *api;
};

/**
 * @brief Validate if rate is valid for a clock.
 *
 * This function checks if a given rate is valid for a given clock.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param clk Clock data.
 * @param rate Clock frequency.
 *
 * @retval true if rate is valid for clock, false otherwise.
 */
static inline bool imx_ccm_rate_is_valid(const struct device *dev,
					 struct imx_ccm_clock *clk,
					 uint32_t rate)
{
	struct imx_ccm_data *data = dev->data;

	if (!data->api || !data->api->rate_is_valid) {
		return -EINVAL;
	}

	return data->api->rate_is_valid(dev, clk, rate);
}

/**
 * @brief Assign a clock parent.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param clk Clock data.
 * @param parent Parent data.
 *
 * @retval 0 if successful, negative value otherwise.
 */
static inline int imx_ccm_assign_parent(const struct device *dev,
					struct imx_ccm_clock *clk,
					struct imx_ccm_clock *parent)
{
	struct imx_ccm_data *data = dev->data;

	if (!data->api || !data->api->on_off) {
		return -EINVAL;
	}

	return data->api->assign_parent(dev, clk, parent);
}

/**
 * @brief Turn on or off a clock.
 *
 * Some clocks may not support gating operations. In such cases, this
 * function should still return 0 as if it were successful.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param clk Clock data.
 * @param on Should the clock be turned on or off?
 *
 * @retval 0 if successful, negative value otherwise.
 */
static inline int imx_ccm_on_off(const struct device *dev,
				 struct imx_ccm_clock *clk, bool on)
{
	struct imx_ccm_data *data = dev->data;

	if (!data->api || !data->api->on_off) {
		return -EINVAL;
	}

	return data->api->on_off(dev, clk, on);
}

/**
 * @brief Set a clock's frequency.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param clk Clock data.
 * @param rate The requested frequency.
 *
 * @retval positive value representing the obtained clock rate.
 * @retval -ENOTSUP if operation is not supported by the clock.
 * @retval negative value if any error occurs.
 */
static inline int imx_ccm_set_clock_rate(const struct device *dev,
					 struct imx_ccm_clock *clk,
					 uint32_t rate)
{
	struct imx_ccm_data *data = dev->data;

	if (!data->api || !data->api->set_clock_rate) {
		return -EINVAL;
	}

	return data->api->set_clock_rate(dev, clk, rate);
}

/**
 * @brief Retrieve a clock's data.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param clk_id Clock ID.
 * @param clk Clock data
 *
 * @retval 0 if successful, negative value otherwise.
 */
static inline int imx_ccm_get_clock(const struct device *dev,
				    uint32_t clk_id,
				    struct imx_ccm_clock **clk)
{
	struct imx_ccm_data *data = dev->data;

	if (!data->api || !data->api->get_clock) {
		return -EINVAL;
	}

	return data->api->get_clock(clk_id, clk);
}

/**
 * @brief Get parent's rate if we were to assign rate to clock clk.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param clk Targeted clock data.
 * @param parent Targeted clock's parent data.
 * @param rate Rate we wish to assign the clock.
 * @param parent_rate The rate we need to assign the parent clock.
 *
 * @retval 0 if successful, negative value otherwise.
 * @retval -EPERM if setting parent's rate is not allowed.
 * @retval -EALREADY if parent is configured such that it allows
 * clock clk to be set to rate rate.
 * @retval -ENOTSUP if there's no rate the parent can be assigned
 * such that clock clk can be set to rate rate.
 * @retval negative value if not successful.
 */
static inline int imx_ccm_get_parent_rate(const struct device *dev,
					  struct imx_ccm_clock *clk,
					  struct imx_ccm_clock *parent,
					  uint32_t rate,
					  uint32_t *parent_rate)
{
	struct imx_ccm_data *data = dev->data;

	if (!data->api || !data->api->get_parent_rate) {
		return -EINVAL;
	}

	return data->api->get_parent_rate(clk, parent, rate, parent_rate);
}

/**
 * @brief Perform SoC-specific CCM initialization.
 *
 * Apart from SoC-specific initialization, it's expected that this
 * function will also set the CCM driver API from struct imx_ccm_data.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful, negative value otherwise.
 */
int imx_ccm_init(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCUX_CCM_H_ */
