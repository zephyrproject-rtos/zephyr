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

/**
 * @brief The HardResetCounter is used to retry the Hard Reset whenever there
 *	  is no response from the remote device (see Section 6.6.6)
 */
#define N_HARD_RESET_COUNT 2

/**
 * @brief Policy Engine Layer Flags
 */
enum pe_flags {
	/** Accept message received from port partner */
	PE_FLAGS_ACCEPT                 = 0,
	/**
	 * Protocol Error was determined based on error recovery
	 * current state
	 */
	PE_FLAGS_PROTOCOL_ERROR         = 1,
	/** A message we requested to be sent has been transmitted */
	PE_FLAGS_TX_COMPLETE            = 2,
	/** A message sent by a port partner has been received */
	PE_FLAGS_MSG_RECEIVED           = 3,
	/**
	 * A hard reset has been requested by the DPM but has not been sent,
	 * not currently used
	 */
	PE_FLAGS_HARD_RESET_PENDING     = 4,
	/** An explicit contract is in place with our port partner */
	PE_FLAGS_EXPLICIT_CONTRACT      = 5,
	/**
	 * Waiting for Sink Capabailities timed out.  Used for retry error
	 * handling
	 */
	PE_FLAGS_SNK_WAIT_CAP_TIMEOUT   = 6,
	/**
	 * Flag to note current Atomic Message Sequence (AMS) is interruptible.
	 * If this flag is not set the AMS is non-interruptible. This flag must
	 * be set in the interruptible's message state entry.
	 */
	PE_FLAGS_INTERRUPTIBLE_AMS      = 7,
	/** Flag to trigger sending a Data Role Swap */
	PE_FLAGS_DR_SWAP_TO_DFP         = 8,
	/** Flag is set when an AMS is initiated by the Device Policy Manager */
	PE_FLAGS_DPM_INITIATED_AMS      = 9,
	/** Flag to note message was discarded due to incoming message */
	PE_FLAGS_MSG_DISCARDED          = 10,
	/** Flag to trigger sending a soft reset */
	PE_FLAGS_SEND_SOFT_RESET        = 11,
	/**
	 * This flag is set when a Wait message is received in response to a
	 * Sink REQUEST
	 */
	PE_FLAGS_WAIT_SINK_REQUEST      = 12,
	/**
	 * This flag is set when a Wait message is received in response to a
	 * Data Role Swap
	 */
	PE_FLAGS_WAIT_DATA_ROLE_SWAP    = 13
};

/**
 * @brief Policy Engine Layer States
 */
enum usbc_pe_state {
	/** PE_SNK_Startup */
	PE_SNK_STARTUP,
	/** PE_SNK_Discovery */
	PE_SNK_DISCOVERY,
	/** PE_SNK_Wait_for_Capabilities */
	PE_SNK_WAIT_FOR_CAPABILITIES,
	/** PE_SNK_Evaluate_Capability */
	PE_SNK_EVALUATE_CAPABILITY,
	/** PE_SNK_Select_Capability */
	PE_SNK_SELECT_CAPABILITY,
	/** PE_SNK_Transition_Sink */
	PE_SNK_TRANSITION_SINK,
	/** PE_SNK_Ready */
	PE_SNK_READY,
	/** PE_SNK_Hard_Reset */
	PE_SNK_HARD_RESET,
	/** PE_SNK_Transition_to_default */
	PE_SNK_TRANSITION_TO_DEFAULT,
	/** PE_SNK_Give_Sink_Cap */
	PE_SNK_GIVE_SINK_CAP,
	/** PE_SNK_Get_Source_Cap */
	PE_SNK_GET_SOURCE_CAP,
	/**PE_Send_Soft_Reset */
	PE_SEND_SOFT_RESET,
	/** PE_Soft_Reset */
	PE_SOFT_RESET,
	/** PE_Send_Not_Supported */
	PE_SEND_NOT_SUPPORTED,
	/** PE_DRS_Evaluate_Swap */
	PE_DRS_EVALUATE_SWAP,
	/** PE_DRS_Send_Swap */
	PE_DRS_SEND_SWAP,
	/** PE_SNK_Chunk_Received */
	PE_SNK_CHUNK_RECEIVED,

	/** PE_Suspend. Not part of the PD specification. */
	PE_SUSPEND,
};

static const struct smf_state pe_states[];
static void pe_set_state(const struct device *dev,
			 const enum usbc_pe_state state);
static enum usbc_pe_state pe_get_state(const struct device *dev);
static enum usbc_pe_state pe_get_last_state(const struct device *dev);
static void pe_send_soft_reset(const struct device *dev,
			       const enum pd_packet_type type);
