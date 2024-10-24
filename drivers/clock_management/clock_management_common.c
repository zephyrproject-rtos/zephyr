/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management.h>
#include <string.h>
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
	const uint32_t frequency;
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

/*
 * Internal API definition for clock outputs
 *
 * Since clock outputs only need to implement the "notify" API, we use a reduced
 * API pointer. Since the notify field is the first entry in both structures, we
 * can still receive callbacks via this API structure.
 */
struct clock_management_output_api {
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/* Notify clock consumer of rate change */
	int (*notify)(const struct clk *clk_hw, const struct clk *parent,
		      const struct clock_management_event *event);
#endif
};

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
	int ret;

	if (clk_state->num_clocks == 0) {
		/* Use runtime clock setting */
		ret = clock_round_rate(data->parent, clk_state->frequency);

		if (ret != clk_state->frequency) {
			return -ENOTSUP;
		}

		ret = clock_set_rate(data->parent, clk_state->frequency);
		if (ret != clk_state->frequency) {
			return -ENOTSUP;
		}

		return 0;
	}

	/* Apply this clock state */
	for (uint8_t i = 0; i < clk_state->num_clocks; i++) {
		const struct clock_setting *cfg = &clk_state->clock_settings[i];

		ret = clock_configure(cfg->clock, cfg->clock_config_data);
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

	if (!clk) {
		return -EINVAL;
	}

	data = GET_CLK_CORE(clk)->hw_data;

	/* Read rate */
	return clock_get_rate(data->parent);
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
	int ret = -ENOENT;
	const struct clock_output_state *best_state = NULL;
	int best_delta = INT32_MAX;
	struct clock_management_rate_req *combined_req;

	if (!clk) {
		return -EINVAL;
	}

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
		return -ENOENT;
	}
	/*
	 * We now know the new constraint is compatible. Now, save the
	 * updated constraint set as the shared set for this clock producer.
	 * We deliberately exclude the constraints of the clock output
	 * making this request, as the intermediate states of the clock
	 * producer may not be compatible with the new constraint. If we
	 * added the new constraint now then the clock would fail to
	 * reconfigure to an otherwise valid state, because the rates
	 * passed to clock_output_notify_consumer() would be rejected
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
	LOG_DBG("Request for range %u-%u issued to clock %s",
		combined_req->min_freq, combined_req->max_freq,
		GET_CLK_CORE(clk)->clk_name);
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
		    (state->frequency > combined_req->max_freq)) {
			/* This state isn't accurate enough */
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
	ret = clock_round_rate(data->parent, combined_req->max_freq);
out:
	if (ret >= 0) {
		/* A frequency was returned, check if it satisfies constraints */
		if ((combined_req->min_freq > ret) ||
		    (combined_req->max_freq < ret)) {
			return -ENOENT;
		}
	}
	/* Apply the clock state */
	ret = clock_set_rate(data->parent, combined_req->max_freq);
	if (ret < 0) {
		return ret;
	}
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	/* New clock state applied. Save the new combined constraint set. */
	memcpy(data->combined_req, combined_req, sizeof(*data->combined_req));
	/* Save the new constraint set for the consumer */
	memcpy(clk->req, req, sizeof(*clk->req));
#endif
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

	data = GET_CLK_CORE(clk)->hw_data;

	if (state >= data->num_states) {
		return -EINVAL;
	}

	clk_state = data->output_states[state];

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	struct clock_management_rate_req temp;
	/* Remove old constraint for this consumer */
	clock_remove_constraint(GET_CLK_CORE(clk), &temp, clk);

	/* Make sure this state fits within other consumer's constraints */
	if ((temp.min_freq > clk_state->frequency) ||
	    (temp.max_freq < clk_state->frequency)) {
		return -EINVAL;
	}

	/* Save new constraint set */
	memcpy(data->combined_req, &temp, sizeof(*data->combined_req));
#endif

	ret = clock_apply_state(GET_CLK_CORE(clk), clk_state);
	if (ret < 0) {
		return ret;
	}
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (clk_state->locking) {
		/* Set a constraint based on this clock state */
		const struct clock_management_rate_req constraint = {
			.min_freq = clk_state->frequency,
			.max_freq = clk_state->frequency,
		};

		/* Remove old constraint for this consumer */
		clock_remove_constraint(GET_CLK_CORE(clk), &temp, clk);
		/* Add new constraint and save it */
		clock_add_constraint(&temp, &constraint);
		memcpy(data->combined_req, &temp, sizeof(*data->combined_req));
		memcpy(clk->req, &constraint, sizeof(*clk->req));
	}
#endif
	return clk_state->frequency;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)

/*
 * This function passes clock notification callbacks from parent
 * clocks of this output on to any clock consumers that
 * have registered for callbacks
 */
static int clock_output_notify_consumer(const struct clk *clk_hw,
					const struct clk *parent,
					const struct clock_management_event *event)
{
	const struct clock_output_data *data;
	int ret;

	data = clk_hw->hw_data;

	/* Check if the new rate is permitted given constraints */
	if ((data->combined_req->min_freq > event->new_rate) ||
	    (data->combined_req->max_freq < event->new_rate)) {
#ifdef CONFIG_CLOCK_MANAGEMENT_CLK_NAME
		LOG_DBG("Clock %s rejected frequency %d",
			clk_hw->clk_name, event->new_rate);
#endif
		return -ENOTSUP;
	}

	if (event->type == CLOCK_MANAGEMENT_QUERY_RATE_CHANGE) {
		/* No need to forward to consumers */
		return 0;
	}

	for (const struct clock_output *consumer = data->consumer_start;
	     consumer < data->consumer_end; consumer++) {
		if (consumer->cb->clock_callback) {
			ret = consumer->cb->clock_callback(event,
						consumer->cb->user_data);
			if (ret) {
				/* Consumer rejected new rate */
				return ret;
			}
		}
	}
	return 0;
}
#endif

const struct clock_management_output_api clock_output_api = {
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.notify = clock_output_notify_consumer,
#endif
};

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
		IF_ENABLED(DT_NODE_HAS_PROP(node, clocks), (                   \
		.clock_settings = {                                            \
			DT_FOREACH_PROP_ELEM_SEP(node, clocks,                 \
						 CLOCK_SETTINGS_GET, (,))      \
		},))                                                           \
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME,                          \
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
	struct clock_management_rate_req combined_req_##inst = {                     \
		.min_freq = 0,                                                 \
		.max_freq = INT32_MAX,                                         \
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
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &CONCAT(clock_output_, DT_INST_DEP_ORD(inst)),    \
			     (struct clock_management_driver_api *)&clock_output_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_OUTPUT_DEFINE)

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
/**
 * @brief Helper to issue a clock callback to all children nodes
 *
 * Helper function to issue a callback to all children of a given clock, with
 * a new clock rate. This function will call clock_notify on all children of
 * the given clock, with the provided rate as the parent rate
 *
 * @param clk_hw Clock object to issue callbacks for
 * @param event Clock reconfiguration event
 * @return 0 on success
 * @return CLK_NO_CHILDREN to indicate clock has no children actively using it,
 *         and may safely shut down.
 * @return -errno from @ref clock_notify on any other failure
 */
int clock_notify_children(const struct clk *clk_hw,
			  const struct clock_management_event *event)
{
	const clock_handle_t *handle = clk_hw->children;
	int ret;
	bool children_disconnected = true;

	while (*handle != CLOCK_LIST_END) {
		ret = clock_notify(clk_from_handle(*handle), clk_hw, event);
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

#endif /* CONFIG_CLOCK_MANAGEMENT_RUNTIME */
