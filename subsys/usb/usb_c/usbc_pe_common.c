/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/smf.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(usbc_stack, CONFIG_USBC_STACK_LOG_LEVEL);

#include "usbc_stack.h"
#include "usbc_pe_common_internal.h"
#include "usbc_pe_snk_states_internal.h"
#include "usbc_pe_src_states_internal.h"

static const struct smf_state pe_states[PE_STATE_COUNT];

/**
 * @brief Handle common DPM requests
 *
 * @retval true if request was handled, else false
 */
bool common_dpm_requests(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	if (pe->dpm_request > REQUEST_TC_END) {
		atomic_set_bit(pe->flags, PE_FLAGS_DPM_INITIATED_AMS);

		if (pe->dpm_request == REQUEST_PE_DR_SWAP) {
			pe_set_state(dev, PE_DRS_SEND_SWAP);
			return true;
		} else if (pe->dpm_request == REQUEST_PE_SOFT_RESET_SEND) {
			pe_set_state(dev, PE_SEND_SOFT_RESET);
			return true;
		}
	}

	return false;
}

/**
 * @brief Initializes the PE state machine and enters the PE_SUSPEND state.
 */
void pe_subsys_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Save the port device object so states can access it */
	pe->dev = dev;

	/* Initialize the state machine */
	smf_set_initial(SMF_CTX(pe), &pe_states[PE_SUSPEND]);
}

/**
 * @brief Starts the Policy Engine layer
 */
void pe_start(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	data->pe_enabled = true;
}

/**
 * @brief Suspend the Policy Engine layer
 */
void pe_suspend(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	data->pe_enabled = false;

	/*
	 * While we are paused, exit all states
	 * and wait until initialized again.
	 */
	pe_set_state(dev, PE_SUSPEND);
}

/**
 * @brief Initialize the Policy Engine layer
 */
static void pe_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Clear all flags */
	atomic_clear(pe->flags);

	/* Initialize common timers */
	usbc_timer_init(&pe->pd_t_sender_response, PD_T_NO_RESPONSE_MAX_MS);
	usbc_timer_init(&pe->pd_t_chunking_not_supported, PD_T_CHUNKING_NOT_SUPPORTED_NOM_MS);

	/* Initialize common counters */
	pe->hard_reset_counter = 0;

#ifdef CONFIG_USBC_CSM_SINK_ONLY
	pe_snk_init(dev);
#else
	pe_src_init(dev);
#endif
}

/**
 * @brief Tests if the Policy Engine layer is running
 */
bool pe_is_running(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->pe_sm_state == SM_RUN;
}

/**
 * @brief Run the Policy Engine layer
 */
void pe_run(const struct device *dev, const int32_t dpm_request)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	switch (data->pe_sm_state) {
	case SM_PAUSED:
		if (data->pe_enabled == false) {
			break;
		}
	/* fall through */
	case SM_INIT:
		pe_init(dev);
		data->pe_sm_state = SM_RUN;
	/* fall through */
	case SM_RUN:
		if (data->pe_enabled == false) {
			data->pe_sm_state = SM_PAUSED;
			break;
		}

		if (prl_is_running(dev) == false) {
			break;
		}

		/* Get any DPM Requests */
		pe->dpm_request = dpm_request;

		/*
		 * 8.3.3.3.8 PE_SNK_Hard_Reset State
		 * The Policy Engine Shall transition to the PE_SNK_Hard_Reset
		 * state from any state when:
		 * - Hard Reset request from Device Policy Manager
		 */
		if (dpm_request == REQUEST_PE_HARD_RESET_SEND) {
			pe_set_state(dev, PE_SNK_HARD_RESET);
		}

		/* Run state machine */
		smf_run_state(SMF_CTX(pe));

		break;
	}
}

/**
 * @brief Sets Data Role
 */
void pe_set_data_role(const struct device *dev, enum tc_data_role dr)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Update data role */
	pe->data_role = dr;

	/* Notify TCPC of role update */
	tcpc_set_roles(data->tcpc, pe->power_role, pe->data_role);
}

/**
 * @brief Gets the current data role
 */
enum tc_data_role pe_get_data_role(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->pe->data_role;
}

/**
 * @brief Gets the current power role
 */
enum tc_power_role pe_get_power_role(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->pe->power_role;
}

/**
 * @brief Gets the current cable plug role
 */
enum tc_cable_plug pe_get_cable_plug(const struct device *dev)
{
	return PD_PLUG_FROM_DFP_UFP;
}

/**
 * @brief Informs the Policy Engine that a soft reset was received.
 */
void pe_got_soft_reset(const struct device *dev)
{
	/*
	 * The PE_SRC_Soft_Reset state Shall be entered from any state when a
	 * Soft_Reset Message is received from the Protocol Layer.
	 */
	pe_set_state(dev, PE_SOFT_RESET);
}

/**
 * @brief Informs the Policy Engine that a message was successfully sent
 */