static void policy_notify(const struct device *dev,
			  const enum usbc_policy_notify_t notify);

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
void pe_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	pe->flags = ATOMIC_INIT(0);

	usbc_timer_init(&pe->pd_t_typec_sink_wait_cap, PD_T_TYPEC_SINK_WAIT_CAP_MAX_MS);
	usbc_timer_init(&pe->pd_t_sender_response, PD_T_SENDER_RESPONSE_NOM_MS);
	usbc_timer_init(&pe->pd_t_ps_transition, PD_T_SPR_PS_TRANSITION_NOM_MS);
	usbc_timer_init(&pe->pd_t_chunking_not_supported, PD_T_CHUNKING_NOT_SUPPORTED_NOM_MS);
	usbc_timer_init(&pe->pd_t_wait_to_resend, PD_T_SINK_REQUEST_MIN_MS);

	pe->data_role = TC_ROLE_UFP;
	pe->hard_reset_counter = 0;

	pe_set_state(dev, PE_SNK_STARTUP);
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
void pe_run(const struct device *dev,
	    const int32_t dpm_request)
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

		/*
		 * 8.3.3.3.8 PE_SNK_Hard_Reset State
		 * The Policy Engine Shall transition to the PE_SNK_Hard_Reset
		 * state from any state when:
		 * - Hard Reset request from Device Policy Manager
		 */
		if (dpm_request == REQUEST_PE_HARD_RESET_SEND) {
			pe_set_state(dev, PE_SNK_HARD_RESET);
		} else {
			/* Pass the DPM request along to the state machine */
			pe->dpm_request = dpm_request;
		}

		/* Run state machine */
		smf_run_state(SMF_CTX(pe));
		break;
	}
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

	atomic_set_bit(&pe->flags, PE_FLAGS_TX_COMPLETE);
}

/**
 * @brief Informs the Policy Engine of an error.
 */
void pe_report_error(const struct device *dev,
		     const enum pe_error e,
		     const enum pd_packet_type type)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/*
	 * Generate Hard Reset if Protocol Error occurred
	 * while in PE_Send_Soft_Reset state.
	 */
	if (pe_get_state(dev) == PE_SEND_SOFT_RESET) {
		pe_set_state(dev, PE_SNK_HARD_RESET);
		return;
	}

	/*
	 * See section 8.3.3.4.1.1 PE_SRC_Send_Soft_Reset State:
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
	/* All error types besides transmit errors are Protocol Errors. */
	if ((e != ERR_XMIT &&
	     atomic_test_bit(&pe->flags, PE_FLAGS_INTERRUPTIBLE_AMS) == false) ||
	     e == ERR_XMIT ||
	     (atomic_test_bit(&pe->flags, PE_FLAGS_EXPLICIT_CONTRACT) == false &&
	     type == PD_PACKET_SOP)) {
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
	atomic_clear_bit(&pe->flags, PE_FLAGS_DPM_INITIATED_AMS);
	atomic_set_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED);
}

/**
 * @brief Called by the Protocol Layer to informs the Policy Engine
 *	  that a message has been received.
 */
void pe_message_received(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	atomic_set_bit(&pe->flags, PE_FLAGS_MSG_RECEIVED);
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

	atomic_clear_bit(&pe->flags, PE_FLAGS_HARD_RESET_PENDING);
}

/**
 * @brief Indicates if an explicit contract is in place
 */
bool pe_is_explicit_contract(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	return atomic_test_bit(&pe->flags, PE_FLAGS_EXPLICIT_CONTRACT);
}

/**
 * @brief Return true if the PE is is within an atomic messaging sequence
 *	  that it initiated with a SOP* port partner.
 */
bool pe_dpm_initiated_ams(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	return atomic_test_bit(&pe->flags, PE_FLAGS_DPM_INITIATED_AMS);
}

/** Private Policy Engine Layer API below */

/**
 * @brief Sets a Policy Engine state
 */
static void pe_set_state(const struct device *dev,
			 const enum usbc_pe_state state)
{
	struct usbc_port_data *data = dev->data;

	smf_set_state(SMF_CTX(data->pe), &pe_states[state]);
}

/**
 * @brief Get the Policy Engine's current state
 */
static enum usbc_pe_state pe_get_state(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->pe->ctx.current - &pe_states[0];
}

/**
 * @brief Get the Policy Engine's previous state
 */
static enum usbc_pe_state pe_get_last_state(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->pe->ctx.previous - &pe_states[0];
}

/**
 * @brief Send a soft reset message
 */
static void pe_send_soft_reset(const struct device *dev,
			       const enum pd_packet_type type)
{
	struct usbc_port_data *data = dev->data;

	data->pe->soft_reset_sop = type;
	pe_set_state(dev, PE_SEND_SOFT_RESET);
}

/**
 * @brief Send a Power Delivery Data Message
 */
static inline void send_data_msg(const struct device *dev,
				 const enum pd_packet_type type,
				 const enum pd_data_msg_type msg)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Clear any previous TX status before sending a new message */
	atomic_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE);
	prl_send_data_msg(dev, type, msg);
}

/**
 * @brief Send a Power Delivery Control Message
 */
