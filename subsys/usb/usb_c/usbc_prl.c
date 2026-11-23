/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(usbc_stack, CONFIG_USBC_STACK_LOG_LEVEL);

#include "usbc_stack.h"
#include "usbc_config.h"

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
	/** Flag to note a message has been received */
	PRL_FLAGS_RX_COMPLETE = 10,
	/**
	 * Flag to note the pending transmission belongs to the chunking
	 * handlers. Chunks and Chunk Requests are internal to them, so the
	 * Policy Engine must not be told each one was sent; only the handler
	 * knows when the whole Extended Message is done.
	 */
	PRL_FLAGS_TX_CHUNK = 11,
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

#ifdef CONFIG_USBC_CHUNKING
/**
 * @brief Chunked Rx Handler Flags
 */
enum rch_flags {
	/** A chunk arrived from the Protocol Layer */
	RCH_FLAGS_MSG_RECEIVED = 0,
	/** Reassembly failed */
	RCH_FLAGS_ERROR = 1,
};

/**
 * @brief Chunked Tx Handler Flags
 */
enum tch_flags {
	/** The Policy Engine staged an Extended Message */
	TCH_FLAGS_MSG_REQUESTED = 0,
	/** A Chunk Request arrived from the receiver */
	TCH_FLAGS_CHUNK_REQUEST = 1,
};

/**
 * @brief Chunked Tx Handler States
 *	  See Figure 6-55 Chunked Tx Handler State Diagram
 */
enum usbc_prl_tch_state_t {
	/** TCH_Wait_For_Message_Request_From_Policy_Engine */
	TCH_WAIT_FOR_MESSAGE_REQUEST_FROM_PE,
	/** TCH_Construct_Chunked_Message */
	TCH_CONSTRUCT_CHUNKED_MESSAGE,
	/** TCH_Sending_Chunked_Message */
	TCH_SENDING_CHUNKED_MESSAGE,
	/** TCH_Wait_Chunk_Request */
	TCH_WAIT_CHUNK_REQUEST,
	/** TCH_Message_Sent */
	TCH_MESSAGE_SENT,
	/** TCH_Report_Error */
	TCH_REPORT_ERROR,

	/** Number of TCH States */
	TCH_STATE_COUNT
};

/**
 * @brief Chunked Rx Handler States
 *	  See Figure 6-56 Chunked Rx Handler State Diagram
 */
enum usbc_prl_rch_state_t {
	/** RCH_Wait_For_Message_From_Protocol_Layer */
	RCH_WAIT_FOR_MESSAGE_FROM_PROTOCOL_LAYER,
	/** RCH_Processing_Extended_Message */
	RCH_PROCESSING_EXTENDED_MESSAGE,
	/** RCH_Requesting_Chunk */
	RCH_REQUESTING_CHUNK,
	/** RCH_Waiting_Chunk */
	RCH_WAITING_CHUNK,
	/** RCH_Report_Error */
	RCH_REPORT_ERROR,

	/** Number of RCH States */
	RCH_STATE_COUNT
};
#endif /* CONFIG_USBC_CHUNKING */

static const struct smf_state prl_tx_states[PRL_TX_STATE_COUNT];
static const struct smf_state prl_hr_states[PRL_HR_STATE_COUNT];
#ifdef CONFIG_USBC_CHUNKING
static const struct smf_state prl_rch_states[RCH_STATE_COUNT];
static void prl_rch_set_state(const struct device *dev, const enum usbc_prl_rch_state_t state);
static const struct smf_state prl_tch_states[TCH_STATE_COUNT];
static void prl_tch_set_state(const struct device *dev, const enum usbc_prl_tch_state_t state);
#endif

#ifdef CONFIG_USBC_LOG_PD_MSG_NAMES
/**
 * @brief Human-readable names for PD Control Message types.
 *	  Names match Table 6.4 Control Message Types in the PD specification.
 */
static const char *const pd_ctrl_msg_names[PD_CTRL_MSG_COUNT] = {
	[PD_CTRL_GOOD_CRC] = "GoodCRC",
	[PD_CTRL_GOTO_MIN] = "GotoMin",
	[PD_CTRL_ACCEPT] = "Accept",
	[PD_CTRL_REJECT] = "Reject",
	[PD_CTRL_PING] = "Ping",
	[PD_CTRL_PS_RDY] = "PS_RDY",
	[PD_CTRL_GET_SOURCE_CAP] = "Get_Source_Cap",
	[PD_CTRL_GET_SINK_CAP] = "Get_Sink_Cap",
	[PD_CTRL_DR_SWAP] = "DR_Swap",
	[PD_CTRL_PR_SWAP] = "PR_Swap",
	[PD_CTRL_VCONN_SWAP] = "VCONN_Swap",
	[PD_CTRL_WAIT] = "Wait",
	[PD_CTRL_SOFT_RESET] = "Soft_Reset",
	[PD_CTRL_DATA_RESET] = "Data_Reset",
	[PD_CTRL_DATA_RESET_COMPLETE] = "Data_Reset_Complete",
	[PD_CTRL_NOT_SUPPORTED] = "Not_Supported",
	[PD_CTRL_GET_SOURCE_CAP_EXT] = "Get_Source_Cap_Extended",
	[PD_CTRL_GET_STATUS] = "Get_Status",
	[PD_CTRL_FR_SWAP] = "FR_Swap",
	[PD_CTRL_GET_PPS_STATUS] = "Get_PPS_Status",
	[PD_CTRL_GET_COUNTRY_CODES] = "Get_Country_Codes",
	[PD_CTRL_GET_SINK_CAP_EXT] = "Get_Sink_Cap_Extended",
	[PD_CTRL_GET_SOURCE_INFO] = "Get_Source_Info",
	[PD_CTRL_GET_REVISION] = "Get_Revision",
};
BUILD_ASSERT(ARRAY_SIZE(pd_ctrl_msg_names) == PD_CTRL_MSG_COUNT,
	     "pd_ctrl_msg_names array size mismatch with pd_ctrl_msg_type enum");

