/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB-C Power Policy Engine (PE)
 *
 * The information in this file was taken from the USB PD
 * Specification Revision 3.0, Version 2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/smf.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(usbc_stack, CONFIG_USBC_STACK_LOG_LEVEL);

#include "usbc_stack.h"

/**
 * @brief Initialize the Source Policy Engine layer
 */
void pe_src_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Initial role of source is DFP */
	pe_set_data_role(dev, TC_ROLE_DFP);

	/* Reject Sink Request by default */
	pe->snk_request_reply = SNK_REQUEST_REJECT;

	/* Initialize timers */
	usbc_timer_init(&pe->pd_t_typec_send_source_cap, PD_T_TYPEC_SEND_SOURCE_CAP_MIN_MS);
	usbc_timer_init(&pe->pd_t_ps_hard_reset, PD_T_PS_HARD_RESET_MAX_MS);

	/* Goto startup state */
	pe_set_state(dev, PE_SRC_STARTUP);
}

/**
 * @brief Handle source-specific DPM requests
 */
bool source_dpm_requests(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	if (pe->dpm_request == REQUEST_GET_SNK_CAPS) {
		atomic_set_bit(pe->flags, PE_FLAGS_DPM_INITIATED_AMS);
		pe_set_state(dev, PE_GET_SINK_CAP);
		return true;
	} else if (pe->dpm_request == REQUEST_PE_GOTO_MIN) {
		atomic_set_bit(pe->flags, PE_FLAGS_DPM_INITIATED_AMS);
		pe_set_state(dev, PE_SRC_TRANSITION_SUPPLY);
		return true;
	}

	return false;
}

/**
 * @brief Send Source Caps to Sink
 */
static void send_src_caps(struct policy_engine *pe)
{
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct pd_msg *msg = &prl_tx->emsg;
	const uint32_t *pdos;
	uint32_t num_pdos = 0;

	/* This callback must be implemented */
	__ASSERT(data->policy_cb_get_src_caps != NULL,
			"Callback pointer should not be NULL");

	data->policy_cb_get_src_caps(dev, &pdos, &num_pdos);

	msg->len = PD_CONVERT_PD_HEADER_COUNT_TO_BYTES(num_pdos);
	memcpy(msg->data, pdos, msg->len);
	pe_send_data_msg(dev, PD_PACKET_SOP, PD_DATA_SOURCE_CAP);
}

/**
 * @brief 8.3.3.2.1 PE_SRC_Startup State
 */
void pe_src_startup_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SRC_Startup");

	/* Reset CapsCounter */
	pe->caps_counter = 0;

	/* Reset the protocol layer */
	prl_reset(dev);

	/* Set power role to Source */
	pe->power_role = TC_ROLE_SOURCE;

	/* Invalidate explicit contract */
	atomic_clear_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT);

	policy_notify(dev, NOT_PD_CONNECTED);
}

void pe_src_startup_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * Once the reset process completes, the Policy Engine Shall
	 * transition to the PE_SRC_Send_Capabilities state
	 */
	if (prl_is_running(dev)) {
		pe_set_state(dev, PE_SRC_SEND_CAPABILITIES);
	}
}

/**
 * @brief 8.3.3.2.2 PE_SRC_Discovery State
 */
void pe_src_discovery_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SRC_Discovery");

	/*
	 * Start the SourceCapabilityTimer in order to trigger sending a
	 * Source_Capabilities message
	 */
	usbc_timer_start(&pe->pd_t_typec_send_source_cap);
}

void pe_src_discovery_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * The Policy Engine Shall transition to the PE_SRC_Send_Capabilities state when:
	 *	1) The SourceCapabilityTimer times out
	 *	2) And CapsCounter ≤ nCapsCount
	 */
	if (usbc_timer_expired(&pe->pd_t_typec_send_source_cap)
			&& pe->caps_counter <= PD_N_CAPS_COUNT) {
		pe_set_state(dev, PE_SRC_SEND_CAPABILITIES);
	}
}

void pe_src_discovery_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	usbc_timer_stop(&pe->pd_t_typec_send_source_cap);
}

/**
 * @brief 8.3.3.2.3 PE_SRC_Send_Capabilities State
 */
void pe_src_send_capabilities_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Request present source capabilities from Device Policy Manager */
	send_src_caps(pe);
	/* Increment CapsCounter */
	pe->caps_counter++;
	/* Init submachine */
	pe->submachine = SM_WAIT_FOR_TX;

	LOG_INF("PE_SRC_Send_Capabilities");
}