static inline void send_ctrl_msg(const struct device *dev,
				 const enum pd_packet_type type,
				 const enum pd_ctrl_msg_type msg)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Clear any previous TX status before sending a new message */
	atomic_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE);
	prl_send_ctrl_msg(dev, type, msg);
}

/**
 * @brief Request desired voltage from source.
 */
static void pe_send_request_msg(const struct device *dev,
				const uint32_t rdo)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct pd_msg *msg = &prl_tx->emsg;
	uint8_t rdo_bytes[4];

	msg->len = sizeof(rdo);
	sys_put_le32(rdo, rdo_bytes);
	memcpy(msg->data, rdo_bytes, msg->len);
	send_data_msg(dev, PD_PACKET_SOP, PD_DATA_REQUEST);
}

/**
 * @brief Transitions state after receiving an extended message.
 */
static void extended_message_not_supported(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	uint32_t *payload = (uint32_t *)prl_rx->emsg.data;
	union pd_ext_header ext_header;

	ext_header.raw_value = *payload;

	if (ext_header.chunked &&
	    ext_header.data_size > PD_MAX_EXTENDED_MSG_CHUNK_LEN) {
		pe_set_state(dev, PE_SNK_CHUNK_RECEIVED);
	} else {
		pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
	}
}

/**
 * @brief Handle common DPM requests
 *
 * @retval True if the request was handled, else False
 */
static bool common_dpm_requests(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	if (pe->dpm_request > REQUEST_TC_END) {
		atomic_set_bit(&pe->flags, PE_FLAGS_DPM_INITIATED_AMS);

		if (pe->dpm_request == REQUEST_PE_DR_SWAP) {
			pe_set_state(dev, PE_DRS_SEND_SWAP);
		} else if (pe->dpm_request == REQUEST_PE_SOFT_RESET_SEND) {
			pe_set_state(dev, PE_SEND_SOFT_RESET);
		}
		return true;
	}

	return false;
}

/**
 * @brief Handle sink-specific DPM requests
 */
static bool sink_dpm_requests(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	if (pe->dpm_request > REQUEST_TC_END) {
		atomic_set_bit(&pe->flags, PE_FLAGS_DPM_INITIATED_AMS);

		if (pe->dpm_request == REQUEST_PE_GET_SRC_CAPS) {
			pe_set_state(dev, PE_SNK_GET_SOURCE_CAP);
		}
		return true;
	}

	return false;
}

/**
 * @brief Check if a specific control message was received
 */
static bool received_control_message(const struct device *dev,
				     const union pd_header header,
				     const enum pd_ctrl_msg_type mt)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	if (prl_rx->emsg.len == 0 &&
	    header.message_type == mt &&
	    header.extended == 0) {
		return true;
	}

	return false;
}

/**
 * @brief Check if a specific data message was received
 */
static bool received_data_message(const struct device *dev,
				  const union pd_header header,
				  const enum pd_data_msg_type mt)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	if (prl_rx->emsg.len > 0 &&
	    header.message_type == mt &&
	    header.extended == 0) {
		return true;
	}

	return false;
}

/**
 * @brief Check a DPM policy
 */
static bool policy_check(const struct device *dev,
			 const enum usbc_policy_check_t pc)
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
static void policy_notify(const struct device *dev,
			  const enum usbc_policy_notify_t notify)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_notify) {
		data->policy_cb_notify(dev, notify);
	}
}

/**
 * @brief Notify the DPM of a WAIT message reception
 */
static bool policy_wait_notify(const struct device *dev,
			       const enum usbc_policy_wait_t notify)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_wait_notify) {
		return data->policy_cb_wait_notify(dev, notify);
	}

	return false;
}

/**
 * @brief Send the received source caps to the DPM
 */
static void policy_set_src_cap(const struct device *dev,
			       const uint32_t *pdos,
			       const int num_pdos)
{
	struct usbc_port_data *data = dev->data;

	if (data->policy_cb_set_src_cap) {
		data->policy_cb_set_src_cap(dev, pdos, num_pdos);
	}
}

/**
 * @brief Get a Request Data Object from the DPM
 */
static uint32_t policy_get_request_data_object(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	/* This callback must be implemented */
	__ASSERT(data->policy_cb_get_rdo != NULL,
		 "Callback pointer should not be NULL");

	return data->policy_cb_get_rdo(dev);
}

/**
 * @brief Check if the sink is a default level
 */
static bool policy_is_snk_at_default(const struct device *dev)
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
static void policy_get_snk_cap(const struct device *dev,
			       uint32_t **pdos,
			       int *num_pdos)
{
	struct usbc_port_data *data = dev->data;

	/* This callback must be implemented */
	__ASSERT(data->policy_cb_get_snk_cap != NULL,
		 "Callback pointer should not be NULL");

	data->policy_cb_get_snk_cap(dev, pdos, num_pdos);
}

/**
 * @brief PE_SNK_Startup Entry State
 */
