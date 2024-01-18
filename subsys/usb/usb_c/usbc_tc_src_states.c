/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(usbc_stack, CONFIG_USBC_STACK_LOG_LEVEL);

#include "usbc_stack.h"
#include "usbc_tc_src_states_internal.h"
#include <zephyr/drivers/usb_c/usbc_ppc.h>

/**
 * @brief Spec. Release 1.3, section 4.5.2.2.7 Unattached.SRC State
 *
 * When in the Unattached.SRC state, the port is waiting to detect the
 * presence of a Sink or an Accessory.
 *
 * Requirements:
 *   1: The port shall not drive VBUS or VCONN.
 *      NOTE: Implemented in the tc_attached_src_exit
 *            function and initially set the tc_init function.
 *
 *   2: The port shall provide a separate Rp termination on the CC1 and
 *      CC2 pins.
 *      NOTE: Implemented in the tc_cc_rp super state.
 */

void tc_unattached_src_entry(void *obj)
{
	LOG_INF("Unattached.SRC");
}

void tc_unattached_src_run(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;

	/*
	 * Transition to AttachWait.SRC when:
	 *   The SRC.Rd is detected on either CC1 or CC2 pin or
	 *   SRC.Ra is detected on both CC1 and CC2 pins.
	 *   NOTE: Audio Adapter Accessory Mode is not supported, so
	 *   SRC.Ra will not be checked.
	 */
	if (tcpc_is_cc_at_least_one_rd(tc->cc1, tc->cc2)) {
		tc_set_state(dev, TC_ATTACH_WAIT_SRC_STATE);
	}
}

/**
 * @brief Spec. Release 1.3, section 4.5.2.2.6 UnattachedWait.SRC State
 *
 * When in the UnattachedWait.SRC state, the port is discharging the CC pin
 * that was providing VCONN in the previous Attached.SRC state.
 *
 * Requirements:
 *  1: The port shall not enable VBUS or VCONN.
 *     NOTE: Implemented in tc_attached_src_exit
 *
 *  2: The port shall continue to provide an Rp termination on the CC pin not
 *     being discharged.
 *     NOTE: Implemented in TC_CC_RP_SUPER_STATE super state.
 *
 *  3: The port shall provide an Rdch termination on the CC pin being
 *     discharged.
 *     NOTE: Implemented in tc_unattached_wait_src_entry
 */

void tc_unattached_wait_src_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;

	LOG_INF("UnattachedWait.SRC");

	/* Start discharging VCONN */
	tcpc_vconn_discharge(tcpc, true);

	/* Start VCONN off timer */
	usbc_timer_start(&tc->tc_t_vconn_off);
}

void tc_unattached_wait_src_run(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;

	/* CC Debounce time should be enough time for VCONN to discharge */
	if (usbc_timer_expired(&tc->tc_t_vconn_off)) {
		tc_set_state(dev, TC_UNATTACHED_SRC_STATE);
	}
}

void tc_unattached_wait_src_exit(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;

	/* Stop discharging VCONN */
	tcpc_vconn_discharge(tcpc, false);

	/* Stop timer */
	usbc_timer_stop(&tc->tc_t_vconn_off);
}

/**
 * @brief Spec. Release 1.3, section 4.5.2.2.8 AttachWait.SRC State
 *
 * The AttachWait.SRC state is used to ensure that the state of both of
 * the CC1 and CC2 pins is stable after a Sink is connected.
 *
 * Requirements:
 *   The requirements for this state are identical to Unattached.SRC.
 */

void tc_attach_wait_src_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;

	LOG_INF("AttachWait.SRC");

	/* Initialize the cc state to open */
	tc->cc_state = TC_CC_NONE;
}

