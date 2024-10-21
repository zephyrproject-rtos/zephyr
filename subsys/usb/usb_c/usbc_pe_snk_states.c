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

#include "usbc_pe_common_internal.h"
#include "usbc_stack.h"

/**
 * @brief Initialize the Source Policy Engine layer
 */
void pe_snk_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/* Initial role of sink is UFP */
	pe_set_data_role(dev, TC_ROLE_UFP);

	/* Initialize timers */
	usbc_timer_init(&pe->pd_t_typec_sink_wait_cap, PD_T_TYPEC_SINK_WAIT_CAP_MAX_MS);
	usbc_timer_init(&pe->pd_t_ps_transition, PD_T_SPR_PS_TRANSITION_NOM_MS);
	usbc_timer_init(&pe->pd_t_wait_to_resend, PD_T_SINK_REQUEST_MIN_MS);

	/* Goto startup state */
	pe_set_state(dev, PE_SNK_STARTUP);
}

/**
 * @brief Handle sink-specific DPM requests
 */
void sink_dpm_requests(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	struct policy_engine *pe = data->pe;

	/*
	 * Handle any common DPM Requests
	 */
	if (common_dpm_requests(dev)) {
		return;
	}

	/*
	 * Handle Sink DPM Requests
	 */
	if (pe->dpm_request > REQUEST_TC_END) {
		atomic_set_bit(pe->flags, PE_FLAGS_DPM_INITIATED_AMS);

		if (pe->dpm_request == REQUEST_PE_GET_SRC_CAPS) {
			pe_set_state(dev, PE_SNK_GET_SOURCE_CAP);
		}
	}
}

/**
 * @brief PE_SNK_Startup Entry State
 */
void pe_snk_startup_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Startup");

	/* Reset the protocol layer */
	prl_reset(dev);

	/* Set power role to Sink */
	pe->power_role = TC_ROLE_SINK;

	/* Invalidate explicit contract */
	atomic_clear_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT);

	policy_notify(dev, NOT_PD_CONNECTED);
}

/**
 * @brief PE_SNK_Startup Run State
 */
void pe_snk_startup_run(void *obj)
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
void pe_snk_discovery_entry(void *obj)
{
	LOG_INF("PE_SNK_Discovery");
}

/**
 * @brief PE_SNK_Discovery Run State
 */
void pe_snk_discovery_run(void *obj)
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
void pe_snk_wait_for_capabilities_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SNK_Wait_For_Capabilities");

	/* Initialize and start the SinkWaitCapTimer */
	usbc_timer_start(&pe->pd_t_typec_sink_wait_cap);
}

/**
 * @brief PE_SNK_Wait_For_Capabilities Run State
 */
void pe_snk_wait_for_capabilities_run(void *obj)
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
	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		header = prl_rx->emsg.header;
		if (received_data_message(dev, header, PD_DATA_SOURCE_CAP)) {
			pe_set_state(dev, PE_SNK_EVALUATE_CAPABILITY);
			return;
		}
	}

	/* When the SinkWaitCapTimer times out, perform a Hard Reset. */
	if (usbc_timer_expired(&pe->pd_t_typec_sink_wait_cap)) {
		atomic_set_bit(pe->flags, PE_FLAGS_SNK_WAIT_CAP_TIMEOUT);
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
}

/**
 * @brief PE_SNK_Wait_For_Capabilities Exit State
 */
void pe_snk_wait_for_capabilities_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Stop SinkWaitCapTimer */
	usbc_timer_stop(&pe->pd_t_typec_sink_wait_cap);
}

/**
 * @brief PE_SNK_Evaluate_Capability Entry State
 */
void pe_snk_evaluate_capability_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;
	uint32_t *pdos = (uint32_t *)prl_rx->emsg.data;
	uint32_t num_pdo_objs = PD_CONVERT_BYTES_TO_PD_HEADER_COUNT(prl_rx->emsg.len);

	LOG_INF("PE_SNK_Evaluate_Capability");

	/* Inform the DPM of the reception of the source capabilities */
	policy_notify(dev, SOURCE_CAPABILITIES_RECEIVED);

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
void pe_snk_select_capability_entry(void *obj)
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
 *	  NOTE: Sender Response Timer is handled in super state.
 */