static void pe_snk_startup_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Startup");

	/* Reset the protocol layer */
	prl_reset(dev);

	/* Set power role to Sink */
	pe->power_role = TC_ROLE_SINK;

	/* Invalidate explicit contract */
	atomic_clear_bit(&pe->flags, PE_FLAGS_EXPLICIT_CONTRACT);

	policy_notify(dev, NOT_PD_CONNECTED);
}

/**
 * @brief PE_SNK_Startup Run State
 */
static void pe_snk_startup_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * Once the reset process completes, the Policy Engine Shall
	 * transition to the PE_SNK_Discovery state
	 */
	if (prl_is_running(dev)) {
		pe_set_state(dev, PE_SNK_DISCOVERY);
	}
}

/**
 * @brief PE_SNK_Discovery Entry State
 */
static void pe_snk_discovery_entry(void *obj)
{
	LOG_INF("PE_SNK_Discovery");
}

/**
 * @brief PE_SNK_Discovery Run State
 */
static void pe_snk_discovery_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *vbus = data->vbus;

	/*
	 * Transition to the PE_SNK_Wait_for_Capabilities state when
	 * VBUS has been detected
	 */
	if (usbc_vbus_check_level(vbus, TC_VBUS_PRESENT)) {
		pe_set_state(dev, PE_SNK_WAIT_FOR_CAPABILITIES);
	}
}

/**
 * @brief PE_SNK_Wait_For_Capabilities Entry State
 */
static void pe_snk_wait_for_capabilities_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SNK_Wait_For_Capabilities");

	/* Initialize and start the SinkWaitCapTimer */
	usbc_timer_start(&pe->pd_t_typec_sink_wait_cap);
}

/**
 * @brief PE_SNK_Wait_For_Capabilities Run State
 */
static void pe_snk_wait_for_capabilities_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	/*
	 * Transition to the PE_SNK_Evaluate_Capability state when:
	 *  1) A Source_Capabilities Message is received.
	 */
	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		header = prl_rx->emsg.header;
		if ((header.extended == false) &&
		    received_data_message(dev, header, PD_DATA_SOURCE_CAP)) {
			pe_set_state(dev, PE_SNK_EVALUATE_CAPABILITY);
			return;
		}
	}

	/* When the SinkWaitCapTimer times out, perform a Hard Reset. */
	if (usbc_timer_expired(&pe->pd_t_typec_sink_wait_cap)) {
		atomic_set_bit(&pe->flags, PE_FLAGS_SNK_WAIT_CAP_TIMEOUT);
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
}

/**
 * @brief PE_SNK_Wait_For_Capabilities Exit State
 */
static void pe_snk_wait_for_capabilities_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Stop SinkWaitCapTimer */
	usbc_timer_stop(&pe->pd_t_typec_sink_wait_cap);
}

/**
 * @brief PE_SNK_Evaluate_Capability Entry State
 */
static void pe_snk_evaluate_capability_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;
	uint32_t *pdos = (uint32_t *)prl_rx->emsg.data;
	uint32_t num_pdo_objs = PD_CONVERT_BYTES_TO_PD_HEADER_COUNT(prl_rx->emsg.len);

	LOG_INF("PE_SNK_Evaluate_Capability");

	header = prl_rx->emsg.header;

	/* Reset Hard Reset counter to zero */
	pe->hard_reset_counter = 0;

	/* Set to highest revision supported by both ports */
	prl_set_rev(dev, PD_PACKET_SOP, MIN(PD_REV30, header.specification_revision));

	/* Send source caps to Device Policy Manager for saving */
	policy_set_src_cap(dev, pdos, num_pdo_objs);

	/* Transition to PE_Snk_Select_Capability */
	pe_set_state(dev, PE_SNK_SELECT_CAPABILITY);
}

/**
 * @brief PE_SNK_Select_Capability Entry State
 */
static void pe_snk_select_capability_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	uint32_t rdo;

	LOG_INF("PE_SNK_Select_Capability");

	/* Get selected source cap from Device Policy Manager */
	rdo = policy_get_request_data_object(dev);

	/* Send Request */
	pe_send_request_msg(dev, rdo);
	/* Inform Device Policy Manager that we are PD Connected */
	policy_notify(dev, PD_CONNECTED);
}


/**
 * @brief PE_SNK_Select_Capability Run State
 */
