/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(usbc_stack, CONFIG_USBC_STACK_LOG_LEVEL);

#include "usbc_stack.h"

/**
 * @file
 * @brief USB Power Delivery Protocol Layer (PRL)
 *
 * The PRL implementation in this file is base on
 * Specification Revision 3.1, Version 1.3
 */

/**
 * @brief Protocol Layer Flags
 *
 * @note: These flags are used in multiple state machines and could have
 *	  different meanings in each state machine.
 */
enum prl_flags {
	/** Flag to note message transmission completed */
	PRL_FLAGS_TX_COMPLETE = 0,
	/** Flag to note message was discarded */
	PRL_FLAGS_TX_DISCARDED = 1,
	/** Flag to note PRL waited for SINK_OK CC state before transmitting */
	PRL_FLAGS_WAIT_SINK_OK = 2,
	/** Flag to note transmission error occurred */
	PRL_FLAGS_TX_ERROR = 3,
	/** Flag to note PE triggered a hard reset */
	PRL_FLAGS_PE_HARD_RESET = 4,
	/** Flag to note hard reset has completed */
	PRL_FLAGS_HARD_RESET_COMPLETE = 5,
	/** Flag to note port partner sent a hard reset */
	PRL_FLAGS_PORT_PARTNER_HARD_RESET = 6,
	/**
	 * Flag to note a message transmission has been requested. It is only
	 * cleared when the message is sent to the TCPC layer.
	 */
	PRL_FLAGS_MSG_XMIT = 7,
	/** Flag to track if first message in AMS is pending */
	PRL_FLAGS_FIRST_MSG_PENDING = 8,
	/* Flag to note that PRL requested to set SINK_NG CC state */
	PRL_FLAGS_SINK_NG = 9,
};

/**
 * @brief Protocol Layer Transmission States
 */
enum usbc_prl_tx_state_t {
	/** PRL_Tx_PHY_Layer_Reset */
	PRL_TX_PHY_LAYER_RESET,
	/** PRL_Tx_Wait_for_Message_Request */
	PRL_TX_WAIT_FOR_MESSAGE_REQUEST,
	/** PRL_Tx_Layer_Reset_for_Transmit */
	PRL_TX_LAYER_RESET_FOR_TRANSMIT,
	/** PRL_Tx_Wait_for_PHY_response */
	PRL_TX_WAIT_FOR_PHY_RESPONSE,
	/** PRL_Tx_Snk_Start_of_AMS */
	PRL_TX_SNK_START_AMS,
	/** PRL_Tx_Snk_Pending */
	PRL_TX_SNK_PENDING,
	/** PRL_Tx_Discard_Message */
	PRL_TX_DISCARD_MESSAGE,
	/** PRL_TX_SRC_Source_Tx */
	PRL_TX_SRC_SOURCE_TX,
	/** PRL_TX_SRC_Pending */
	PRL_TX_SRC_PENDING,

	/** PRL_Tx_Suspend. Not part of the PD specification. */
	PRL_TX_SUSPEND,

	/** Number of PRL_TX States */
	PRL_TX_STATE_COUNT
};

/**
 * @brief Protocol Layer Hard Reset States
 */
enum usbc_prl_hr_state_t {
	/** PRL_HR_Wait_For_Request */
	PRL_HR_WAIT_FOR_REQUEST,
	/** PRL_HR_Reset_Layer */
	PRL_HR_RESET_LAYER,
	/** PRL_HR_Wait_For_PHY_Hard_Reset_Complete */
	PRL_HR_WAIT_FOR_PHY_HARD_RESET_COMPLETE,
	/** PRL_HR_Wait_For_PE_Hard_Reset_Complete */
	PRL_HR_WAIT_FOR_PE_HARD_RESET_COMPLETE,

	/** PRL_Hr_Suspend. Not part of the PD specification. */
	PRL_HR_SUSPEND,

	/** Number of PRL_HR States */
	PRL_HR_STATE_COUNT
};

static const struct smf_state prl_tx_states[PRL_TX_STATE_COUNT];
static const struct smf_state prl_hr_states[PRL_HR_STATE_COUNT];

static void prl_tx_construct_message(const struct device *dev);
static void prl_rx_wait_for_phy_message(const struct device *dev);
static void prl_hr_set_state(const struct device *dev, const enum usbc_prl_hr_state_t state);
static void prl_tx_set_state(const struct device *dev, const enum usbc_prl_tx_state_t state);
static void prl_init(const struct device *dev);
static enum usbc_prl_hr_state_t prl_hr_get_state(const struct device *dev);

/**
 * @brief Initializes the TX an HR state machines and enters the
 *	  PRL_TX_SUSPEND and PRL_TX_SUSPEND states respectively.
 */
void prl_subsys_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;

	/* Save the port device objects so states can access it */
	prl_tx->dev = dev;
	prl_hr->dev = dev;

	/* Initialize the state machines */
	smf_set_initial(SMF_CTX(prl_hr), &prl_hr_states[PRL_HR_SUSPEND]);
	smf_set_initial(SMF_CTX(prl_tx), &prl_tx_states[PRL_TX_SUSPEND]);
}

/**
 * @brief Test if the Protocol Layer State Machines are running
 *
 * @retval TRUE if the state machines are running
 * @retval FALSE if the state machines are paused
 */
bool prl_is_running(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->prl_sm_state == SM_RUN;
}

/**
 * @brief Directs the Protocol Layer to perform a hard reset. This function
 *	  is called from the Policy Engine.
 */