void pe_snk_select_capability_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_DISCARDED)) {
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
	}

	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
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
			atomic_set_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT);
			pe_set_state(dev, PE_SNK_TRANSITION_SINK);
		} else if (received_control_message(dev, header, PD_CTRL_REJECT) ||
			   received_control_message(dev, header, PD_CTRL_WAIT)) {
			/*
			 * We had a previous explicit contract, so transition to
			 * PE_SNK_Ready
			 */
			if (atomic_test_bit(pe->flags, PE_FLAGS_EXPLICIT_CONTRACT)) {
				if (received_control_message(dev, header, PD_CTRL_WAIT)) {
					/*
					 * Inform Device Policy Manager that Sink
					 * Request needs to Wait
					 */
					if (policy_wait_notify(dev, WAIT_SINK_REQUEST)) {
						atomic_set_bit(pe->flags,
							       PE_FLAGS_WAIT_SINK_REQUEST);
						usbc_timer_start(&pe->pd_t_wait_to_resend);
					}
				}

				pe_set_state(dev, PE_SNK_READY);
			} else {
				/*
				 * No previous explicit contract, so transition
				 * to PE_SNK_Wait_For_Capabilities
				 */
				pe_set_state(dev, PE_SNK_WAIT_FOR_CAPABILITIES);
			}
		} else {
			pe_send_soft_reset(dev, prl_rx->emsg.type);
		}
		return;
	}
}

/**
 * @brief PE_SNK_Transition_Sink Entry State
 */
void pe_snk_transition_sink_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SNK_Transition_Sink");

	/* Initialize and run PSTransitionTimer */
	usbc_timer_start(&pe->pd_t_ps_transition);
}

/**
 * @brief PE_SNK_Transition_Sink Run State
 */
void pe_snk_transition_sink_run(void *obj)
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
	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
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
void pe_snk_transition_sink_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	/* Initialize and run PSTransitionTimer */
	usbc_timer_stop(&pe->pd_t_ps_transition);
}

/**
 * @brief PE_SNK_Ready Entry State
 */
void pe_snk_ready_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;

	LOG_INF("PE_SNK_Ready");

	/* Clear AMS Flags */
	atomic_clear_bit(pe->flags, PE_FLAGS_INTERRUPTIBLE_AMS);
	atomic_clear_bit(pe->flags, PE_FLAGS_DPM_INITIATED_AMS);
}

/**
 * @brief PE_SNK_Ready Run State
 */
void pe_snk_ready_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	/*
	 * Handle incoming messages before discovery and DPMs other than hard
	 * reset
	 */
	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		union pd_header header = prl_rx->emsg.header;

		/* Extended Message Request */
		if (header.extended) {
			extended_message_not_supported(dev);
			return;
		} else if (header.number_of_data_objects > 0) {
			/* Data Messages */
			switch (header.message_type) {
			case PD_DATA_SOURCE_CAP:
				pe_set_state(dev, PE_SNK_EVALUATE_CAPABILITY);
				break;
			case PD_DATA_VENDOR_DEF:
				/**
				 * VDM is unsupported. PD2.0 ignores and PD3.0
				 * reply with not supported.
				 */
				if (prl_get_rev(dev, PD_PACKET_SOP) > PD_REV20) {
					pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
				}
				break;
			default:
				pe_set_state(dev, PE_SEND_NOT_SUPPORTED);
			}
			return;
		} else {
			/* Control Messages */
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
		if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_WAIT_SINK_REQUEST)) {
			pe_set_state(dev, PE_SNK_SELECT_CAPABILITY);
			return;
		} else if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_WAIT_DATA_ROLE_SWAP)) {
			pe_set_state(dev, PE_DRS_SEND_SWAP);
			return;
		}
	}

	/*
	 * Handle Device Policy Manager Requests
	 */
	sink_dpm_requests(dev);
}

void pe_snk_ready_exit(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * If the Source is initiating an AMS, then notify the
	 * PRL that the first message in an AMS will follow.
	 */
	if (pe_dpm_initiated_ams(dev)) {
		prl_first_msg_notificaiton(dev);
	}
}