static void pe_snk_select_capability_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		/*
		 * The sent REQUEST message was discarded.  This can be at
		 * the start of an AMS or in the middle. Handle what to
		 * do based on where we came from.
		 * 1) SE_SNK_EVALUATE_CAPABILITY: sends SoftReset
		 * 2) SE_SNK_READY: goes back to SNK Ready
		 */
		if (pe_get_last_state(dev) == PE_SNK_EVALUATE_CAPABILITY) {
			pe_send_soft_reset(dev, PD_PACKET_SOP);
		} else {
			pe_set_state(dev, PE_SNK_READY);
		}
	} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE)) {
		/* Start the SenderResponseTimer */
		usbc_timer_start(&pe->pd_t_sender_response);
	}

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		header = prl_rx->emsg.header;

		/*
		 * Transition to the PE_SNK_Transition_Sink state when:
		 *  1) An Accept Message is received from the Source.
		 *
		 * Transition to the PE_SNK_Wait_for_Capabilities state when:
		 *  1) There is no Explicit Contract in place and
		 *  2) A Reject Message is received from the Source or
		 *  3) A Wait Message is received from the Source.
		 *
		 * Transition to the PE_SNK_Ready state when:
		 *  1) There is an Explicit Contract in place and
		 *  2) A Reject Message is received from the Source or
		 *  3) A Wait Message is received from the Source.
		 *
		 * Transition to the PE_SNK_Hard_Reset state when:
		 *  1) A SenderResponseTimer timeout occurs.
		 */
		/* Only look at control messages */
		if (received_control_message(dev, header, PD_CTRL_ACCEPT)) {
			/* explicit contract is now in place */
			atomic_set_bit(&pe->flags, PE_FLAGS_EXPLICIT_CONTRACT);
			pe_set_state(dev, PE_SNK_TRANSITION_SINK);
		} else if (received_control_message(dev, header, PD_CTRL_REJECT) ||
			   received_control_message(dev, header, PD_CTRL_WAIT)) {
			/*
			 * We had a previous explicit contract, so transition to
			 * PE_SNK_Ready
			 */
			if (atomic_test_bit(&pe->flags, PE_FLAGS_EXPLICIT_CONTRACT)) {
				if (received_control_message(dev, header, PD_CTRL_WAIT)) {
					/*
					 * Inform Device Policy Manager that Sink
					 * Request needs to Wait
					 */
					if (policy_wait_notify(dev, WAIT_SINK_REQUEST)) {
						atomic_set_bit(&pe->flags,
								PE_FLAGS_WAIT_SINK_REQUEST);
						usbc_timer_start(&pe->pd_t_wait_to_resend);
					}
				}

				pe_set_state(dev, PE_SNK_READY);
			}
			/*
			 * No previous explicit contract, so transition
			 * to PE_SNK_Wait_For_Capabilities
			 */
			else {
				pe_set_state(dev, PE_SNK_WAIT_FOR_CAPABILITIES);
			}
		} else {
			pe_send_soft_reset(dev, prl_rx->emsg.type);
		}
		return;
	}

	/* When the SenderResponseTimer times out, perform a Hard Reset. */
	if (usbc_timer_expired(&pe->pd_t_sender_response)) {
		policy_notify(dev, PORT_PARTNER_NOT_RESPONSIVE);
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
}

/**
 * @brief PE_SNK_Select_Capability Exit State
 */
static void pe_snk_select_capability_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Stop SenderResponse Timer */
	usbc_timer_stop(&pe->pd_t_sender_response);
}

/**
 * @brief PE_SNK_Transition_Sink Entry State
 */
static void pe_snk_transition_sink_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SNK_Transition_Sink");

	/* Initialize and run PSTransitionTimer */
	usbc_timer_start(&pe->pd_t_ps_transition);
}

/**
 * @brief PE_SNK_Transition_Sink Run State
 */
static void pe_snk_transition_sink_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	/*
	 * Transition to the PE_SNK_Ready state when:
	 *  1) A PS_RDY Message is received from the Source.
	 *
	 * Transition to the PE_SNK_Hard_Reset state when:
	 *  1) A Protocol Error occurs.
	 */
	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		header = prl_rx->emsg.header;

		/*
		 * PS_RDY message received
		 */
		if (received_control_message(dev, header, PD_CTRL_PS_RDY)) {
			/*
			 * Inform the Device Policy Manager to Transition
			 * the Power Supply
			 */
			policy_notify(dev, TRANSITION_PS);
			pe_set_state(dev, PE_SNK_READY);
		} else {
			/* Protocol Error */
			pe_set_state(dev, PE_SNK_HARD_RESET);
		}
		return;
	}

	/*
	 * Timeout will lead to a Hard Reset
	 */
	if (usbc_timer_expired(&pe->pd_t_ps_transition)) {
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
}

/**
 * @brief PE_SNK_Transition_Sink Exit State
 */
static void pe_snk_transition_sink_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Initialize and run PSTransitionTimer */
	usbc_timer_stop(&pe->pd_t_ps_transition);
}

/**
 * @brief PE_SNK_Ready Entry State
 */
static void pe_snk_ready_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SNK_Ready");

	/* Clear AMS Flags */
	atomic_clear_bit(&pe->flags, PE_FLAGS_INTERRUPTIBLE_AMS);
	atomic_clear_bit(&pe->flags, PE_FLAGS_DPM_INITIATED_AMS);
}

/**
 * @brief PE_SNK_Ready Run State
 */
