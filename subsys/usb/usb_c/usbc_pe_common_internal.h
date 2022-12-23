/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_PE_COMMON_INTERNAL_H_
#define ZEPHYR_SUBSYS_USBC_PE_COMMON_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/drivers/usb_c/usbc_tc.h>
#include <zephyr/smf.h>
#include "usbc_timer.h"

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

/**
 * @brief Policy Engine Layer Flags
 */
enum pe_flags {
	/** Accept message received from port partner */
	PE_FLAGS_ACCEPT = 0,
	/**
	 * Protocol Error was determined based on error recovery
	 * current state
	 */
	PE_FLAGS_PROTOCOL_ERROR = 1,
	/** A message we requested to be sent has been transmitted */
	PE_FLAGS_TX_COMPLETE = 2,
	/** A message sent by a port partner has been received */
	PE_FLAGS_MSG_RECEIVED = 3,
	/**
	 * A hard reset has been requested by the DPM but has not been sent,
	 * not currently used
	 */
	PE_FLAGS_HARD_RESET_PENDING = 4,
	/** An explicit contract is in place with our port partner */
	PE_FLAGS_EXPLICIT_CONTRACT = 5,
	/**
	 * Waiting for Sink Capabailities timed out.  Used for retry error
	 * handling
	 */
	PE_FLAGS_SNK_WAIT_CAP_TIMEOUT = 6,
	/**
	 * Flag to note current Atomic Message Sequence (AMS) is interruptible.
	 * If this flag is not set the AMS is non-interruptible. This flag must
	 * be set in the interruptible's message state entry.
	 */
	PE_FLAGS_INTERRUPTIBLE_AMS = 7,
	/** Flag to trigger sending a Data Role Swap */
	PE_FLAGS_DR_SWAP_TO_DFP = 8,
	/** Flag is set when an AMS is initiated by the Device Policy Manager */
	PE_FLAGS_DPM_INITIATED_AMS = 9,
	/** Flag to note message was discarded due to incoming message */
	PE_FLAGS_MSG_DISCARDED = 10,
	/** Flag to trigger sending a soft reset */
	PE_FLAGS_SEND_SOFT_RESET = 11,
	/**
	 * This flag is set when a Wait message is received in response to a
	 * Sink REQUEST
	 */
	PE_FLAGS_WAIT_SINK_REQUEST = 12,
	/**
	 * This flag is set when a Wait message is received in response to a
	 * Data Role Swap
	 */
	PE_FLAGS_WAIT_DATA_ROLE_SWAP = 13
};

/**
 * @brief Policy Engine State Machine Object
 */
struct policy_engine {
	/** state machine context */
	struct smf_ctx ctx;
	/** Port device */
	const struct device *dev;
	/** state machine flags */
	atomic_t flags;
	/** current port power role (SOURCE or SINK) */
	enum tc_power_role power_role;
	/** current port data role (DFP or UFP) */
	enum tc_data_role data_role;
	/** port address where soft resets are sent */
	enum pd_packet_type soft_reset_sop;
	/** DPM request */
	enum usbc_policy_request_t dpm_request;

	/* Counters */

	/**
	 * This counter is used to retry the Hard Reset whenever there is no
	 * response from the remote device.
	 */
	uint32_t hard_reset_counter;

	/* Timers */

	/** tTypeCSinkWaitCap timer */
	struct usbc_timer_t pd_t_typec_sink_wait_cap;
	/** tSenderResponse timer */
	struct usbc_timer_t pd_t_sender_response;
	/** tPSTransition timer */
	struct usbc_timer_t pd_t_ps_transition;
	/** tSinkRequest timer */
	struct usbc_timer_t pd_t_sink_request;
	/** tChunkingNotSupported timer */
	struct usbc_timer_t pd_t_chunking_not_supported;
	/** Time to wait before resending message after WAIT reception */
	struct usbc_timer_t pd_t_wait_to_resend;
};

/**
 * @brief Sets a Policy Engine state
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param state next PE State to enter
 */
void pe_set_state(const struct device *dev, const enum usbc_pe_state state);

/**
 * @brief Get the Policy Engine's current state
 *
 * @param dev Pointer to the device structure for the driver instance
 * @retval current PE state
 */
enum usbc_pe_state pe_get_state(const struct device *dev);

/**
 * @brief Get the Policy Engine's previous state
 *
 * @param dev Pointer to the device structure for the driver instance
 * @retval last PE state
 */
enum usbc_pe_state pe_get_last_state(const struct device *dev);

/**
 * @brief Send a soft reset message
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param type SOP* to send soft reset message
 */
void pe_send_soft_reset(const struct device *dev, const enum pd_packet_type type);

/**
 * @brief Send a Power Delivery Data Message
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param type SOP* to send message
 * @param msg PD data message to send
 */
void pe_send_data_msg(const struct device *dev,
		   const enum pd_packet_type type,
		   const enum pd_data_msg_type msg);

/**
 * @brief Send a Power Delivery Control Message
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param type SOP* to send message
 * @param msg PD control message to send
 */
void pe_send_ctrl_msg(const struct device *dev,
		   const enum pd_packet_type type,
		   const enum pd_ctrl_msg_type msg);

/**
 * @brief Request desired voltage from source.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param rdo Request Data Object to send
 */
void pe_send_request_msg(const struct device *dev, const uint32_t rdo);

/**
 * @brief Transitions state after receiving an extended message.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void extended_message_not_supported(const struct device *dev);

/**
 * @brief Check if a specific control message was received
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param header message header containing the message
 * @param mt message type to check
 * @retval true if the header contains the message type, else false
 */
bool received_control_message(const struct device *dev, const union pd_header header,
			      const enum pd_ctrl_msg_type mt);

/**
 * @brief Check if a specific data message was received
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param header message header containing the message
 * @param mt message type to check
 * @param true if the header contains the message type, else false
 */
bool received_data_message(const struct device *dev, const union pd_header header,
			   const enum pd_data_msg_type mt);

/**
 * @brief Check a DPM policy
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param pc The DPM policy to check
 * @retval true if the DPM approves the check, else false
 */
bool policy_check(const struct device *dev, const enum usbc_policy_check_t pc);

/**
 * @brief Notify the DPM of a policy change
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param notify The notification to send the the DPM
 */
void policy_notify(const struct device *dev, const enum usbc_policy_notify_t notify);

/**
 * @brief Notify the DPM of a WAIT message reception
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param notify Wait message to send to DPM
 * @retval true if the Policy Engine should wait and try the action again
 */
bool policy_wait_notify(const struct device *dev, const enum usbc_policy_wait_t notify);

/**
 * @brief Send the received source caps to the DPM
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param pdos pointer to pdos to send
 * @param num_pdos number of pdos to send
 */
void policy_set_src_cap(const struct device *dev, const uint32_t *pdos, const int num_pdos);

/**
 * @brief Get a Request Data Object from the DPM
 *
 * @param dev Pointer to the device structure for the driver instance
 * @retval the RDO from the DPM
 */
uint32_t policy_get_request_data_object(const struct device *dev);

/**
 * @brief Check if the sink is a default level
 *
 * @param dev Pointer to the device structure for the driver instance
 * @retval true if sink is at default value, else false
 */
bool policy_is_snk_at_default(const struct device *dev);

/**
 * @brief Get sink caps from the DPM
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param pdos pointer to pdo sink caps
 * @param num_pdos number of pdo sink caps
 */
void policy_get_snk_cap(const struct device *dev, uint32_t **pdos, int *num_pdos);

#endif /* ZEPHYR_SUBSYS_USBC_PE_COMMON_INTERNAL_H_ */
