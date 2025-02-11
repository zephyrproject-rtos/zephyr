/*
 * Copyright (c) 2023 The Chromium OS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb_c/usbc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include "power_ctrl.h"

#define USBC_PORT0_NODE		DT_ALIAS(usbc_port0)
#define USBC_PORT0_POWER_ROLE	DT_ENUM_IDX(USBC_PORT0_NODE, power_role)

/* A Source power role evauates to 1. See usbc_tc.h: TC_ROLE_SOURCE */
#if (USBC_PORT0_POWER_ROLE != 1)
#error "Unsupported board: Only Source device supported"
#endif

#define SOURCE_PDO(node_id, prop, idx)	(DT_PROP_BY_IDX(node_id, prop, idx)),

/* usbc.rst port data object start */
/**
 * @brief A structure that encapsulates Port data.
 */
static struct port0_data_t {
	/** Source Capabilities */
	uint32_t src_caps[DT_PROP_LEN(USBC_PORT0_NODE, source_pdos)];
	/** Number of Source Capabilities */
	int src_cap_cnt;
	/** CC Rp value */
	int rp;
	/** Sink Request RDO */
	union pd_rdo sink_request;
	/** Requested Object Pos */
	int obj_pos;
	/** VCONN CC line*/
	enum tc_cc_polarity vconn_pol;
	/** True if power supply is ready */
	bool ps_ready;
	/** True if power supply should transition to a new level */
	bool ps_tran_start;
	/** Log Sink Requested RDO to console */
	atomic_t show_sink_request;
} port0_data = {
	.rp = DT_ENUM_IDX(USBC_PORT0_NODE, typec_power_opmode),
	.src_caps = {DT_FOREACH_PROP_ELEM(USBC_PORT0_NODE, source_pdos, SOURCE_PDO)},
	.src_cap_cnt = DT_PROP_LEN(USBC_PORT0_NODE, source_pdos),
};

/* usbc.rst port data object end */

static void dump_sink_request_rdo(const uint32_t rdo)
{
	union pd_rdo request;

	request.raw_value = rdo;

	LOG_INF("REQUEST RDO: %08x", rdo);
	LOG_INF("\tObject Position:\t %d", request.fixed.object_pos);
	LOG_INF("\tGiveback:\t\t %d", request.fixed.giveback);
	LOG_INF("\tCapability Mismatch:\t %d", request.fixed.cap_mismatch);
	LOG_INF("\tUSB Comm Capable:\t %d", request.fixed.usb_comm_capable);
	LOG_INF("\tNo USB Suspend:\t\t %d", request.fixed.no_usb_suspend);
	LOG_INF("\tUnchunk Ext MSG Support: %d", request.fixed.unchunked_ext_msg_supported);
	LOG_INF("\tOperating Current:\t %d mA",
			PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(request.fixed.operating_current));
	if (request.fixed.giveback) {
		LOG_INF("\tMax Operating Current:\t %d mA",
		PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(request.fixed.min_or_max_operating_current));
	} else {
		LOG_INF("\tMin Operating Current:\t %d mA",
		PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(request.fixed.min_or_max_operating_current));
	}
}

/* usbc.rst callbacks start */
/**
 * @brief PE calls this function when it needs to set the Rp on CC
 */
int port0_policy_cb_get_src_rp(const struct device *dev,
			       enum tc_rp_value *rp)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	*rp = dpm_data->rp;

	return 0;
}

/**
 * @brief PE calls this function to Enable (5V) or Disable (0V) the
 *	  Power Supply
 */
int port0_policy_cb_src_en(const struct device *dev, bool en)
{
	source_ctrl_set(en ? SOURCE_5V : SOURCE_0V);

	return 0;
}

/**
 * @brief PE calls this function to Enable or Disable VCONN
 */
int port0_policy_cb_vconn_en(const struct device *dev, enum tc_cc_polarity pol, bool en)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	dpm_data->vconn_pol = pol;

	if (en == false) {
		/* Disable VCONN on CC1 and CC2 */
		vconn_ctrl_set(VCONN_OFF);
	} else if (pol == TC_POLARITY_CC1) {
		/* set VCONN on CC1 */
		vconn_ctrl_set(VCONN1_ON);
	} else {
		/* set VCONN on CC2 */
		vconn_ctrl_set(VCONN2_ON);
	}

	return 0;
}