static void pe_snk_ready_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	/*
	 * Handle incoming messages before discovery and DPMs other than hard
	 * reset
	 */
	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		union pd_header header = prl_rx->emsg.header;

		/* Extended Message Request */
		if (header.extended) {
			extended_message_not_supported(dev);
			return;
		}
		/* Data Messages */
		else if (header.number_of_data_objects > 0) {
			switch (header.message_type) {
			case PD_DATA_SOURCE_CAP:
				pe_set_state(dev, PE_SNK_EVALUATE_CAPABILITY);
				break;
			default:
				pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
			}
			return;
		}
		/* Control Messages */
		else {
			switch (header.message_type) {
			case PD_CTRL_GOOD_CRC:
				/* Do nothing */
				break;
			case PD_CTRL_PING:
				/* Do nothing */
				break;
			case PD_CTRL_GET_SINK_CAP:
				pe_set_state(dev, PE_SNK_GIVE_SINK_CAP);
				return;
			case PD_CTRL_DR_SWAP:
				pe_set_state(dev, PE_DRS_EVALUATE_SWAP);
				return;
			case PD_CTRL_NOT_SUPPORTED:
				/* Do nothing */
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
				return;
			/*
			 * Receiving an unknown or unsupported message
			 * shall be responded to with a not supported message.
			 */
			default:
				pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
				return;
			}
		}
	}

	/*
	 * Check if we are waiting to resend any messages
	 */
	if (usbc_timer_expired(&pe->pd_t_wait_to_resend)) {
		if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_WAIT_SINK_REQUEST)) {
			pe_set_state(dev, PE_SNK_SELECT_CAPABILITY);
			return;
		} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_WAIT_DATA_ROLE_SWAP)) {
			pe_set_state(dev, PE_DRS_SEND_SWAP);
			return;
		}
	}

	/*
	 * Handle Device Policy Manager Requests
	 */
	common_dpm_requests(dev);
	sink_dpm_requests(dev);
}

/**
 * @brief PE_SNK_Hard_Reset Entry State
 */
static void pe_snk_hard_reset_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;

	LOG_INF("PE_SNK_Hard_Reset");

	/*
	 * Note: If the SinkWaitCapTimer times out and the HardResetCounter is
	 *       greater than nHardResetCount the Sink Shall assume that the
	 *       Source is non-responsive.
	 */
	if (atomic_test_bit(&pe->flags, PE_FLAGS_SNK_WAIT_CAP_TIMEOUT) &&
	    pe->hard_reset_counter > N_HARD_RESET_COUNT) {
		/* Inform the DPM that the port partner is not responsive */
		policy_notify(dev, PORT_PARTNER_NOT_RESPONSIVE);

		/* Pause the Policy Engine */
		data->pe_enabled = false;
		return;
	}

	atomic_clear_bit(&pe->flags, PE_FLAGS_SNK_WAIT_CAP_TIMEOUT);
	atomic_clear_bit(&pe->flags, PE_FLAGS_PROTOCOL_ERROR);

	/* Request the generation of Hard Reset Signaling by the PHY Layer */
	prl_execute_hard_reset(dev);
	/* Increment the HardResetCounter */
	pe->hard_reset_counter++;
}

/**
 * @brief PE_SNK_Hard_Reset Run State
 */
static void pe_snk_hard_reset_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * Transition to the PE_SNK_Transition_to_default state when:
	 *  1) The Hard Reset is complete.
	 */
	if (atomic_test_bit(&pe->flags, PE_FLAGS_HARD_RESET_PENDING)) {
		return;
	}

	pe_set_state(dev, PE_SNK_TRANSITION_TO_DEFAULT);
}

/**
 * @brief PE_SNK_Transition_to_default Entry State
 */
static void pe_snk_transition_to_default_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Transition_to_default");

	/* Reset flags */
	pe->flags = ATOMIC_INIT(0);
	pe->data_role = TC_ROLE_UFP;

	/*
	 * Indicate to the Device Policy Manager that the Sink Shall
	 * transition to default
	 */
	policy_notify(dev, SNK_TRANSITION_TO_DEFAULT);
	/*
	 * Request the Device Policy Manger that the Port Data Role is
	 * set to UFP
	 */
	policy_notify(dev, DATA_ROLE_IS_UFP);
}

/**
 * @brief PE_SNK_Transition_to_default Run State
 */
static void pe_snk_transition_to_default_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * Wait until Device Policy Manager has transitioned the sink to
	 * default level
	 */
	if (policy_is_snk_at_default(dev)) {
		/* Inform the Protocol Layer that the Hard Reset is complete */
		prl_hard_reset_complete(dev);
		pe_set_state(dev, PE_SNK_STARTUP);
	}
}

/**
 * @brief PE_SNK_Get_Source_Cap Entry State
 */
static void pe_snk_get_source_cap_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Get_Source_Cap");

	/* Send a Get_Source_Cap Message */
	send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_GET_SOURCE_CAP);
}

/**
 * @brief PE_SNK_Get_Source_Cap Run State
 */
static void pe_snk_get_source_cap_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	/* Wait until message is sent or dropped */
	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE)) {
		pe_set_state(dev, PE_SNK_READY);
	} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		pe_send_soft_reset(dev, prl_rx->emsg.type);
	}
}

