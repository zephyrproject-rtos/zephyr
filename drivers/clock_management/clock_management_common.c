/*
 * Copyright 2024 NXP
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_management.h>
#include <zephyr/drivers/clock_management/clock_helpers.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include "clock_management_common.h"
LOG_MODULE_REGISTER(clock_management, CONFIG_CLOCK_MANAGEMENT_LOG_LEVEL);

#define DT_DRV_COMPAT clock_output


/*
 * If runtime clocking is disabled, we have no need to store clock output
 * structures for every consumer, so consumers simply get a pointer to the
 * underlying clock object. This macro handles the difference in accessing
 * the clock based on if runtime clocking is enabled or not.
 */
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
#define GET_CLK_CORE(_clk) (_clk->clk_core)
#else
#define GET_CLK_CORE(_clk) ((const struct clk *)_clk)
#endif

k_spinlock_key_t spinlock_key;
struct k_spinlock clock_mgmt_spinlock;

/*
 * Describes a clock setting. This structure records the
 * clock to configure, as well as the clock-specific configuration
 * data to pass to that clock
 */
struct clock_setting {
	const struct clk *const clock;
	const void *clock_config_data;
};

/*
 * Describes statically defined clock output states. Each state
 * contains an array of settings for parent nodes of this clock output,
 * a frequency that will result from applying those settings,
 */
struct clock_output_state {
	/* Number of clock nodes to configure */
	const uint8_t num_clocks;
	/* Clock configuration settings for each clock */
	const struct clock_setting clock_settings[];
};

/* Clock output node private data */
struct clock_output_data {
	/* Parent clock of this output node */
	const struct clk *parent;
	/* Statically defined clock output states */
	const struct clock_output_state *const *output_states;
	/* Number of output states */
	uint8_t num_output_states;
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	/*
	 * Array with the following contents:
	 * idx 0: number of active requests
	 * idx i + 1: number of requests that have marked state i as allowed
	 */
	uint8_t *request_states;
	/* Token for the consumer which has locked the clock */
	struct clock_management_data **consumer;
	/* Start of the consumer array (defined by the linker) */
	struct clock_output *consumer_start;
	/* End of the consumer array (defined by the linker) */
	struct clock_output *consumer_end;
#endif
};

/* Index in the "request_states" array for active request count */
#define CLOCK_OUTPUT_ACTIVE_REQUEST_COUNT_IDX 0
/* Start of array with the number of requests that have marked state i as allowed */
#define CLOCK_OUTPUT_REQUEST_COUNT_IDX 1

/* Section used to identify clock types */
TYPE_SECTION_START_EXTERN(struct clk, clk);
TYPE_SECTION_END_EXTERN(struct clk, clk_root);
TYPE_SECTION_END_EXTERN(struct clk, clk_mux);
TYPE_SECTION_END_EXTERN(struct clk, clk_leaf);
TYPE_SECTION_END_EXTERN(struct clk, clk_standard);

/*
 * Helper function to get the type of a clock.
 * Uses the section location to determine clock type.
 */
static uint8_t clock_get_type(const struct clk *clk_hw)
{
	/*
	 * Since all these sections are contiguous in ROM, we only need to check
	 * if a clock is within the clk section, and then just which section it
	 * lies before
	 */
	if (clk_hw < TYPE_SECTION_END(clk_root)) {
		return CLK_TYPE_ROOT;
	} else if (clk_hw < TYPE_SECTION_END(clk_standard)) {
		return CLK_TYPE_STANDARD;
	} else if (clk_hw < TYPE_SECTION_END(clk_mux)) {
		return CLK_TYPE_MUX;
	} else if (clk_hw < TYPE_SECTION_END(clk_leaf)) {
		return CLK_TYPE_LEAF;
	}
	__builtin_unreachable();
}

/**
 * @brief Check the rate of a given clock
 *
 * This function is primarily used by the clock subsystem but drivers can call
 * into it as well where needed. It recursively calls itself until it encounters
 * clock whose rate is known or can be calculated, then calls recalc_rate on
 * children clocks to determine a final rate
 *
 * @param clk_hw Clock to check the rate for
 * @return clock rate on success, or negative value on error
 */
clock_freq_t clock_management_clk_rate(const struct clk *clk_hw)
{
	clock_freq_t current_rate, ret;

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (clk_hw->subsys_data->rate != 0) {
		return clk_hw->subsys_data->rate;
	}
#endif

	if (clock_get_type(clk_hw) == CLK_TYPE_ROOT) {
		/* Base case- just get the rate of this clock */
		current_rate = clock_get_rate(clk_hw);
	} else if (clock_get_type(clk_hw) == CLK_TYPE_STANDARD) {
		/* Recursive. Single parent clock, use recalc_rate */
		ret = clock_management_clk_rate(GET_CLK_PARENT(clk_hw));
		if (ret < 0) {
			return ret;
		}
		current_rate = clock_recalc_rate(clk_hw, ret);
	} else {
		/* Recursive. Multi parent clock, get the parent and return its rate */
		ret = clock_get_parent(clk_hw);
		if (ret == -ENOTCONN) {
			/* Clock has no parent, it is disconnected */
			return 0;
		} else if (ret < 0) {
			/* Error getting parent */
			return ret;
		}
		current_rate = clock_management_clk_rate(GET_CLK_PARENTS(clk_hw)[ret]);
	}

	IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME, (
		if (current_rate >= 0) {
			LOG_DBG("Clock %s returns rate %d", clk_hw->clk_name, current_rate);
		}
	))
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (current_rate >= 0) {
		/* Cache rate */
		clk_hw->subsys_data->rate = current_rate;
	}