void prl_execute_hard_reset(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;

	/* Only allow async. function calls when state machine is running */
	if (prl_is_running(dev) == false) {
		return;
	}

	atomic_set_bit(&prl_hr->flags, PRL_FLAGS_PE_HARD_RESET);
	prl_hr_set_state(dev, PRL_HR_RESET_LAYER);
}

/**
 * @brief Instructs the Protocol Layer that a hard reset is complete.
 *	  This function is called from the Policy Engine.
 */
void prl_hard_reset_complete(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;

	atomic_set_bit(&prl_hr->flags, PRL_FLAGS_HARD_RESET_COMPLETE);
}

/**
 * @brief Directs the Protocol Layer to construct and transmit a Power Delivery
 *	  Control message.
 */
void prl_send_ctrl_msg(const struct device *dev, const enum pd_packet_type type,
		       const enum pd_ctrl_msg_type msg)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;

	/* set packet type */
	prl_tx->emsg.type = type;
	/* set message type */
	prl_tx->msg_type = msg;
	/* control message. set data len to zero */
	prl_tx->emsg.len = 0;

	atomic_set_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT);
}

/**
 * @brief Directs the Protocol Layer to construct and transmit a Power Delivery
 *        Data message.
 *
 * @note: Before calling this function prl_tx->emsg.data and prl_tx->emsg.len
 *	  must be set.
 */
void prl_send_data_msg(const struct device *dev, const enum pd_packet_type type,
		       const enum pd_data_msg_type msg)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;

	/* set packet type */
	prl_tx->emsg.type = type;
	/* set message type */
	prl_tx->msg_type = msg;

	atomic_set_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT);
}

/**
 * @brief Directs the Protocol Layer to reset the revision of each packet type
 *	  to its default value.
 */
void prl_set_default_pd_revision(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	/*
	 * Initialize to highest revision supported. If the port or cable
	 * partner doesn't support this revision, the Protocol Engine will
	 * lower this value to the revision supported by the partner.
	 */
	data->rev[PD_PACKET_SOP] = PD_REV30;
	data->rev[PD_PACKET_SOP_PRIME] = PD_REV30;
	data->rev[PD_PACKET_PRIME_PRIME] = PD_REV30;
	data->rev[PD_PACKET_DEBUG_PRIME] = PD_REV30;
	data->rev[PD_PACKET_DEBUG_PRIME_PRIME] = PD_REV30;
}

/**
 * @brief Start the Protocol Layer state machines
 */
void prl_start(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	data->prl_enabled = true;
}

/**
 * @brief Pause the Protocol Layer state machines
 */
void prl_suspend(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	data->prl_enabled = false;

	/*
	 * While we are paused, exit all states
	 * and wait until initialized again.
	 */
	prl_tx_set_state(dev, PRL_TX_SUSPEND);
	prl_hr_set_state(dev, PRL_HR_SUSPEND);
}

/**
 * @brief Reset the Protocol Layer state machines
 */
void prl_reset(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	if (data->prl_enabled) {
		data->prl_sm_state = SM_INIT;
	}
}

/**
 * @brief Inform the PRL that the first message in an AMS is being sent
 */
void prl_first_msg_notificaiton(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;

	atomic_set_bit(&prl_tx->flags, PRL_FLAGS_FIRST_MSG_PENDING);
}

/**
 * @brief Run the Protocol Layer state machines
 */
void prl_run(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;

	switch (data->prl_sm_state) {
	case SM_PAUSED:
		if (data->prl_enabled == false) {
			break;
		}
	/* fall through */
	case SM_INIT:
		prl_init(dev);
		data->prl_sm_state = SM_RUN;
	/* fall through */
	case SM_RUN:
		if (data->prl_enabled == false) {
			data->prl_sm_state = SM_PAUSED;
			/* Disable RX */
			tcpc_set_rx_enable(data->tcpc, false);
			break;
		}

		/* Run Protocol Layer Hard Reset state machine */
		smf_run_state(SMF_CTX(prl_hr));

		/*
		 * During Hard Reset no USB Power Delivery Protocol Messages
		 * are sent or received; only Hard Reset Signaling is present
		 * after which the communication channel is assumed to have
		 * been disabled by the Physical Layer until completion of
		 * the Hard Reset.
		 */
		if (prl_hr_get_state(dev) == PRL_HR_WAIT_FOR_REQUEST) {
			/* Run Protocol Layer Message Reception */
			prl_rx_wait_for_phy_message(dev);

			/* Run Protocol Layer Message Tx state machine */
			smf_run_state(SMF_CTX(prl_tx));
		}
		break;
	}
}

/**
 * @brief Set revision for the give packet type. This function is called
 *	  from the Policy Engine.
 */
void prl_set_rev(const struct device *dev, const enum pd_packet_type type,
		 const enum pd_rev_type rev)
{
	struct usbc_port_data *data = dev->data;

	data->rev[type] = rev;
}

/**
 * @brief Get the revision for the give packet type.
 *	  This function is called from the Policy Engine.
 */
enum pd_rev_type prl_get_rev(const struct device *dev, const enum pd_packet_type type)
{
	struct usbc_port_data *data = dev->data;

	return data->rev[type];
}

/** Private Protocol Layer API below */

/**
 * @brief Alert Handler called by the TCPC driver
 */