/**
 * @brief PE calls this function to get the Source Caps that will be sent
 *	  to the Sink
 */
int port0_policy_cb_get_src_caps(const struct device *dev,
			const uint32_t **pdos, uint32_t *num_pdos)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	*pdos = dpm_data->src_caps;
	*num_pdos = dpm_data->src_cap_cnt;

	return 0;
}

/**
 * @brief PE calls this function to verify that a Sink's request if valid
 */
static enum usbc_snk_req_reply_t port0_policy_cb_check_sink_request(const struct device *dev,
					const uint32_t request_msg)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);
	union pd_fixed_supply_pdo_source pdo;
	uint32_t obj_pos;
	uint32_t op_current;

	dpm_data->sink_request.raw_value = request_msg;
	obj_pos = dpm_data->sink_request.fixed.object_pos;
	op_current =
		PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(dpm_data->sink_request.fixed.operating_current);

	if (obj_pos == 0 || obj_pos > dpm_data->src_cap_cnt) {
		return SNK_REQUEST_REJECT;
	}

	pdo.raw_value = dpm_data->src_caps[obj_pos - 1];

	if (dpm_data->sink_request.fixed.operating_current > pdo.max_current) {
		return SNK_REQUEST_REJECT;
	}

	dpm_data->obj_pos = obj_pos;

	atomic_set_bit(&port0_data.show_sink_request, 0);

	/*
	 * Clear PS ready. This will be set to true after PS is ready after
	 * it transitions to the new level.
	 */
	port0_data.ps_ready = false;

	return SNK_REQUEST_VALID;
}

/**
 * @brief PE calls this function to check if the Power Supply is at the requested
 *	  level
 */
static bool port0_policy_cb_is_ps_ready(const struct device *dev)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);


	/* Return true to inform that the Power Supply is ready */
	return dpm_data->ps_ready;
}

/**
 * @brief PE calls this function to check if the Present Contract is still
 *	  valid
 */
static bool port0_policy_cb_present_contract_is_valid(const struct device *dev,
					const uint32_t present_contract)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);
	union pd_fixed_supply_pdo_source pdo;
	union pd_rdo request;
	uint32_t obj_pos;
	uint32_t op_current;

	request.raw_value = present_contract;
	obj_pos = request.fixed.object_pos;
	op_current = PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(request.fixed.operating_current);

	if (obj_pos == 0 || obj_pos > dpm_data->src_cap_cnt) {
		return false;
	}

	pdo.raw_value = dpm_data->src_caps[obj_pos - 1];

	if (request.fixed.operating_current > pdo.max_current) {
		return false;
	}

	return true;
}

/* usbc.rst callbacks end */

/* usbc.rst notify start */
static void port0_notify(const struct device *dev,
			      const enum usbc_policy_notify_t policy_notify)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	switch (policy_notify) {
	case PROTOCOL_ERROR:
		break;
	case MSG_DISCARDED:
		break;
	case MSG_ACCEPT_RECEIVED:
		break;
	case MSG_REJECTED_RECEIVED:
		break;
	case MSG_NOT_SUPPORTED_RECEIVED:
		break;
	case TRANSITION_PS:
		dpm_data->ps_tran_start = true;
		break;
	case PD_CONNECTED:
		break;
	case NOT_PD_CONNECTED:
		break;
	case DATA_ROLE_IS_UFP:
		break;
	case DATA_ROLE_IS_DFP:
		break;
	case PORT_PARTNER_NOT_RESPONSIVE:
		LOG_INF("Port Partner not PD Capable");
		break;
	case HARD_RESET_RECEIVED:
		/*
		 * This notification is sent from the PE_SRC_Transition_to_default
		 * state and requires the following:
		 *	1: Vconn should be turned OFF
		 *	2: Reset of the local hardware
		 */

		/* Power off VCONN */
		vconn_ctrl_set(VCONN_OFF);
		/* Transition PS to Default level */
		source_ctrl_set(SOURCE_5V);
		break;
	default:
	}
}
/* usbc.rst notify end */