#endif
	return current_rate;
}

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME

/**
 * Helper function to recursively disable children of a given clock
 *
 * This function will disable all children of the specified clock with a usage
 * count of zero, as well as the current clock if it is unused.
 */
static void clk_disable_unused(const struct clk *clk_hw)
{
	const clock_handle_t *handle = clk_hw->children;
	const struct clk *child;

	/* Recursively disable unused children */
	while (*handle != CLOCK_LIST_END) {
		child = clk_from_handle(*handle);
		clk_disable_unused(child);
		handle++;
	}

	/* Check if the current clock is unused */
	if (clk_hw->subsys_data->usage_cnt == 0) {
		/* Disable the clock */
		clock_onoff(clk_hw, false);
	}
}

#endif

/**
 * @brief Disable unused clocks within the system
 *
 * Disable unused clocks within the system. This API will gate all clocks in
 * the system with a usage count of zero, when CONFIG_CLOCK_MANAGEMENT_RUNTIME
 * is enabled.
 */
void clock_management_disable_unused(void)
{
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	STRUCT_SECTION_FOREACH_ALTERNATE(clk_root, clk, clk) {
		clk_disable_unused(clk);
	}
#endif
}

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME

/**
 * Helper function to notify clock of reconfiguration event
 *
 * @param clk_hw Clock which will have rate reconfigured
 * @param old_freq Current frequency of clock
 * @param new_freq New frequency that clock will configure to
 * @param ev_type Type of clock notification event
 * @return 0 if notification chain succeeded, or error if not
 */
static int clock_notify_children(const struct clk *clk_hw,
				 clock_freq_t old_freq,
				 clock_freq_t new_freq,
				 enum clock_management_event_type ev_type)
{
	const struct clock_management_event event = {
		.type = ev_type,
		.old_rate = old_freq,
		.new_rate = new_freq
	};
	const clock_handle_t *handle = clk_hw->children;
	const struct clock_output_data *data;
	const struct clock_output *consumer;
	struct clock_management_callback *cb;
	const struct clk *child;
	int ret, parent_idx;
	clock_freq_t child_newrate, child_oldrate;

	if (*handle == CLOCK_LIST_END) {
		/* Base case- clock leaf (output node) */
		data = clk_hw->hw_data;
		/* Check if the clock is locked. If so reject reconfiguration */
		if (*(data->consumer) != NULL) {
			IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME, (
				LOG_DBG("Clock %s is locked",
					clk_hw->clk_name);
			))
			return -EPERM;
		}
		/* Notify consumers */
		for (consumer = data->consumer_start;
		     consumer < data->consumer_end; consumer++) {
			cb = consumer->cb;
			if (cb->clock_callback) {
				ret = cb->clock_callback(&event,
						   cb->user_data);
				if (ret) {
					/* Consumer rejected new rate */
					return ret;
				}
			}
		}
	} else {
		/* Recursive case- clock with children */
		for (handle = clk_hw->children; *handle != CLOCK_LIST_END; handle++) {
			/* Recalculate rate of this child */
			child = clk_from_handle(*handle);
			if (clock_get_type(child) == CLK_TYPE_LEAF) {
				/* Child is a clock output node, just notify it */
				child_oldrate = old_freq;
				child_newrate = new_freq;
			} else if (clock_get_type(child) == CLK_TYPE_STANDARD) {
				/* Single parent clock, use recalc */
				child_newrate = clock_recalc_rate(child, new_freq);
				if (child_newrate < 0) {
					IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME, (
						LOG_DBG("Clock %s rejected rate %u",
							clk_hw->clk_name, new_freq);
					))
					return child_newrate;
				}
				child_oldrate = clock_recalc_rate(child, old_freq);
				if (child_oldrate < 0) {
					return child_oldrate;
				}
			} else {
				/* Multi parent clock, see if it is connected */
				parent_idx = clock_get_parent(child);
				if (parent_idx == -ENOTCONN) {
					/* Clock has no parent, it is disconnected */
					continue;
				} else if (parent_idx < 0) {
					/* Error getting parent */
					return parent_idx;
				}
				if (GET_CLK_PARENTS(child)[parent_idx] != clk_hw) {
					/* Disconnected */
					continue;
				}
				ret = clock_mux_validate_parent(child, new_freq, parent_idx);
				if (ret < 0) {
					IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME, (
						LOG_DBG("Mux %s rejected rate %u, parent %s",
							child->clk_name, new_freq,
							GET_CLK_PARENTS(child)
							[parent_idx]->clk_name);
					))
					return ret;
				}
				/* Clock is connected. Child rate will match parent */
				child_newrate = new_freq;
				child_oldrate = old_freq;
			}
			/* Notify its children of new rate */
			ret = clock_notify_children(child, child_oldrate,
						    child_newrate, ev_type);
			if (ret < 0) {
				return ret;
			}
		}
	}
	if (ev_type == CLOCK_MANAGEMENT_POST_RATE_CHANGE) {
		/* Update the clock's shared data */
		clk_hw->subsys_data->rate = new_freq;
	}

	return 0;
}