/**
 * @brief Human-readable names for PD Data Message types.
 *	  Names match Table 6.5 Data Message Types in the PD specification.
 */
static const char *const pd_data_msg_names[PD_DATA_MSG_COUNT] = {
	[PD_DATA_SOURCE_CAP] = "Source_Capabilities",
	[PD_DATA_REQUEST] = "Request",
	[PD_DATA_BIST] = "BIST",
	[PD_DATA_SINK_CAP] = "Sink_Capabilities",
	[PD_DATA_BATTERY_STATUS] = "Battery_Status",
	[PD_DATA_ALERT] = "Alert",
	[PD_DATA_GET_COUNTRY_INFO] = "Get_Country_Info",
	[PD_DATA_ENTER_USB] = "Enter_USB",
	[PD_DATA_EPR_REQUEST] = "EPR_Request",
	[PD_DATA_EPR_MODE] = "EPR_Mode",
	[PD_DATA_SOURCE_INFO] = "Source_Info",
	[PD_DATA_REVISION] = "Revision",
	[PD_DATA_VENDOR_DEF] = "Vendor_Defined",
};
BUILD_ASSERT(ARRAY_SIZE(pd_data_msg_names) == PD_DATA_MSG_COUNT,
	     "pd_data_msg_names array size mismatch with pd_data_msg_type enum");

/**
 * @brief Human-readable names for PD Extended Message types.
 *	  Names match Table 6.47 Extended Message Types in the PD specification.
 */
static const char *const pd_ext_msg_names[PD_EXT_MSG_COUNT] = {
	[PD_EXT_SOURCE_CAP] = "Source_Capabilities_Extended",
	[PD_EXT_STATUS] = "Status",
	[PD_EXT_GET_BATTERY_CAP] = "Get_Battery_Cap",
	[PD_EXT_GET_BATTERY_STATUS] = "Get_Battery_Status",
	[PD_EXT_BATTERY_CAP] = "Battery_Capabilities",
	[PD_EXT_GET_MANUFACTURER_INFO] = "Get_Manufacturer_Info",
	[PD_EXT_MANUFACTURER_INFO] = "Manufacturer_Info",
	[PD_EXT_SECURITY_REQUEST] = "Security_Request",
	[PD_EXT_SECURITY_RESPONSE] = "Security_Response",
	[PD_EXT_FIRMWARE_UPDATE_REQUEST] = "Firmware_Update_Request",
	[PD_EXT_FIRMWARE_UPDATE_RESPONSE] = "Firmware_Update_Response",
	[PD_EXT_PPS_STATUS] = "PPS_Status",
	[PD_EXT_COUNTRY_INFO] = "Country_Info",
	[PD_EXT_COUNTRY_CODES] = "Country_Codes",
	[PD_EXT_SINK_CAP] = "Sink_Capabilities_Extended",
	[PD_EXT_EXTENDED_CONTROL] = "Extended_Control",
	[PD_EXT_EPR_SOURCE_CAP] = "EPR_Source_Capabilities",
	[PD_EXT_EPR_SINK_CAP] = "EPR_Sink_Capabilities",
	[PD_EXT_VENDOR_DEFINED] = "Vendor_Defined_Extended",
};
BUILD_ASSERT(ARRAY_SIZE(pd_ext_msg_names) == PD_EXT_MSG_COUNT,
	     "pd_ext_msg_names array size mismatch with pd_ext_msg_type enum");
#endif /* CONFIG_USBC_LOG_PD_MSG_NAMES */

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

#ifdef CONFIG_USBC_CHUNKING
	data->prl_rch->dev = dev;
	usbc_timer_init(&data->prl_rch->pd_t_chunk_sender_response,
			PD_T_CHUNK_SENDER_RESPONSE_NOM_MS);
	smf_set_initial(SMF_CTX(data->prl_rch),
			&prl_rch_states[RCH_WAIT_FOR_MESSAGE_FROM_PROTOCOL_LAYER]);

	data->prl_tch->dev = dev;
	usbc_timer_init(&data->prl_tch->pd_t_chunk_sender_request,
			PD_T_CHUNK_SENDER_REQUEST_NOM_MS);
	smf_set_initial(SMF_CTX(data->prl_tch),
			&prl_tch_states[TCH_WAIT_FOR_MESSAGE_REQUEST_FROM_PE]);
#endif
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
			if (atomic_test_and_clear_bit(&data->prl_rx->flags,
						      PRL_FLAGS_RX_COMPLETE)) {
				prl_rx_wait_for_phy_message(dev);
			}

#ifdef CONFIG_USBC_CHUNKING
			/* Run the chunking handlers */
			smf_run_state(SMF_CTX(data->prl_rch));
			smf_run_state(SMF_CTX(data->prl_tch));
#endif

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
 * @brief Get the human-readable name of a PD message, selecting the
 *	  Control, Data, or Extended lookup table based on the header fields.
 *
 * @param msg Message to get the name of
 * @return String name of the message type
 */