void pe_src_send_capabilities_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	switch (pe->submachine) {
	case SM_WAIT_FOR_TX:
		/*
		 * When message is sent, the Policy Engine Shall:
		 *	1) Stop the NoResponseTimer .
		 *	2) Reset the HardResetCounter and CapsCounter to zero.
		 *	3) Initialize and run the SenderResponseTimer
		 */
		if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE)) {
			usbc_timer_stop(&pe->pd_t_no_response);
			pe->hard_reset_counter = 0;
			pe->caps_counter = 0;
			pe->submachine = SM_WAIT_FOR_RX;
		}
		/*
		 * The Policy Engine Shall transition to the PE_SRC_Discovery
		 * state when:
		 *	1) The Protocol Layer indicates that the Message has
		 *	   not been sent
		 *	2) And we are presently not Connected.
		 */
		else if ((atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_XMIT_ERROR) ||
			atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_DISCARDED))
			&& (atomic_test_bit(pe->flags, PE_FLAGS_PD_CONNECTED) == false)) {
			pe_set_state(dev, PE_SRC_DISCOVERY);
		}
		break;
	case SM_WAIT_FOR_RX:
		/*
		 * The Policy Engine Shall transition to the PE_SRC_Negotiate_Capability state when:
		 *	1) A Request Message is received from the Sink.
		 */
		if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
			union pd_header header = prl_rx->emsg.header;

			if (received_data_message(dev, header, PD_DATA_REQUEST)) {
				/* Set to highest revision supported by both ports */
				prl_set_rev(dev, PD_PACKET_SOP,
					MIN(PD_REV30, header.specification_revision));
				pe_set_state(dev, PE_SRC_NEGOTIATE_CAPABILITY);
			}
		}
		/*
		 * The Policy Engine Shall transition to the PE_SRC_Hard_Reset
		 * state when:
		 *	1) The SenderResponseTimer times out
		 */
		else if (usbc_timer_expired(&pe->pd_t_sender_response)) {
			pe_set_state(dev, PE_SRC_HARD_RESET);
		}
		break;
	}
}

/**
 * @brief 8.3.3.2.4 PE_SRC_Negotiate_Capability State
 */
void pe_src_negotiate_capability_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	LOG_INF("PE_SRC_Negotiate_Capability");

	/* Get sink request */
	pe->snk_request = *(uint32_t *)prl_rx->emsg.data;

	/*
	 * Ask the Device Policy Manager to evaluate the Request
	 * from the Attached Sink.
	 */
	pe->snk_request_reply =
		policy_check_sink_request(dev, pe->snk_request);

	/*
	 * The Policy Engine Shall transition to the
	 * PE_SRC_Transition_Supply state when:
	 *	1) The Request can be met.
	 */
	if (pe->snk_request_reply == SNK_REQUEST_VALID) {
		pe_set_state(dev, PE_SRC_TRANSITION_SUPPLY);
	}
	/*
	 * The Policy Engine Shall transition to the
	 * PE_SRC_Capability_Response state when:
	 *	1) The Request cannot be met.
	 *	2) Or the Request can be met later from the Power Reserve.
	 */
	else {
		pe_set_state(dev, PE_SRC_CAPABILITY_RESPONSE);
	}
}

/**
 * @brief 8.3.3.2.5 PE_SRC_Transition_Supply State
 */
void pe_src_transition_supply_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SRC_Transition_Supply");

	/*
	 * If snk_request_reply is set, this state was entered
	 * from PE_SRC_Negotiate_Capability. So send Accept Message
	 * and inform the Device Policy Manager that it Shall transition
	 * the power supply to the Requested power level.
	 */
	if (pe->snk_request_reply == SNK_REQUEST_VALID) {
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_ACCEPT);
		policy_notify(dev, TRANSITION_PS);
	}
	/*
	 * If snk_request_reply is not valid, this state was entered
	 * from PE_SRC_Ready. So send GotoMin Message.
	 */
	else {
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_GOTO_MIN);
	}
}

void pe_src_transition_supply_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * The Policy Engine Shall transition to the PE_SRC_Ready state when:
	 *	1) The Device Policy Manager informs the Policy Engine that
	 *	   the power supply is ready.
	 */
	if (atomic_test_bit(pe->flags, PE_FLAGS_TX_COMPLETE)) {
		if (policy_is_ps_ready(dev)) {
			pe_set_state(dev, PE_SRC_READY);
		}
	}
	/*
	 * The Policy Engine Shall transition to the PE_SRC_Hard_Reset
	 * state when:
	 *	1) A Protocol Error occurs.
	 */
	else if (atomic_test_bit(pe->flags, PE_FLAGS_PROTOCOL_ERROR)) {
		pe_set_state(dev, PE_SRC_HARD_RESET);
	}
}