/**
 * Helper function to handle reconfiguration process for clock
 *
 * @param clk_hw Clock which will have rate reconfigured
 * @param cfg_param Configuration parameter to pass into clock_configure
 * @return 0 if change was applied successfully, or error if not
 */
static int clock_tree_configure(const struct clk *clk_hw,
				const void *cfg_param)
{
	clock_freq_t current_rate, new_rate, parent_rate;
	int ret, parent_idx;

	if (clock_get_type(clk_hw) == CLK_TYPE_ROOT) {
		current_rate =  clock_get_rate(clk_hw);
		if (current_rate < 0) {
			return current_rate;
		}
		new_rate = clock_root_configure_recalc(clk_hw,
					cfg_param);
		if (new_rate < 0) {
			return new_rate;
		}
	} else if (clock_get_type(clk_hw) == CLK_TYPE_STANDARD) {
		/* Single parent clock */
		parent_rate = clock_management_clk_rate(
			GET_CLK_PARENT(clk_hw));
		if (parent_rate < 0) {
			return parent_rate;
		}
		current_rate = clock_recalc_rate(clk_hw, parent_rate);
		if (current_rate < 0) {
			return current_rate;
		}
		new_rate = clock_configure_recalc(clk_hw, cfg_param,
					     parent_rate);
		if (new_rate < 0) {
			return new_rate;
		}
	} else {
		/* Multi parent clock */
		current_rate = clock_management_clk_rate(clk_hw);
		if (current_rate < 0) {
			return current_rate;
		}
		/* Get new parent rate */
		parent_idx = clock_mux_configure_recalc(clk_hw, cfg_param);
		if (parent_idx < 0) {
			return parent_idx;
		}
		new_rate = clock_management_clk_rate(GET_CLK_PARENTS(clk_hw)[parent_idx]);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_mux_validate_parent(clk_hw, new_rate, parent_idx);
		if (ret < 0) {
			IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME, (
				LOG_DBG("Mux %s rejected rate %u, parent %s",
					clk_hw->clk_name, new_rate,
					GET_CLK_PARENTS(clk_hw)[parent_idx]->clk_name);
			))
			return ret;
		}
	}

	/* Validate children can accept rate */
	ret = clock_notify_children(clk_hw, current_rate, new_rate,
				    CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
	if (ret < 0) {
		return ret;
	}
	/* Now, notify children rates will change */
	ret = clock_notify_children(clk_hw, current_rate, new_rate,
				    CLOCK_MANAGEMENT_PRE_RATE_CHANGE);
	if (ret < 0) {
		return ret;
	}
	/* Apply the new rate */
	ret = clock_configure(clk_hw, cfg_param);
	if (ret < 0) {
		return ret;
	}
	/* Now, notify children rates have changed */
	ret = clock_notify_children(clk_hw, current_rate, new_rate,
				    CLOCK_MANAGEMENT_POST_RATE_CHANGE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
 * @brief Checks the children of a clock to validate they can support a given rate
 *
 * This function will validate that children of the provided clock can support
 * the new rate proposed. Some clock implementations may need to call this if
 * they will reconfigure into intermediate states in the process of changing
 * their rate, to make sure the clock tree can also support those rates.
 *
 * @param clk_hw Clock to check children for
 * @param new_rate Proposed new rate of the clock
 * @return 0 if all children can support the new rate, or negative value on error
 */
int clock_children_check_rate(const struct clk *clk_hw, clock_freq_t new_rate)
{
	clock_freq_t current_rate;

	current_rate = clock_management_clk_rate(clk_hw);
	if (current_rate < 0) {
		return current_rate;
	}
	return clock_notify_children(clk_hw, current_rate, new_rate,
				     CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
}

/**
 * @brief Lock a clock output to block further reconfiguration
 *
 * Locks a clock output, preventing any reconfiguration from occurring to the
 * clock until @ref clock_management_unlock is called by the same clock consumer.
 * This does not prevent other consumers from issuing requests to the clock, but
 * they will be denied unless the request can be satisfied within changing the
 * state the clock has selected.
 * @param data Clock management data structure for the device
 * @param clk clock output to lock
 * @return 0 on success
 * @return -EBUSY if the clock is already locked by another consumer
 */
int clock_management_lock(const struct clock_management_data *data, clock_output_t clk)
{
	const struct clock_output_data *clk_data;

	if (!data || clk >= data->num_outputs) {
		return -EINVAL;
	}
	clk_data = GET_CLK_CORE(data->clock_outputs[clk])->hw_data;
	/* Set the lock token on the clock output */
	if (*(clk_data->consumer) == data) {
		/* Clock is already locked by us, no-op */
		return 0;
	} else if (*(clk_data->consumer) != NULL) {
		/* Clock is already locked by another consumer */
		return -EBUSY;
	}
	/* Mark clock as locked */
	*(clk_data->consumer) = (struct clock_management_data *)data;
	return 0;
}

/**
 * @brief Unlock a clock output to allow further reconfiguration
 *
 * Unlocks a clock output, allowing any reconfiguration to occur to the
 * clock. Should only be called by a consumer that has previously locked the
 * clock using @ref clock_management_lock.
 * @param data Clock management data structure for the device
 * @param clk clock output to unlock
 * @return 0 on success
 * @return -EPERM if the clock is not locked by the calling consumer
 */
int clock_management_unlock(const struct clock_management_data *data, clock_output_t clk)
{
	const struct clock_output_data *clk_data;

	if (!data || clk >= data->num_outputs) {
		return -EINVAL;
	}
	clk_data = GET_CLK_CORE(data->clock_outputs[clk])->hw_data;
	if (*(clk_data->consumer) == NULL) {
		/* Clock is already unlocked */
		return 0;
	} else if (*(clk_data->consumer) != data) {
		/* Clock is locked by another consumer */
		return -EPERM;
	}
	/* Mark clock as unlocked */
	*(clk_data->consumer) = NULL;
	return 0;
}

#else /* CONFIG_CLOCK_MANAGEMENT_RUNTIME */

/**
 * @brief Checks the children of a clock to validate they can support a given rate
 *
 * This function will validate that children of the provided clock can support
 * the new rate proposed. Some clock implementations may need to call this if
 * they will reconfigure into intermediate states in the process of changing
 * their rate, to make sure the clock tree can also support those rates.
 *
 * @param clk_hw Clock to check children for
 * @param new_rate Proposed new rate of the clock
 * @return 0 if all children can support the new rate, or negative value on error
 */
int clock_children_check_rate(const struct clk *clk_hw, clock_freq_t new_rate)
{
	/* No-op */
	return 0;
}

/**
 * Helper function to handle reconfiguration process for clock
 *
 * @param clk_hw Clock which will have rate reconfigured
 * @param cfg_param Configuration parameter to pass into clock_configure
 * @return 0 if change was applied successfully, or error if not
 */
static int clock_tree_configure(const struct clk *clk_hw,
				const void *cfg_param)
{
	return -ENOTSUP;
}

#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)

/* Forwards declaration */
static clock_freq_t clock_management_round_internal(const struct clk *clk_hw,
						    clock_freq_t req_freq);

/**
 * Helper function to find the best parent of a multiplexer for a requested rate.
 * This is needed both in the round_rate and set_rate phases of clock configuration.
 *
 * @param clk_hw Multiplexer to find best parent for
 * @param req_freq Requested clock frequency
 * @param best_parent Set to best parent found for request
 * @return best possible rate on success, or negative value on error
 */
static clock_freq_t clock_management_best_parent(const struct clk *clk_hw,
						 clock_freq_t req_freq,
						 int *best_parent)
{
	int ret;
	uint32_t best_delta = UINT32_MAX, delta;
	clock_freq_t cand_rate, current_rate, best_rate;
	const struct clk *cand_parent;
	const struct clk_mux_subsys_data *mux_data = clk_hw->hw_data;

	/* Evaluate each parent clock. If one fails for any reason, just skip it */
	for (int i = 0; i < mux_data->parent_cnt; i++) {
		cand_parent = mux_data->parents[i];
		cand_rate = clock_management_round_internal(cand_parent, req_freq);
		if (cand_rate < 0) {
			continue; /* Not a candidate */
		}
		ret = clock_mux_validate_parent(clk_hw, cand_rate, i);
		if (ret < 0) {
			continue; /* Not a candidate */
		}
		current_rate = clock_management_clk_rate(clk_hw);
		if (current_rate < 0) {
			continue; /* Not a candidate */
		}
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME, (
			LOG_DBG("Mux %s offers rate %u from parent %s",
				clk_hw->clk_name, cand_rate, cand_parent->clk_name);
		))
		/* Validate that this rate can work for the children */
		ret = clock_notify_children(clk_hw, current_rate, cand_rate,
					    CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
		if (ret < 0) {
			/* Clock won't be able to reconfigure for this rate */
			continue;
		}
		delta = abs(cand_rate - req_freq);
		if (delta < best_delta) {
			best_delta = delta;
			best_rate = cand_rate;
			*best_parent = i;
		}
	}
	/* If we didn't find a suitable clock, indicate error here */
	return (best_delta == UINT32_MAX) ? -ENOTSUP : best_rate;
}

/**
 * @brief Helper function to determine best clock configuration for a request
 *
 * This helper function determines the best clock configuration for a given
 * request. The function will select the clock configuration that results in a
 * frequency closest to the requested frequency.
 *
 * @param clk_hw Clock to find configuration for
 * @param req_freq Requested clock frequency
 * @return best possible rate on success, or negative value on error
 */
static clock_freq_t clock_management_round_internal(const struct clk *clk_hw,
						    clock_freq_t req_freq)
{
	int ret;
	clock_freq_t parent_rate, current_rate, best_rate;
	int best_parent;

	if (clock_get_type(clk_hw) == CLK_TYPE_MUX) {
		/* Mux clocks don't support round_rate, we implement it generically */
		best_rate = clock_management_best_parent(clk_hw, req_freq,
							&best_parent);
	} else if (clock_get_type(clk_hw) == CLK_TYPE_ROOT) {
		/* No need to check parents */
		current_rate = clock_get_rate(clk_hw);
		if (current_rate < 0) {
			return current_rate;
		}
		best_rate = clock_root_best_rate(clk_hw, req_freq, false);
		if (best_rate < 0) {
			/* Clock can't reconfigure, use the current rate */
			best_rate = current_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, best_rate,
					    CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	} else {
		/* Standard clock, check what rate the parent can offer */
		parent_rate = clock_management_round_internal(GET_CLK_PARENT(clk_hw),
							     req_freq);
		if (parent_rate < 0) {
			return parent_rate;
		}
		current_rate = clock_management_clk_rate(clk_hw);
		if (current_rate < 0) {
			return current_rate;
		}
		/* Check what rate this clock can offer with its parent offering */
		best_rate = clock_best_rate(clk_hw, req_freq, parent_rate, false);
		if (best_rate < 0) {
			/* Clock can't reconfigure, use the current rate */
			best_rate = current_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, best_rate,
					    CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	}

	return best_rate;
}

/**
 * @brief Helper function to set best clock configuration for a request
 *
 * This helper function determines the best clock configuration for a given
 * frequency request and applies it to the clock tree.
 *
 * @param clk_hw Clock to find configuration for
 * @param req_freq Requested clock frequency
 * @return new rate on success, or negative value on error
 */
static clock_freq_t clock_management_set_internal(const struct clk *clk_hw,
						  clock_freq_t req_freq)
{
	int ret;
	clock_freq_t parent_rate, current_rate, new_rate;
	int best_parent;

	current_rate = clock_management_clk_rate(clk_hw);
	if (current_rate < 0) {
		return current_rate;
	}
	if (clock_get_type(clk_hw) == CLK_TYPE_MUX) {
		/* Find the best parent and select that one */
		new_rate = clock_management_best_parent(clk_hw, req_freq,
						       &best_parent);
		if (new_rate < 0) {
			return new_rate;
		}
		/* Set the parent's rate */
		new_rate = clock_management_set_internal(
				GET_CLK_PARENTS(clk_hw)[best_parent], req_freq);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate,
					     CLOCK_MANAGEMENT_PRE_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
		ret = clock_set_parent(clk_hw, best_parent);
		if (ret < 0) {
			return ret;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate,
					     CLOCK_MANAGEMENT_POST_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	} else if (clock_get_type(clk_hw) == CLK_TYPE_ROOT) {
		new_rate = clock_management_round_internal(clk_hw, req_freq);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate,
					     CLOCK_MANAGEMENT_PRE_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
		/* Root clock parent can be set directly (base case) */
		new_rate = clock_root_best_rate(clk_hw, new_rate, true);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate,
					     CLOCK_MANAGEMENT_POST_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	} else {
		/* Set parent rate, then child rate */
		parent_rate = clock_management_set_internal(GET_CLK_PARENT(clk_hw),
							   req_freq);
		if (parent_rate < 0) {
			return parent_rate;
		}
		new_rate = clock_management_round_internal(clk_hw, req_freq);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate,
					     CLOCK_MANAGEMENT_PRE_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
		new_rate = clock_best_rate(clk_hw, new_rate, parent_rate, true);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate,
					     CLOCK_MANAGEMENT_POST_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	}
	return new_rate;
}

/**
 * @brief Determine the best rate a clock can produce, and optionally apply it
 *
 * When @p commit is false, determines the best rate a clock can produce using
 * its parents without modifying hardware. When @p commit is true, also applies
 * the rate to hardware.
 *
 * @param clk_hw Clock to query/set rate for
 * @param rate_req Requested rate
 * @param commit if true, apply the rate to hardware
 * @return best possible rate on success, or negative value on error
 */
clock_freq_t clock_management_best_rate(const struct clk *clk_hw, clock_freq_t rate_req,
										bool commit)
{
	if (commit) {
		return clock_management_set_internal(clk_hw, rate_req);
	}

	return clock_management_round_internal(clk_hw, rate_req);
}

#endif /* CONFIG_CLOCK_MANAGEMENT_SET_RATE */

/**
 * Helper function to apply a clock state
 *
 * @param clk_hw Clock output to apply clock state for
 * @param clk_state State to apply
 * @return 0 if state applied successfully, or error returned from
 * `clock_configure` if not
 */
static int clock_apply_state(const struct clk *clk_hw,
			     const struct clock_output_state *clk_state)
{
	int ret;

	for (uint8_t i = 0; i < clk_state->num_clocks; i++) {
		const struct clock_setting *cfg = &clk_state->clock_settings[i];

		if (IS_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME)) {
			ret = clock_tree_configure(cfg->clock,
						   cfg->clock_config_data);
		} else {
			ret = clock_configure(cfg->clock, cfg->clock_config_data);
		}

		if (ret < 0) {
			/* Configure failed, exit */
			return ret;
		}
	}
	return 0;
}

/**
 * @brief Get clock rate for given output
 *
 * Gets output clock rate in Hz for provided clock output.
 * @param data Clock management data structure for the device
 * @param clk Clock output to read rate of
 * @return -EINVAL if parameters are invalid
 * @return -ENOSYS if clock does not implement get_rate API
 * @return -EIO if clock could not be read
 * @return frequency of clock output in HZ
 */
int clock_management_get_rate(const struct clock_management_data *data, clock_output_t clk)
{
	const struct clock_output_data *output_data;
	int ret;

	if (!data || clk >= data->num_outputs) {
		return -EINVAL;
	}

	spinlock_key = k_spin_lock(&clock_mgmt_spinlock);

	output_data = GET_CLK_CORE(data->clock_outputs[clk])->hw_data;
	/* Read rate */
	ret = clock_management_clk_rate(output_data->parent);

	k_spin_unlock(&clock_mgmt_spinlock, spinlock_key);
	return ret;
}

static int clock_management_onoff(const struct clk *clk_hw, bool on)
{
	const struct clk *child = clk_hw, *parent;
	int ret = 0;

	/* Walk up parents tree, turning on clocks as we go */
	while (true) {
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
		if ((child->subsys_data->usage_cnt == 1) && (!on)) {
			/* Turn off the clock */
			ret = clock_onoff(child, on);
		} else if ((child->subsys_data->usage_cnt == 0) && (on)) {
			/* Turn on the clock */
			ret = clock_onoff(child, on);
		}
		if (ret < 0) {
			return ret;
		}
		child->subsys_data->usage_cnt += on ? (1) : (-1);
#else
		ret = clock_onoff(child, on);
		if (ret < 0) {
			return ret;
		}
#endif
		/* Get parent clock */
		if (clock_get_type(child) == CLK_TYPE_ROOT) {
			/* No parent clock, we're done */
			break;
		} else if (clock_get_type(child) == CLK_TYPE_STANDARD) {
			/* Single parent clock */
			parent = GET_CLK_PARENT(child);
		} else {
			/* Multi parent clock */
			ret = clock_get_parent(child);
			if (ret == -ENOTCONN) {
				/* Clock has no parent, it is disconnected */
				return 0;
			} else if (ret < 0) {
				/* Error getting parent */
				return ret;
			}
			parent = GET_CLK_PARENTS(child)[ret];
		}
		child = parent;
	}

	return ret;
}

/**
 * @brief Enable a clock output and its sources
 *
 * Turns a clock output and its sources on. This function will
 * unconditionally enable the clock and its sources.
 * @param data Clock management data structure for the device
 * @param clk clock output to turn on
 * @return -EINVAL if parameters are invalid
 * @return -ENOSYS if clock does not implement on_off API
 * @return -EIO if clock could not be turned on
 * @return -EBUSY if clock cannot be modified at this time
 * @return negative errno for other error turning clock on or off
 * @return 0 on success
 */
int clock_management_on(const struct clock_management_data *data, clock_output_t clk)
{
	const struct clock_output_data *output_data;
	int ret;

	if (!data || clk >= data->num_outputs) {
		return -EINVAL;
	}

	output_data = GET_CLK_CORE(data->clock_outputs[clk])->hw_data;

	spinlock_key = k_spin_lock(&clock_mgmt_spinlock);

	ret = clock_management_onoff(output_data->parent, true);

	k_spin_unlock(&clock_mgmt_spinlock, spinlock_key);
	return ret;
}

/**
 * @brief Disable a clock output and its sources
 *
 * Turns a clock output and its sources off. This function will
 * unconditionally disable the output and its sources.
 * @param data Clock management data structure for the device
 * @param clk clock output to turn off
 * @return -EINVAL if parameters are invalid
 * @return -ENOSYS if clock does not implement on_off API
 * @return -EIO if clock could not be turned off
 * @return -EBUSY if clock cannot be modified at this time
 * @return negative errno for other error turning clock on or off
 * @return 0 on success
 */
int clock_management_off(const struct clock_management_data *data, clock_output_t clk)
{
	const struct clock_output_data *output_data;
	int ret;

	if (!data || clk >= data->num_outputs) {
		return -EINVAL;
	}

	output_data = GET_CLK_CORE(data->clock_outputs[clk])->hw_data;

	spinlock_key = k_spin_lock(&clock_mgmt_spinlock);

	ret = clock_management_onoff(output_data->parent, false);

	k_spin_unlock(&clock_mgmt_spinlock, spinlock_key);
	return ret;
}

#ifdef CONFIG_CLOCK_MANAGEMENT_SET_RATE

/**
 * @brief Request a frequency for the clock output
 *
 * Requests a new rate for a clock output. The clock will configure to
 * the closest available state to the requested frequency. Requires
 * `CONFIG_CLOCK_MANAGEMENT_SET_RATE` to be set.
 * @param data Clock management data structure for the device
 * @param clk Clock output to request rate for
 * @param freq Rate request for clock output
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if request could not be satisfied
 * @return -EPERM if clock is not configurable
 * @return -EIO if configuration of a clock failed
 * @return -ENOTSUP if clock management set rate is not supported
 * @return frequency of clock output in HZ on success
 */
int clock_management_req_rate(const struct clock_management_data *data, clock_output_t clk,
			      clock_freq_t freq)
{
	const struct clock_output_data *output_data;
	clock_freq_t ret = -ENOENT;

	if (!data || clk >= data->num_outputs) {
		return -EINVAL;
	}

	spinlock_key = k_spin_lock(&clock_mgmt_spinlock);

	output_data = GET_CLK_CORE(data->clock_outputs[clk])->hw_data;

#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_DBG("Request for rate %u issued to clock %s",
		freq, GET_CLK_CORE(data->clock_outputs[clk])->clk_name);
#endif

	ret = clock_management_round_internal(output_data->parent, freq);

	if (ret >= 0) {
		ret = clock_management_set_internal(output_data->parent, freq);
	}

	k_spin_unlock(&clock_mgmt_spinlock, spinlock_key);
	return ret;
}

#endif

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME

/**
 * @brief Helper to apply or remove votes for a clock management request
 *
 * This function will apply or remove votes for clock output states based
 * on the provided request.
 * @param request Clock management request to apply or remove votes for
 * @param apply true to apply the request, false to remove it
 */
void clock_management_eval_request(const struct clock_management_request *request, bool apply)
{
	const struct clk *clk;
	const struct clock_output_data *output_data;
	uint8_t num_states;
	uint8_t *active_request_count;
	uint8_t *allowed_state_count;


	for (uint8_t i = 0; i < request->num_clks; i++) {
		clk = request->clk_reqs[i].clk;
		output_data = clk->hw_data;
		num_states = output_data->num_output_states;
		active_request_count = output_data->request_states +
			CLOCK_OUTPUT_ACTIVE_REQUEST_COUNT_IDX;
		allowed_state_count = output_data->request_states +
			CLOCK_OUTPUT_REQUEST_COUNT_IDX;
		if (apply) {
			*active_request_count += 1;
		} else {
			*active_request_count -= 1;
		}
		for (uint8_t j = 0; j < num_states; j++) {
			if ((BIT(j) & request->clk_reqs[i].allowed_states) == 0) {
				/* This state is not allowed for this clock */
				continue;
			}
			if (apply) {
				/* Add a vote for this state */
				allowed_state_count[j]++;
			} else {
				/* Remove a vote for this state */
				allowed_state_count[j]--;
			}
		}
	}
}

#endif

/**
 * @brief Helper to apply the best clock states for a clock management request
 *
 * This helper evaluates each clock in a request and applies the best clock
 * state based on the votes.
 * @param data Clock management data structure the request is a member of
 * @param request Clock management request to evaluate
 * @param apply true to apply the request, false to remove it
 * @return 0 on success, or negative value on error
 */
int clock_management_best_states(const struct clock_management_data *data,
			const struct clock_management_request *request, bool apply)
{
	const struct clk *clk;
	const struct clock_output_data *output_data;
	int ret = 0;
	uint8_t num_states;
	uint8_t *active_request_count;
	uint8_t *allowed_state_count;

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	/* Make sure all clocks affected by this request are unlocked */
	for (uint8_t i = 0; i < request->num_clks; i++) {
		clk = request->clk_reqs[i].clk;
		output_data = clk->hw_data;
		if ((*(output_data->consumer) != NULL) &&
			(*(output_data->consumer) != data)) {
			/* Clock is locked by another consumer */
			return -EPERM;
		}
	}
	for (uint8_t i = 0; i < request->num_clks; i++) {
		clk = request->clk_reqs[i].clk;
		output_data = clk->hw_data;
		/* Unlock the clock */
		*(output_data->consumer) = NULL;
	}
#endif

	for (uint8_t i = 0; i < request->num_clks; i++) {
		clk = request->clk_reqs[i].clk;
		output_data = clk->hw_data;
		num_states = output_data->num_output_states;
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
		active_request_count = output_data->request_states +
			CLOCK_OUTPUT_ACTIVE_REQUEST_COUNT_IDX;
		allowed_state_count = output_data->request_states +
			CLOCK_OUTPUT_REQUEST_COUNT_IDX;
		if (*active_request_count == 0) {
			/* No active requests, no need to apply a state */
			continue;
		}
#endif /* CONFIG_CLOCK_MANAGEMENT_RUNTIME */
		/*
		 * States are already sorted by rank, so the first
		 * state we find that satisfies all active requests is the best one
		 */
		for (uint8_t j = 0; j < num_states; j++) {
			if (IS_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME) &&
				(allowed_state_count[j] < *active_request_count)) {
				/* Not all requests have voted for this state, skip it */
				continue;
			} else if (apply && ((BIT(j) & request->clk_reqs[i].allowed_states) == 0)) {
				/* This state is not allowed for this clock, skip it */
				continue;
			}
			/* If both checks pass, found a valid state */
			IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME, (
				LOG_DBG("Applying state %d for clock %s",
					j, clk->clk_name);
			))
			ret = clock_apply_state(clk, output_data->output_states[j]);
			if (ret < 0) {
				/* Failed to apply state, exit */
				return ret;
			}
			/* No need to search further states */
			break;
		}
	}

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (request->locking && apply) {
		for (uint8_t i = 0; i < request->num_clks; i++) {
			clk = request->clk_reqs[i].clk;
			output_data = clk->hw_data;
			/* Lock the clock */
			*(output_data->consumer) = (struct clock_management_data *)data;
		}
	}
#endif
	return ret;
}

/**
 * @brief Request a clock state
 *
 * Request a clock state. This request may apply to multiple clock outputs,
 * and is defined in devicetree using the "clock-request-n" property.
 * The request can be retrieved using the @ref CLOCK_MANAGEMENT_DT_GET_REQUEST macro
 * @param data Clock management data structure for the device
 * @param request Clock management request to apply
 * @return -EIO if configuration of a clock failed
 * @return -EINVAL if parameters are invalid
 * @return -EPERM if clock is not configurable
 * @return 0 on success
 */
int clock_management_request_state(const struct clock_management_data *data,
				   clock_request_t request)
{
	int ret;

	if (!data || request >= data->num_requests) {
		return -EINVAL;
	}

	spinlock_key = k_spin_lock(&clock_mgmt_spinlock);

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (*data->active_request != CLOCK_MANAGEMENT_NO_REQUEST) {
		/* A request is already active, remove it first */
		clock_management_eval_request(data->clock_requests[*data->active_request], false);
		/* Apply new best state */
		ret = clock_management_best_states(data,
			data->clock_requests[*data->active_request], false);
		/*
		 * Clear active request, even if we failed to apply new state
		 * (we already cleared votes for it)
		 */
		*data->active_request = CLOCK_MANAGEMENT_NO_REQUEST;
		if (ret < 0) {
			goto out;
		}
	}
	/* Apply new request votes */
	clock_management_eval_request(data->clock_requests[request], true);
	ret = clock_management_best_states(data, data->clock_requests[request], true);
	if (ret < 0) {
		/* Request failed, remove its votes */
		clock_management_eval_request(data->clock_requests[request], false);
		goto out;
	}
	/* Record new request */
	*data->active_request = request;
out:
#else
	/* Just apply new request, no voting occurs */
	ret = clock_management_best_states(data, data->clock_requests[request], true);
#endif
	k_spin_unlock(&clock_mgmt_spinlock, spinlock_key);
	return ret;
}

#define CLOCK_STATE_NAME(node)                                                 \
	CONCAT(clock_state_, DT_DEP_ORD(DT_PARENT(node)), _,                   \
	       DT_NODE_CHILD_IDX(node))

/* This macro gets settings for a specific clock within a state */
#define CLOCK_SETTINGS_GET(node, prop, idx)                                    \
	{                                                                      \
		.clock = CLOCK_DT_GET(DT_PHANDLE_BY_IDX(node, prop, idx)),     \
		.clock_config_data = Z_CLOCK_MANAGEMENT_CLK_DATA_GET(node, prop,     \
							       idx),           \
	}

/* This macro defines clock configuration data for a clock state */
#define CLOCK_STATE_DEFINE(node)                                               \
	IF_ENABLED(DT_NODE_HAS_PROP(node, clocks), (                           \
	DT_FOREACH_PROP_ELEM(node, clocks, Z_CLOCK_MANAGEMENT_CLK_DATA_DEFINE);))    \
	static const struct clock_output_state CLOCK_STATE_NAME(node) = {      \
		.num_clocks = DT_PROP_LEN_OR(node, clocks, 0),                 \
		.clock_settings = {                                            \
			DT_FOREACH_PROP_ELEM_SEP(node, clocks,                 \
						 CLOCK_SETTINGS_GET, (,))      \
		}, \
	};
/* This macro gets clock configuration data for a clock state */
#define CLOCK_STATE_GET(node) &CLOCK_STATE_NAME(node),

#define CLOCK_OUTPUT_LIST_START_NAME(inst)                                     \
	CONCAT(_clk_output_, DT_INST_DEP_ORD(inst), _list_start)

#define CLOCK_OUTPUT_LIST_END_NAME(inst)                                       \
	CONCAT(_clk_output_, DT_INST_DEP_ORD(inst), _list_end)

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
#define CLOCK_OUTPUT_RUNTIME_DEFINE(inst)                                      \
	static struct clock_management_data *CONCAT(output_consumer, inst); \
	static uint8_t request_states_##inst[DT_INST_CHILD_NUM(inst) + 1]; \
	extern struct clock_output CLOCK_OUTPUT_LIST_START_NAME(inst);         \
	extern struct clock_output CLOCK_OUTPUT_LIST_END_NAME(inst);
#define CLOCK_OUTPUT_RUNTIME_INIT(inst)                                        \
	.consumer = &CONCAT(output_consumer, inst),                            \
	.request_states = request_states_##inst,                               \
	.consumer_start = &CLOCK_OUTPUT_LIST_START_NAME(inst),                 \
	.consumer_end = &CLOCK_OUTPUT_LIST_END_NAME(inst),
#else
#define CLOCK_OUTPUT_RUNTIME_DEFINE(inst)
#define CLOCK_OUTPUT_RUNTIME_INIT(inst)
#endif

#define CLOCK_OUTPUT_DEFINE(inst)                                              \
	CLOCK_OUTPUT_RUNTIME_DEFINE(inst)                                      \
	DT_INST_CLOCK_STATE_SORTED_FOREACH(inst, CLOCK_STATE_DEFINE)           \
	static const struct clock_output_state *const                          \
	output_states_##inst[] = {                                             \
		DT_INST_CLOCK_STATE_SORTED_FOREACH(inst, CLOCK_STATE_GET)          \
	};                                                                     \
	static const struct clock_output_data                                  \
	CONCAT(clock_output_, DT_INST_DEP_ORD(inst)) = {                       \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.output_states = output_states_##inst,                         \
		.num_output_states = ARRAY_SIZE(output_states_##inst),       \
		CLOCK_OUTPUT_RUNTIME_INIT(inst)                                \
	};                                                                     \
	LEAF_CLOCK_DT_INST_DEFINE(inst,                                        \
			     &CONCAT(clock_output_, DT_INST_DEP_ORD(inst)));

DT_INST_FOREACH_STATUS_OKAY(CLOCK_OUTPUT_DEFINE)