static const char *pd_msg_name(const struct pd_msg *msg)
{
#ifdef CONFIG_USBC_LOG_PD_MSG_NAMES
	const uint8_t msg_type = msg->header.message_type;

	if (msg->header.extended) {
		/* Extended Message */
		return (msg_type < ARRAY_SIZE(pd_ext_msg_names) && pd_ext_msg_names[msg_type])
			       ? pd_ext_msg_names[msg_type]
			       : "Unknown Extended message";
	} else if (msg->header.number_of_data_objects > 0) {
		/* Data Message */
		return (msg_type < ARRAY_SIZE(pd_data_msg_names) && pd_data_msg_names[msg_type])
			       ? pd_data_msg_names[msg_type]
			       : "Unknown Data message";
	}
	/* Control Message */
	return (msg_type < ARRAY_SIZE(pd_ctrl_msg_names) && pd_ctrl_msg_names[msg_type])
		       ? pd_ctrl_msg_names[msg_type]
		       : "Unknown Control message";
#else
	ARG_UNUSED(msg);

	return "";
#endif /* CONFIG_USBC_LOG_PD_MSG_NAMES */
}

/**
 * @brief Alert Handler called by the TCPC driver
 */
static void alert_handler(const struct device *tcpc, void *port_dev, enum tcpc_alert alert)
{
	const struct device *dev = (const struct device *)port_dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;
	struct protocol_hard_reset_t *prl_hr = data->prl_hr;

	switch (alert) {
	case TCPC_ALERT_MSG_STATUS:
		atomic_set_bit(&prl_rx->flags, PRL_FLAGS_RX_COMPLETE);
		break;
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
static uint16_t get_sop_star_header(const struct device *dev)
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

	/*
	 * A caller marks an Extended Message by setting the extended bit in
	 * emsg.header before queueing it. Every other field is regenerated
	 * here, so that bit is the only part of the caller's header that
	 * survives. emsg.data then holds the two byte Extended Message Header
	 * followed by the payload, and emsg.len covers both.
	 */
	header.extended = prl_tx->emsg.header.extended;

	if (header.extended) {
		/*
		 * Extended Messages are padded out to a whole number of data
		 * objects. tcpci_tcpm_transmit_data() takes its byte count
		 * from this field rather than from emsg.len, so the padding
		 * has to be counted here and the bytes themselves zeroed.
		 */
		uint32_t objects = DIV_ROUND_UP(prl_tx->emsg.len, sizeof(uint32_t));
		uint32_t padded = objects * sizeof(uint32_t);

		if (padded > prl_tx->emsg.len && padded <= sizeof(prl_tx->emsg.data)) {
			memset(&prl_tx->emsg.data[prl_tx->emsg.len], 0, padded - prl_tx->emsg.len);
		}

		header.number_of_data_objects = objects;
	} else {
		header.number_of_data_objects =
			PD_CONVERT_BYTES_TO_PD_HEADER_COUNT(prl_tx->emsg.len);
	}

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
	uint8_t num_data_objs;
	int i;

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

	num_data_objs = prl_tx->emsg.header.number_of_data_objects;

	/* Log the outgoing message */
	LOG_INF("SEND 0x%04x/%d[%s]", prl_tx->emsg.header.raw_value, num_data_objs,
		pd_msg_name(&prl_tx->emsg));
	for (i = 0; i < num_data_objs; i++) {
		LOG_INF("\t[%d]%08X ", i, *((uint32_t *)prl_tx->emsg.data + i));
	}

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
	atomic_clear(&data->prl_tx->flags);

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
	atomic_clear(&prl_hr->flags);
	usbc_timer_init(&prl_hr->pd_t_hard_reset_complete, PD_T_HARD_RESET_COMPLETE_MAX_MS);
	prl_hr_set_state(dev, PRL_HR_WAIT_FOR_REQUEST);

	/* Initialize the PRL_TX state machine */
	atomic_clear(&prl_tx->flags);
	prl_tx->last_xmit_type = PD_PACKET_SOP;
	for (i = 0; i < NUM_SOP_STAR_TYPES; i++) {
		prl_tx->msg_id_counter[i] = 0;
	}
	usbc_timer_init(&prl_tx->pd_t_tx_timeout, PD_T_TX_TIMEOUT_MS);
	usbc_timer_init(&prl_tx->pd_t_sink_tx, PD_T_SINK_TX_MAX_MS);
	prl_tx_set_state(dev, PRL_TX_PHY_LAYER_RESET);

	/* Initialize the PRL_RX state machine */
	atomic_clear(&prl_rx->flags);
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
	atomic_clear(&prl_tx->flags);
}

/**
 * @brief PRL_Tx_Wait_for_Message_Request Run State
 */
static enum smf_state_result prl_tx_wait_for_message_request_run(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;
	struct usbc_port_data *data = dev->data;

	/* Clear any AMS flags and state if we are no longer in an AMS */
	if (pe_dpm_initiated_ams(dev) == false) {
		if (IS_ENABLED(CONFIG_USBC_CSM_SUPPORTS_SOURCE)) {
			/* Note PRL_Tx_Src_Sink_Tx is embedded here. */
			if (atomic_test_and_clear_bit(&prl_tx->flags, PRL_FLAGS_SINK_NG)) {
				tc_select_src_collision_rp(dev, SINK_TX_OK);
			}
		}
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
			if (IS_ENABLED(CONFIG_USBC_CSM_SUPPORTS_SOURCE) &&
			    pe_get_power_role(dev) == TC_ROLE_SOURCE) {
				atomic_set_bit(&prl_tx->flags, PRL_FLAGS_SINK_NG);
				prl_tx_set_state(dev, PRL_TX_SRC_SOURCE_TX);
			} else {
				atomic_set_bit(&prl_tx->flags, PRL_FLAGS_WAIT_SINK_OK);
				prl_tx_set_state(dev, PRL_TX_SNK_START_AMS);
			}
			return SMF_EVENT_PROPAGATE;
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
		return SMF_EVENT_PROPAGATE;
	}
	return SMF_EVENT_PROPAGATE;
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
static enum smf_state_result prl_tx_wait_for_phy_response_run(void *obj)
{
	struct protocol_layer_tx_t *prl_tx = (struct protocol_layer_tx_t *)obj;
	const struct device *dev = prl_tx->dev;

	/* Wait until TX is complete */
	if (atomic_test_and_clear_bit(&prl_tx->flags, PRL_FLAGS_TX_DISCARDED)) {
		/* NOTE: PRL_TX_DISCARD_MESSAGE State embedded here. */
		/* Inform Policy Engine Message was discarded */
		pe_report_discard(dev);
		prl_tx_set_state(dev, PRL_TX_PHY_LAYER_RESET);
		return SMF_EVENT_PROPAGATE;
	}
	if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_COMPLETE)) {
		/* NOTE: PRL_TX_Message_Sent State embedded here. */
		/* Inform Policy Engine Message was sent */
		if (!IS_ENABLED(CONFIG_USBC_CHUNKING) ||
		    !atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_CHUNK)) {
			pe_message_sent(dev);
		}
		/*
		 * This event reduces the time of informing the policy engine
		 * of the transmission by one state machine cycle
		 */
		prl_tx_set_state(dev, PRL_TX_WAIT_FOR_MESSAGE_REQUEST);
		return SMF_EVENT_PROPAGATE;
	} else if (usbc_timer_expired(&prl_tx->pd_t_tx_timeout) ||
		   atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_ERROR)) {
		/*
		 * NOTE: PRL_Tx_Transmission_Error State embedded
		 * here.
		 */
		/* Report Error To Policy Engine */
		pe_report_error(dev, ERR_XMIT, prl_tx->last_xmit_type);
		prl_tx_set_state(dev, PRL_TX_WAIT_FOR_MESSAGE_REQUEST);
		return SMF_EVENT_PROPAGATE;
	}
	return SMF_EVENT_PROPAGATE;
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