static void alert_handler(const struct device *tcpc, void *port_dev, enum tcpc_alert alert)
{
	const struct device *dev = (const struct device *)port_dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;

	switch (alert) {
	case TCPC_ALERT_HARD_RESET_RECEIVED:
		atomic_set_bit(&prl_hr->flags, PRL_FLAGS_PORT_PARTNER_HARD_RESET);
		break;
	case TCPC_ALERT_TRANSMIT_MSG_FAILED:
		atomic_set_bit(&prl_tx->flags, PRL_FLAGS_TX_ERROR);
		break;
	case TCPC_ALERT_TRANSMIT_MSG_DISCARDED:
		atomic_set_bit(&prl_tx->flags, PRL_FLAGS_TX_DISCARDED);
		break;
	case TCPC_ALERT_TRANSMIT_MSG_SUCCESS:
		atomic_set_bit(&prl_tx->flags, PRL_FLAGS_TX_COMPLETE);
		break;
	/* These alerts are ignored and will just wake the thread. */
	default:
		break;
	}

	/* Wake the thread if it's sleeping */
	k_wakeup(data->port_thread);
}

/**
 * @brief Set the Protocol Layer Message Transmission state
 */
static void prl_tx_set_state(const struct device *dev, const enum usbc_prl_tx_state_t state)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;

	__ASSERT(state < ARRAY_SIZE(prl_tx_states), "invalid prl_tx_state %d", state);
	smf_set_state(SMF_CTX(prl_tx), &prl_tx_states[state]);
}

/**
 * @brief Set the Protocol Layer Hard Reset state
 */
static void prl_hr_set_state(const struct device *dev, const enum usbc_prl_hr_state_t state)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;

	__ASSERT(state < ARRAY_SIZE(prl_hr_states), "invalid prl_hr_state %d", state);
	smf_set_state(SMF_CTX(prl_hr), &prl_hr_states[state]);
}

/**
 * @brief Get the Protocol Layer Hard Reset state
 */
static enum usbc_prl_hr_state_t prl_hr_get_state(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;

	return prl_hr->ctx.current - &prl_hr_states[0];
}

/**
 * @brief Increment the message ID counter for the last transmitted packet type
 */
static void increment_msgid_counter(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;

	/* If the last message wasn't an SOP* message, no need to increment */
	if (prl_tx->last_xmit_type >= NUM_SOP_STAR_TYPES) {
		return;
	}

	prl_tx->msg_id_counter[prl_tx->last_xmit_type] =
		(prl_tx->msg_id_counter[prl_tx->last_xmit_type] + 1) & PD_MESSAGE_ID_COUNT;
}

/**
 * @brief Get the SOP* header for the current received message
 */
static uint32_t get_sop_star_header(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	const bool is_sop_packet = prl_tx->emsg.type == PD_PACKET_SOP;
	union pd_header header;

	/* SOP vs SOP'/SOP" headers are different. Replace fields as needed */
	header.message_type = prl_tx->msg_type;
	header.port_data_role = is_sop_packet ? pe_get_data_role(dev) : 0;
	header.specification_revision = data->rev[prl_tx->emsg.type];
	header.port_power_role = is_sop_packet ? pe_get_power_role(dev) : pe_get_cable_plug(dev);
	header.message_id = prl_tx->msg_id_counter[prl_tx->emsg.type];
	header.number_of_data_objects = PD_CONVERT_BYTES_TO_PD_HEADER_COUNT(prl_tx->emsg.len);
	header.extended = false;

	return header.raw_value;
}

/**
 * @brief Construct and transmit a message
 */
static void prl_tx_construct_message(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	const struct device *tcpc = data->tcpc;

	/* The header is unused for hard reset, etc. */
	prl_tx->emsg.header.raw_value =
		prl_tx->emsg.type < NUM_SOP_STAR_TYPES ? get_sop_star_header(dev) : 0;

	/* Save SOP* so the correct msg_id_counter can be incremented */
	prl_tx->last_xmit_type = prl_tx->emsg.type;

	/*
	 * PRL_FLAGS_TX_COMPLETE could be set if this function is called before
	 * the Policy Engine is informed of the previous transmission. Clear
	 * the flag so that this message can be sent.
	 */
	atomic_clear_bit(&prl_tx->flags, PRL_FLAGS_TX_COMPLETE);

	/* Clear PRL_FLAGS_MSG_XMIT flag */
	atomic_clear_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT);

	/*
	 * Pass message to PHY Layer. It handles retries in hardware as
	 * software cannot handle the required timing ~ 1ms (tReceive + tRetry)
	 */
	tcpc_transmit_data(tcpc, &prl_tx->emsg);
}

/**
 * @brief Transmit a Hard Reset Message
 */
static void prl_hr_send_msg_to_phy(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	const struct device *tcpc = data->tcpc;

	/* Header is not used for hard reset */
	prl_tx->emsg.header.raw_value = 0;
	prl_tx->emsg.type = PD_PACKET_TX_HARD_RESET;

	/*
	 * These flags could be set if this function is called before the
	 * Policy Engine is informed of the previous transmission. Clear the
	 * flags so that this message can be sent.
	 */
	data->prl_tx->flags = ATOMIC_INIT(0);

	/* Pass message to PHY Layer */
	tcpc_transmit_data(tcpc, &prl_tx->emsg);
}

/**
 * @brief Initialize the Protocol Layer State Machines
 */
