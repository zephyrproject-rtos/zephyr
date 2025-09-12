/*
 * Copyright 2024 NXP
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
#define GET_CLK_CORE(clk) (clk->clk_core)
#else
#define GET_CLK_CORE(clk) ((const struct clk *)clk)
#endif

K_MUTEX_DEFINE(clock_management_mutex);

/** Calculates clock rank factor, which scales with frequency */
#define CLK_RANK(clk_hw, freq)                                                 \
	(((clk_hw)->rank) + (((uint32_t)(clk_hw)->rank_factor) * (freq)))

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
	/* Frequency resulting from this setting */
	const clock_freq_t frequency;
	/* Rank of this setting */
	uint32_t rank;
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/* Should this state lock the clock configuration? */
	const bool locking;
#endif
	/* Clock configuration settings for each clock */
	const struct clock_setting clock_settings[];
};

/* Clock output node private data */
struct clock_output_data {
	/* Parent clock of this output node */
	const struct clk *parent;
	/* Number of statically defined clock states */
	const uint8_t num_states;
	/* Statically defined clock output states */
	const struct clock_output_state *const *output_states;
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	/* Start of the consumer array (defined by the linker) */
	struct clock_output *consumer_start;
	/* End of the consumer array (defined by the linker) */
	struct clock_output *consumer_end;
	/* Tracks the constraints placed by all users of this output clock */
	struct clock_management_rate_req *combined_req;
#endif
};

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
	if (clk_hw >= TYPE_SECTION_START(clk) &&
	    clk_hw < TYPE_SECTION_END(clk_root)) {
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

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME

/**
 * Helper function to add a constraint to an existing set. NOTE: this function
 * assumes the new constraint is compatible with the current set.
 * @param current Current constraint set, updated with new constraint
 * @param new New constraint to add
 */
static void clock_add_constraint(struct clock_management_rate_req *current,
				 const struct clock_management_rate_req *new)
{
	if (new->min_freq > current->min_freq) {
		/* Tighter minimum frequency found */
		current->min_freq = new->min_freq;
	}
	if (new->max_freq < current->max_freq) {
		/* Tighter maximum frequency found */
		current->max_freq = new->max_freq;
	}
	if (new->max_rank < current->max_rank) {
		/* Tighter maximum rank found */
		current->max_rank = new->max_rank;
	}
}

/**
 * Helper function to remove the constraint currently associated with
 * @p consumer. This function updates the shared constraints for
 * @p clk_hw, without the constraints of @p consumer included
 * @param clk_hw Clock output to remove constraint from
 * @param combined New constraint set without the consumer's constraints
 * @param consumer Consumer whose constraint should be removed
 */
static void clock_remove_constraint(const struct clk *clk_hw,
				    struct clock_management_rate_req *combined,
				    const struct clock_output *consumer)
{
	const struct clock_output_data *data = clk_hw->hw_data;
	/* New combined constraint set. Start with the loosest definition. */
	combined->min_freq = 0;
	combined->max_freq = INT32_MAX;
	combined->max_rank = CLOCK_MANAGEMENT_ANY_RANK;

	for (const struct clock_output *child = data->consumer_start;
	     child < data->consumer_end; child++) {
		if (child == consumer) {
			/*
			 * This consumer is updating its constraint and should
			 * not be considered
			 */
			continue;
		}
		clock_add_constraint(combined, child->req);
	}
}

#endif

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
 * @param parent_rank Summed rank of parent clocks. Ignored if @p ev_type is not
 *        CLOCK_MANAGEMENT_QUERY_RATE_CHANGE
 * @param ev_type Type of clock notification event
 * @return 0 if notification chain succeeded, or error if not
 */
static int clock_notify_children(const struct clk *clk_hw,
				 clock_freq_t old_freq,
				 clock_freq_t new_freq,
				 uint32_t parent_rank,
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
	uint32_t child_rank;
	clock_freq_t child_newrate, child_oldrate;

	if (*handle == CLOCK_LIST_END) {
		/* Base case- clock leaf (output node) */
		data = clk_hw->hw_data;
		/* Check if the new rate is permitted given constraints */
		if (ev_type == CLOCK_MANAGEMENT_QUERY_RATE_CHANGE) {
			if ((data->combined_req->min_freq > event.new_rate) ||
			    (data->combined_req->max_freq < event.new_rate) ||
			    (data->combined_req->max_rank < parent_rank)) {
				IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME,
					   (LOG_DBG("Clock %s rejected frequency %d, rank %u",
					    clk_hw->clk_name, event.new_rate, parent_rank)));
				return -ENOTSUP;
			}
		} else {
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
				child_rank = parent_rank;
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
				child_rank = parent_rank + CLK_RANK(child, child_newrate);
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
				child_rank = parent_rank + CLK_RANK(child, child_newrate);
			}
			/* Notify its children of new rate */
			ret = clock_notify_children(child, child_oldrate,
						    child_newrate, child_rank,
						    ev_type);
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
 * @param new_rank New rank that clock state reports
 * @param cfg_param Configuration parameter to pass into clock_configure
 * @return 0 if change was applied successfully, or error if not
 */
static int clock_tree_configure(const struct clk *clk_hw,
				uint32_t new_rank,
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
	ret = clock_notify_children(clk_hw, current_rate, new_rate, new_rank,
				    CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
	if (ret < 0) {
		return ret;
	}
	/* Now, notify children rates will change */
	ret = clock_notify_children(clk_hw, current_rate, new_rate, 0,
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
	ret = clock_notify_children(clk_hw, current_rate, new_rate, 0,
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
	return clock_notify_children(clk_hw, current_rate, CLK_RANK(clk_hw, new_rate),
				     new_rate, CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
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
				uint32_t new_rank,
				const void *cfg_param)
{
	return -ENOTSUP;
}

#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)

/* Forwards declaration */
static clock_freq_t clock_management_round_internal(const struct clk *clk_hw,
						    const struct clock_management_rate_req *req,
						    uint32_t *best_rank,
						    bool prefer_rank);

/**
 * Helper function to find the best parent of a multiplexer for a requested rate.
 * This is needed both in the round_rate and set_rate phases of clock configuration.
 *
 * @param clk_hw Multiplexer to find best parent for
 * @param req Requested clock frequency and ranking bounds
 * @param best_parent Set to best parent found for request
 * @param best_rank Set to best rank found for request
 * @param prefer_rank Controls ranking mode.
 * @return best possible rate on success, or negative value on error
 */
static clock_freq_t clock_management_best_parent(const struct clk *clk_hw,
						 const struct clock_management_rate_req *req,
						 int *best_parent,
						 uint32_t *best_rank,
						 bool prefer_rank)
{
	int ret;
	uint32_t best_delta = UINT32_MAX, cand_rank, delta;
	clock_freq_t cand_rate, current_rate, best_rate;
	const struct clk *cand_parent;
	const struct clk_mux_subsys_data *mux_data = clk_hw->hw_data;
	bool constraints_possible = false;

	*best_rank = UINT32_MAX;
	/* Evaluate each parent clock. If one fails for any reason, just skip it */
	for (int i = 0; i < mux_data->parent_cnt; i++) {
		cand_parent = mux_data->parents[i];
		cand_rate = clock_management_round_internal(cand_parent, req,
							    &cand_rank, prefer_rank);
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
					    cand_rank, CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
		if (ret < 0) {
			/* Clock won't be able to reconfigure for this rate */
			continue;
		}
		delta = abs(cand_rate - req->max_freq);
		if (((prefer_rank && (cand_rank < *best_rank)) ||
		    (!prefer_rank && (delta < best_delta))) &&
		    (cand_rate >= req->min_freq) && (cand_rate <= req->max_freq)) {
			/* Clock can hit constraints, and is better ranked/
			 * more accurate than our current choice
			 */
			constraints_possible = true;
			best_delta = delta;
			best_rate = cand_rate;
			*best_rank = cand_rank;
			*best_parent = i;
		} else if (!constraints_possible && (delta < best_delta)) {
			/* Fallback case- if we haven't found a clock that
			 * hits the constraints, just select the most accurate
			 * one for the request
			 */
			best_delta = delta;
			best_rate = cand_rate;
			*best_rank = cand_rank;
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
 * request, and can select configurations based on their ranking or frequency.
 * When @p prefer_rank is set, the function will select the lowest ranked clock
 * configuration that satisfies the request. Otherwise the function will select
 * the clock configuration that results in a frequency closest to the maximum
 * frequency specified by the @p req parameter.
 *
 * @param clk_hw Clock to find configuration for
 * @param req Requested clock frequency and ranking bounds
 * @param best_rank Set to best rank found for request
 * @param prefer_rank Controls ranking mode.
 */
static clock_freq_t clock_management_round_internal(const struct clk *clk_hw,
						    const struct clock_management_rate_req *req,
						    uint32_t *best_rank,
						    bool prefer_rank)
{
	int ret;
	clock_freq_t parent_rate, current_rate, best_rate;
	int best_parent, parent_rank = 0;

	if (clock_get_type(clk_hw) == CLK_TYPE_MUX) {
		/* Mux clocks don't support round_rate, we implement it generically */
		best_rate = clock_management_best_parent(clk_hw, req,
							&best_parent,
							&parent_rank,
							prefer_rank);
	} else if (clock_get_type(clk_hw) == CLK_TYPE_ROOT) {
		/* No need to check parents */
		current_rate = clock_get_rate(clk_hw);
		if (current_rate < 0) {
			return current_rate;
		}
		best_rate = clock_root_round_rate(clk_hw, req->max_freq);
		if (best_rate < 0) {
			/* Clock can't reconfigure, use the current rate */
			best_rate = current_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, best_rate,
					    CLK_RANK(clk_hw, best_rate),
					    CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	} else {
		/* Standard clock, check what rate the parent can offer */
		parent_rate = clock_management_round_internal(GET_CLK_PARENT(clk_hw), req,
							      &parent_rank, prefer_rank);
		if (parent_rate < 0) {
			return parent_rate;
		}
		current_rate = clock_management_clk_rate(clk_hw);
		if (current_rate < 0) {
			return current_rate;
		}
		/* Check what rate this clock can offer with its parent offering */
		best_rate = clock_round_rate(clk_hw, req->max_freq, parent_rate);
		if (best_rate < 0) {
			/* Clock can't reconfigure, use the current rate */
			best_rate = current_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, best_rate,
					    parent_rank,
					    CLOCK_MANAGEMENT_QUERY_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	}

	*best_rank = CLK_RANK(clk_hw, best_rate) + parent_rank;

	return best_rate;
}

/**
 * @brief Helper function to set best clock configuration for a request
 *
 * This helper function determines the best clock configuration for a given
 * request, and can select configurations based on their ranking or frequency.
 * When @p prefer_rank is set, the function will select the lowest ranked clock
 * configuration that satisfies the request. Otherwise the function will select
 * the clock configuration that results in a frequency closest to the maximum
 * frequency specified by the @p req parameter.
 * This function will apply the configuration to the clock tree, versus
 * the round_internal function which only finds the best configuration.
 *
 * @param clk_hw Clock to find configuration for
 * @param req Requested clock frequency and ranking bounds
 * @param prefer_rank Controls ranking mode.
 */
static clock_freq_t clock_management_set_internal(const struct clk *clk_hw,
						 const struct clock_management_rate_req *req,
						 bool prefer_rank)
{
	int ret;
	clock_freq_t parent_rate, current_rate, new_rate;
	int best_parent;
	uint32_t best_rank; /* Unused */
	struct clock_management_rate_req set_req = {
		.min_freq = req->min_freq,
		.max_rank = req->max_rank,
	};

	current_rate = clock_management_clk_rate(clk_hw);
	if (current_rate < 0) {
		return current_rate;
	}
	if (clock_get_type(clk_hw) == CLK_TYPE_MUX) {
		/* Find the best parent and select that one */
		set_req.min_freq = set_req.max_freq =
			clock_management_best_parent(clk_hw, req,
						  &best_parent, &best_rank,
						  prefer_rank);
		if (set_req.max_freq < 0) {
			return set_req.max_freq;
		}
		/* Set the parent's rate */
		new_rate = clock_management_set_internal(GET_CLK_PARENTS(clk_hw)[best_parent],
						&set_req, prefer_rank);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate, 0,
					     CLOCK_MANAGEMENT_PRE_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
		ret = clock_set_parent(clk_hw, best_parent);
		if (ret < 0) {
			return ret;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate, 0,
					     CLOCK_MANAGEMENT_POST_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	} else if (clock_get_type(clk_hw) == CLK_TYPE_ROOT) {
		new_rate = clock_management_round_internal(clk_hw, req, &best_rank,
							   prefer_rank);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate, 0,
					     CLOCK_MANAGEMENT_PRE_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
		/* Root clock parent can be set directly (base case) */
		new_rate = clock_root_set_rate(clk_hw, new_rate);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate, 0,
					     CLOCK_MANAGEMENT_POST_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	} else {
		/* Set parent rate, then child rate */
		parent_rate = clock_management_set_internal(GET_CLK_PARENT(clk_hw), req,
							    prefer_rank);
		if (parent_rate < 0) {
			return parent_rate;
		}
		new_rate = clock_management_round_internal(clk_hw, req,
						     &best_rank, prefer_rank);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate, 0,
					     CLOCK_MANAGEMENT_PRE_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
		new_rate = clock_set_rate(clk_hw, new_rate, parent_rate);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = clock_notify_children(clk_hw, current_rate, new_rate, 0,
					     CLOCK_MANAGEMENT_POST_RATE_CHANGE);
		if (ret < 0) {
			return ret;
		}
	}
	return new_rate;
}

/**
 * @brief Determine the best rate a clock can produce
 *
 * This function is used to determine the best rate a clock can produce using
 * its parents.
 *
 * @param clk_hw Clock to round rate for
 * @param rate_req Requested rate to round
 * @return best possible rate on success, or negative value on error
 */
clock_freq_t clock_management_round_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{
	uint32_t best_rank; /* Unused */
	struct clock_management_rate_req req = {
		.min_freq = rate_req,
		.max_freq = rate_req,
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
	};
	return clock_management_round_internal(clk_hw, &req, &best_rank, false);
}

/**
 * @brief Set the rate of a clock
 *
 * This function is used to set the rate of a clock.
 *
 * @param clk_hw Clock to set rate for
 * @param rate_req Requested rate to set
 * @return rate clock is set to on success, or negative value on error
 */
clock_freq_t clock_management_set_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{

	struct clock_management_rate_req req = {
		.min_freq = rate_req,
		.max_freq = rate_req,
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
	};
	return clock_management_set_internal(clk_hw, &req, false);
}

#else


static clock_freq_t clock_management_round_internal(const struct clk *clk_hw,
						    const struct clock_management_rate_req *req,
						    uint32_t *best_rank,
						    bool prefer_rank)
{
	return -ENOTSUP;
}

clock_freq_t clock_management_round_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{
	return -ENOTSUP;
}

clock_freq_t clock_management_set_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{
	return -ENOTSUP;
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
	const struct clock_output_data *data = clk_hw->hw_data;
	clock_freq_t new_rate = 0;
	int ret;

	if (clk_state->num_clocks == 0) {
		/* Use runtime clock setting */
		new_rate = clock_management_set_rate(data->parent, clk_state->frequency);

		if (new_rate < 0) {
			return new_rate;
		}
		if (new_rate != clk_state->frequency) {
			return -ENOTSUP;
		}

		return 0;
	}

	/* Apply this clock state */
	for (uint8_t i = 0; i < clk_state->num_clocks; i++) {
		const struct clock_setting *cfg = &clk_state->clock_settings[i];

		if (IS_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME)) {
			ret = clock_tree_configure(cfg->clock, clk_state->rank,
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
 * @param clk Clock output to read rate of
 * @return -EINVAL if parameters are invalid
 * @return -ENOSYS if clock does not implement get_rate API
 * @return -EIO if clock could not be read
 * @return frequency of clock output in HZ
 */
int clock_management_get_rate(const struct clock_output *clk)
{
	const struct clock_output_data *data;
	int ret;

	if (!clk) {
		return -EINVAL;
	}

	k_mutex_lock(&clock_management_mutex, K_FOREVER);

	data = GET_CLK_CORE(clk)->hw_data;
	/* Read rate */
	ret = clock_management_clk_rate(data->parent);

	k_mutex_unlock(&clock_management_mutex);
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
 * @param clk clock output to turn off
 * @return -ENOSYS if clock does not implement on_off API
 * @return -EIO if clock could not be turned off
 * @return -EBUSY if clock cannot be modified at this time
 * @return negative errno for other error turning clock on or off
 * @return 0 on success
 */
int clock_management_on(const struct clock_output *clk)
{
	const struct clock_output_data *data = GET_CLK_CORE(clk)->hw_data;
	int ret;

	k_mutex_lock(&clock_management_mutex, K_FOREVER);

	ret = clock_management_onoff(data->parent, true);

	k_mutex_unlock(&clock_management_mutex);
	return ret;
}

/**
 * @brief Disable a clock output and its sources
 *
 * Turns a clock output and its sources off. This function will
 * unconditionally disable the output and its sources.
 * @param clk clock output to turn off
 * @return -ENOSYS if clock does not implement on_off API
 * @return -EIO if clock could not be turned off
 * @return -EBUSY if clock cannot be modified at this time
 * @return negative errno for other error turning clock on or off
 * @return 0 on success
 */
int clock_management_off(const struct clock_output *clk)
{
	const struct clock_output_data *data = GET_CLK_CORE(clk)->hw_data;
	int ret;

	k_mutex_lock(&clock_management_mutex, K_FOREVER);

	ret = clock_management_onoff(data->parent, false);

	k_mutex_unlock(&clock_management_mutex);
	return ret;

}

/**
 * @brief Request a frequency for the clock output
 *
 * Requests a new rate for a clock output. The clock will select the best state
 * given the constraints provided in @p req. If enabled via
 * `CONFIG_CLOCK_MANAGEMENT_RUNTIME`, existing constraints on the clock will be
 * accounted for when servicing this request. Additionally, if enabled via
 * `CONFIG_CLOCK_MANAGEMENT_SET_RATE`, the clock will dynamically request a new rate
 * from its parent if none of the statically defined states satisfy the request.
 * An error will be returned if the request cannot be satisfied.
 * @param clk Clock output to request rate for
 * @param req Rate request for clock output
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if request could not be satisfied
 * @return -EPERM if clock is not configurable
 * @return -EIO if configuration of a clock failed
 * @return frequency of clock output in HZ on success
 */
int clock_management_req_rate(const struct clock_output *clk,
			const struct clock_management_rate_req *req)
{
	const struct clock_output_data *data;
	clock_freq_t ret = -ENOENT;
	const struct clock_output_state *best_state = NULL;
	int best_delta = INT32_MAX;
	uint32_t best_rank;
	struct clock_management_rate_req *combined_req;

	if (!clk) {
		return -EINVAL;
	}

	k_mutex_lock(&clock_management_mutex, K_FOREVER);

	data = GET_CLK_CORE(clk)->hw_data;

 #ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	struct clock_management_rate_req new_req;
	/*
	 * Remove previous constraint associated with this clock output
	 * from the clock producer.
	 */
	clock_remove_constraint(GET_CLK_CORE(clk), &new_req, clk);
	/*
	 * Check if the new request is compatible with the
	 * new shared constraint set
	 */
	if ((new_req.min_freq > req->max_freq) ||
	    (new_req.max_freq < req->min_freq)) {
		ret = -ENOENT;
		goto out;
	}
	/*
	 * We now know the new constraint is compatible. Now, save the
	 * updated constraint set as the shared set for this clock producer.
	 * We deliberately exclude the constraints of the clock output
	 * making this request, as the intermediate states of the clock
	 * producer may not be compatible with the new constraint. If we
	 * added the new constraint now then the clock would fail to
	 * reconfigure to an otherwise valid state, because the rates
	 * passed to clock_notify_children() would be rejected
	 */
	memcpy(data->combined_req, &new_req, sizeof(*data->combined_req));
	/*
	 * Add this new request to the shared constraint set before using
	 * the set for clock requests.
	 */
	clock_add_constraint(&new_req, req);
	combined_req = &new_req;
#else
	/*
	 * We don't combine requests in this case, just use the clock
	 * request directly
	 */
	combined_req = (struct clock_management_rate_req *)req;
#endif

#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_DBG("Request for range %u-%u issued to clock %s. Max rank %u",
		combined_req->min_freq, combined_req->max_freq,
		GET_CLK_CORE(clk)->clk_name, combined_req->max_rank);
#endif

	/*
	 * Now, check if any of the statically defined clock states are
	 * valid
	 */
	for (uint8_t i = 0; i < data->num_states; i++) {
		const struct clock_output_state *state =
			data->output_states[i];
		int cand_delta;

		if ((state->frequency < combined_req->min_freq) ||
		    (state->frequency > combined_req->max_freq) ||
		    (state->rank > combined_req->max_rank)) {
			/* This state does not qualify */
			continue;
		}
		cand_delta = state->frequency - combined_req->min_freq;
		/*
		 * If new delta is better than current best delta,
		 * we found a new best state
		 */
		if (best_delta > cand_delta) {
			/* New best state found */
			best_delta = cand_delta;
			best_state = state;
			best_rank = state->rank;
		}
	}
	if (best_state != NULL) {
		/* Apply this clock state */
		ret = clock_apply_state(GET_CLK_CORE(clk), best_state);
		if (ret == 0) {
			ret = best_state->frequency;
			goto out;
		}
	}
	/* No best setting was found, try runtime clock setting */
	ret = clock_management_round_internal(data->parent, combined_req,
					      &best_rank, false);
out:
	if (ret >= 0) {
		/* A frequency was returned, check if it satisfies constraints */
		if ((combined_req->min_freq > ret) ||
		    (combined_req->max_freq < ret) ||
		    (best_rank > combined_req->max_rank)) {
			ret = -ENOENT;
		}
	}
#ifdef CONFIG_CLOCK_MANAGEMENT_SET_RATE
	if (best_delta != 0 && ret >= 0) {
		/* Only set rate if no matching static state exists */
		ret = clock_management_set_internal(data->parent, combined_req,
						    false);
	}
#endif
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (ret >= 0) {
		/* New clock state applied. Save the new combined constraint set. */
		memcpy(data->combined_req, combined_req, sizeof(*data->combined_req));
		/* Save the new constraint set for the consumer */
		memcpy(clk->req, req, sizeof(*clk->req));
	}
#endif

	k_mutex_unlock(&clock_management_mutex);
	return ret;
}

/**
 * @brief Request the best ranked clock configuration for a given frequency range
 *
 * Requests the clock framework select the best ranked clock configuration
 * for a given frequency range. Clock ranks are calculated per clock node
 * by summing the fixed "clock-ranking" property with the "clock-rank-factor"
 * property times the output frequency (divided by 255). A clock configuration's
 * rank is the sum of all the ranks for the clocks used in that configuration.
 * @param clk Clock output to make request for
 * @param req Upper and lower bounds on frequency
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if request could not be satisfied
 * @return -EPERM if clock is not configurable
 * @return -EIO if configuration of a clock failed
 * @return frequency of clock output in HZ on success
 */
int clock_management_req_ranked(const struct clock_output *clk,
				const struct clock_management_rate_req *req)
{
	const struct clock_output_data *data;
	clock_freq_t ret = -ENOENT;
	const struct clock_output_state *best_state = NULL;
	uint32_t best_rank = UINT32_MAX;
	struct clock_management_rate_req *combined_req;

	if (!clk) {
		return -EINVAL;
	}

	k_mutex_lock(&clock_management_mutex, K_FOREVER);

	data = GET_CLK_CORE(clk)->hw_data;

 #ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	struct clock_management_rate_req new_req;
	/*
	 * Remove previous constraint associated with this clock output
	 * from the clock producer.
	 */
	clock_remove_constraint(GET_CLK_CORE(clk), &new_req, clk);
	/*
	 * Check if the new request is compatible with the
	 * new shared constraint set
	 */
	if ((new_req.min_freq > req->max_freq) ||
	    (new_req.max_freq < req->min_freq)) {
		ret = -ENOENT;
		goto out;
	}
	/*
	 * We now know the new constraint is compatible. Now, save the
	 * updated constraint set as the shared set for this clock producer.
	 * We deliberately exclude the constraints of the clock output
	 * making this request, as the intermediate states of the clock
	 * producer may not be compatible with the new constraint. If we
	 * added the new constraint now then the clock would fail to
	 * reconfigure to an otherwise valid state, because the rates
	 * passed to clock_notify_children() would be rejected
	 */
	memcpy(data->combined_req, &new_req, sizeof(*data->combined_req));
	/*
	 * Add this new request to the shared constraint set before using
	 * the set for clock requests.
	 */
	clock_add_constraint(&new_req, req);
	combined_req = &new_req;
#else
	/*
	 * We don't combine requests in this case, just use the clock
	 * request directly
	 */
	combined_req = (struct clock_management_rate_req *)req;
#endif

#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
	LOG_DBG("Request for range %u-%u issued to clock %s. Max rank %u",
		combined_req->min_freq, combined_req->max_freq,
		GET_CLK_CORE(clk)->clk_name, combined_req->max_rank);
#endif

	/*
	 * Now, check if any of the statically defined clock states are
	 * valid
	 */
	for (uint8_t i = 0; i < data->num_states; i++) {
		const struct clock_output_state *state =
			data->output_states[i];
		if ((state->frequency < combined_req->min_freq) ||
		    (state->frequency > combined_req->max_freq) ||
		    (state->rank > combined_req->max_rank)) {
			/* This state does not qualify */
			continue;
		}
		/*
		 * If new rank is better than current best rank,
		 * we found a new best state
		 */
		if (best_rank > state->rank) {
			/* New best state found */
			best_rank = state->rank;
			best_state = state;
		}
	}
	if (best_state != NULL) {
		/* Apply this clock state */
		ret = clock_apply_state(GET_CLK_CORE(clk), best_state);
		if (ret == 0) {
			ret = best_state->frequency;
			goto out;
		}
	}
	/* No best setting was found, try runtime clock setting */
	ret = clock_management_round_internal(data->parent, combined_req,
						&best_rank, true);
out:
	if (ret >= 0) {
		/* A frequency was returned, check if it satisfies constraints */
		if ((combined_req->min_freq > ret) ||
		    (combined_req->max_freq < ret) ||
		    (best_rank > combined_req->max_rank)) {
			ret = -ENOENT;
		}
	}
#ifdef CONFIG_CLOCK_MANAGEMENT_SET_RATE
	if (((best_state == NULL) || (best_rank < best_state->rank)) && (ret >= 0)) {
		/* Only use runtime setting if we found a better state */
		ret = clock_management_set_internal(data->parent, combined_req, true);
	}
#endif
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (ret >= 0) {
		/* New clock state applied. Save the new combined constraint set. */
		memcpy(data->combined_req, combined_req, sizeof(*data->combined_req));
		/* Save the new constraint set for the consumer */
		memcpy(clk->req, req, sizeof(*clk->req));
	}
#endif
	k_mutex_unlock(&clock_management_mutex);
	return ret;
}

/**
 * @brief Apply a clock state based on a devicetree clock state identifier
 *
 * Apply a clock state based on a clock state identifier. State identifiers are
 * defined devices that include a "clock-states" devicetree property, and may be
 * retrieved using the @ref DT_CLOCK_MANAGEMENT_STATE macro
 * @param clk Clock output to apply state for
 * @param state Clock management state ID to apply
 * @return -EIO if configuration of a clock failed
 * @return -EINVAL if parameters are invalid
 * @return -EPERM if clock is not configurable
 * @return frequency of clock output in HZ on success
 */
int clock_management_apply_state(const struct clock_output *clk,
			   clock_management_state_t state)
{
	const struct clock_output_data *data;
	const struct clock_output_state *clk_state;
	int ret;

	if (!clk) {
		return -EINVAL;
	}

	k_mutex_lock(&clock_management_mutex, K_FOREVER);

	data = GET_CLK_CORE(clk)->hw_data;

	if (state >= data->num_states) {
		ret = -EINVAL;
		goto out;
	}

	clk_state = data->output_states[state];

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	struct clock_management_rate_req temp;
	/* Remove old constraint for this consumer */
	clock_remove_constraint(GET_CLK_CORE(clk), &temp, clk);

	/* Make sure this state fits within other consumer's constraints */
	if ((temp.min_freq > clk_state->frequency) ||
	    (temp.max_freq < clk_state->frequency)) {
		ret = -EINVAL;
		goto out;
	}

	/* Save new constraint set */
	memcpy(data->combined_req, &temp, sizeof(*data->combined_req));
#endif

	ret = clock_apply_state(GET_CLK_CORE(clk), clk_state);
	if (ret < 0) {
		goto out;
	}
	ret = clk_state->frequency;
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (clk_state->locking) {
		/* Set a constraint based on this clock state */
		const struct clock_management_rate_req constraint = {
			.min_freq = clk_state->frequency,
			.max_freq = clk_state->frequency,
			.max_rank = clk_state->rank,
		};

		/* Remove old constraint for this consumer */
		clock_remove_constraint(GET_CLK_CORE(clk), &temp, clk);
		/* Add new constraint and save it */
		clock_add_constraint(&temp, &constraint);
		memcpy(data->combined_req, &temp, sizeof(*data->combined_req));
		memcpy(clk->req, &constraint, sizeof(*clk->req));
	}
#endif
out:
	k_mutex_unlock(&clock_management_mutex);
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
		.frequency = DT_PROP(node, clock_frequency),                   \
		.rank = DT_PROP(node, rank),                                   \
		IF_ENABLED(DT_NODE_HAS_PROP(node, clocks), (                   \
		.clock_settings = {                                            \
			DT_FOREACH_PROP_ELEM_SEP(node, clocks,                 \
						 CLOCK_SETTINGS_GET, (,))      \
		},))                                                           \
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME,                    \
		(.locking = DT_PROP(node, locking_state),))                    \
	};
/* This macro gets clock configuration data for a clock state */
#define CLOCK_STATE_GET(node) &CLOCK_STATE_NAME(node)

#define CLOCK_OUTPUT_LIST_START_NAME(inst)                                     \
	CONCAT(_clk_output_, DT_INST_DEP_ORD(inst), _list_start)

#define CLOCK_OUTPUT_LIST_END_NAME(inst)                                       \
	CONCAT(_clk_output_, DT_INST_DEP_ORD(inst), _list_end)

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
#define CLOCK_OUTPUT_RUNTIME_DEFINE(inst)                                      \
	extern struct clock_output CLOCK_OUTPUT_LIST_START_NAME(inst);         \
	extern struct clock_output CLOCK_OUTPUT_LIST_END_NAME(inst);           \
	struct clock_management_rate_req combined_req_##inst = {               \
		.min_freq = 0,                                                 \
		.max_freq = INT32_MAX,                                         \
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,                         \
	};
#define CLOCK_OUTPUT_RUNTIME_INIT(inst)                                        \
	.consumer_start = &CLOCK_OUTPUT_LIST_START_NAME(inst),                 \
	.consumer_end = &CLOCK_OUTPUT_LIST_END_NAME(inst),                     \
	.combined_req = &combined_req_##inst,
#else
#define CLOCK_OUTPUT_RUNTIME_DEFINE(inst)
#define CLOCK_OUTPUT_RUNTIME_INIT(inst)
#endif

#define CLOCK_OUTPUT_DEFINE(inst)                                              \
	CLOCK_OUTPUT_RUNTIME_DEFINE(inst)                                      \
	DT_INST_FOREACH_CHILD(inst, CLOCK_STATE_DEFINE)                        \
	static const struct clock_output_state *const                          \
	output_states_##inst[] = {                                             \
		DT_INST_FOREACH_CHILD_SEP(inst, CLOCK_STATE_GET, (,))          \
	};                                                                     \
	static const struct clock_output_data                                  \
	CONCAT(clock_output_, DT_INST_DEP_ORD(inst)) = {                       \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.num_states = DT_INST_CHILD_NUM(inst),                         \
		.output_states = output_states_##inst,                         \
		CLOCK_OUTPUT_RUNTIME_INIT(inst)                                \
	};                                                                     \
	LEAF_CLOCK_DT_INST_DEFINE(inst,                                        \
			     &CONCAT(clock_output_, DT_INST_DEP_ORD(inst)));

DT_INST_FOREACH_STATUS_OKAY(CLOCK_OUTPUT_DEFINE)