void pe_message_sent(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	atomic_set_bit(pe->flags, PE_FLAGS_TX_COMPLETE);
}

/**
 * @brief See section 8.3.3.4.1.1 PE_SRC_Send_Soft_Reset State:
 *
 * The PE_Send_Soft_Reset state shall be entered from
 * any state when
 * * A Protocol Error is detected by Protocol Layer during a
 *   Non-Interruptible AMS or
 * * A message has not been sent after retries or
 * * When not in an explicit contract and
 *   * Protocol Errors occurred on SOP during an Interruptible AMS or
 *   * Protocol Errors occurred on SOP during any AMS where the first
 *     Message in the sequence has not yet been sent i.e. an unexpected
 *     Message is received instead of the expected GoodCRC Message
 *     response.
 */
static bool pe_soft_reset_is_required(const struct device *dev, const enum pd_packet_type type)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Protocol Error not on SOP */
	if (type != PD_PACKET_SOP) {
		return false;
	}

	if (atomic_test_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT)) {
		/*
		 * If the first Message in an AMS has been passed to the
		 * Protocol Layer by the Policy Engine but has not yet been sent
		 * when the Protocol Error occurs, the Policy Engine Shall Not
		 * issue a Soft Reset
		 */
		if (!atomic_test_bit(pe->flags, PE_FLAGS_FIRST_MSG_SENT)) {
			return false;
		}

		/*
		 * If the Protocol Error occurs during an Interruptible AMS then
		 * the Policy Engine Shall Not issue a Soft Reset
		 */
		if (atomic_test_bit(pe->flags, PE_FLAGS_INTERRUPTIBLE_AMS)) {
			return false;
		}
	}

	return true;
}

/**
 * @brief Informs the Policy Engine of an error.
 */
void pe_report_error(const struct device *dev, const enum pe_error e,
		     const enum pd_packet_type type)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/*
	 * Generate Hard Reset if Protocol Error occurred
	 * while in PE_Send_Soft_Reset state.
	 */
	if (pe_get_state(dev) == PE_SEND_SOFT_RESET ||
	    pe_get_state(dev) == PE_SOFT_RESET) {
		atomic_set_bit(pe->flags, PE_FLAGS_PROTOCOL_ERROR);
		return;
	}

	/* Transmit error */
	if (e == ERR_XMIT) {
		atomic_set_bit(pe->flags, PE_FLAGS_MSG_XMIT_ERROR);
	}
	/* All error types besides transmit errors are Protocol Errors. */
	else if (pe_soft_reset_is_required(dev, type)) {
		policy_notify(dev, PROTOCOL_ERROR);
		pe_send_soft_reset(dev, type);
	}
	/*
	 * Transition to PE_Snk_Ready by a Protocol
	 * Error during an Interruptible AMS.
	 */
	else {
		pe_set_state(dev, PE_SNK_READY);
	}
}

/**
 * @brief Informs the Policy Engine of a discard.
 */
void pe_report_discard(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/*
	 * Clear local AMS indicator as our AMS message was discarded, and flag
	 * the discard for the PE
	 */
	pe_dpm_end_ams(dev);
	atomic_set_bit(pe->flags, PE_FLAGS_MSG_DISCARDED);
}

/**
 * @brief Called by the Protocol Layer to informs the Policy Engine
 *	  that a message has been received.
 */
void pe_message_received(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	atomic_set_bit(pe->flags, PE_FLAGS_MSG_RECEIVED);
}

/**
 * @brief Informs the Policy Engine that a hard reset was received.
 */
void pe_got_hard_reset(const struct device *dev)
{
	pe_set_state(dev, PE_SNK_TRANSITION_TO_DEFAULT);
}

/**
 * @brief Informs the Policy Engine that a hard reset was sent.
 */
void pe_hard_reset_sent(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	atomic_clear_bit(pe->flags, PE_FLAGS_HARD_RESET_PENDING);
}

/**
 * @brief Indicates if an explicit contract is in place
 */
bool pe_is_explicit_contract(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	return atomic_test_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT);
}

/**
 * @brief Return true if the PE is is within an atomic messaging sequence
 *	  that it initiated with a SOP* port partner.
 */
bool pe_dpm_initiated_ams(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	return atomic_test_bit(pe->flags, PE_FLAGS_DPM_INITIATED_AMS);
}

/**
 * @brief End and atomic messaging sequence
 */
void pe_dpm_end_ams(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	atomic_clear_bit(pe->flags, PE_FLAGS_DPM_INITIATED_AMS);
}

/**
 * @brief First message in AMS has been sent
 */
void pe_first_msg_sent(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	atomic_set_bit(pe->flags, PE_FLAGS_FIRST_MSG_SENT);
}

/** Private Policy Engine Layer API below */

/**
 * @brief Sets a Policy Engine state
 */
