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
 * @brief Policy Engine Errors
 */
enum pe_error {
	/** Transmit error */
	ERR_XMIT,
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
	PE_CHUNK_RECEIVED,

	/** PE_Suspend. Not part of the PD specification. */
	PE_SUSPEND,

	/**
	 * NOTE: The states below should not be called directly. They're used
	 * internally by the state machine.
	 */

	/** PE_SENDER_RESPONSE_PARENT. Not part of the PD specification. */
	PE_SENDER_RESPONSE_PARENT,

	/** Number of PE States */
	PE_STATE_COUNT
};

/**
 * @brief Policy Engine Layer Flags
 */
enum pe_flags {
	/** Accept message received from port partner */
	PE_FLAGS_ACCEPT = 0,
	/** A message we requested to be sent has been transmitted */
	PE_FLAGS_TX_COMPLETE = 1,
	/** A message sent by a port partner has been received */
	PE_FLAGS_MSG_RECEIVED = 2,
	/**
	 * A hard reset has been requested by the DPM but has not been sent,
	 * not currently used
	 */
	PE_FLAGS_HARD_RESET_PENDING = 3,
	/** An explicit contract is in place with our port partner */
	PE_FLAGS_EXPLICIT_CONTRACT = 4,
	/**
	 * Waiting for Sink Capabailities timed out.  Used for retry error
	 * handling
	 */
	PE_FLAGS_SNK_WAIT_CAP_TIMEOUT = 5,
	/**
	 * Flag to note current Atomic Message Sequence (AMS) is interruptible.
	 * If this flag is not set the AMS is non-interruptible. This flag must
	 * be set in the interruptible's message state entry.
	 */
	PE_FLAGS_INTERRUPTIBLE_AMS = 6,
	/** Flag to trigger sending a Data Role Swap */
	PE_FLAGS_DR_SWAP_TO_DFP = 7,
	/** Flag is set when an AMS is initiated by the Device Policy Manager */
	PE_FLAGS_DPM_INITIATED_AMS = 8,
	/** Flag to note message was discarded due to incoming message */
	PE_FLAGS_MSG_DISCARDED = 9,
	/** Flag to trigger sending a soft reset */
	PE_FLAGS_SEND_SOFT_RESET = 10,
	/**
	 * This flag is set when a Wait message is received in response to a
	 * Sink REQUEST
	 */
	PE_FLAGS_WAIT_SINK_REQUEST = 11,
	/**
	 * This flag is set when a Wait message is received in response to a
	 * Data Role Swap
	 */
	PE_FLAGS_WAIT_DATA_ROLE_SWAP = 12,
	/**
	 * This flag is set when a protocol error occurs.
	 */
	PE_FLAGS_PROTOCOL_ERROR = 13,
	/** This flag is set when a transmit error occurs. */
	PE_FLAGS_MSG_XMIT_ERROR = 14,

	/** Number of PE Flags */
	PE_FLAGS_COUNT
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
	ATOMIC_DEFINE(flags, PE_FLAGS_COUNT);
	/** current port power role (SOURCE or SINK) */
	enum tc_power_role power_role;
	/** current port data role (DFP or UFP) */
	enum tc_data_role data_role;
	/** port address where soft resets are sent */
	enum pd_packet_type soft_reset_sop;
	/** DPM request */
	enum usbc_policy_request_t dpm_request;
	/** generic variable used for simple in state statemachines */
	uint32_t submachine;

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
 * @brief First message in AMS has been sent
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_first_msg_sent(const struct device *dev);

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

/**
 * @brief Handle common DPM requests
 *
 * @param dev Pointer to the device structure for the driver instance
 * @retval true if request was handled, else false
 */
bool common_dpm_requests(const struct device *dev);

/**
 * @brief This function must only be called in the subsystem init function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void pe_subsys_init(const struct device *dev);

/**
 * @brief Start the Policy Engine Layer state machine. This is only called
 *	  from the Type-C state machine.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_start(const struct device *dev);

/**
 * @brief Suspend the Policy Engine Layer state machine. This is only called
 *	  from the Type-C state machine.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_suspend(const struct device *dev);

/**
 * @brief Run the Policy Engine Layer state machine. This is called from the
 *	  subsystems port stack thread
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dpm_request Device Policy Manager request
 */
void pe_run(const struct device *dev,
	    const int32_t dpm_request);

/**
 * @brief Query if the Policy Engine is running
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval TRUE if the Policy Engine is running
 * @retval FALSE if the Policy Engine is not running
 */
bool pe_is_running(const struct device *dev);

/**
 * @brief Informs the Policy Engine that a message was successfully sent
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_message_sent(const struct device *dev);

/**
 * @brief Informs the Policy Engine of an error.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param  e policy error
 * @param type port partner address where error was generated
 */
void pe_report_error(const struct device *dev,
		     const enum pe_error e,
		     const enum pd_packet_type type);

/**
 * @brief Informs the Policy Engine that a transmit message was discarded
 *	  because of an incoming message.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_report_discard(const struct device *dev);

/**
 * @brief Called by the Protocol Layer to informs the Policy Engine
 *	  that a message has been received.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_message_received(const struct device *dev);

/**
 * @brief Informs the Policy Engine that a hard reset was received.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_got_hard_reset(const struct device *dev);

/**
 * @brief Informs the Policy Engine that a soft reset was received.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_got_soft_reset(const struct device *dev);

/**
 * @brief Informs the Policy Engine that a hard reset was sent.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_hard_reset_sent(const struct device *dev);

/**
 * @brief Indicates if an explicit contract is in place
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval true if an explicit contract is in place, else false
 */
bool pe_is_explicit_contract(const struct device *dev);

/*
 * @brief Informs the Policy Engine that it should invalidate the
 *	  explicit contract.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void pe_invalidate_explicit_contract(const struct device *dev);

/**
 * @brief Return true if the PE is is within an atomic messaging sequence
 *	  that it initiated with a SOP* port partner.
 *
 * @note The PRL layer polls this instead of using AMS_START and AMS_END
 *	  notification from the PE that is called out by the spec
 *
 * @param dev Pointer to the device structure for the driver instance
 */
bool pe_dpm_initiated_ams(const struct device *dev);

/**
 * @brief Get the current data role
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval data role
 */
enum tc_data_role pe_get_data_role(const struct device *dev);

/**
 * @brief Sets the data role and updates the TCPC
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dr Data Role to be set
 */
void pe_set_data_role(const struct device *dev, enum tc_data_role dr);

/**
 * @brief Get the current power role
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval power role
 */
enum tc_power_role pe_get_power_role(const struct device *dev);

/**
 * @brief Get cable plug role
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval cable plug role
 */
enum tc_cable_plug pe_get_cable_plug(const struct device *dev);

#endif /* ZEPHYR_SUBSYS_USBC_PE_COMMON_INTERNAL_H_ */