#ifdef CONFIG_USBC_CSM_SUPPORTS_SOURCE
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

static enum smf_state_result prl_tx_src_source_tx_run(void *obj)
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
	return SMF_EVENT_PROPAGATE;
}
#endif
#ifdef CONFIG_USBC_CSM_SUPPORTS_SINK
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
static enum smf_state_result prl_tx_snk_start_ams_run(void *obj)
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
	return SMF_EVENT_PROPAGATE;
}
#endif
#ifdef CONFIG_USBC_CSM_SUPPORTS_SOURCE
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
static enum smf_state_result prl_tx_src_pending_run(void *obj)
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
	return SMF_EVENT_PROPAGATE;
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

#ifdef CONFIG_USBC_CSM_SUPPORTS_SINK
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
static enum smf_state_result prl_tx_snk_pending_run(void *obj)
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
	return SMF_EVENT_PROPAGATE;
}
#endif

static void prl_tx_suspend_entry(void *obj)
{
	LOG_INF("PRL_TX_SUSPEND");
}

static enum smf_state_result prl_tx_suspend_run(void *obj)
{
	/* Do nothing */
	return SMF_EVENT_PROPAGATE;
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
	atomic_clear(&prl_hr->flags);
}

/**
 * @brief PRL_HR_Wait_for_Request Run State
 */
static enum smf_state_result prl_hr_wait_for_request_run(void *obj)
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
	return SMF_EVENT_PROPAGATE;
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
	atomic_clear(&prl_rx->flags);
	/* Reset all Protocol Layer message transmission flags */
	atomic_clear(&prl_tx->flags);

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
static enum smf_state_result prl_hr_wait_for_phy_hard_reset_complete_run(void *obj)
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
	return SMF_EVENT_PROPAGATE;
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
static enum smf_state_result prl_hr_wait_for_pe_hard_reset_complete_run(void *obj)
{
	struct protocol_hard_reset_t *prl_hr = (struct protocol_hard_reset_t *)obj;
	const struct device *dev = prl_hr->dev;

	/* Wait for Hard Reset complete indication from Policy Engine */
	if (atomic_test_bit(&prl_hr->flags, PRL_FLAGS_HARD_RESET_COMPLETE)) {
		prl_hr_set_state(dev, PRL_HR_WAIT_FOR_REQUEST);
	}
	return SMF_EVENT_PROPAGATE;
}

static void prl_hr_suspend_entry(void *obj)
{
	LOG_INF("PRL_HR_SUSPEND");
}

static enum smf_state_result prl_hr_suspend_run(void *obj)
{
	/* Do nothing */
	return SMF_EVENT_PROPAGATE;
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
	int8_t msid;
	uint8_t num_data_objs;
	uint8_t power_role;
	int i;

	/* Get the message */
	if (tcpc_get_rx_pending_msg(tcpc, rx_emsg) <= 0) {
		/* No pending message or problem getting the message */
		return;
	}

	num_data_objs = rx_emsg->header.number_of_data_objects;
	msid = rx_emsg->header.message_id;
	msg_type = rx_emsg->header.message_type;
	pkt_type = rx_emsg->type;
	power_role = rx_emsg->header.port_power_role;