static void prl_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;
	int i;

	LOG_INF("PRL_INIT");

	/* Set all packet types to default revision */
	prl_set_default_pd_revision(dev);

	/*
	 * Set TCPC alert handler so we are notified when messages
	 * are received, transmitted, etc.
	 */
	tcpc_set_alert_handler_cb(data->tcpc, alert_handler, (void *)dev);

	/* Initialize the PRL_HR state machine */
	prl_hr->flags = ATOMIC_INIT(0);
	usbc_timer_init(&prl_hr->pd_t_hard_reset_complete, PD_T_HARD_RESET_COMPLETE_MAX_MS);
	prl_hr_set_state(dev, PRL_HR_WAIT_FOR_REQUEST);

	/* Initialize the PRL_TX state machine */
	prl_tx->flags = ATOMIC_INIT(0);
	prl_tx->last_xmit_type = PD_PACKET_SOP;
	for (i = 0; i < NUM_SOP_STAR_TYPES; i++) {
		prl_tx->msg_id_counter[i] = 0;
	}
	usbc_timer_init(&prl_tx->pd_t_tx_timeout, PD_T_TX_TIMEOUT_MS);
	usbc_timer_init(&prl_tx->pd_t_sink_tx, PD_T_SINK_TX_MAX_MS);
	prl_tx_set_state(dev, PRL_TX_PHY_LAYER_RESET);

	/* Initialize the PRL_RX state machine */
	prl_rx->flags = ATOMIC_INIT(0);
	for (i = 0; i < NUM_SOP_STAR_TYPES; i++) {
		prl_rx->msg_id[i] = -1;
	}
}

/**
 * @brief PRL_Tx_PHY_Layer_Reset State
 */
static void prl_tx_phy_layer_reset_entry(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;

	LOG_INF("PRL_Tx_PHY_Layer_Reset");

	/* Enable communications */
	tcpc_set_rx_enable(tcpc, tc_is_in_attached_state(dev));

	/* Reset complete */
	prl_tx_set_state(dev, PRL_TX_WAIT_FOR_MESSAGE_REQUEST);
}

/**
 * @brief PRL_Tx_Wait_for_Message_Request Entry State
 */
static void prl_tx_wait_for_message_request_entry(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;

	LOG_INF("PRL_Tx_Wait_for_Message_Request");

	/* Clear outstanding messages */
	prl_tx->flags = ATOMIC_INIT(0);
}

/**
 * @brief PRL_Tx_Wait_for_Message_Request Run State
 */
static void prl_tx_wait_for_message_request_run(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;
	struct usbc_port_data *data = dev->data;

	/* Clear any AMS flags and state if we are no longer in an AMS */
	if (pe_dpm_initiated_ams(dev) == false) {
#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
		/* Note PRL_Tx_Src_Sink_Tx is embedded here. */
		if (atomic_test_and_clear_bit(&prl_tx->flags, PRL_FLAGS_SINK_NG)) {
			tc_select_src_collision_rp(dev, SINK_TX_OK);
		}
#endif
		atomic_clear_bit(&prl_tx->flags, PRL_FLAGS_WAIT_SINK_OK);
	}

	/*
	 * Check if we are starting an AMS and need to wait and/or set the CC
	 * lines appropriately.
	 */
	if (data->rev[PD_PACKET_SOP] == PD_REV30 && pe_dpm_initiated_ams(dev)) {
		if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_WAIT_SINK_OK) ||
			atomic_test_bit(&prl_tx->flags, PRL_FLAGS_SINK_NG)) {
			/*
			 * If we are already in an AMS then allow the
			 * multi-message AMS to continue.
			 *
			 * Fall Through using the current AMS
			 */
		} else {
			/*
			 * Start of AMS notification received from
			 * Policy Engine
			 */
			if (IS_ENABLED(CONFIG_USBC_CSM_SOURCE_ONLY) &&
				pe_get_power_role(dev) == TC_ROLE_SOURCE) {
				atomic_set_bit(&prl_tx->flags, PRL_FLAGS_SINK_NG);
				prl_tx_set_state(dev, PRL_TX_SRC_SOURCE_TX);
			} else {
				atomic_set_bit(&prl_tx->flags, PRL_FLAGS_WAIT_SINK_OK);
				prl_tx_set_state(dev, PRL_TX_SNK_START_AMS);
			}
			return;
		}
	}

	/* Handle non Rev 3.0 or subsequent messages in AMS sequence */
	if (atomic_test_and_clear_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT)) {
		/*
		 * Soft Reset Message pending
		 */
		if ((prl_tx->msg_type == PD_CTRL_SOFT_RESET) && (prl_tx->emsg.len == 0)) {
			prl_tx_set_state(dev, PRL_TX_LAYER_RESET_FOR_TRANSMIT);
		} else {
			/* Message pending (except Soft Reset) */

			/* NOTE: PRL_TX_Construct_Message State embedded here */
			prl_tx_construct_message(dev);
			prl_tx_set_state(dev, PRL_TX_WAIT_FOR_PHY_RESPONSE);
		}
		return;
	}
}

/**
 * @brief PRL_Tx_Layer_Reset_for_Transmit Entry State
 */
static void prl_tx_layer_reset_for_transmit_entry(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	LOG_INF("PRL_Tx_Layer_Reset_for_Transmit");

	if (prl_tx->emsg.type < NUM_SOP_STAR_TYPES) {
		/*
		 * This state is only used during soft resets. Reset only the
		 * matching message type.
		 *
		 * From section 6.3.13 Soft Reset Message in the USB PD 3.0
		 * v2.0 spec, Soft_Reset Message Shall be targeted at a
		 * specific entity depending on the type of SOP* Packet used.
		 */
		prl_tx->msg_id_counter[prl_tx->emsg.type] = 0;
		/*
		 * From section 6.11.2.3.2, the MessageID should be cleared
		 * from the PRL_Rx_Layer_Reset_for_Receive state. However, we
		 * don't implement a full state machine for PRL RX states so
		 * clear the MessageID here.
		 */
		prl_rx->msg_id[prl_tx->emsg.type] = -1;
	}

	/* NOTE: PRL_Tx_Construct_Message State embedded here */
	prl_tx_construct_message(dev);
	prl_tx_set_state(dev, PRL_TX_WAIT_FOR_PHY_RESPONSE);
}

