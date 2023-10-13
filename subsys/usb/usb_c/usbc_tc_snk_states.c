/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(usbc_stack, CONFIG_USBC_STACK_LOG_LEVEL);

#include "usbc_stack.h"
#include "usbc_tc_snk_states_internal.h"
#include "usbc_tc_common_internal.h"
#include <zephyr/drivers/usb_c/usbc_ppc.h>

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
	if (atomic_test_and_clear_bit(&tc->flags, TC_FLAGS_RP_SUBSTATE_CHANGE)) {
		if (data->policy_cb_notify) {
			data->policy_cb_notify(dev, dpm_pwr_change_notify);
		}
	}
}

/**
 * @brief Unattached.SNK Entry
 */
void tc_unattached_snk_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;

	LOG_INF("Unattached.SNK");

	/*
	 * Allow the state machine to immediately check the state of CC lines and go into
	 * Attach.Wait state in case the Rp value is detected on the CC lines
	 */
	usbc_bypass_next_sleep(tc->dev);
}

/**
 * @brief Unattached.SNK Run
 */
void tc_unattached_snk_run(void *obj)
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
void tc_attach_wait_snk_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;

	LOG_INF("AttachWait.SNK");

	tc->cc_state = TC_CC_NONE;

	/*
	 * Allow the debounce timers to start immediately without additional delay added
	 * by going into sleep
	 */
	usbc_bypass_next_sleep(tc->dev);
}

/**
 * @brief AttachWait.SNK Run
 */
void tc_attach_wait_snk_run(void *obj)
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
		if (CONFIG_USBC_STATE_MACHINE_CYCLE_TIME >= TC_T_CC_DEBOUNCE_MIN_MS) {
			/* Make sure the debounce time won't be longer than specified */
			usbc_bypass_next_sleep(tc->dev);
		}

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

	/*
	 * In case of no VBUS present, this call prevents going into the sleep and allows for
	 * faster VBUS detection. In case of VBUS present, allows for immediate execution of logic
	 * from new state.
	 */
	usbc_bypass_next_sleep(tc->dev);
}

void tc_attach_wait_snk_exit(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;

	usbc_timer_stop(&tc->tc_t_cc_debounce);
}

/**
 * @brief Attached.SNK Entry
 */
void tc_attached_snk_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;
	int ret;

	LOG_INF("Attached.SNK");

	/* Set CC polarity */
	ret = tcpc_set_cc_polarity(tcpc, tc->cc_polarity);
	if (ret != 0) {
		LOG_ERR("Couldn't set CC polarity to %d: %d", tc->cc_polarity, ret);
		tc_set_state(dev, TC_ERROR_RECOVERY_STATE);
		return;
	}

	/* Enable PD */
	tc_pd_enable(dev, true);

	/* Enable sink path for the PPC */
	if (data->ppc != NULL) {
		ret = ppc_set_snk_ctrl(data->ppc, true);
		if (ret != 0 && ret != -ENOTSUP) {
			LOG_ERR("Couldn't enable PPC sink path: %d", ret);
		}
	}
}

/**
 * @brief Attached.SNK and DebugAccessory.SNK Run
 */
void tc_attached_snk_run(void *obj)
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
void tc_attached_snk_exit(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	int ret;

	/* Disable PD */
	tc_pd_enable(dev, false);

	/* Disable sink path for the PPC */
	if (data->ppc != NULL) {
		ret = ppc_set_snk_ctrl(data->ppc, false);
		if (ret != 0 && ret != -ENOTSUP) {
			LOG_ERR("Couldn't disable PPC sink path: %d", ret);
		}
	}
}

/**
 * @brief Rd on CC lines Entry
 */
void tc_cc_rd_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;
	int ret;

	ret = tcpc_set_cc(tcpc, TC_CC_RD);
	if (ret != 0) {
		LOG_ERR("Couldn't set CC lines to Rd: %d", ret);
		tc_set_state(dev, TC_ERROR_RECOVERY_STATE);
	}
}
