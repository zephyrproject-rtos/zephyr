/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_STACK_PRIV_H_
#define ZEPHYR_SUBSYS_USBC_STACK_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/usb_c/usbc.h>

#include "usbc_tc_common_internal.h"
#include "usbc_pe_common_internal.h"
#include "usbc_prl.h"

#define PRIV_PORT_REQUEST_SUSPEND -1
#define PRIV_PORT_REQUEST_START	  -2

/**
 * @brief Each layer of the stack is composed of state machines that can be
 *	  in one of the following states.
 */
enum usbc_sm_state {
	/** The state machine is paused */
	SM_PAUSED,
	/** The state machine is initializing */
	SM_INIT,
	/** The state machine is running */
	SM_RUN
};

/**
 * @brief Port config
 */
struct usbc_port_config {
	/**
	 * The usbc stack initializes this pointer that creates the
	 * main thread for this port
	 */
	void (*create_thread)(const struct device *dev);
	/** The thread stack for this port's thread */
	k_thread_stack_t *stack;
};

/**
 * @brief Request FIFO
 */
struct request_value {
	/** First word is reserved for use by FIFO */
	void *fifo_reserved;
	/** Request value */
	int32_t val;
};

/**
 * @brief Port data
 */
struct usbc_port_data {
	/** This port's thread */
	k_tid_t port_thread;
	/** This port thread's data */
	struct k_thread thread_data;

	/* Type-C layer data */

	/** Type-C state machine object */
	struct tc_sm_t *tc;
	/** Enables or Disables the Type-C state machine */
	bool tc_enabled;
	/** The state of the Type-C state machine */
	enum usbc_sm_state tc_sm_state;

	/* Policy Engine layer data */

	/** Policy Engine state machine object */
	struct policy_engine *pe;
	/** Enables or Disables the Policy Engine state machine */
	bool pe_enabled;
	/** The state of the Policy Engine state machine */
	enum usbc_sm_state pe_sm_state;

	/* Protocol Layer data */

	/** Protocol Receive Layer state machine object */
	struct protocol_layer_rx_t *prl_rx;
	/** Protocol Transmit Layer state machine object */
	struct protocol_layer_tx_t *prl_tx;
	/** Protocol Hard Reset Layer state machine object */
	struct protocol_hard_reset_t *prl_hr;
	/** Enables or Disables the Protocol Layer state machine */
	bool prl_enabled;
	/** The state of the Protocol Layer state machine */
	enum usbc_sm_state prl_sm_state;

	/* Common data for all layers */

	/** Power Delivery revisions for each packet type */
	enum pd_rev_type rev[NUM_SOP_STAR_TYPES];
	/** The Type-C Port Controller on this port */
	const struct device *tcpc;
	/** VBUS Measurement and control device on this port */
	const struct device *vbus;

	/** Device Policy Manager Request FIFO */
	struct k_fifo request_fifo;
	/** Device Policy manager Request */
	struct request_value request;

	/* USB-C Callbacks */

	/**
	 * Callback used by the Policy Engine to ask the Device Policy Manager
	 * if a particular policy should be allowed
	 */
	bool (*policy_cb_check)(const struct device *dev,
				const enum usbc_policy_check_t policy_check);
	/**
	 * Callback used by the Policy Engine to notify the Device Policy
	 * Manager of a policy change
	 */
	void (*policy_cb_notify)(const struct device *dev,
				 const enum usbc_policy_notify_t policy_notify);
	/**
	 * Callback used by the Policy Engine to notify the Device Policy
	 * Manager of WAIT message reception
	 */
	bool (*policy_cb_wait_notify)(const struct device *dev,
				      const enum usbc_policy_wait_t policy_notify);
	/**
	 * Callback used by the Policy Engine to get the Sink Capabilities
	 * from the Device Policy Manager
	 */
	int (*policy_cb_get_snk_cap)(const struct device *dev, uint32_t **pdos, int *num_pdos);

	/**
	 * Callback used by the Policy Engine to send the received Source
	 * Capabilities to the Device Policy Manager
	 */
	void (*policy_cb_set_src_cap)(const struct device *dev, const uint32_t *pdos,
				      const int num_pdos);
	/**
	 * Callback used by the Policy Engine to get the Request Data Object
	 * (RDO) from the Device Policy Manager
	 */
	uint32_t (*policy_cb_get_rdo)(const struct device *dev);

	/**
	 * Callback used by the Policy Engine to check if Sink Power Supply
	 * is at default level
	 */
	bool (*policy_cb_is_snk_at_default)(const struct device *dev);

	/** Device Policy Manager data */
	void *dpm_data;
};

#endif /* ZEPHYR_SUBSYS_USBC_STACK_PRIV_H_ */