/**
 * @brief PRL_Tx_Wait_for_PHY_response Entry State
 */
static void prl_tx_wait_for_phy_response_entry(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;

	LOG_INF("PRL_Tx_Wait_for_PHY_response");
	usbc_timer_start(&prl_tx->pd_t_tx_timeout);
}

/**
 * @brief PRL_Tx_Wait_for_PHY_response Run State
 */
static void prl_tx_wait_for_phy_response_run(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;

	/* Wait until TX is complete */
	if (atomic_test_and_clear_bit(&prl_tx->flags, PRL_FLAGS_TX_DISCARDED)) {
		/* NOTE: PRL_TX_DISCARD_MESSAGE State embedded here. */
		/* Inform Policy Engine Message was discarded */
		pe_report_discard(dev);
		prl_tx_set_state(dev, PRL_TX_PHY_LAYER_RESET);
		return;
	}
	if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_COMPLETE)) {
		/* NOTE: PRL_TX_Message_Sent State embedded here. */
		/* Inform Policy Engine Message was sent */
		pe_message_sent(dev);
		/*
		 * This event reduces the time of informing the policy engine
		 * of the transmission by one state machine cycle
		 */
		prl_tx_set_state(dev, PRL_TX_WAIT_FOR_MESSAGE_REQUEST);
		return;
	} else if (usbc_timer_expired(&prl_tx->pd_t_tx_timeout) ||
		   atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_ERROR)) {
		/*
		 * NOTE: PRL_Tx_Transmission_Error State embedded
		 * here.
		 */
		/* Report Error To Policy Engine */
		pe_report_error(dev, ERR_XMIT, prl_tx->last_xmit_type);
		prl_tx_set_state(dev, PRL_TX_WAIT_FOR_MESSAGE_REQUEST);
		return;
	}
}

/**
 * @brief PRL_Tx_Wait_for_PHY_response Exit State
 */
static void prl_tx_wait_for_phy_response_exit(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;

	usbc_timer_stop(&prl_tx->pd_t_tx_timeout);

	/* Increment messageId counter */
	increment_msgid_counter(dev);
}

#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
/**
 * @brief 6.11.2.2.2.1 PRL_Tx_Src_Source_Tx
 */
static void prl_tx_src_source_tx_entry(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;

	LOG_INF("PRL_Tx_Src_Tx");

	/* Set Rp = SinkTxNG */
	tc_select_src_collision_rp(dev, SINK_TX_NG);
}

static void prl_tx_src_source_tx_run(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;

	if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT)) {
		/*
		 * Don't clear pending XMIT flag here. Wait until we send so
		 * we can detect if we dropped this message or not.
		 */
		prl_tx_set_state(dev, PRL_TX_SRC_PENDING);
	}
}
#endif
#if CONFIG_USBC_CSM_SINK_ONLY
/**
 * @brief PRL_Tx_Snk_Start_of_AMS Entry State
 */
static void prl_tx_snk_start_ams_entry(void *obj)
{
	LOG_INF("PRL_Tx_Snk_Start_of_AMS");
}

/**
 * @brief PRL_Tx_Snk_Start_of_AMS Run State
 */
static void prl_tx_snk_start_ams_run(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;

	if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT)) {
		/*
		 * Don't clear pending XMIT flag here. Wait until we send so
		 * we can detect if we dropped this message or not.
		 */
		prl_tx_set_state(dev, PRL_TX_SNK_PENDING);
	}
}
#endif
#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
/**
 * @brief PRL_Tx_Src_Pending Entry State
 */
static void prl_tx_src_pending_entry(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;

	LOG_INF("PRL_Tx_Src_Pending");

	/* Start SinkTxTimer */
	usbc_timer_start(&prl_tx->pd_t_sink_tx);
}

/**
 * @brief PRL_Tx_Src_Pending Run State
 */
static void prl_tx_src_pending_run(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;

	if (usbc_timer_expired(&prl_tx->pd_t_sink_tx)) {
		/*
		 * We clear the pending XMIT flag here right before we send so
		 * we can detect if we discarded this message or not
		 */
		atomic_clear_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT);

		/* Soft Reset Message pending & SinkTxTimer timeout */
		if ((prl_tx->msg_type == PD_CTRL_SOFT_RESET) && (prl_tx->emsg.len == 0)) {
			prl_tx_set_state(dev, PRL_TX_LAYER_RESET_FOR_TRANSMIT);
		}
		/* Message pending (except Soft Reset) & SinkTxTimer timeout */
		else {
			/* If this is the first AMS message, inform the PE that it's been sent */
			if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_FIRST_MSG_PENDING)) {
				atomic_clear_bit(&prl_tx->flags, PRL_FLAGS_FIRST_MSG_PENDING);
				pe_first_msg_sent(dev);
			}

			prl_tx_construct_message(dev);
			prl_tx_set_state(dev, PRL_TX_WAIT_FOR_PHY_RESPONSE);
		}
	}
}

/**
 * @brief PRL_Tx_Src_Pending Exit State
 */
static void prl_tx_src_pending_exit(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;

	/* Stop SinkTxTimer */
	usbc_timer_stop(&prl_tx->pd_t_sink_tx);
}
#endif

#ifdef CONFIG_USBC_CSM_SINK_ONLY
/**
 * @brief PRL_Tx_Snk_Pending Entry State
 */
static void prl_tx_snk_pending_entry(void *obj)
{
	LOG_INF("PRL_Tx_Snk_Pending");
}

/**
 * @brief PRL_Tx_Snk_Pending Run State
 */