void pe_set_state(const struct device *dev, const enum usbc_pe_state state)
{
	struct usbc_port_data *data = dev->data;

	__ASSERT(state < ARRAY_SIZE(pe_states), "invalid pe_state %d", state);
	smf_set_state(SMF_CTX(data->pe), &pe_states[state]);
}

/**
 * @brief Get the Policy Engine's current state
 */
enum usbc_pe_state pe_get_state(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->pe->ctx.current - &pe_states[0];
}

/**
 * @brief Get the Policy Engine's previous state
 */
enum usbc_pe_state pe_get_last_state(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->pe->ctx.previous - &pe_states[0];
}

/**
 * @brief Send a soft reset message
 */
void pe_send_soft_reset(const struct device *dev, const enum pd_packet_type type)
{
	struct usbc_port_data *data = dev->data;

	data->pe->soft_reset_sop = type;
	pe_set_state(dev, PE_SEND_SOFT_RESET);
}

/**
 * @brief Send a Power Delivery Data Message
 */
void pe_send_data_msg(const struct device *dev, const enum pd_packet_type type,
			const enum pd_data_msg_type msg)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Clear any previous TX status before sending a new message */
	atomic_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE);
	prl_send_data_msg(dev, type, msg);
}

/**
 * @brief Send a Power Delivery Control Message
 */
void pe_send_ctrl_msg(const struct device *dev, const enum pd_packet_type type,
			const enum pd_ctrl_msg_type msg)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Clear any previous TX status before sending a new message */
	atomic_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE);
	prl_send_ctrl_msg(dev, type, msg);
}

/**
 * @brief Request desired voltage from source.
 */
void pe_send_request_msg(const struct device *dev, const uint32_t rdo)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct pd_msg *msg = &prl_tx->emsg;
	uint8_t rdo_bytes[4];

	msg->len = sizeof(rdo);
	sys_put_le32(rdo, rdo_bytes);
	memcpy(msg->data, rdo_bytes, msg->len);
	pe_send_data_msg(dev, PD_PACKET_SOP, PD_DATA_REQUEST);
}

/**
 * @brief Transitions state after receiving an extended message.
 */
void extended_message_not_supported(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	uint32_t *payload = (uint32_t *)prl_rx->emsg.data;
	union pd_ext_header ext_header;

	ext_header.raw_value = *payload;

	if (ext_header.chunked && ext_header.data_size > PD_MAX_EXTENDED_MSG_CHUNK_LEN) {
		pe_set_state(dev, PE_CHUNK_RECEIVED);
	} else {
		pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
	}
}

/**
 * @brief Check if a specific control message was received
 */
bool received_control_message(const struct device *dev, const union pd_header header,
			      const enum pd_ctrl_msg_type mt)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	if (prl_rx->emsg.len == 0 && header.message_type == mt && header.extended == 0) {
		return true;
	}

	return false;
}

/**
 * @brief Check if a specific data message was received
 */
bool received_data_message(const struct device *dev, const union pd_header header,
			   const enum pd_data_msg_type mt)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	if (prl_rx->emsg.len > 0 && header.message_type == mt && header.extended == 0) {
		return true;
	}

	return false;
}

/**
 * @brief Check a DPM policy
 */
bool policy_check(const struct device *dev, const enum usbc_policy_check_t pc)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_check) {
		return data->policy_cb_check(dev, pc);
	} else {
		return false;
	}
}

/**
 * @brief Notify the DPM of a policy change
 */
void policy_notify(const struct device *dev, const enum usbc_policy_notify_t notify)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_notify) {
		data->policy_cb_notify(dev, notify);
	}
}

/**
 * @brief Notify the DPM of a WAIT message reception
 */
bool policy_wait_notify(const struct device *dev, const enum usbc_policy_wait_t notify)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_wait_notify) {
		return data->policy_cb_wait_notify(dev, notify);
	}

	return false;
}

#ifdef CONFIG_USBC_CSM_SINK_ONLY

/**
 * @brief Get a Request Data Object from the DPM
 */
uint32_t policy_get_request_data_object(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	/* This callback must be implemented */
	__ASSERT(data->policy_cb_get_rdo != NULL, "Callback pointer should not be NULL");

	return data->policy_cb_get_rdo(dev);
}

/**
 * @brief Send the received source caps to the DPM
 */
void policy_set_src_cap(const struct device *dev, const uint32_t *pdos, const int num_pdos)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_set_src_cap) {
		data->policy_cb_set_src_cap(dev, pdos, num_pdos);
	}
}

/**
 * @brief Check if the sink is a default level
 */
bool policy_is_snk_at_default(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_is_snk_at_default) {
		return data->policy_cb_is_snk_at_default(dev);
	}

	return true;
}

/**
 * @brief Get sink caps from the DPM
 */
void policy_get_snk_cap(const struct device *dev, uint32_t **pdos, int *num_pdos)
{
	struct usbc_port_data *data = dev->data;

	/* This callback must be implemented */
	__ASSERT(data->policy_cb_get_snk_cap != NULL, "Callback pointer should not be NULL");

	data->policy_cb_get_snk_cap(dev, pdos, num_pdos);
}