void tc_attach_wait_src_run(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *vbus = data->vbus;
	enum tc_cc_states new_cc_state;

	/* Is a connection detected? */
	if (tcpc_is_cc_at_least_one_rd(tc->cc1, tc->cc2)) {
		/* UFP attached */
		new_cc_state = TC_CC_UFP_ATTACHED;
	} else {
		/* No UFP */
		tc_set_state(dev, TC_UNATTACHED_SRC_STATE);
		return;
	}

	/* Debounce the cc state */
	if (new_cc_state != tc->cc_state) {
		/* Start debouce timer */
		usbc_timer_start(&tc->tc_t_cc_debounce);
		tc->cc_state = new_cc_state;
	}

	/* Wait for CC debounce */
	if (usbc_timer_running(&tc->tc_t_cc_debounce) &&
		!usbc_timer_expired(&tc->tc_t_cc_debounce)) {
		return;
	}

	/*
	 * The port shall transition to Attached.SRC when VBUS is at vSafe0V
	 * and the SRC.Rd state is detected on exactly one of the CC1 or CC2
	 * pins for at least tCCDebounce.
	 */
	if (usbc_vbus_check_level(vbus, TC_VBUS_SAFE0V)) {
		if (new_cc_state == TC_CC_UFP_ATTACHED) {
			tc_set_state(dev, TC_ATTACHED_SRC_STATE);
		}
	}
}

void tc_attach_wait_src_exit(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;

	/* Stop debounce timer */
	usbc_timer_stop(&tc->tc_t_cc_debounce);
}

/**
 * @brief Spec. Release 1.3, section 4.5.2.2.9 Attached.SRC State
 *
 * When in the Attached.SRC state, the port is attached and operating as a
 * Source. When the port initially enters this state it is also operating
 * as a DFP. Subsequently, the initial power and data roles can be changed
 * using USB PD commands.
 *
 * Requirements:
 *  1: If the port needs to determine the orientation of the connector, it
 *     shall do so only upon entry to the Attached.SRC state by detecting
 *     which of the CC1 or CC2 pins is connected through the
 *     cable, i.e., which CC pin is in the SRC.Rd state.
 *     NOTE: Implemented in tc_attached_src_entry.
 *
 * 2: If the port has entered this state from the AttachWait.SRC state,
 *    the SRC.Rd state will be on only one of the CC1 or CC2 pins. The
 *    port shall source current on this CC pin and monitor its state.
 *    NOTE: Implemented in the super state of AttachWait.SRC.
 *
 * 3: The port shall provide an Rp
 *    NOTE: Implemented in the super state of AttachWait.SRC.
 *
 * 5: The port shall supply VBUS current at the level it advertises on Rp.
 *    NOTE: Implemented in tc_attached_src_entry.
 *
 * 7: The port shall not initiate any USB PD communications until VBUS
 *    reaches vSafe5V.
 *    NOTE: Implemented in tc_attached_src_run.
 *
 * 8: The port may negotiate a USB PD PR_Swap, DR_Swap or VCONN_Swap.
 *    NOTE: Implemented in tc_attached_src_run.
 *
 * 9: If the port supplies VCONN, it shall do so within t_VCONN_ON.
 *    NOTE: Implemented in tc_attached_src_entry.
 */

void tc_attached_src_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;
	int ret;

	LOG_INF("Attached.SRC");

	/* Initial data role for source is DFP */
	tcpc_set_roles(tcpc, TC_ROLE_SOURCE, TC_ROLE_DFP);

	/* Set cc polarity */
	ret = tcpc_set_cc_polarity(tcpc, tc->cc_polarity);
	if (ret != 0) {
		LOG_ERR("Couldn't set CC polarity to %d: %d", tc->cc_polarity, ret);
		tc_set_state(dev, TC_ERROR_RECOVERY_STATE);
		return;
	}

	/* Start sourcing VBUS */
	if (data->policy_cb_src_en(dev, true) == 0) {
		/* Start sourcing VCONN */
		if (policy_check(dev, CHECK_VCONN_CONTROL)) {
			if (tcpc_set_vconn(tcpc, true) == 0) {
				atomic_set_bit(&tc->flags, TC_FLAGS_VCONN_ON);
			} else {
				LOG_ERR("VCONN can't be enabled\n");
			}
		}
	} else {
		LOG_ERR("Power Supply can't be enabled\n");
	}

	/* Enable PD */
	tc_pd_enable(dev, true);

	/* Enable the VBUS sourcing by the PPC */
	if (data->ppc != NULL) {
		ret = ppc_set_src_ctrl(data->ppc, true);
		if (ret < 0 && ret != -ENOSYS) {
			LOG_ERR("Couldn't disable PPC source");
		}
	}
}