static void prl_tx_snk_pending_run(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;
	enum tc_cc_voltage_state cc1;
	enum tc_cc_voltage_state cc2;

	/*
	 * Wait unit the SRC applies SINK_TX_OK so we can transmit.
	 */
	tcpc_get_cc(tcpc, &cc1, &cc2);

	/*
	 * We clear the pending XMIT flag here right before we send so
	 * we can detect if we discarded this message or not
	 */
	atomic_clear_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT);

	/*
	 * The Protocol Layer Shall transition to the
	 * PRL_Tx_Layer_Reset_for_Transmit state when a Soft_Reset
	 * Message is pending.
	 */
	if ((prl_tx->msg_type == PD_CTRL_SOFT_RESET) && (prl_tx->emsg.len == 0)) {
		prl_tx_set_state(dev, PRL_TX_LAYER_RESET_FOR_TRANSMIT);
	} else if (cc1 == TC_CC_VOLT_RP_3A0 || cc2 == TC_CC_VOLT_RP_3A0) {
		/* If this is the first AMS message, inform the PE that it's been sent */
		if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_FIRST_MSG_PENDING)) {
			atomic_clear_bit(&prl_tx->flags, PRL_FLAGS_FIRST_MSG_PENDING);
			pe_first_msg_sent(dev);
		}

		/*
		 * The Protocol Layer Shall transition to the PRL_Tx_Construct_Message
		 * state when Rp is set to SinkTxOk and a Soft_Reset Message is not
		 * pending.
		 */

		/*
		 * Message pending (except Soft Reset) &
		 * Rp = SinkTxOk
		 */
		prl_tx_construct_message(dev);
		prl_tx_set_state(dev, PRL_TX_WAIT_FOR_PHY_RESPONSE);
	}
}
#endif

static void prl_tx_suspend_entry(void *obj)
{
	LOG_INF("PRL_TX_SUSPEND");
}

static void prl_tx_suspend_run(void *obj)
{
	/* Do nothing */
}

/**
 * All necessary Protocol Hard Reset States (Section 6.12.2.4)
 */

/**
 * @brief PRL_HR_Wait_for_Request Entry State
 *
 * @note This state is not part of the PRL_HR State Diagram found in
 *	 Figure 6-66. The PRL_HR state machine waits here until a
 *	 Hard Reset is requested by either the Policy Engine or the
 *	 PHY Layer.
 */
static void prl_hr_wait_for_request_entry(void *obj)
{
	struct protocol_hard_reset_t *prl_hr = (struct protocol_hard_reset_t *)obj;

	LOG_INF("PRL_HR_Wait_for_Request");

	/* Reset all Protocol Layer Hard Reset flags */
	prl_hr->flags = ATOMIC_INIT(0);
}

/**
 * @brief PRL_HR_Wait_for_Request Run State
 */
static void prl_hr_wait_for_request_run(void *obj)
{
	struct protocol_hard_reset_t *prl_hr = (struct protocol_hard_reset_t *)obj;
	const struct device *dev = prl_hr->dev;

	/*
	 * The PRL_FLAGS_PE_HARD_RESET flag is set when a Hard Reset request is
	 * received from the Policy Engine.
	 *
	 * The PRL_FLAGS_PORT_PARTNER_HARD_RESET flag is set when Hard Reset
	 * signaling is received by the PHY Layer.
	 */
	if (atomic_test_bit(&prl_hr->flags, PRL_FLAGS_PE_HARD_RESET) ||
	    atomic_test_bit(&prl_hr->flags, PRL_FLAGS_PORT_PARTNER_HARD_RESET)) {
		/* Start Hard Reset */
		prl_hr_set_state(dev, PRL_HR_RESET_LAYER);
	}
}

/**
 * @brief PRL_HR_Reset_Layer Entry State
 */
static void prl_hr_reset_layer_entry(void *obj)
{
	struct protocol_hard_reset_t *prl_hr = (struct protocol_hard_reset_t *)obj;
	const struct device *dev = prl_hr->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	const struct device *tcpc = data->tcpc;
	int i;

	LOG_INF("PRL_HR_Reset_Layer");

	/* Reset all Protocol Layer message reception flags */
	prl_rx->flags = ATOMIC_INIT(0);
	/* Reset all Protocol Layer message transmission flags */
	prl_tx->flags = ATOMIC_INIT(0);

	/* Hard reset resets messageIDCounters for all TX types */
	for (i = 0; i < NUM_SOP_STAR_TYPES; i++) {
		prl_rx->msg_id[i] = -1;
		prl_tx->msg_id_counter[i] = 0;
	}

	/* Disable RX */
	tcpc_set_rx_enable(tcpc, false);

	/*
	 * PD r3.0 v2.0, ss6.2.1.1.5:
	 * After a physical or logical (USB Type-C Error Recovery) Attach, a
	 * Port discovers the common Specification Revision level between
	 * itself and its Port Partner and/or the Cable Plug(s), and uses this
	 * Specification Revision level until a Detach, Hard Reset or Error
	 * Recovery happens.
	 *
	 * This covers the Hard Reset case.
	 */
	prl_set_default_pd_revision(dev);

	/*
	 * Protocol Layer message transmission transitions to
	 * PRL_Tx_Wait_For_Message_Request state.
	 */
	prl_tx_set_state(dev, PRL_TX_PHY_LAYER_RESET);

	/*
	 * Protocol Layer message reception transitions to
	 * PRL_Rx_Wait_for_PHY_Message state.
	 *
	 * Note: The PRL_Rx_Wait_for_PHY_Message state is implemented
	 *	 as a single function, named prl_rx_wait_for_phy_message.
	 */

	/*
	 * Protocol Layer reset Complete &
	 * Hard Reset was initiated by Policy Engine
	 */
	if (atomic_test_bit(&prl_hr->flags, PRL_FLAGS_PE_HARD_RESET)) {
		/*
		 * Request PHY to perform a Hard Reset. Note
		 * PRL_HR_Request_Reset state is embedded here.
		 */
		prl_hr_send_msg_to_phy(dev);
		prl_hr_set_state(dev, PRL_HR_WAIT_FOR_PHY_HARD_RESET_COMPLETE);
	} else {
		/*
		 * Protocol Layer reset complete &
		 * Hard Reset was initiated by Port Partner
		 */

		/* Inform Policy Engine of the Hard Reset */
		pe_got_hard_reset(dev);
		prl_hr_set_state(dev, PRL_HR_WAIT_FOR_PE_HARD_RESET_COMPLETE);
	}
}