#else /* CONFIG_USBC_CSM_SOURCE_ONLY */

/**
 * @brief Send the received sink caps to the DPM
 */
void policy_set_port_partner_snk_cap(const struct device *dev,
			const uint32_t *pdos,
			const int num_pdos)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_set_port_partner_snk_cap) {
		data->policy_cb_set_port_partner_snk_cap(dev, pdos, num_pdos);
	}
}

/**
 * @brief Check if Sink Request can be met by DPM
 */
enum usbc_snk_req_reply_t policy_check_sink_request(const struct device *dev,
				const uint32_t request_msg)
{
	struct usbc_port_data *data = dev->data;

	/* This callback must be implemented */
	__ASSERT(data->policy_cb_check_sink_request != NULL,
		"Callback pointer should not be NULL");

	return data->policy_cb_check_sink_request(dev, request_msg);
}

/**
 * @brief Check if the present contract is still valid
 */
bool policy_present_contract_is_valid(const struct device *dev,
				const uint32_t present_contract)
{
	struct usbc_port_data *data = dev->data;

	/* This callback must be implemented */
	__ASSERT(data->policy_present_contract_is_valid != NULL,
		"Callback pointer should not be NULL");

	return data->policy_present_contract_is_valid(dev, present_contract);
}

/**
 * @brief Check if the power supply is ready
 */
bool policy_is_ps_ready(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	/* This callback must be implemented */
	__ASSERT(data->policy_is_ps_ready != NULL,
		"Callback pointer should not be NULL");

	return data->policy_is_ps_ready(dev);
}

/**
 * @brief Ask the DPM to change the Source Caps.
 *	  Returns true if source caps have been updated, else false
 */
bool policy_change_src_caps(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_change_src_caps == NULL) {
		return false;
	}

	return data->policy_change_src_caps(dev);
}

#endif /* CONFIG_USBC_CSM_SINK_ONLY */

/**
 * @brief PE_DRS_Evaluate_Swap Entry state
 */
static void pe_drs_evaluate_swap_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/* Get evaluation of Data Role Swap request from Device Policy Manager */
	if (policy_check(dev, (pe->data_role == TC_ROLE_UFP) ? CHECK_DATA_ROLE_SWAP_TO_DFP
							     : CHECK_DATA_ROLE_SWAP_TO_UFP)) {
		/*
		 * PE_DRS_DFP_UFP_Accept_Swap and PE_DRS_UFP_DFP_Accept_Swap
		 * State embedded here
		 */
		/* Send Accept message */
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_ACCEPT);
	} else {
		/*
		 * PE_DRS_DFP_UFP_Reject_Swap and PE_DRS_UFP_DFP_Reject_Swap
		 * State embedded here
		 */
		/* Send Reject message */
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_REJECT);
	}
}

/**
 * @brief PE_DRS_Evaluate_Swap Run state
 */
static void pe_drs_evaluate_swap_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE)) {
		/* Only update data roles if last message sent was Accept */
		if (prl_tx->msg_type == PD_CTRL_ACCEPT) {
			/* Update Data Role */
			pe_set_data_role(dev, (pe->data_role == TC_ROLE_UFP)
						? TC_ROLE_DFP : TC_ROLE_UFP);
			/* Inform Device Policy Manager of Data Role Change */
			policy_notify(dev, (pe->data_role == TC_ROLE_UFP) ? DATA_ROLE_IS_UFP
									  : DATA_ROLE_IS_DFP);
		}
		pe_set_state(dev, PE_SNK_READY);
	} else if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		/*
		 * Inform Device Policy Manager that the message was
		 * discarded
		 */
		policy_notify(dev, MSG_DISCARDED);
		pe_send_soft_reset(dev, prl_rx->emsg.type);
	}
}

/**
 * @brief PE_DRS_Send_Swap Entry state
 *	  NOTE: 8.3.3.18.1.5 PE_DRS_DFP_UFP_Send_Swap State
 *		8.3.3.18.2.5 PE_DRS_UFP_DFP_Send_Swap State
 */
static void pe_drs_send_swap_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/* Send Swap DR message */
	pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_DR_SWAP);
}

/**
 * @brief PE_DRS_Send_Swap Run state
 *	  NOTE: Sender Response Timer is handled in super state.
 */