void tc_attached_src_run(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;

	/* Monitor for CC disconnection */
	if (tcpc_is_cc_open(tc->cc1, tc->cc2)) {
		/*
		 * A Source that is supplying VCONN or has yielded VCONN source
		 * responsibility to the Sink through USBPD VCONN_Swap messaging
		 * shall transition to UnattachedWait.SRC when the SRC.Open state
		 * is detected on the monitored CC pin. The Source shall detect
		 * the SRC.Open state within tSRCDisconnect, but should detect
		 * it as quickly as possible.
		 */
		if (atomic_test_and_clear_bit(&tc->flags, TC_FLAGS_VCONN_ON)) {
			tc_set_state(dev, TC_UNATTACHED_WAIT_SRC_STATE);
		}
		/*
		 * A Source that is not supplying VCONN and has not yielded
		 * VCONN responsibility to the Sink through USBPD VCONN_Swap
		 * messaging shall transition to Unattached.SRC when the
		 * SRC.Open state is detected on the monitored CC pin. The
		 * Source shall detect the SRC.Open state within tSRCDisconnect,
		 * but should detect it as quickly as possible.
		 */
		else {
			tc_set_state(dev, TC_UNATTACHED_SRC_STATE);
		}
	}
}

void tc_attached_src_exit(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;
	int ret;

	__ASSERT(data->policy_cb_src_en != NULL,
			"policy_cb_src_en must not be NULL");

	/* Disable PD */
	tc_pd_enable(dev, false);

	/* Stop sourcing VBUS */
	data->policy_cb_src_en(dev, false);

	/* Disable the VBUS sourcing by the PPC */
	if (data->ppc != NULL) {
		ret = ppc_set_src_ctrl(data->ppc, false);
		if (ret < 0 && ret != -ENOSYS) {
			LOG_ERR("Couldn't disable PPC source");
		}
	}

	/* Stop sourcing VCONN */
	ret = tcpc_set_vconn(tcpc, false);
	if (ret != 0 && ret != -ENOSYS) {
		LOG_ERR("Couldn't disable VCONN source");
	}
}

/**
 * @brief This is a super state for Source States that
 *	  requirement the Rp value placed on the CC lines.
 */
void tc_cc_rp_entry(void *obj)
{
	struct tc_sm_t *tc = (struct tc_sm_t *)obj;
	const struct device *dev = tc->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;
	enum tc_rp_value rp = TC_RP_USB;
	int ret;

	/*
	 * Get initial Rp value from Device Policy Manager or use
	 * default TC_RP_USB.
	 */
	if (data->policy_cb_get_src_rp) {
		data->policy_cb_get_src_rp(dev, &rp);
	}

	/* Select Rp value */
	ret = tcpc_select_rp_value(tcpc, rp);
	if (ret != 0 && ret != -ENOTSUP) {
		LOG_ERR("Couldn't set Rp value to %d: %d", rp, ret);
		tc_set_state(dev, TC_ERROR_RECOVERY_STATE);
		return;
	}

	/* Place Rp on CC lines */
	ret = tcpc_set_cc(tcpc, TC_CC_RP);
	if (ret != 0) {
		LOG_ERR("Couldn't set CC lines to Rp: %d", ret);
		tc_set_state(dev, TC_ERROR_RECOVERY_STATE);
	}
}
