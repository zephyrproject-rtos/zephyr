/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(usbc_stack, CONFIG_USBC_STACK_LOG_LEVEL);

#include "usbc_stack.h"

/**
 * @brief Type-C Layer Flags
 */
enum tc_flags {
	/**
	 * Flag to track Rp resistor change when the sink attached
	 * sub-state runs
	 */
	TC_FLAGS_RP_SUBSTATE_CHANGE = 0,
};

/**
 * @brief Type-C States
 */
enum tc_state_t {
	/** Super state that opens the CC lines */
	TC_CC_OPEN_SUPER_STATE,
	/** Super state that applies Rd to the CC lines */
	TC_CC_RD_SUPER_STATE,
	/** Disabled State */
	TC_DISABLED_STATE,
	/** Error Recovery State */
	TC_ERROR_RECOVERY_STATE,
	/** Unnattached Sink State */
	TC_UNATTACHED_SNK_STATE,
	/** Attach Wait Sink State */
	TC_ATTACH_WAIT_SNK_STATE,
	/** Attached Sink State */
	TC_ATTACHED_SNK_STATE,
};

static const struct smf_state tc_snk_states[];
static void tc_init(const struct device *dev);
static void tc_set_state(const struct device *dev,
			 const enum tc_state_t state);
static enum tc_state_t tc_get_state(const struct device *dev);
static void pd_enable(const struct device *dev,
		      const bool enable);

/**
 * @brief Initializes the state machine and enters the Disabled state
 */
void tc_subsys_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct tc_sm_t *tc = data->tc;

	/* Save the port device object so states can access it */
	tc->dev = dev;

	/* Initialize the state machine */
	smf_set_initial(SMF_CTX(tc), &tc_snk_states[TC_DISABLED_STATE]);
}

/**
 * @brief Runs the Type-C layer
 */
void tc_run(const struct device *dev,
	    const int32_t dpm_request)
{
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;
	struct tc_sm_t *tc = data->tc;

	/* These requests are implicitly set by the Device Policy Manager */
	if (dpm_request == PRIV_PORT_REQUEST_START) {
		data->tc_enabled = true;
	} else if (dpm_request == PRIV_PORT_REQUEST_SUSPEND) {
		data->tc_enabled = false;
		tc_set_state(dev, TC_DISABLED_STATE);
	}

	switch (data->tc_sm_state) {
	case SM_PAUSED:
		if (data->tc_enabled == false) {
			break;
		}
	/* fall through */
	case SM_INIT:
		/* Initialize the Type-C layer */
		tc_init(dev);
		data->tc_sm_state = SM_RUN;
	/* fall through */
	case SM_RUN:
		if (data->tc_enabled == false) {
			pd_enable(dev, false);
			data->tc_sm_state = SM_PAUSED;
			break;
		}

		/* Sample CC lines */
		tcpc_get_cc(tcpc, &tc->cc1, &tc->cc2);

		/* Detect polarity */
		tc->cc_polarity = (tc->cc1 > tc->cc2) ?
			TC_POLARITY_CC1 : TC_POLARITY_CC2;

		/* Execute any asyncronous Device Policy Manager Requests */
		if (dpm_request == REQUEST_TC_ERROR_RECOVERY) {
			/* Transition to Error Recovery State */
			tc_set_state(dev, TC_ERROR_RECOVERY_STATE);
		} else if (dpm_request == REQUEST_TC_DISABLED) {
			/* Transition to Disabled State */
			tc_set_state(dev, TC_DISABLED_STATE);
		}

		/* Run state machine */
		smf_run_state(SMF_CTX(tc));
	}
}

/**
 * @brief Checks if the TC Layer is in an Attached state
 */
bool tc_is_in_attached_state(const struct device *dev)
{
	return (tc_get_state(dev) == TC_ATTACHED_SNK_STATE);
}

/**
 * @brief Initializes the Type-C layer
 */
static void tc_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct tc_sm_t *tc = data->tc;

	/* Initialize the timers */
	usbc_timer_init(&tc->tc_t_error_recovery, TC_T_ERROR_RECOVERY_SOURCE_MIN_MS);
	usbc_timer_init(&tc->tc_t_cc_debounce, TC_T_CC_DEBOUNCE_MAX_MS);
	usbc_timer_init(&tc->tc_t_rp_value_change, TC_T_RP_VALUE_CHANGE_MAX_MS);

	/* Clear the flags */
	tc->flags = ATOMIC_INIT(0);

	/* Initialize the TCPC */
	tcpc_init(data->tcpc);

	/* Initialize the state machine */
	/*
	 * Start out in error recovery state so the CC lines are opened for a
	 * short while if this is a system reset.
	 */
	tc_set_state(dev, TC_ERROR_RECOVERY_STATE);
}

/**
 * @brief Sets a Type-C state
 */