static void pe_drs_send_swap_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		header = prl_rx->emsg.header;
		if (received_control_message(dev, header, PD_CTRL_REJECT)) {
			/*
			 * Inform Device Policy Manager that Data Role Swap
			 * was Rejected
			 */
			policy_notify(dev, MSG_REJECTED_RECEIVED);
		} else if (received_control_message(dev, header, PD_CTRL_WAIT)) {
			/*
			 * Inform Device Policy Manager that Data Role Swap
			 * needs to Wait
			 */
			if (policy_wait_notify(dev, WAIT_DATA_ROLE_SWAP)) {
				atomic_set_bit(pe->flags, PE_FLAGS_WAIT_DATA_ROLE_SWAP);
				usbc_timer_start(&pe->pd_t_wait_to_resend);
			}
		} else if (received_control_message(dev, header, PD_CTRL_ACCEPT)) {
			/* Update Data Role */
			pe->data_role = (pe->data_role == TC_ROLE_UFP) ? TC_ROLE_DFP : TC_ROLE_UFP;
			/* Notify TCPC of role update */
			tcpc_set_roles(data->tcpc, pe->power_role, pe->data_role);
			/* Inform Device Policy Manager of Data Role Change */
			policy_notify(dev, (pe->data_role == TC_ROLE_UFP) ? DATA_ROLE_IS_UFP
									  : DATA_ROLE_IS_DFP);
		} else {
			/*
			 * A Protocol Error during a Data Role Swap when the
			 * DFP/UFP roles are changing shall directly trigger
			 * a Type-C Error Recovery.
			 */
			usbc_request(dev, REQUEST_TC_ERROR_RECOVERY);
			return;
		}

		/* return to ready state */
		pe_set_state(dev, PE_SNK_READY);
		return;
	} else if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		/*
		 * Inform Device Policy Manager that the message
		 * was discarded
		 */
		policy_notify(dev, MSG_DISCARDED);
		pe_set_state(dev, PE_SNK_READY);
		return;
	}
}

/**
 * PE_Get_Sink_Cap Entry state
 * @brief 8.3.3.18.7.1 PE_DR_SRC_Get_Source_Cap State
 * @brief 8.3.3.18.9.1 PE_DR_SNK_Get_Sink_Cap State
 */
void pe_get_sink_cap_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * On entry to the PE_DR_SNK_Get_Sink_Cap state the Policy Engine
	 * Shall send a Get_Sink_Cap Message and initialize and run the
	 * SenderResponseTimer.
	 */
	pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_GET_SINK_CAP);
	/* Initialize Submachine */
	pe->submachine = SM_WAIT_FOR_TX;
}

/**
 * @brief PE_Get_Sink_Cap Run state
 *        NOTE: Sender Response Timer is handled in super state.
 */
void pe_get_sink_cap_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	switch (pe->submachine) {
	case SM_WAIT_FOR_TX:
		if (!atomic_test_and_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE)) {
			break;
		}
		pe->submachine = SM_WAIT_FOR_RX;
		/* fall through */
	case SM_WAIT_FOR_RX:
		if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
			header = prl_rx->emsg.header;

			if (prl_rx->emsg.type == PD_PACKET_SOP) {
				if (received_data_message(dev, header, PD_DATA_SINK_CAP)) {
#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
					uint32_t *pdos = (uint32_t *)prl_rx->emsg.data;
					uint32_t num_pdos =
					PD_CONVERT_BYTES_TO_PD_HEADER_COUNT(prl_rx->emsg.len);

					policy_set_port_partner_snk_cap(dev, pdos, num_pdos);
					pe_set_state(dev, PE_SRC_READY);
#else
					pe_set_state(dev, PE_SNK_READY);
#endif
					return;
				} else if (received_control_message(dev, header, PD_CTRL_REJECT) ||
					received_control_message(dev,
							header, PD_CTRL_NOT_SUPPORTED)) {
#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
					pe_set_state(dev, PE_SRC_READY);
#else
					pe_set_state(dev, PE_SNK_READY);
#endif
					return;
				}
				/* Unexpected messages fall through to soft reset */
			}
			pe_send_soft_reset(dev, PD_PACKET_SOP);
			return;
		}
		/*
		 * Inform Device Policy Manager that the message
		 * was discarded
		 */
		else if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_DISCARDED)) {
			policy_notify(dev, MSG_DISCARDED);
			pe_set_state(dev, PE_SNK_READY);
			return;
		}
	}
}

static void pe_suspend_entry(void *obj)
{
	LOG_INF("PE_SUSPEND");
}

static void pe_suspend_run(void *obj)
{
	/* DO NOTHING */
}

/**
 * @brief The PE_SOFT_RESET state has two embedded states
 *	  that handle sending an accept message.
 */
enum pe_soft_reset_submachine_states {
	/* Send Accept message sub state */
	PE_SOFT_RESET_RUN_SEND_ACCEPT_MSG,
	/* Wait for Accept message to be sent or an error sub state */
	PE_SOFT_RESET_RUN_SEND_ACCEPT_MSG_COMPLETE
};

/**
 * @brief 8.3.3.4.2.2 PE_SNK_Soft_Reset State
 */
static void pe_soft_reset_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/* Reset the protocol layer */
	prl_reset(dev);

	/* Initialize PE Submachine */
	pe->submachine = PE_SOFT_RESET_RUN_SEND_ACCEPT_MSG;
}