/* usbc.rst check start */
bool port0_policy_check(const struct device *dev,
			const enum usbc_policy_check_t policy_check)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	switch (policy_check) {
	case CHECK_POWER_ROLE_SWAP:
		/* Reject power role swaps */
		return false;
	case CHECK_DATA_ROLE_SWAP_TO_DFP:
		/* Accept data role swap to DFP */
		return true;
	case CHECK_DATA_ROLE_SWAP_TO_UFP:
		/* Reject data role swap to UFP */
		return false;
	case CHECK_SRC_PS_AT_DEFAULT_LEVEL:
		/*
		 * This check is sent from the PE_SRC_Transition_to_default
		 * state and requires the following:
		 *	1: Vconn should be turned ON
		 *	2: Return TRUE when Power Supply is at default level
		 */

		/* Power on VCONN */
		vconn_ctrl_set(dpm_data->vconn_pol);

		/* PS should be at default level after receiving a Hard Reset */
		return true;
	default:
		/* Reject all other policy checks */
		return false;

	}
}
/* usbc.rst check end */

int main(void)
{
	const struct device *usbc_port0;

	/* Get the device for this port */
	usbc_port0 = DEVICE_DT_GET(USBC_PORT0_NODE);
	if (!device_is_ready(usbc_port0)) {
		LOG_ERR("PORT0 device not ready");
		return 0;
	}

	/* usbc.rst register start */
	/* Register USB-C Callbacks */

	/* Register Policy Check callback */
	usbc_set_policy_cb_check(usbc_port0, port0_policy_check);
	/* Register Policy Notify callback */
	usbc_set_policy_cb_notify(usbc_port0, port0_notify);
	/* Register Policy callback to set the Rp on CC lines */
	usbc_set_policy_cb_get_src_rp(usbc_port0, port0_policy_cb_get_src_rp);
	/* Register Policy callback to enable or disable power supply */
	usbc_set_policy_cb_src_en(usbc_port0, port0_policy_cb_src_en);
	/* Register Policy callback to enable or disable vconn */
	usbc_set_vconn_control_cb(usbc_port0, port0_policy_cb_vconn_en);
	/* Register Policy callback to send the source caps to the sink */
	usbc_set_policy_cb_get_src_caps(usbc_port0, port0_policy_cb_get_src_caps);
	/* Register Policy callback to check if the sink request is valid */
	usbc_set_policy_cb_check_sink_request(usbc_port0, port0_policy_cb_check_sink_request);
	/* Register Policy callback to check if the power supply is ready */
	usbc_set_policy_cb_is_ps_ready(usbc_port0, port0_policy_cb_is_ps_ready);
	/* Register Policy callback to check if Present Contract is still valid */
	usbc_set_policy_cb_present_contract_is_valid(usbc_port0,
				port0_policy_cb_present_contract_is_valid);

	/* usbc.rst register end */

	/* usbc.rst user data start */
	/* Set Application port data object. This object is passed to the policy callbacks */
	usbc_set_dpm_data(usbc_port0, &port0_data);
	/* usbc.rst user data end */

	/* Flag to show sink request */
	port0_data.show_sink_request = ATOMIC_INIT(0);

	/* Init power supply transition start */
	port0_data.ps_tran_start = false;
	/* Init power supply ready */
	port0_data.ps_ready = false;

	/* usbc.rst usbc start */
	/* Start the USB-C Subsystem */
	usbc_start(usbc_port0);
	/* usbc.rst usbc end */

	while (1) {
		/* Perform Application Specific functions */

		/* Transition PS to new level */
		if (port0_data.ps_tran_start) {
			/*
			 * Transition Power Supply to new voltage.
			 * Okay if this blocks.
			 */
			source_ctrl_set(port0_data.obj_pos);
			port0_data.ps_ready = true;
			port0_data.ps_tran_start = false;
		}

		/* Display Sink Requests */
		if (atomic_test_and_clear_bit(&port0_data.show_sink_request, 0)) {
			/* Display the Sink request */
			dump_sink_request_rdo(port0_data.sink_request.raw_value);
		}
		/* Arbitrary delay */
		k_msleep(10);
	}

	return 0;
}