/**
 * @brief PRL_HR_Wait_for_PHY_Hard_Reset_Complete Entry State
 */
static void prl_hr_wait_for_phy_hard_reset_complete_entry(void *obj)
{
	struct protocol_hard_reset_t *prl_hr = (struct protocol_hard_reset_t *)obj;

	LOG_INF("PRL_HR_Wait_for_PHY_Hard_Reset_Complete");

	/*
	 * Start the HardResetCompleteTimer and wait for the PHY Layer to
	 * indicate that the Hard Reset completed.
	 */
	usbc_timer_start(&prl_hr->pd_t_hard_reset_complete);
}

/**
 * @brief PRL_HR_Wait_for_PHY_Hard_Reset_Complete Run State
 */
static void prl_hr_wait_for_phy_hard_reset_complete_run(void *obj)
{
	struct protocol_hard_reset_t *prl_hr = (struct protocol_hard_reset_t *)obj;
	const struct device *dev = prl_hr->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;

	/*
	 * Wait for hard reset from PHY or timeout
	 */
	if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_COMPLETE) ||
	    usbc_timer_expired(&prl_hr->pd_t_hard_reset_complete)) {
		/* PRL_HR_PHY_Hard_Reset_Requested */
		/* Inform Policy Engine Hard Reset was sent */
		pe_hard_reset_sent(dev);
		prl_hr_set_state(dev, PRL_HR_WAIT_FOR_PE_HARD_RESET_COMPLETE);
	}
}

/**
 * @brief PRL_HR_Wait_for_PHY_Hard_Reset_Complete Exit State
 */
static void prl_hr_wait_for_phy_hard_reset_complete_exit(void *obj)
{
	struct protocol_hard_reset_t *prl_hr = (struct protocol_hard_reset_t *)obj;

	/* Stop the HardResetCompleteTimer */
	usbc_timer_stop(&prl_hr->pd_t_hard_reset_complete);
}

/**
 * @brief PRL_HR_Wait_For_PE_Hard_Reset_Complete Entry State
 */
static void prl_hr_wait_for_pe_hard_reset_complete_entry(void *obj)
{
	LOG_INF("PRL_HR_Wait_For_PE_Hard_Reset_Complete");
}

/**
 * @brief PRL_HR_Wait_For_PE_Hard_Reset_Complete Run State
 */
static void prl_hr_wait_for_pe_hard_reset_complete_run(void *obj)
{
	struct protocol_hard_reset_t *prl_hr = (struct protocol_hard_reset_t *)obj;
	const struct device *dev = prl_hr->dev;

	/* Wait for Hard Reset complete indication from Policy Engine */
	if (atomic_test_bit(&prl_hr->flags, PRL_FLAGS_HARD_RESET_COMPLETE)) {
		prl_hr_set_state(dev, PRL_HR_WAIT_FOR_REQUEST);
	}
}

static void prl_hr_suspend_entry(void *obj)
{
	LOG_INF("PRL_HR_SUSPEND");
}

static void prl_hr_suspend_run(void *obj)
{
	/* Do nothing */
}

/**
 * @brief This function implements both the Protocol Layer Message Reception
 *	  State Machine. See Figure 6-55 Protocol layer Message reception
 *
 *	  The states of the two state machines can be identified by the
 *	  comments preceded by a NOTE: <state name>
 */