	/* Dump the received packet content */
	LOG_INF("RECV 0x%04x/%d[%s]", rx_emsg->header.raw_value, num_data_objs,
		pd_msg_name(rx_emsg));
	for (i = 0; i < num_data_objs; i++) {
		LOG_INF("\t[%d]%08X ", i, *((uint32_t *)rx_emsg->data + i));
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

#ifdef CONFIG_USBC_CHUNKING
	/*
	 * Every Extended Message goes to the Chunked Rx Handler, including one
	 * that arrives complete in a single chunk. The handler strips the
	 * Extended Message Header and passes the payload up, so a consumer
	 * sees the same shape however many chunks were involved.
	 */
	if (rx_emsg->header.extended) {
		union pd_ext_header ext_header;

		ext_header.raw_value = sys_get_le16(rx_emsg->data);

		if (ext_header.chunked && ext_header.request_chunk) {
			/* A Chunk Request for a message we are sending. */
			data->prl_tch->chunk_number_requested = ext_header.chunk_number;
			atomic_set_bit(&data->prl_tch->flags, TCH_FLAGS_CHUNK_REQUEST);

			/* As above: this return skips pe_message_received(). */
			usbc_bypass_next_sleep(dev);
			return;
		}

		atomic_set_bit(&data->prl_rch->flags, RCH_FLAGS_MSG_RECEIVED);

		/*
		 * Returning here skips pe_message_received(), which is where a
		 * normal message asks the stack for one more lap. Without this
		 * every chunk would wait out a full
		 * CONFIG_USBC_STATE_MACHINE_CYCLE_TIME before being looked at.
		 */
		usbc_bypass_next_sleep(dev);
		return;
	}
#endif /* CONFIG_USBC_CHUNKING */

	/* Pass message to Policy Engine */
	pe_message_received(dev);
}

#ifdef CONFIG_USBC_CHUNKING
/**
 * @brief Payload bytes carried by a chunk, excluding the Extended Message Header
 */
static uint32_t rch_chunk_payload_len(const struct pd_msg *msg)
{
	if (msg->len < sizeof(uint16_t)) {
		return 0;
	}

	return MIN(msg->len - sizeof(uint16_t), PD_MAX_EXTENDED_MSG_CHUNK_LEN);
}

/**
 * @brief Queue an Extended Message chunk on the Tx state machine.
 *
 * Shared by both chunking handlers so that all chunk traffic is marked with
 * PRL_FLAGS_TX_CHUNK and stays invisible to the Policy Engine.
 *
 * @param ext_header Extended Message Header for this chunk
 * @param payload Chunk payload, or NULL for a header-only Chunk Request
 * @param payload_len Number of payload bytes
 */
static void prl_chunk_queue_tx(const struct device *dev, enum pd_packet_type pkt_type,
			       uint8_t msg_type, union pd_ext_header ext_header,
			       const uint8_t *payload, uint32_t payload_len)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;

	prl_tx->emsg.type = pkt_type;
	prl_tx->emsg.header.raw_value = 0;
	prl_tx->emsg.header.extended = 1;
	sys_put_le16(ext_header.raw_value, prl_tx->emsg.data);

	if (payload_len > 0) {
		memcpy(&prl_tx->emsg.data[sizeof(uint16_t)], payload, payload_len);
	}

	prl_tx->emsg.len = sizeof(uint16_t) + payload_len;
	prl_tx->msg_type = msg_type;

	atomic_set_bit(&prl_tx->flags, PRL_FLAGS_TX_CHUNK);
	atomic_set_bit(&prl_tx->flags, PRL_FLAGS_MSG_XMIT);
}

/**
 * @brief Queue a Chunk Request for the next chunk.
 *
 * A Chunk Request is an Extended Message carrying nothing but its Extended
 * Message Header, with Request Chunk set and Data Size zero.
 */
static void rch_send_chunk_request(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rch_t *rch = data->prl_rch;
	union pd_ext_header ext_header = {
		.chunked = 1,
		.request_chunk = 1,
		.chunk_number = rch->chunk_number_expected,
		.data_size = 0,
	};

	prl_chunk_queue_tx(dev, rch->pkt_type, rch->msg_type, ext_header, NULL, 0);
}

static void rch_wait_for_message_from_protocol_layer_entry(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;

	rch->chunk_number_expected = 0;
	rch->num_bytes_received = 0;
	atomic_clear_bit(&rch->flags, RCH_FLAGS_ERROR);
}

static enum smf_state_result rch_wait_for_message_from_protocol_layer_run(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;

	if (atomic_test_and_clear_bit(&rch->flags, RCH_FLAGS_MSG_RECEIVED)) {
		prl_rch_set_state(rch->dev, RCH_PROCESSING_EXTENDED_MESSAGE);
	}

	return SMF_EVENT_PROPAGATE;
}

/**
 * @brief Fold one chunk into the reassembly buffer and decide what comes next.
 */