static void pe_soft_reset_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	if (!prl_is_running(dev)) {
		return;
	}

	switch (pe->submachine) {
	case PE_SOFT_RESET_RUN_SEND_ACCEPT_MSG:
		/* Send Accept message to SOP */
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_ACCEPT);
		/* Move to next substate */
		pe->submachine = PE_SOFT_RESET_RUN_SEND_ACCEPT_MSG_COMPLETE;
		break;
	case PE_SOFT_RESET_RUN_SEND_ACCEPT_MSG_COMPLETE:
		/*
		 * The Policy Engine Shall transition to the
		 * PE_SRC_Send_Capabilities state when:
		 *      1: Accept message sent to SOP
		 */
		if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE)) {
			pe_set_state(dev, PE_SNK_WAIT_FOR_CAPABILITIES);
		} else if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_PROTOCOL_ERROR)) {
			/*
			 * The Policy Engine Shall transition to the
			 * PE_SRC_Hard_Reset state when:
			 *      1: Protocol Layer indicates that a
			 *         transmission error has occurred.
			 */
			pe_set_state(dev, PE_SNK_HARD_RESET);
		}
		break;
	}
}

/**
 * @brief PE_Send_Soft_Reset Entry State
 *	  NOTE: Sender Response Timer is handled in super state.
 */
static void pe_send_soft_reset_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Send_Soft_Reset");

	/* Reset Protocol Layer */
	prl_reset(dev);
	atomic_set_bit(pe->flags, PE_FLAGS_SEND_SOFT_RESET);
}

/**
 * @brief PE_Send_Soft_Reset Run State
 */
static void pe_send_soft_reset_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	if (prl_is_running(dev) == false) {
		return;
	}

	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_SEND_SOFT_RESET)) {
		/* Send Soft Reset message */
		pe_send_ctrl_msg(dev, pe->soft_reset_sop, PD_CTRL_SOFT_RESET);
		return;
	}

	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		/* Inform Device Policy Manager that the message was discarded */
		policy_notify(dev, MSG_DISCARDED);
		pe_set_state(dev, PE_SNK_READY);
	} else if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		/*
		 * The Policy Engine Shall transition to the PE_SNK_Wait_for_Capabilities
		 * state when:
		 *      1: An Accept Message has been received on SOP
		 */
		header = prl_rx->emsg.header;

		if (received_control_message(dev, header, PD_CTRL_ACCEPT)) {
			pe_set_state(dev, PE_SNK_WAIT_FOR_CAPABILITIES);
		}
	} else if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_PROTOCOL_ERROR)) {
		/*
		 * The Policy Engine Shall transition to the PE_SNK_Hard_Reset state when:
		 *      1: A SenderResponseTimer timeout occurs (Handled in Super State)
		 *      2: Or the Protocol Layer indicates that a transmission error has occurred
		 */
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
}

/**
 * @brief 8.3.3.6.2.1 PE_SNK_Send_Not_Supported State
 */
static void pe_send_not_supported_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_Not_Supported");

	/* Notify the Device Policy Manager of unsupported message reception */
	policy_notify(dev, MSG_NOT_SUPPORTED_RECEIVED);

	/* Request the Protocol Layer to send a Not_Supported or Reject Message. */
	if (prl_get_rev(dev, PD_PACKET_SOP) > PD_REV20) {
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_NOT_SUPPORTED);
	} else {
		pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_REJECT);
	}
}

static void pe_send_not_supported_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	if (atomic_test_bit(pe->flags, PE_FLAGS_TX_COMPLETE) ||
			atomic_test_bit(pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		atomic_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE);
		atomic_clear_bit(pe->flags, PE_FLAGS_MSG_DISCARDED);
		pe_set_state(dev, PE_SNK_READY);
	}
}

/**
 * @brief 8.3.3.6.2.3 PE_SNK_Chunk_Received State
 */
static void pe_chunk_received_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SNK_Chunk_Received");

	/*
	 * On entry to the PE_SNK_Chunk_Received state, the Policy Engine
	 * Shall initialize and run the ChunkingNotSupportedTimer.
	 */
	usbc_timer_start(&pe->pd_t_chunking_not_supported);
}

/**
 * @brief PE_Chunk_Received Run State
 */
static void pe_chunk_received_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * The Policy Engine Shall transition to PE_SNK_Send_Not_Supported
	 * when:
	 *	1: The ChunkingNotSupportedTimer has timed out.
	 */
	if (usbc_timer_expired(&pe->pd_t_chunking_not_supported)) {
		pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
	}
}

/*
 * @brief Super State for any message that requires
 *	  Sender Response Timer functionality
 */