static void tc_set_state(const struct device *dev,
			 const enum tc_state_t state)
{
	struct usbc_port_data *data = dev->data;
	struct tc_sm_t *tc = data->tc;

	smf_set_state(SMF_CTX(tc), &tc_snk_states[state]);
}

/**
 * @brief Get the Type-C current state
 */
static enum tc_state_t tc_get_state(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->tc->ctx.current - &tc_snk_states[0];
}

/**
 * @brief Enable Power Delivery
 */
static void pd_enable(const struct device *dev,
		      const bool enable)
{
	if (enable) {
		prl_start(dev);
		pe_start(dev);
	} else {
		prl_suspend(dev);
		pe_suspend(dev);
	}
}

/**
 * @brief Sink power sub states. Only called if a PD contract is not in place
 */
static void sink_power_sub_states(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	enum tc_cc_voltage_state cc;
	enum tc_cc_voltage_state new_cc_voltage;
	enum usbc_policy_check_t dpm_pwr_change_notify;
	struct tc_sm_t *tc = data->tc;

	/* Get the active CC line */
	cc = tc->cc_polarity ? tc->cc2 : tc->cc1;

	if (cc == TC_CC_VOLT_RP_DEF) {
		/*
		 * This sub-state supports Sinks consuming current within the
		 * lowest range (default) of Source-supplied current.
		 */
		new_cc_voltage = TC_CC_VOLT_RP_DEF;
		dpm_pwr_change_notify = POWER_CHANGE_DEF;
	} else if (cc == TC_CC_VOLT_RP_1A5) {
		/*
		 * This sub-state supports Sinks consuming current within the
		 * two lower ranges (default and 1.5 A) of Source-supplied
		 * current.
		 */
		new_cc_voltage = TC_CC_VOLT_RP_1A5;
		dpm_pwr_change_notify = POWER_CHANGE_1A5;
	} else if (cc == TC_CC_VOLT_RP_3A0) {
		/*
		 * This sub-state supports Sinks consuming current within all
		 * three ranges (default, 1.5 A and 3.0 A) of Source-supplied
		 * current.
		 */
		new_cc_voltage = TC_CC_VOLT_RP_3A0;
		dpm_pwr_change_notify = POWER_CHANGE_3A0;
	} else {
		/* Disconnect detected */
		new_cc_voltage = TC_CC_VOLT_OPEN;
		dpm_pwr_change_notify = POWER_CHANGE_0A0;
	}

	/* Debounce the Rp state */
	if (new_cc_voltage != tc->cc_voltage) {
		tc->cc_voltage = new_cc_voltage;
		atomic_set_bit(&tc->flags, TC_FLAGS_RP_SUBSTATE_CHANGE);
		usbc_timer_start(&tc->tc_t_rp_value_change);
	}

	/* Wait for Rp debounce */
	if (usbc_timer_expired(&tc->tc_t_rp_value_change) == false) {
		return;
	}

	/* Notify DPM of sink sub-state power change */
	if (atomic_test_and_clear_bit(&tc->flags,
				TC_FLAGS_RP_SUBSTATE_CHANGE)) {
		if (data->policy_cb_notify) {
			data->policy_cb_notify(dev, dpm_pwr_change_notify);
		}
	}
}

/**
 * @brief Unattached.SNK Entry
 */
static void tc_unattached_snk_entry(void *obj)
{
	LOG_INF("Unattached.SNK");
}

/**
 * @brief Unattached.SNK Run
 */
static void tc_unattached_snk_run(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;

	/*
	 * Transition to AttachWait.SNK when the SNK.Rp state is present
	 * on at least one of its CC pins.
	 */
	if (tcpc_is_cc_rp(tc->cc1) || tcpc_is_cc_rp(tc->cc2)) {
		tc_set_state(dev, TC_ATTACH_WAIT_SNK_STATE);
	}
}

/**
 * @brief AttachWait.SNK Entry
 */
static void tc_attach_wait_snk_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;

	LOG_INF("AttachWait.SNK");

	tc->cc_state = TC_CC_NONE;
}

/**
 * @brief AttachWait.SNK Run
 */
static void tc_attach_wait_snk_run(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *vbus = data->vbus;
	enum tc_cc_states new_cc_state;
	bool vbus_present;

	if (tcpc_is_cc_rp(tc->cc1) || tcpc_is_cc_rp(tc->cc2)) {
		new_cc_state = TC_CC_DFP_ATTACHED;
	} else {
		new_cc_state = TC_CC_NONE;
	}

	/* Debounce the cc state */
	if (new_cc_state != tc->cc_state) {
		usbc_timer_start(&tc->tc_t_cc_debounce);
		tc->cc_state = new_cc_state;
	}

	/* Wait for CC debounce */
	if (usbc_timer_running(&tc->tc_t_cc_debounce) &&
		usbc_timer_expired(&tc->tc_t_cc_debounce) == false) {
		return;
	}

	/* Transition to UnAttached.SNK if CC lines are open */
	if (new_cc_state == TC_CC_NONE) {
		tc_set_state(dev, TC_UNATTACHED_SNK_STATE);
	}

	/*
	 * The port shall transition to Attached.SNK after the state of only
	 * one of the CC1 or CC2 pins is SNK.Rp for at least tCCDebounce and
	 * VBUS is detected.
	 */
	vbus_present = usbc_vbus_check_level(vbus, TC_VBUS_PRESENT);

	if (vbus_present) {
		tc_set_state(dev, TC_ATTACHED_SNK_STATE);
	}
}

