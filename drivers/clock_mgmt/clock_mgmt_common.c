/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt.h>

#ifdef CONFIG_CLOCK_MGMT_NOTIFY
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
int clock_notify_children(const struct clk *clk_hw, uint32_t clk_rate)
{
	const clock_handle_t *handle = clk_hw->children;
	int ret;
	bool children_disconnected = true;

	while (*handle != CLOCK_LIST_END) {
		ret = clock_notify(clk_from_handle(*handle), clk_hw, clk_rate);
		if (ret == 0) {
			/* At least one child is using this clock */
			children_disconnected = false;
		} else if ((ret < 0) && (ret != -ENOTCONN)) {
			/* ENOTCONN simply means MUX is disconnected.
			 * other return codes should be propagated.
			 */
			return ret;
		}
		handle++;
	}
	return children_disconnected ? CLK_NO_CHILDREN : 0;
}
#endif

#ifdef CONFIG_CLOCK_MGMT_NOTIFY

/*
 * Common handler used to notify clock consumers of clock events.
 * This handler is used by the clock management subsystem to notify consumers
 * via callback that a parent was reconfigured
 */
int clock_mgmt_notify_consumer(const struct clk *clk_hw, const struct clk *parent,
			       uint32_t parent_rate)
{
	const struct clock_mgmt *clock_mgmt = clk_hw->hw_data;
	struct clock_mgmt_callback *callback = clock_mgmt->callback;

	if (!callback->clock_callback) {
		/* No callback installed */
		return 0;
	}

	/* Call clock notification callback */
	for (uint8_t i = 0; i < clock_mgmt->output_count; i++) {
		if (parent == clock_mgmt->outputs[i]) {
			/* Issue callback */
			return callback->clock_callback(i, parent_rate,
						 callback->user_data);
		}
	}
	return 0;
}

/* API structure used by clock management code for clock consumers */
const struct clock_mgmt_clk_api clock_consumer_api = {
	.notify = clock_mgmt_notify_consumer,
};

#endif

/**
 * @brief Set new clock state
 *
 * Sets new clock state. This function will apply a clock state as defined
 * in devicetree. Clock states can configure clocks systemwide, or only for
 * the relevant peripheral driver. Clock states are defined as clock-state-"n"
 * properties of the devicetree node for the given driver.
 * @param clk_cfg Clock management structure
 * @param state_idx Clock state index
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if state index could not be found
 * @return -ENOSYS if clock does not implement configure API
 * @return -EIO if state could not be set
 * @return -EBUSY if clocks cannot be modified at this time
 * @return 0 on success
 */
int clock_mgmt_apply_state(const struct clock_mgmt *clk_cfg,
			   uint8_t state_idx)
{
	const struct clock_mgmt_state *state;
	int ret;

	if (!clk_cfg) {
		return -EINVAL;
	}

	if (clk_cfg->state_count <= state_idx) {
		return -ENOENT;
	}

	state = clk_cfg->states[state_idx];

	for (uint8_t i = 0; i < state->num_clocks; i++) {
		ret = clock_configure(state->clocks[i],
				      state->clock_config_data[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