static void rch_processing_extended_message_entry(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;
	const struct device *dev = rch->dev;
	struct usbc_port_data *data = dev->data;
	struct pd_msg *rx = &data->prl_rx->emsg;
	union pd_ext_header ext_header;
	uint32_t payload_len;

	ext_header.raw_value = sys_get_le16(rx->data);

	if (ext_header.chunk_number != rch->chunk_number_expected) {
		LOG_ERR("RCH: chunk %u arrived, expected %u", ext_header.chunk_number,
			rch->chunk_number_expected);
		prl_rch_set_state(dev, RCH_REPORT_ERROR);
		return;
	}

	if (ext_header.data_size > PD_MAX_EXTENDED_MSG_LEN) {
		LOG_ERR("RCH: data size %u exceeds MaxExtendedMsgLen", ext_header.data_size);
		prl_rch_set_state(dev, RCH_REPORT_ERROR);
		return;
	}

	payload_len = rch_chunk_payload_len(rx);

	/* Do not run off the end of the reassembly buffer on a malformed chunk. */
	if (rch->num_bytes_received + payload_len > sizeof(rch->emsg.data)) {
		LOG_ERR("RCH: reassembly would overflow at %u bytes", rch->num_bytes_received);
		prl_rch_set_state(dev, RCH_REPORT_ERROR);
		return;
	}

	if (rch->chunk_number_expected == 0) {
		/* First chunk fixes the message identity for the whole transfer. */
		rch->msg_type = rx->header.message_type;
		rch->pkt_type = rx->type;
		rch->emsg.type = rx->type;
		rch->emsg.header = rx->header;
	}

	memcpy(&rch->emsg.data[rch->num_bytes_received], &rx->data[sizeof(uint16_t)], payload_len);
	rch->num_bytes_received += payload_len;
	rch->chunk_number_expected++;

	if (rch->num_bytes_received >= ext_header.data_size) {
		/*
		 * Complete. Hand the payload up in prl_rx->emsg without the
		 * Extended Message Header: emsg.len is the Data Size, so the
		 * header carries nothing the consumer does not already have.
		 * Keeping it would also cost two bytes the buffer does not
		 * have, because a maximum sized Extended Message is exactly
		 * PD_MAX_EXTENDED_MSG_LEN of payload.
		 */
		rx->type = rch->emsg.type;
		rx->header = rch->emsg.header;
		memcpy(rx->data, rch->emsg.data, ext_header.data_size);
		rx->len = ext_header.data_size;

		LOG_DBG("RCH: reassembled %u bytes in %u chunks", ext_header.data_size,
			rch->chunk_number_expected);

		pe_message_received(dev);
		prl_rch_set_state(dev, RCH_WAIT_FOR_MESSAGE_FROM_PROTOCOL_LAYER);
		return;
	}

	prl_rch_set_state(dev, RCH_REQUESTING_CHUNK);
}

static void rch_requesting_chunk_entry(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;

	rch_send_chunk_request(rch->dev);
}

static enum smf_state_result rch_requesting_chunk_run(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;
	struct usbc_port_data *data = rch->dev->data;
	struct protocol_layer_tx_t *prl_tx = data->prl_tx;

	if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_ERROR)) {
		prl_rch_set_state(rch->dev, RCH_REPORT_ERROR);
	} else if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_COMPLETE)) {
		prl_rch_set_state(rch->dev, RCH_WAITING_CHUNK);
	}

	return SMF_EVENT_PROPAGATE;
}

static void rch_waiting_chunk_entry(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;

	usbc_timer_start(&rch->pd_t_chunk_sender_response);
}

static enum smf_state_result rch_waiting_chunk_run(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;

	if (atomic_test_and_clear_bit(&rch->flags, RCH_FLAGS_MSG_RECEIVED)) {
		prl_rch_set_state(rch->dev, RCH_PROCESSING_EXTENDED_MESSAGE);
	} else if (usbc_timer_expired(&rch->pd_t_chunk_sender_response)) {
		LOG_ERR("RCH: timed out waiting for chunk %u", rch->chunk_number_expected);
		prl_rch_set_state(rch->dev, RCH_REPORT_ERROR);
	}

	return SMF_EVENT_PROPAGATE;
}

static void rch_waiting_chunk_exit(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;

	usbc_timer_stop(&rch->pd_t_chunk_sender_response);
}

static void rch_report_error_entry(void *obj)
{
	struct protocol_layer_rch_t *rch = (struct protocol_layer_rch_t *)obj;

	pe_report_error(rch->dev, ERR_XMIT, rch->pkt_type);
	prl_rch_set_state(rch->dev, RCH_WAIT_FOR_MESSAGE_FROM_PROTOCOL_LAYER);
}

/* clang-format off */
/**
 * @brief Chunked Rx Handler State table
 */
static const struct smf_state prl_rch_states[RCH_STATE_COUNT] = {
	[RCH_WAIT_FOR_MESSAGE_FROM_PROTOCOL_LAYER] = SMF_CREATE_STATE(
		rch_wait_for_message_from_protocol_layer_entry,
		rch_wait_for_message_from_protocol_layer_run,
		NULL,
		NULL,
		NULL),
	[RCH_PROCESSING_EXTENDED_MESSAGE] = SMF_CREATE_STATE(
		rch_processing_extended_message_entry,
		NULL,
		NULL,
		NULL,
		NULL),
	[RCH_REQUESTING_CHUNK] = SMF_CREATE_STATE(
		rch_requesting_chunk_entry,
		rch_requesting_chunk_run,
		NULL,
		NULL,
		NULL),
	[RCH_WAITING_CHUNK] = SMF_CREATE_STATE(
		rch_waiting_chunk_entry,
		rch_waiting_chunk_run,
		rch_waiting_chunk_exit,
		NULL,
		NULL),
	[RCH_REPORT_ERROR] = SMF_CREATE_STATE(
		rch_report_error_entry,
		NULL,
		NULL,
		NULL,
		NULL),
};
/* clang-format on */

static void prl_rch_set_state(const struct device *dev, const enum usbc_prl_rch_state_t state)
{
	struct usbc_port_data *data = dev->data;

	smf_set_state(SMF_CTX(data->prl_rch), &prl_rch_states[state]);

	/*
	 * Run the new state without waiting for the next cycle, as pe_set_state
	 * does. Reassembly is a long chain of short states, so a cycle time of
	 * delay on each one dominates the transfer.
	 */
	usbc_bypass_next_sleep(dev);
}

/**
 * @brief Stage an Extended Message for chunked transmission.
 */