/**
 * @brief PE_SNK_Get_Source_Cap Exit State
 */
static void pe_snk_get_source_cap_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	usbc_timer_stop(&pe->pd_t_sender_response);
}

/**
 * @brief PE_Send_Soft_Reset Entry State
 */
static void pe_send_soft_reset_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Send_Soft_Reset");

	/* Reset Protocol Layer */
	prl_reset(dev);
	atomic_set_bit(&pe->flags, PE_FLAGS_SEND_SOFT_RESET);
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

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_SEND_SOFT_RESET)) {
		/* Send Soft Reset message */
		send_ctrl_msg(dev, pe->soft_reset_sop, PD_CTRL_SOFT_RESET);
		return;
	}

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		/* Inform Device Policy Manager that the message was discarded */
		policy_notify(dev, MSG_DISCARDED);
		pe_set_state(dev, PE_SNK_READY);
	} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE)) {
		/* Start SenderResponse timer */
		usbc_timer_start(&pe->pd_t_sender_response);
	} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		header = prl_rx->emsg.header;

		if (received_control_message(dev, header, PD_CTRL_ACCEPT)) {
			pe_set_state(dev, PE_SNK_WAIT_FOR_CAPABILITIES);
		}
	} else if (atomic_test_bit(&pe->flags, PE_FLAGS_PROTOCOL_ERROR) ||
		   usbc_timer_expired(&pe->pd_t_sender_response)) {
		if (atomic_test_bit(&pe->flags, PE_FLAGS_PROTOCOL_ERROR)) {
			atomic_clear_bit(&pe->flags, PE_FLAGS_PROTOCOL_ERROR);
		} else {
			policy_notify(dev, PORT_PARTNER_NOT_RESPONSIVE);
		}
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
}

/**
 * @brief PE_Send_Soft_Reset Exit State
 */
static void pe_send_soft_reset_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Stop Sender Response Timer */
	usbc_timer_stop(&pe->pd_t_sender_response);
}

/**
 * @brief PE_SNK_Soft_Reset Entry State
 */
static void pe_soft_reset_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Soft_Reset");

	/* Reset the Protocol Layer */
	prl_reset(dev);
	atomic_set_bit(&pe->flags, PE_FLAGS_SEND_SOFT_RESET);
}

/**
 * @brief PE_SNK_Soft_Reset Run State
 */
static void  pe_soft_reset_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	if (prl_is_running(dev) == false) {
		return;
	}

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_SEND_SOFT_RESET)) {
		/* Send Accept message */
		send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_ACCEPT);
		return;
	}

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE)) {
		pe_set_state(dev, PE_SNK_WAIT_FOR_CAPABILITIES);
	} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_PROTOCOL_ERROR)) {
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
}

/**
 * @brief PE_Not_Supported Entry State
 */
static void pe_send_not_supported_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_Not_Supported");

	/* Request the Protocol Layer to send a Not_Supported or Reject Message. */
	if (prl_get_rev(dev, PD_PACKET_SOP) > PD_REV20) {
		send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_NOT_SUPPORTED);
	} else {
		send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_REJECT);
	}
}

/**
 * @brief PE_Not_Supported Run State
 */
static void pe_send_not_supported_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	if (atomic_test_bit(&pe->flags, PE_FLAGS_TX_COMPLETE) ||
			atomic_test_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		atomic_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE);
		atomic_clear_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED);
		pe_set_state(dev, PE_SNK_READY);
	}
}

/**
 * @brief PE_Chunk_Received Entry State
 */
static void pe_chunk_received_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SNK_Chunk_Received");

	usbc_timer_start(&pe->pd_t_chunking_not_supported);
}

/**
 * @brief PE_Chunk_Received Run State
 */
static void pe_chunk_received_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	if (usbc_timer_expired(&pe->pd_t_chunking_not_supported)) {
		pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
	}
}

/**
 * @brief PE_SNK_Give_Sink_Cap Entry state
 */
static void pe_snk_give_sink_cap_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct pd_msg *msg = &prl_tx->emsg;
	uint32_t *pdos;
	uint32_t num_pdos;

	/* Get present sink capabilities from Device Policy Manager */
	policy_get_snk_cap(dev, &pdos, &num_pdos);

	msg->len = PD_CONVERT_PD_HEADER_COUNT_TO_BYTES(num_pdos);
	memcpy(msg->data, (uint8_t *)pdos, msg->len);
	send_data_msg(dev, PD_PACKET_SOP, PD_DATA_SINK_CAP);
}

/**
 * @brief PE_SNK_Give_Sink_Cap Run state
 */
static void pe_snk_give_sink_cap_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	/* Wait until message is sent or dropped */
	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE)) {
		pe_set_state(dev, PE_SNK_READY);
	} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		pe_send_soft_reset(dev, prl_rx->emsg.type);
	}
}

/**
 * @brief PE_DRS_Evaluate_Swap Entry state
 */
