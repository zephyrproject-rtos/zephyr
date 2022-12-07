/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_PE_H_
#define ZEPHYR_SUBSYS_USBC_PE_H_

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

#endif /* ZEPHYR_SUBSYS_USBC_PE_H_ */