int prl_send_ext_data_msg(const struct device *dev, const enum pd_packet_type type,
			  const enum pd_ext_msg_type msg, const uint8_t *data, uint32_t len)
{
	struct usbc_port_data *port = dev->data;
	struct protocol_layer_tch_t *tch = port->prl_tch;

	if (len > PD_MAX_EXTENDED_MSG_LEN) {
		return -EMSGSIZE;
	}

	if (atomic_test_bit(&tch->flags, TCH_FLAGS_MSG_REQUESTED)) {
		return -EBUSY;
	}

	if (len > 0) {
		memcpy(tch->payload, data, len);
	}

	tch->data_size = len;
	tch->pkt_type = type;
	tch->msg_type = msg;

	atomic_set_bit(&tch->flags, TCH_FLAGS_MSG_REQUESTED);

	return 0;
}

static void tch_wait_for_message_request_from_pe_entry(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;

	tch->chunk_number_to_send = 0;
	tch->num_bytes_sent = 0;
}

static enum smf_state_result tch_wait_for_message_request_from_pe_run(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;

	if (atomic_test_bit(&tch->flags, TCH_FLAGS_MSG_REQUESTED)) {
		prl_tch_set_state(tch->dev, TCH_CONSTRUCT_CHUNKED_MESSAGE);
	}

	return SMF_EVENT_PROPAGATE;
}

/**
 * @brief Build and queue the chunk identified by chunk_number_to_send.
 */
static void tch_construct_chunked_message_entry(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;
	uint32_t offset = tch->chunk_number_to_send * PD_MAX_EXTENDED_MSG_CHUNK_LEN;
	uint32_t payload_len;
	union pd_ext_header ext_header = {
		.chunked = 1,
		.request_chunk = 0,
		.chunk_number = tch->chunk_number_to_send,
		.data_size = tch->data_size,
	};

	if (offset > tch->data_size) {
		LOG_ERR("TCH: chunk %u starts past the end of a %u byte message",
			tch->chunk_number_to_send, tch->data_size);
		prl_tch_set_state(tch->dev, TCH_REPORT_ERROR);
		return;
	}

	payload_len = MIN(tch->data_size - offset, PD_MAX_EXTENDED_MSG_CHUNK_LEN);

	prl_chunk_queue_tx(tch->dev, tch->pkt_type, tch->msg_type, ext_header,
			   &tch->payload[offset], payload_len);

	tch->num_bytes_sent = offset + payload_len;

	prl_tch_set_state(tch->dev, TCH_SENDING_CHUNKED_MESSAGE);
}

static enum smf_state_result tch_sending_chunked_message_run(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;
	struct usbc_port_data *port = tch->dev->data;
	struct protocol_layer_tx_t *prl_tx = port->prl_tx;

	if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_ERROR)) {
		prl_tch_set_state(tch->dev, TCH_REPORT_ERROR);
	} else if (atomic_test_bit(&prl_tx->flags, PRL_FLAGS_TX_COMPLETE)) {
		if (tch->num_bytes_sent >= tch->data_size) {
			prl_tch_set_state(tch->dev, TCH_MESSAGE_SENT);
		} else {
			prl_tch_set_state(tch->dev, TCH_WAIT_CHUNK_REQUEST);
		}
	}

	return SMF_EVENT_PROPAGATE;
}

static void tch_wait_chunk_request_entry(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;

	atomic_clear_bit(&tch->flags, TCH_FLAGS_CHUNK_REQUEST);
	usbc_timer_start(&tch->pd_t_chunk_sender_request);
}

static enum smf_state_result tch_wait_chunk_request_run(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;

	if (atomic_test_and_clear_bit(&tch->flags, TCH_FLAGS_CHUNK_REQUEST)) {
		if (tch->chunk_number_requested != tch->chunk_number_to_send + 1) {
			LOG_ERR("TCH: chunk %u requested, expected %u", tch->chunk_number_requested,
				tch->chunk_number_to_send + 1);
			prl_tch_set_state(tch->dev, TCH_REPORT_ERROR);
			return SMF_EVENT_PROPAGATE;
		}

		tch->chunk_number_to_send = tch->chunk_number_requested;
		prl_tch_set_state(tch->dev, TCH_CONSTRUCT_CHUNKED_MESSAGE);
	} else if (usbc_timer_expired(&tch->pd_t_chunk_sender_request)) {
		LOG_ERR("TCH: no request for chunk %u", tch->chunk_number_to_send + 1);
		prl_tch_set_state(tch->dev, TCH_REPORT_ERROR);
	}

	return SMF_EVENT_PROPAGATE;
}

static void tch_wait_chunk_request_exit(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;

	usbc_timer_stop(&tch->pd_t_chunk_sender_request);
}

static void tch_message_sent_entry(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;

	LOG_DBG("TCH: sent %u bytes in %u chunks", tch->data_size, tch->chunk_number_to_send + 1);

	atomic_clear_bit(&tch->flags, TCH_FLAGS_MSG_REQUESTED);
	pe_message_sent(tch->dev);
	prl_tch_set_state(tch->dev, TCH_WAIT_FOR_MESSAGE_REQUEST_FROM_PE);
}

static void tch_report_error_entry(void *obj)
{
	struct protocol_layer_tch_t *tch = (struct protocol_layer_tch_t *)obj;

	atomic_clear_bit(&tch->flags, TCH_FLAGS_MSG_REQUESTED);
	pe_report_error(tch->dev, ERR_XMIT, tch->pkt_type);
	prl_tch_set_state(tch->dev, TCH_WAIT_FOR_MESSAGE_REQUEST_FROM_PE);
}