void pe_src_transition_supply_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/* Send PS_RDY message */
	if (pe->snk_request_reply == SNK_REQUEST_VALID) {
		/* Clear request reply and reject by default */
		pe->snk_request_reply = SNK_REQUEST_REJECT;
		/* Send PS Ready */
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_PS_RDY);
		/* Explicit Contract is now in place */
		atomic_set_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT);
		/* Update present contract */
		pe->present_contract = pe->snk_request;
	}
}

/**
 * @brief 8.3.3.2.6 PE_SRC_Ready State
 */
void pe_src_ready_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SRC_Ready");

	/*
	 * If the transition into PE_SRC_Ready is the result of Protocol Error
	 * that has not caused a Soft Reset then the notification to the
	 * Protocol Layer of the end of the AMS Shall Not be sent since there
	 * is a Message to be processed.
	 *
	 * Else on entry to the PE_SRC_Ready state the Source Shall notify the
	 * Protocol Layer of the end of the Atomic Message Sequence (AMS).
	 */
	if (atomic_test_and_clear_bit(pe->flags,
				PE_FLAGS_PROTOCOL_ERROR_NO_SOFT_RESET)) {
		pe_dpm_end_ams(dev);
	}
}

void pe_src_ready_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	/* Handle incoming messages */
	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		union pd_header header = prl_rx->emsg.header;

		/*
		 * Extended Message Requests
		 */
		if (header.extended) {
			extended_message_not_supported(dev);
		}
		/*
		 * Data Message Requests
		 */
		else if (header.number_of_data_objects > 0) {
			switch (header.message_type) {
			case PD_DATA_REQUEST:
				pe_set_state(dev, PE_SRC_NEGOTIATE_CAPABILITY);
				break;
			default:
				pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
			}
		}
		/*
		 * Control Message Requests
		 */
		else {
			switch (header.message_type) {
			case PD_CTRL_GOOD_CRC:
				/* Do nothing */
				break;
			case PD_CTRL_NOT_SUPPORTED:
				/* Notify DPM */
				policy_notify(dev, MSG_NOT_SUPPORTED_RECEIVED);
				break;
			case PD_CTRL_PING:
				/* Do nothing */
				break;
			case PD_CTRL_GET_SOURCE_CAP:
				pe_set_state(dev, PE_SRC_SEND_CAPABILITIES);
				break;
			case PD_CTRL_DR_SWAP:
				pe_set_state(dev, PE_DRS_EVALUATE_SWAP);
				break;
			/*
			 * USB PD 3.0 6.8.1:
			 * Receiving an unexpected message shall be responded
			 * to with a soft reset message.
			 */
			case PD_CTRL_ACCEPT:
			case PD_CTRL_REJECT:
			case PD_CTRL_WAIT:
			case PD_CTRL_PS_RDY:
				pe_send_soft_reset(dev, prl_rx->emsg.type);
				break;
				/*
				 * Receiving an unknown or unsupported message
				 * shall be responded to with a not supported
				 * message.
				 */
			default:
				pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
				break;
			}
		}
	} else {
		/* Handle Source DPManager Requests */
		source_dpm_requests(dev);
	}
}

void pe_src_ready_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * If the Source is initiating an AMS, then notify the
	 * PRL that the first message in an AMS will follow.
	 */
	if (pe_dpm_initiated_ams(dev)) {
		prl_first_msg_notificaiton(dev);
	}
}

/**
 * @brief 8.3.3.2.11 PE_SRC_Transition_to_default State
 */
void pe_src_transition_to_default_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * On entry to the PE_SRC_Transition_to_default state the
	 * Policy Engine Shall:
	 *	1: indicate to the Device Policy Manager that the power
	 *	   supply Shall Hard Reset
	 *	2: request a reset of the local hardware
	 *	3: request the Device Policy Manager to set the Port
	 *	   Data Role to DFP and turn off VCONN.
	 *
	 * NOTE: 1, 2 and VCONN off are done by Device Policy Manager when
	 *	 it receives the HARD_RESET_RECEIVED notification.
	 */
	policy_notify(dev, HARD_RESET_RECEIVED);
	pe->data_role = TC_ROLE_DFP;
	policy_notify(dev, DATA_ROLE_IS_DFP);
}

void pe_src_transition_to_default_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * The Policy Engine Shall transition to the PE_SRC_Startup
	 * state when:
	 *	1: The Device Policy Manager indicates that the power
	 *	   supply has reached the default level.
	 */
	if (policy_check(dev, CHECK_SRC_PS_AT_DEFAULT_LEVEL)) {
		pe_set_state(dev, PE_SRC_STARTUP);
	}
}

void pe_src_transition_to_default_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * On exit from the PE_SRC_Transition_to_default state the
	 * Policy Engine Shall:
	 *	1: request the Device Policy Manager to turn on VCONN
	 *	2: inform the Protocol Layer that the Hard Reset is complete.
	 *
	 * NOTE: The Device Policy Manager turns on VCONN when it notifies the
	 *	 PE that the Power Supply is at the default level.
	 */
	prl_hard_reset_complete(dev);
}