static void prl_rx_wait_for_phy_message(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct pd_msg *rx_emsg = &prl_rx->emsg;
	const struct device *tcpc = data->tcpc;
	uint8_t msg_type;
	uint8_t pkt_type;
	uint8_t ext;
	int8_t msid;
	uint8_t num_data_objs;
	uint8_t power_role;

	/* Get the message */
	if (tcpc_receive_data(tcpc, rx_emsg) <= 0) {
		/* No pending message or problem getting the message */
		return;
	}

	num_data_objs = rx_emsg->header.number_of_data_objects;
	msid = rx_emsg->header.message_id;
	msg_type = rx_emsg->header.message_type;
	ext = rx_emsg->header.extended;
	pkt_type = rx_emsg->type;
	power_role = rx_emsg->header.port_power_role;

	/* Dump the received packet content, except for Pings */
	if (msg_type != PD_CTRL_PING) {
		int p;

		LOG_INF("RECV %04x/%d ", rx_emsg->header.raw_value, num_data_objs);
		for (p = 0; p < num_data_objs; p++) {
			LOG_INF("\t[%d]%08x ", p, *((uint32_t *)rx_emsg->data + p));
		}
	}

	/* Ignore messages sent to the cable from our port partner */
	if (pkt_type != PD_PACKET_SOP && power_role == PD_PLUG_FROM_DFP_UFP) {
		return;
	}

	/* Soft Reset Message received from PHY */
	if (num_data_objs == 0 && msg_type == PD_CTRL_SOFT_RESET) {
		/* NOTE: PRL_Rx_Layer_Reset_for_Receive State embedded here */

		/* Reset MessageIdCounter */
		prl_tx->msg_id_counter[pkt_type] = 0;

		/* Clear stored MessageID value */
		prl_rx->msg_id[pkt_type] = -1;

		/*
		 * Protocol Layer message transmission transitions to
		 * PRL_Tx_PHY_Layer_Reset state
		 */
		prl_tx_set_state(dev, PRL_TX_PHY_LAYER_RESET);

		/*
		 * Inform Policy Engine of Soft Reset. Note perform this after
		 * performing the protocol layer reset, otherwise we will lose
		 * the PE's outgoing ACCEPT message to the soft reset.
		 */
		pe_got_soft_reset(dev);
		return;
	}

	/* Ignore if this is a duplicate message. Stop processing */
	if (prl_rx->msg_id[pkt_type] == msid) {
		return;
	}

	/*
	 * Discard any pending TX message if this RX message is from SOP,
	 * except for ping messages.
	 */

	/* Check if message transmit is pending */
	if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT)) {
		/* Don't discard message if a PING was received */
		if ((num_data_objs > 0) || (msg_type != PD_CTRL_PING)) {
			/* Only discard message if received from SOP */
			if (pkt_type == PD_PACKET_SOP) {
				atomic_set_bit(&prl_tx->flags, PRL_FLAGS_TX_DISCARDED);
			}
		}
	}

	/* Store Message Id */
	prl_rx->msg_id[pkt_type] = msid;

	/* Pass message to Policy Engine */
	pe_message_received(dev);
}

/**
 * @brief Protocol Layer Transmit State table
 */
static const struct smf_state prl_tx_states[PRL_TX_STATE_COUNT] = {
	[PRL_TX_PHY_LAYER_RESET] = SMF_CREATE_STATE(
		prl_tx_phy_layer_reset_entry,
		NULL,
		NULL,
		NULL),
	[PRL_TX_WAIT_FOR_MESSAGE_REQUEST] = SMF_CREATE_STATE(
		prl_tx_wait_for_message_request_entry,
		prl_tx_wait_for_message_request_run,
		NULL,
		NULL),
	[PRL_TX_LAYER_RESET_FOR_TRANSMIT] = SMF_CREATE_STATE(
		prl_tx_layer_reset_for_transmit_entry,
		NULL,
		NULL,
		NULL),
	[PRL_TX_WAIT_FOR_PHY_RESPONSE] = SMF_CREATE_STATE(
		prl_tx_wait_for_phy_response_entry,
		prl_tx_wait_for_phy_response_run,
		prl_tx_wait_for_phy_response_exit,
		NULL),
	[PRL_TX_SUSPEND] = SMF_CREATE_STATE(
		prl_tx_suspend_entry,
		prl_tx_suspend_run,
		NULL,
		NULL),
#ifdef CONFIG_USBC_CSM_SINK_ONLY
	[PRL_TX_SNK_START_AMS] = SMF_CREATE_STATE(
		prl_tx_snk_start_ams_entry,
		prl_tx_snk_start_ams_run,
		NULL,
		NULL),
	[PRL_TX_SNK_PENDING] = SMF_CREATE_STATE(
		prl_tx_snk_pending_entry,
		prl_tx_snk_pending_run,
		NULL,
		NULL),
#endif
#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
	[PRL_TX_SRC_SOURCE_TX] = SMF_CREATE_STATE(
		prl_tx_src_source_tx_entry,
		prl_tx_src_source_tx_run,
		NULL,
		NULL),
	[PRL_TX_SRC_PENDING] = SMF_CREATE_STATE(
		prl_tx_src_pending_entry,
		prl_tx_src_pending_run,
		prl_tx_src_pending_exit,
		NULL),
#endif
};
BUILD_ASSERT(ARRAY_SIZE(prl_tx_states) == PRL_TX_STATE_COUNT);

/**
 * @brief Protocol Layer Hard Reset State table
 */
static const struct smf_state prl_hr_states[PRL_HR_STATE_COUNT] = {
	[PRL_HR_WAIT_FOR_REQUEST] = SMF_CREATE_STATE(
		prl_hr_wait_for_request_entry,
		prl_hr_wait_for_request_run,
		NULL,
		NULL),
	[PRL_HR_RESET_LAYER] = SMF_CREATE_STATE(
		prl_hr_reset_layer_entry,
		NULL,
		NULL,
		NULL),
	[PRL_HR_WAIT_FOR_PHY_HARD_RESET_COMPLETE] = SMF_CREATE_STATE(
		prl_hr_wait_for_phy_hard_reset_complete_entry,
		prl_hr_wait_for_phy_hard_reset_complete_run,
		prl_hr_wait_for_phy_hard_reset_complete_exit,
		NULL),
	[PRL_HR_WAIT_FOR_PE_HARD_RESET_COMPLETE] = SMF_CREATE_STATE(
		prl_hr_wait_for_pe_hard_reset_complete_entry,
		prl_hr_wait_for_pe_hard_reset_complete_run,
		NULL,
		NULL),
	[PRL_HR_SUSPEND] = SMF_CREATE_STATE(
		prl_hr_suspend_entry,
		prl_hr_suspend_run,
		NULL,
		NULL),
};
BUILD_ASSERT(ARRAY_SIZE(prl_hr_states) == PRL_HR_STATE_COUNT);