static void pe_drs_evaluate_swap_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/* Get evaluation of Data Role Swap request from Device Policy Manager */
	if (policy_check(dev, (pe->data_role == TC_ROLE_UFP) ?
			 CHECK_DATA_ROLE_SWAP_TO_DFP : CHECK_DATA_ROLE_SWAP_TO_UFP)) {
		/*
		 * PE_DRS_DFP_UFP_Accept_Swap and PE_DRS_UFP_DFP_Accept_Swap
		 * State embedded here
		 */
		/* Send Accept message */
		send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_ACCEPT);
	} else {
		/*
		 * PE_DRS_DFP_UFP_Reject_Swap and PE_DRS_UFP_DFP_Reject_Swap
		 * State embedded here
		 */
		/* Send Reject message */
		send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_REJECT);
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

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE)) {
		/* Only update data roles if last message sent was Accept */
		if (prl_tx->msg_type == PD_CTRL_ACCEPT) {
			/* Update Data Role */
			pe->data_role = (pe->data_role == TC_ROLE_UFP) ? TC_ROLE_DFP : TC_ROLE_UFP;
			/* Notify TCPC of role update */
			tcpc_set_roles(data->tcpc, pe->power_role, pe->data_role);
			/* Inform Device Policy Manager of Data Role Change */
			policy_notify(dev, (pe->data_role == TC_ROLE_UFP) ?
				      DATA_ROLE_IS_UFP : DATA_ROLE_IS_DFP);
		}
		pe_set_state(dev, PE_SNK_READY);
	} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED)) {
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
 */
static void pe_drs_send_swap_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/* Send Swap DR message */
	send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_DR_SWAP);
}

/**
 * @brief PE_DRS_Send_Swap Run state
 */
static void pe_drs_send_swap_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_TX_COMPLETE)) {
		/* Start Sender Response Timer */
		usbc_timer_start(&pe->pd_t_sender_response);
	}

	if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_RECEIVED)) {
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
				atomic_set_bit(&pe->flags, PE_FLAGS_WAIT_DATA_ROLE_SWAP);
				usbc_timer_start(&pe->pd_t_wait_to_resend);
			}
		} else if (received_control_message(dev, header, PD_CTRL_ACCEPT)) {
			/* Update Data Role */
			pe->data_role = (pe->data_role == TC_ROLE_UFP) ? TC_ROLE_DFP : TC_ROLE_UFP;
			/* Notify TCPC of role update */
			tcpc_set_roles(data->tcpc, pe->power_role, pe->data_role);
			/* Inform Device Policy Manager of Data Role Change */
			policy_notify(dev, (pe->data_role == TC_ROLE_UFP) ?
				      DATA_ROLE_IS_UFP : DATA_ROLE_IS_DFP);
		} else {
			/* Protocol Error */
			policy_notify(dev, PROTOCOL_ERROR);
			pe_send_soft_reset(dev, PD_PACKET_SOP);
			return;
		}
		pe_set_state(dev, PE_SNK_READY);
		return;
	} else if (atomic_test_and_clear_bit(&pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		/*
		 * Inform Device Policy Manager that the message
		 * was discarded
		 */
		policy_notify(dev, MSG_DISCARDED);
		pe_set_state(dev, PE_SNK_READY);
		return;
	}

	if (usbc_timer_expired(&pe->pd_t_sender_response)) {
		/* Protocol Error */
		policy_notify(dev, PROTOCOL_ERROR);
		pe_send_soft_reset(dev, PD_PACKET_SOP);
	}
}

/**
 * @brief PE_Send_Not_Supported Exit state
 */
static void pe_drs_send_swap_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Stop Sender Response Timer */
	usbc_timer_stop(&pe->pd_t_sender_response);
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
 * @brief Policy engine State table
 */
static const struct smf_state pe_states[] = {
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
		pe_snk_select_capability_exit,
		NULL),
	[PE_SNK_READY] = SMF_CREATE_STATE(
		pe_snk_ready_entry,
		pe_snk_ready_run,
		NULL,
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
		pe_snk_get_source_cap_exit,
		NULL),
	[PE_SNK_TRANSITION_SINK] = SMF_CREATE_STATE(
		pe_snk_transition_sink_entry,
		pe_snk_transition_sink_run,
		pe_snk_transition_sink_exit,
		NULL),
	[PE_SEND_SOFT_RESET] = SMF_CREATE_STATE(
		pe_send_soft_reset_entry,
		pe_send_soft_reset_run,
		pe_send_soft_reset_exit,
		NULL),
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
		pe_drs_send_swap_exit,
		NULL),
	[PE_SNK_GET_SOURCE_CAP] = SMF_CREATE_STATE(
		pe_snk_get_source_cap_entry,
		pe_snk_get_source_cap_run,
		pe_snk_get_source_cap_exit,
		NULL),
	[PE_SNK_CHUNK_RECEIVED] = SMF_CREATE_STATE(
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