static void tc_attach_wait_snk_exit(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;

	usbc_timer_stop(&tc->tc_t_cc_debounce);
}

/**
 * @brief Attached.SNK Entry
 */
static void tc_attached_snk_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;

	LOG_INF("Attached.SNK");

	/* Set CC polarity */
	tcpc_set_cc_polarity(tcpc, tc->cc_polarity);

	/* Enable PD */
	pd_enable(dev, true);
}

/**
 * @brief Attached.SNK and DebugAccessory.SNK Run
 */
static void tc_attached_snk_run(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *vbus = data->vbus;

	/* Detach detection */
	if (usbc_vbus_check_level(vbus, TC_VBUS_PRESENT) == false) {
		tc_set_state(dev, TC_UNATTACHED_SNK_STATE);
		return;
	}

	/* Run Sink Power Sub-State if not in an explicit contract */
	if (pe_is_explicit_contract(dev) == false) {
		sink_power_sub_states(dev);
	}
}

/**
 * @brief Attached.SNK and DebugAccessory.SNK Exit
 */
static void tc_attached_snk_exit(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;

	/* Disable PD */
	pd_enable(dev, false);
}

/**
 * @brief CC Open Entry
 */
static void tc_cc_open_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;

	tc->cc_voltage = TC_CC_VOLT_OPEN;

	/* Disable VCONN */
	tcpc_set_vconn(tcpc, false);

	/* Open CC lines */
	tcpc_set_cc(tcpc, TC_CC_OPEN);
}

/**
 * @brief Rd on CC lines Entry
 */
static void tc_cc_rd_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;

	tcpc_set_cc(tcpc, TC_CC_RD);
}

/**
 * @brief Disabled Entry
 */
static void tc_disabled_entry(void *obj)
{
	LOG_INF("Disabled");
}

/**
 * @brief Disabled Run
 */
static void tc_disabled_run(void *obj)
{
	/* Do nothing */
}

/**
 * @brief ErrorRecovery Entry
 */
static void tc_error_recovery_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;

	LOG_INF("ErrorRecovery");

	/* Start tErrorRecovery timer */
	usbc_timer_start(&tc->tc_t_error_recovery);
}

/**
 * @brief ErrorRecovery Run
 */
static void tc_error_recovery_run(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;

	/* Wait for expiry */
	if (usbc_timer_expired(&tc->tc_t_error_recovery) == false) {
		return;
	}

	/* Transition to Unattached.SNK */
	tc_set_state(dev, TC_UNATTACHED_SNK_STATE);
}

/**
 * @brief Type-C State Table
 */
static const struct smf_state tc_snk_states[] = {
	/* Super States */
	[TC_CC_OPEN_SUPER_STATE] = SMF_CREATE_STATE(
		tc_cc_open_entry,
		NULL,
		NULL,
		NULL),
	[TC_CC_RD_SUPER_STATE] = SMF_CREATE_STATE(
		tc_cc_rd_entry,
		NULL,
		NULL,
		NULL),
	/* Normal States */
	[TC_UNATTACHED_SNK_STATE] = SMF_CREATE_STATE(
		tc_unattached_snk_entry,
		tc_unattached_snk_run,
		NULL,
		&tc_snk_states[TC_CC_RD_SUPER_STATE]),
	[TC_ATTACH_WAIT_SNK_STATE] = SMF_CREATE_STATE(
		tc_attach_wait_snk_entry,
		tc_attach_wait_snk_run,
		tc_attach_wait_snk_exit,
		&tc_snk_states[TC_CC_RD_SUPER_STATE]),
	[TC_ATTACHED_SNK_STATE] = SMF_CREATE_STATE(
		tc_attached_snk_entry,
		tc_attached_snk_run,
		tc_attached_snk_exit,
		NULL),
	[TC_DISABLED_STATE] = SMF_CREATE_STATE(
		tc_disabled_entry,
		tc_disabled_run,
		NULL,
		&tc_snk_states[TC_CC_OPEN_SUPER_STATE]),
	[TC_ERROR_RECOVERY_STATE] = SMF_CREATE_STATE(
		tc_error_recovery_entry,
		tc_error_recovery_run,
		NULL,
		&tc_snk_states[TC_CC_OPEN_SUPER_STATE]),
};