/* clang-format off */
/**
 * @brief Chunked Tx Handler State table
 */
static const struct smf_state prl_tch_states[TCH_STATE_COUNT] = {
	[TCH_WAIT_FOR_MESSAGE_REQUEST_FROM_PE] = SMF_CREATE_STATE(
		tch_wait_for_message_request_from_pe_entry,
		tch_wait_for_message_request_from_pe_run,
		NULL,
		NULL,
		NULL),
	[TCH_CONSTRUCT_CHUNKED_MESSAGE] = SMF_CREATE_STATE(
		tch_construct_chunked_message_entry,
		NULL,
		NULL,
		NULL,
		NULL),
	[TCH_SENDING_CHUNKED_MESSAGE] = SMF_CREATE_STATE(
		NULL,
		tch_sending_chunked_message_run,
		NULL,
		NULL,
		NULL),
	[TCH_WAIT_CHUNK_REQUEST] = SMF_CREATE_STATE(
		tch_wait_chunk_request_entry,
		tch_wait_chunk_request_run,
		tch_wait_chunk_request_exit,
		NULL,
		NULL),
	[TCH_MESSAGE_SENT] = SMF_CREATE_STATE(
		tch_message_sent_entry,
		NULL,
		NULL,
		NULL,
		NULL),
	[TCH_REPORT_ERROR] = SMF_CREATE_STATE(
		tch_report_error_entry,
		NULL,
		NULL,
		NULL,
		NULL),
};
/* clang-format on */

static void prl_tch_set_state(const struct device *dev, const enum usbc_prl_tch_state_t state)
{
	struct usbc_port_data *data = dev->data;

	smf_set_state(SMF_CTX(data->prl_tch), &prl_tch_states[state]);

	/* Run the new state without waiting, as prl_rch_set_state does. */
	usbc_bypass_next_sleep(dev);
}
#endif /* CONFIG_USBC_CHUNKING */

/* clang-format off */
/**
 * @brief Protocol Layer Transmit State table
 */
static const struct smf_state prl_tx_states[PRL_TX_STATE_COUNT] = {
	[PRL_TX_PHY_LAYER_RESET] = SMF_CREATE_STATE(
		prl_tx_phy_layer_reset_entry,
		NULL,
		NULL,
		NULL,
		NULL),
	[PRL_TX_WAIT_FOR_MESSAGE_REQUEST] = SMF_CREATE_STATE(
		prl_tx_wait_for_message_request_entry,
		prl_tx_wait_for_message_request_run,
		NULL,
		NULL,
		NULL),
	[PRL_TX_LAYER_RESET_FOR_TRANSMIT] = SMF_CREATE_STATE(
		prl_tx_layer_reset_for_transmit_entry,
		NULL,
		NULL,
		NULL,
		NULL),
	[PRL_TX_WAIT_FOR_PHY_RESPONSE] = SMF_CREATE_STATE(
		prl_tx_wait_for_phy_response_entry,
		prl_tx_wait_for_phy_response_run,
		prl_tx_wait_for_phy_response_exit,
		NULL,
		NULL),
	[PRL_TX_SUSPEND] = SMF_CREATE_STATE(
		prl_tx_suspend_entry,
		prl_tx_suspend_run,
		NULL,
		NULL,
		NULL),
#ifdef CONFIG_USBC_CSM_SUPPORTS_SINK
	[PRL_TX_SNK_START_AMS] = SMF_CREATE_STATE(
		prl_tx_snk_start_ams_entry,
		prl_tx_snk_start_ams_run,
		NULL,
		NULL,
		NULL),
	[PRL_TX_SNK_PENDING] = SMF_CREATE_STATE(
		prl_tx_snk_pending_entry,
		prl_tx_snk_pending_run,
		NULL,
		NULL,
		NULL),
#endif
#ifdef CONFIG_USBC_CSM_SUPPORTS_SOURCE
	[PRL_TX_SRC_SOURCE_TX] = SMF_CREATE_STATE(
		prl_tx_src_source_tx_entry,
		prl_tx_src_source_tx_run,
		NULL,
		NULL,
		NULL),
	[PRL_TX_SRC_PENDING] = SMF_CREATE_STATE(
		prl_tx_src_pending_entry,
		prl_tx_src_pending_run,
		prl_tx_src_pending_exit,
		NULL,
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
		NULL,
		NULL),
	[PRL_HR_RESET_LAYER] = SMF_CREATE_STATE(
		prl_hr_reset_layer_entry,
		NULL,
		NULL,
		NULL,
		NULL),
	[PRL_HR_WAIT_FOR_PHY_HARD_RESET_COMPLETE] = SMF_CREATE_STATE(
		prl_hr_wait_for_phy_hard_reset_complete_entry,
		prl_hr_wait_for_phy_hard_reset_complete_run,
		prl_hr_wait_for_phy_hard_reset_complete_exit,
		NULL,
		NULL),
	[PRL_HR_WAIT_FOR_PE_HARD_RESET_COMPLETE] = SMF_CREATE_STATE(
		prl_hr_wait_for_pe_hard_reset_complete_entry,
		prl_hr_wait_for_pe_hard_reset_complete_run,
		NULL,
		NULL,
		NULL),
	[PRL_HR_SUSPEND] = SMF_CREATE_STATE(
		prl_hr_suspend_entry,
		prl_hr_suspend_run,
		NULL,
		NULL,
		NULL),
};
/* clang-format on */
BUILD_ASSERT(ARRAY_SIZE(prl_hr_states) == PRL_HR_STATE_COUNT);