/**
 * 8.3.3.2.8 PE_SRC_Capability_Response State
 */
void pe_src_capability_response_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * On entry to the PE_SRC_Capability_Response state the Policy Engine
	 * Shall request the Protocol Layer to send one of the following:
	 */

	/*
	 * 1: Reject Message – if the request cannot be met or the present
	 *    Contract is Invalid.
	 */
	if (pe->snk_request_reply == SNK_REQUEST_REJECT) {
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_REJECT);
	}
	/*
	 * 2: Wait Message – if the request could be met later from the Power
	 *    Reserve. A Wait Message Shall Not be sent if the present Contract
	 *    is Invalid.
	 */
	else {
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_WAIT);
	}
}

void pe_src_capability_response_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/* Wait until message has been sent */
	if (!atomic_test_and_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE)) {
		return;
	}

	/*
	 * The Policy Engine Shall transition to the PE_SRC_Ready state when:
	 *	1: There is an Explicit Contract AND
	 *	2: A Reject Message has been sent and the present Contract
	 *	   is still Valid OR
	 *	3: A Wait Message has been sent.
	 */
	if (atomic_test_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT) &&
			((pe->snk_request_reply == SNK_REQUEST_REJECT &&
			policy_present_contract_is_valid(dev, pe->present_contract)) ||
			(pe->snk_request_reply == SNK_REQUEST_WAIT))) {
		pe_set_state(dev, PE_SRC_READY);
	}
	/*
	 * The Policy Engine Shall transition to the PE_SRC_Hard_Reset state
	 * when:
	 *	1: There is an Explicit Contract and
	 *	2: The Reject Message has been sent and the present Contract
	 *	   is Invalid
	 */
	else if (atomic_test_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT) &&
			policy_present_contract_is_valid(dev, pe->present_contract) == false) {
		pe_set_state(dev, PE_SRC_HARD_RESET);
	}
	/*
	 * The Policy Engine Shall transition to the PE_SRC_Wait_New_Capabilities
	 * state when:
	 *	1: There is no Explicit Contract and
	 *	2: A Reject Message has been sent or
	 *	3: A Wait Message has been sent.
	 */
	else {
		/* 8.3.3.2.13 PE_SRC_Wait_New_Capabilities embedded here */

		/*
		 * In the PE_SRC_Wait_New_Capabilities State the Device Policy Manager
		 * Should either decide to send no further Source Capabilities or
		 * Should send a different set of Source Capabilities. Continuing
		 * to send the same set of Source Capabilities could result in a live
		 * lock situation.
		 */

		/* Notify DPM to send a different set of Source Capabilities */
		if (policy_change_src_caps(dev)) {
			/* DPM will send different set of Source Capabilities */
			pe_set_state(dev, PE_SRC_SEND_CAPABILITIES);
		} else {
			/*
			 * DPM can not send a different set of Source
			 * Capabilities, so disable port.
			 */
			pe_set_state(dev, PE_SUSPEND);
		}
	}
}

void pe_src_hard_reset_parent_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	pe->submachine = SM_HARD_RESET_START;
}

void pe_src_hard_reset_parent_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	switch (pe->submachine) {
	case SM_HARD_RESET_START:
		/*
		 * Initialize and run the NoResponseTimer.
		 * Note that the NoResponseTimer Shall continue to run
		 * in every state until it is stopped or times out.
		 */
		usbc_timer_start(&pe->pd_t_no_response);

		/* Initialize and run the PSHardResetTimer */
		usbc_timer_start(&pe->pd_t_ps_hard_reset);

		pe->submachine = SM_HARD_RESET_WAIT;
		break;
	case SM_HARD_RESET_WAIT:
		/*
		 * The Policy Engine Shall transition to the
		 * PE_SRC_Transition_to_default state when:
		 * The PSHardResetTimer times out.
		 */
		if (usbc_timer_expired(&pe->pd_t_ps_hard_reset)) {
			pe_set_state(dev, PE_SRC_TRANSITION_TO_DEFAULT);
		}
		break;
	}
}

void pe_src_hard_reset_parent_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Stop the Hard Reset Timer */
	usbc_timer_stop(&pe->pd_t_ps_hard_reset);
}

/**
 * @brief 8.3.3.2.9 PE_SRC_Hard_Reset State
 */
void pe_src_hard_reset_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * On entry to the PE_SRC_Hard_Reset state the
	 * Policy Engine Shall:
	 */

	/*
	 * Request the generation of Hard Reset Signaling by
	 * the PHY Layer
	 */
	prl_execute_hard_reset(dev);

}
