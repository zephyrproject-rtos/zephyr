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
	/** Power Path Controller device on this port */
	const struct device *ppc;

	/** Device Policy Manager Request FIFO */
	struct k_fifo request_fifo;
	/** Device Policy manager Request */
	struct request_value request;

	/** Bypass next sleep and request one more iteration of the USB-C state machines */
	bool bypass_next_sleep;

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

#ifdef CONFIG_USBC_CSM_SINK_ONLY
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
#else /* CONFIG_USBC_CSM_SOURCE_ONLY */
	/**
	 * Callback used by the Policy Engine get the Rp pull-up that should
	 * be placed on the CC lines
	 */
	int (*policy_cb_get_src_rp)(const struct device *dev,
				    enum tc_rp_value *rp);

	/**
	 * Callback used by the Policy Engine to enable and disable the
	 * Source Power Supply
	 */
	int (*policy_cb_src_en)(const struct device *dev, bool en);

	/**
	 * Callback used by the Policy Engine to get the Source Caps that
	 * will be sent to the Sink
	 */
	int (*policy_cb_get_src_caps)(const struct device *dev,
				     const uint32_t **pdos,
				     uint32_t *num_pdos);

	/**
	 * Callback used by the Policy Engine to check if the Sink's request
	 * is valid
	 */
	enum usbc_snk_req_reply_t (*policy_cb_check_sink_request)(const struct device *dev,
					     const uint32_t request_msg);

	/**
	 * Callback used by the Policy Engine to check if the Present Contract
	 * is still valid
	 */
	bool (*policy_present_contract_is_valid)(const struct device *dev,
						const uint32_t present_contract);

	/**
	 * Callback used by the Policy Engine to check if the Source Power Supply
	 * is ready
	 */
	bool (*policy_is_ps_ready)(const struct device *dev);

	/**
	 * Callback used by the Policy Engine to request that a different set of
	 * Source Caps be used
	 */
	bool (*policy_change_src_caps)(const struct device *dev);

	/**
	 * Callback used by the Policy Engine to store the Sink's Capabilities
	 */
	void (*policy_cb_set_port_partner_snk_cap)(const struct device *dev,
					const uint32_t *pdos,
					const int num_pdos);
#endif /* CONFIG_USBC_CSM_SINK_ONLY */
	/** Device Policy Manager data */
	void *dpm_data;
};

#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
/**
 * @brief Function that enables the source path either using callback or by the TCPC.
 * If source and sink paths are controlled by the TCPC, this callback doesn't have to be set.
 *
 * @param dev USB-C connector device
 * @param tcpc Type-C Port Controller device
 * @param en True to enable the sourcing, false to disable
 * @return int 0 if success, -ENOSYS if both callback and TCPC function are not implemented.
 *             In case of error, value from any of the functions is returned
 */
static inline int usbc_policy_src_en(const struct device *dev, const struct device *tcpc, bool en)
{
	struct usbc_port_data *data = dev->data;
	int ret_cb = -ENOSYS;
	int ret_tcpc;

	if (data->policy_cb_src_en != NULL) {
		ret_cb = data->policy_cb_src_en(dev, en);
		if (ret_cb != 0 && ret_cb != -ENOSYS) {
			return ret_cb;
		}
	}

	ret_tcpc = tcpc_set_src_ctrl(tcpc, en);
	if (ret_tcpc == -ENOSYS) {
		return ret_cb;
	}

	return ret_tcpc;
}
#endif

#endif /* ZEPHYR_SUBSYS_USBC_STACK_PRIV_H_ */
