/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_PRL_H_
#define ZEPHYR_SUBSYS_USBC_PRL_H_

#include <zephyr/kernel.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/drivers/usb_c/usbc_tcpc.h>
#include <zephyr/smf.h>

#include "usbc_pe_common_internal.h"
#include "usbc_timer.h"

/**
 * @brief PD counter definitions
 *	  See Table 6-63 Counter parameters
 *	  Parameter Name: nMessageIDCount
 */
#define PD_MESSAGE_ID_COUNT 7

/**
 * @brief Message Reception State Machine Object
 */
struct protocol_layer_rx_t {
	/** state machine flags */
	atomic_t flags;
	/** message ids for all valid port partners */
	int msg_id[NUM_SOP_STAR_TYPES];
	/** Received Power Delivery Messages are stored in emsg */
	struct pd_msg emsg;
};

/**
 * @brief Message Transmission State Machine Object
 */
struct protocol_layer_tx_t {
	/** state machine context */
	struct smf_ctx ctx;
	/** Port device */
	const struct device *dev;
	/** state machine flags */
	atomic_t flags;
	/** last packet type we transmitted */
	enum pd_packet_type last_xmit_type;
	/** Current message type to transmit */
	uint8_t msg_type;
	/**
	 * Power Delivery Messages meant for transmission are stored
	 * in emsg
	 */
	struct pd_msg emsg;

	/* Counters */

	/** message id counters for all 6 port partners */
	uint32_t msg_id_counter[NUM_SOP_STAR_TYPES];

	/* Timers */

	/** tTxTimeout timer */
	struct usbc_timer_t pd_t_tx_timeout;
	/** tSinkTx timer */
	struct usbc_timer_t pd_t_sink_tx;
};

/**
 * @brief Hard Reset State Machine Object
 */
struct protocol_hard_reset_t {
	/** state machine context */
	struct smf_ctx ctx;
	/** Port device */
	const struct device *dev;
	/** state machine flags */
	atomic_t flags;

	/* Timers */

	/** tHardResetComplete timer */
	struct usbc_timer_t pd_t_hard_reset_complete;
};

/**
 * @brief This function must only be called in the subsystem init function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void prl_subsys_init(const struct device *dev);

/**
 * @brief Start the PRL Layer state machine. This is only called from the
 *	  Type-C state machine.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void prl_start(const struct device *dev);

/**
 * @brief Inform the PRL that the first message in an AMS is being sent
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void prl_first_msg_notificaiton(const struct device *dev);

/**
 * @brief Suspends the PRL Layer state machine. This is only called from the
 *	  Type-C state machine.
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void prl_suspend(const struct device *dev);

/**
 * @brief Reset the PRL Layer state machine
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void prl_reset(const struct device *dev);

/**
 * @brief Run the PRL Layer state machine. This is called from the subsystems
 *	  port stack thread
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void prl_run(const struct device *dev);

/**
 * @brief Called from the Policy Engine to signal that a hard reset is complete
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void prl_hard_reset_complete(const struct device *dev);

/**
 * @brief Sets the revision received from the port partner
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param type SOP* packet sent from port partner
 * @param rev Revision sent from the port partner
 */
void prl_set_rev(const struct device *dev, const enum pd_packet_type type,
		 const enum pd_rev_type rev);

/**
 * @brief Gets the revision received associated with a packet type
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param type SOP* packet type to get the revision for
 *
 * @retval revision associated with the packet type
 */
enum pd_rev_type prl_get_rev(const struct device *dev, const enum pd_packet_type type);

/**
 * @brief Instructs the Protocol Layer to send a Power Delivery control message
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param type The port partner to send this message to
 * @param msg The control message to send
 */
void prl_send_ctrl_msg(const struct device *dev, const enum pd_packet_type type,
		       const enum pd_ctrl_msg_type msg);

/**
 * @brief Instructs the Protocol Layer to send a Power Delivery data message
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param type The port partner to send this message to
 * @param msg The data message to send
 */
void prl_send_data_msg(const struct device *dev, const enum pd_packet_type type,
		       const enum pd_data_msg_type msg);

/**
 * @brief Instructs the Protocol Layer to execute a hard reset
 *
 * @param dev Pointer to the device structure for the driver instance
 */
void prl_execute_hard_reset(const struct device *dev);

/**
 * @brief Query if the Protocol Layer is running
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval TRUE if the Protocol Layer is running
 * @retval FALSE if the Protocol Layer is not running
 */
bool prl_is_running(const struct device *dev);

#endif /* ZEPHYR_SUBSYS_USBC_PRL_H_ */