/**
 * @brief PE_SNK_Hard_Reset Entry State
 */
void pe_snk_hard_reset_entry(void *obj)
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
	if (atomic_test_bit(pe->flags, PE_FLAGS_SNK_WAIT_CAP_TIMEOUT) &&
	    pe->hard_reset_counter > PD_N_HARD_RESET_COUNT) {
		/* Inform the DPM that the port partner is not responsive */
		policy_notify(dev, PORT_PARTNER_NOT_RESPONSIVE);

		/* Pause the Policy Engine */
		data->pe_enabled = false;
		return;
	}

	/* Set Hard Reset Pending Flag */
	atomic_set_bit(pe->flags, PE_FLAGS_HARD_RESET_PENDING);

	atomic_clear_bit(pe->flags, PE_FLAGS_SNK_WAIT_CAP_TIMEOUT);

	/* Request the generation of Hard Reset Signaling by the PHY Layer */
	prl_execute_hard_reset(dev);
	/* Increment the HardResetCounter */
	pe->hard_reset_counter++;
}

/**
 * @brief PE_SNK_Hard_Reset Run State
 */
void pe_snk_hard_reset_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	/*
	 * Transition to the PE_SNK_Transition_to_default state when:
	 *  1) The Hard Reset is complete.
	 */
	if (atomic_test_bit(pe->flags, PE_FLAGS_HARD_RESET_PENDING)) {
		return;
	}

	pe_set_state(dev, PE_SNK_TRANSITION_TO_DEFAULT);
}

/**
 * @brief PE_SNK_Transition_to_default Entry State
 */
void pe_snk_transition_to_default_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Transition_to_default");

	/* Reset flags */
	atomic_clear(pe->flags);
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
void pe_snk_transition_to_default_run(void *obj)
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
 *
 */
void pe_snk_get_source_cap_entry(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;

	LOG_INF("PE_SNK_Get_Source_Cap");

	/*
	 * On entry to the PE_SNK_Get_Source_Cap state the Policy Engine
	 * Shall request the Protocol Layer to send a get Source
	 * Capabilities message in order to retrieve the Source’s
	 * capabilities.
	 */
	pe_send_ctrl_msg(dev, PD_PACKET_SOP, PD_CTRL_GET_SOURCE_CAP);
}

/**
 * @brief PE_SNK_Get_Source_Cap Run State
 *	  NOTE: Sender Response Timer is handled in super state.
 */
void pe_snk_get_source_cap_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;
	union pd_header header;

	/* Wait until message is sent or dropped */
	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_RECEIVED)) {
		/*
		 * The Policy Engine Shall transition to the PE_SNK_Evaluate_Capability
		 * State when:
		 *	1: In SPR Mode and SPR Source Capabilities were requested and
		 *	   a Source_Capabilities Message is received
		 */
		header = prl_rx->emsg.header;

		if (received_control_message(dev, header, PD_DATA_SOURCE_CAP)) {
			pe_set_state(dev, PE_SNK_EVALUATE_CAPABILITY);
		}
	}
}

/**
 * @brief PE_SNK_Give_Sink_Cap Entry state
 */
void pe_snk_give_sink_cap_entry(void *obj)
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
	pe_send_data_msg(dev, PD_PACKET_SOP, PD_DATA_SINK_CAP);
}

/**
 * @brief PE_SNK_Give_Sink_Cap Run state
 */
void pe_snk_give_sink_cap_run(void *obj)
{
	struct policy_engine *pe = (struct policy_engine *)obj;
	const struct device *dev = pe->dev;
	struct usbc_port_data *data = dev->data;
	struct protocol_layer_rx_t *prl_rx = data->prl_rx;

	/* Wait until message is sent or dropped */
	if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_TX_COMPLETE)) {
		pe_set_state(dev, PE_SNK_READY);
	} else if (atomic_test_and_clear_bit(pe->flags, PE_FLAGS_MSG_DISCARDED)) {
		pe_send_soft_reset(dev, prl_rx->emsg.type);
	}
}