static void pe_sender_response_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	enum usbc_pe_state current_state = pe_get_state(dev);

	/* Start the Sender Response Timer after the message is sent */
	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE)) {
		/* Start Sender Response Timer */
		usbc_timer_start(&pe->pd_t_sender_response);
	}

	/* Check if the Sender Response Timer has expired */
	if (usbc_timer_expired(&pe->pd_t_sender_response)) {
		/*
		 * Handle Sender Response Timeouts
		 */
		switch (current_state) {
#if CONFIG_USBC_CSM_SINK_ONLY
		/* Sink states */
		case PE_SNK_SELECT_CAPABILITY:
			pe_set_state(dev, PE_SNK_HARD_RESET);
			break;
		case PE_SNK_GET_SOURCE_CAP:
			pe_set_state(dev, PE_SNK_READY);
			break;
#else
		/* Source states */
		case PE_SRC_DISCOVERY:
			/*
			 * The Policy Engine Shall go to the PE_SRC_Disabled state when:
			 *      1) The Port Partners have not been PD Connected
			 *      2) And the NoResponseTimer times out
			 *      3) And the HardResetCounter > nHardResetCount.
			 */
			if ((atomic_test_bit(pe->flags, PE_FLAGS_HAS_BEEN_PD_CONNECTED) == false)
					&& pe->hard_reset_counter > PD_N_HARD_RESET_COUNT) {
				pe_set_state(dev, PE_SUSPEND);
			}
			break;
		case PE_SRC_SEND_CAPABILITIES:
			/*
			 * The Policy Engine Shall go to the ErrorRecovery state when:
			 *      1) The Port Partners have previously been PD Connected
			 *      2) And the NoResponseTimer times out
			 *      3) And the HardResetCounter > nHardResetCount
			 */
			if (atomic_test_bit(pe->flags, PE_FLAGS_HAS_BEEN_PD_CONNECTED)
					&& pe->hard_reset_counter > PD_N_HARD_RESET_COUNT) {
				usbc_request(dev, REQUEST_TC_ERROR_RECOVERY);
			}
			/*
			 * The Policy Engine Shall transition to the PE_SRC_Hard_Reset
			 * state when:
			 *      1) The SenderResponseTimer times out
			 */
			else {
				pe_set_state(dev, PE_SRC_HARD_RESET);
			}
			break;
		case PE_GET_SINK_CAP:
			pe_send_soft_reset(dev, PD_PACKET_SOP);
			break;
#endif
		/*
		 * Common states:
		 * Could transition to a Sink or Source states,
		 * depending on the current Data Role
		 */
		case PE_SEND_SOFT_RESET:
			pe_set_state(dev, PE_SNK_HARD_RESET);
			break;
		case PE_DRS_SEND_SWAP:
			pe_set_state(dev, PE_SNK_READY);
			break;

		/* This should not happen. Implementation error */
		default:
			LOG_INF("Unhandled Sender Response Timeout State!");
		}
	}
}

static void pe_sender_response_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Stop Sender Response Timer */
	usbc_timer_stop(&pe->pd_t_sender_response);
}

/**
 * @brief Policy engine State table
 */
static const struct smf_state pe_states[PE_STATE_COUNT] = {
	/* PE Super States */
	[PE_SENDER_RESPONSE_PARENT] = SMF_CREATE_STATE(
		NULL,
		pe_sender_response_run,
		pe_sender_response_exit,
		NULL),
#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
	[PE_SRC_HARD_RESET_PARENT] = SMF_CREATE_STATE(
		pe_src_hard_reset_parent_entry,
		pe_src_hard_reset_parent_run,
		pe_src_hard_reset_parent_exit,
		NULL),
#endif
#ifdef CONFIG_USBC_CSM_SINK_ONLY
	[PE_SNK_STARTUP] = SMF_CREATE_STATE(
		pe_snk_startup_entry,
		pe_snk_startup_run,
		NULL,
		NULL),
	[PE_SNK_DISCOVERY] = SMF_CREATE_STATE(
		pe_snk_discovery_entry,
		pe_snk_discovery_run,
		NULL,
		NULL),
	[PE_SNK_WAIT_FOR_CAPABILITIES] = SMF_CREATE_STATE(
		pe_snk_wait_for_capabilities_entry,
		pe_snk_wait_for_capabilities_run,
		pe_snk_wait_for_capabilities_exit,
		NULL),
	[PE_SNK_EVALUATE_CAPABILITY] = SMF_CREATE_STATE(
		pe_snk_evaluate_capability_entry,
		NULL,
		NULL,
		NULL),
	[PE_SNK_SELECT_CAPABILITY] = SMF_CREATE_STATE(
		pe_snk_select_capability_entry,
		pe_snk_select_capability_run,
		NULL,
		&pe_states[PE_SENDER_RESPONSE_PARENT]),
	[PE_SNK_READY] = SMF_CREATE_STATE(
		pe_snk_ready_entry,
		pe_snk_ready_run,
		pe_snk_ready_exit,
		NULL),
	[PE_SNK_HARD_RESET] = SMF_CREATE_STATE(
		pe_snk_hard_reset_entry,
		pe_snk_hard_reset_run,
		NULL,
		NULL),
	[PE_SNK_TRANSITION_TO_DEFAULT] = SMF_CREATE_STATE(
		pe_snk_transition_to_default_entry,
		pe_snk_transition_to_default_run,
		NULL,
		NULL),
	[PE_SNK_GIVE_SINK_CAP] = SMF_CREATE_STATE(
		pe_snk_give_sink_cap_entry,
		pe_snk_give_sink_cap_run,
		NULL,
		NULL),
	[PE_SNK_GET_SOURCE_CAP] = SMF_CREATE_STATE(
		pe_snk_get_source_cap_entry,
		pe_snk_get_source_cap_run,
		NULL,
		&pe_states[PE_SENDER_RESPONSE_PARENT]),
	[PE_SNK_TRANSITION_SINK] = SMF_CREATE_STATE(
		pe_snk_transition_sink_entry,
		pe_snk_transition_sink_run,
		pe_snk_transition_sink_exit,
		NULL),
#else
	[PE_SRC_STARTUP] = SMF_CREATE_STATE(
		pe_src_startup_entry,
		pe_src_startup_run,
		NULL,
		NULL),
	[PE_SRC_DISCOVERY] = SMF_CREATE_STATE(
		pe_src_discovery_entry,
		pe_src_discovery_run,
		pe_src_discovery_exit,
		&pe_states[PE_SENDER_RESPONSE_PARENT]),
	[PE_SRC_SEND_CAPABILITIES] = SMF_CREATE_STATE(
		pe_src_send_capabilities_entry,
		pe_src_send_capabilities_run,
		NULL,
		&pe_states[PE_SENDER_RESPONSE_PARENT]),
	[PE_SRC_NEGOTIATE_CAPABILITY] = SMF_CREATE_STATE(
		pe_src_negotiate_capability_entry,
		NULL,
		NULL,
		NULL),
	[PE_SRC_CAPABILITY_RESPONSE] = SMF_CREATE_STATE(
		pe_src_capability_response_entry,
		pe_src_capability_response_run,
		NULL,
		NULL),
	[PE_SRC_TRANSITION_SUPPLY] = SMF_CREATE_STATE(
		pe_src_transition_supply_entry,
		pe_src_transition_supply_run,
		pe_src_transition_supply_exit,
		NULL),
	[PE_SRC_READY] = SMF_CREATE_STATE(
		pe_src_ready_entry,
		pe_src_ready_run,
		pe_src_ready_exit,
		NULL),
	[PE_SRC_TRANSITION_TO_DEFAULT] = SMF_CREATE_STATE(
		pe_src_transition_to_default_entry,
		pe_src_transition_to_default_run,
		pe_src_transition_to_default_exit,
		NULL),
	[PE_SRC_HARD_RESET_RECEIVED] = SMF_CREATE_STATE(
		NULL,
		NULL,
		NULL,
		&pe_states[PE_SRC_HARD_RESET_PARENT]),
	[PE_SRC_HARD_RESET] = SMF_CREATE_STATE(
		pe_src_hard_reset_entry,
		NULL,
		NULL,
		&pe_states[PE_SRC_HARD_RESET_PARENT]),
#endif
	[PE_GET_SINK_CAP] = SMF_CREATE_STATE(
		pe_get_sink_cap_entry,
		pe_get_sink_cap_run,
		NULL,
		&pe_states[PE_SENDER_RESPONSE_PARENT]),
	[PE_SEND_SOFT_RESET] = SMF_CREATE_STATE(
		pe_send_soft_reset_entry,
		pe_send_soft_reset_run,
		NULL,
		&pe_states[PE_SENDER_RESPONSE_PARENT]),
	[PE_SOFT_RESET] = SMF_CREATE_STATE(
		pe_soft_reset_entry,
		pe_soft_reset_run,
		NULL,
		NULL),
	[PE_SEND_NOT_SUPPORTED] = SMF_CREATE_STATE(
		pe_send_not_supported_entry,
		pe_send_not_supported_run,
		NULL,
		NULL),
	[PE_DRS_EVALUATE_SWAP] = SMF_CREATE_STATE(
		pe_drs_evaluate_swap_entry,
		pe_drs_evaluate_swap_run,
		NULL,
		NULL),
	[PE_DRS_SEND_SWAP] = SMF_CREATE_STATE(
		pe_drs_send_swap_entry,
		pe_drs_send_swap_run,
		NULL,
		&pe_states[PE_SENDER_RESPONSE_PARENT]),
	[PE_CHUNK_RECEIVED] = SMF_CREATE_STATE(
		pe_chunk_received_entry,
		pe_chunk_received_run,
		NULL,
		NULL),
	[PE_SUSPEND] = SMF_CREATE_STATE(
		pe_suspend_entry,
		pe_suspend_run,
		NULL,
		NULL),
};
BUILD_ASSERT(ARRAY_SIZE(pe_states) == PE_STATE_COUNT);
