/*
 * Copyright (c) 2022 The Chromium OS Authors.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/usb_c/usbc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include "power_ctrl.h"

#define USBC_PORT0_NODE         DT_ALIAS(usbc_port0)
#define USBC_PORT0_PWRCTRL_NODE DT_ALIAS(usbc_port0_pwrctrl)

#if !DT_NODE_HAS_STATUS(USBC_PORT0_PWRCTRL_NODE, okay)
#error "Unsupported board: usbc-port0-pwrctrl devicetree alias is not defined"
#endif

BUILD_ASSERT(DT_ENUM_IDX(USBC_PORT0_NODE, power_role) == TC_ROLE_CAP_DRP,
	     "Unsupported board: Only Dual Role Power (DRP) device supported");

/* Flag bits for power supply state management */
#define PS_SOURCE_REQUEST_BIT    0 /* Source: Display sink request */
#define PS_SOURCE_TRAN_START_BIT 1 /* Source: Start voltage transition */
#define PS_SOURCE_READY_BIT      2 /* Source: Voltage transition complete */
#define PS_SINK_READY_BIT        3 /* Sink: Source has finished transition */

/* usbc.rst port data object start */
/**
 * @brief A structure that encapsulates Port data.
 */
static struct port0_data_t {
	/** Power controller device */
	const struct device *pwrctrl;
	/** CC Rp value */
	int rp;
	/** Source Capabilities */
	uint32_t src_caps[DT_PROP_LEN(USBC_PORT0_NODE, source_pdos)];
	/** Number of Source Capabilities */
	int src_cap_cnt;
	/** Sink Capabilities */
	uint32_t snk_caps[DT_PROP_LEN(USBC_PORT0_NODE, sink_pdos)];
	/** Number of Sink Capabilities */
	int snk_cap_cnt;
	/* Current Power role */
	enum tc_power_role current_power_role;
	/* Current Data role */
	enum tc_data_role current_data_role;
	/** Sink Request RDO */
	union pd_rdo sink_request;
	/** Requested Object Pos */
	int obj_pos;
	/** VCONN CC line*/
	enum tc_cc_polarity vconn_pol;
	/** Atomic ps_flags for power supply state management */
	atomic_t ps_flags;
	/** Source Capabilities of the port partner */
	uint32_t partner_src_caps[PDO_MAX_DATA_OBJECTS];
	/** Number of Source Capabilities of the port partner */
	int partner_src_cap_cnt;
} port0_data = {
	.rp = DT_ENUM_IDX(USBC_PORT0_NODE, typec_power_opmode),
	.src_caps = DT_PROP(USBC_PORT0_NODE, source_pdos),
	.src_cap_cnt = DT_PROP_LEN(USBC_PORT0_NODE, source_pdos),
	.snk_caps = DT_PROP(USBC_PORT0_NODE, sink_pdos),
	.snk_cap_cnt = DT_PROP_LEN(USBC_PORT0_NODE, sink_pdos),
	.current_power_role = TC_ROLE_SINK,
	.current_data_role = TC_ROLE_DISCONNECTED,
};
/* usbc.rst port data object end */

/**
 * @brief Builds a Request Data Object (RDO) with the following properties:
 *		- Maximum operating current 100mA
 *		- Operating current is 100mA
 *		- Unchunked Extended Messages Not Supported
 *		- No USB Suspend
 *		- Not USB Communications Capable
 *		- No capability mismatch
 *		- Does not Giveback
 *		- Select object position 1 (5V Power Data Object (PDO))
 *
 * @note Generally a sink application would build an RDO from the
 *	 Source Capabilities stored in the dpm_data object
 */
static uint32_t build_rdo(const struct port0_data_t *dpm_data)
{
	union pd_rdo rdo;

	/* Maximum operating current 100mA (GIVEBACK = 0) */
	rdo.fixed.min_or_max_operating_current = PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(100);
	/* Operating current 100mA */
	rdo.fixed.operating_current = PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(100);
	/* Unchunked Extended Messages Not Supported */
	rdo.fixed.unchunked_ext_msg_supported = 0;
	/* No USB Suspend */
	rdo.fixed.no_usb_suspend = 1;
	/* Not USB Communications Capable */
	rdo.fixed.usb_comm_capable = 0;
	/* No capability mismatch */
	rdo.fixed.cap_mismatch = 0;
	/* Don't giveback */
	rdo.fixed.giveback = 0;
	/* Object position 1 (5V PDO) */
	rdo.fixed.object_pos = 1;

	return rdo.raw_value;
}

static void display_pdo(const int idx, const uint32_t pdo_value)
{
	union pd_fixed_supply_pdo_source pdo;

	/* Default to fixed supply pdo source until type is detected */
	pdo.raw_value = pdo_value;

	LOG_INF("PDO %d:0x%08x", idx, pdo.raw_value);
	switch (pdo.type) {
	case PDO_FIXED: {
		LOG_INF("\tType:                FIXED");
		LOG_INF("\tCurrent:             %d",
			PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(pdo.max_current));
		LOG_INF("\tVoltage:             %d",
			PD_CONVERT_FIXED_PDO_VOLTAGE_TO_MV(pdo.voltage));
		LOG_INF("\tPeak Current:        %d", pdo.peak_current);
		LOG_INF("\tUchunked Support:    %d", pdo.unchunked_ext_msg_supported);
		LOG_INF("\tDual Role Data:      %d", pdo.dual_role_data);
		LOG_INF("\tUSB Comms:           %d", pdo.usb_comms_capable);
		LOG_INF("\tUnconstrained Pwr:   %d", pdo.unconstrained_power);
		LOG_INF("\tUSB Suspend:         %d", pdo.usb_suspend_supported);
		LOG_INF("\tDual Role Power:     %d", pdo.dual_role_power);
	} break;
	case PDO_BATTERY: {
		union pd_battery_supply_pdo_source batt_pdo;

		batt_pdo.raw_value = pdo_value;
		LOG_INF("\tType:                BATTERY");
		LOG_INF("\tMin Voltage:         %d",
			PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(batt_pdo.min_voltage));
		LOG_INF("\tMax Voltage:         %d",
			PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(batt_pdo.max_voltage));
		LOG_INF("\tMax Power:           %d",
			PD_CONVERT_BATTERY_PDO_POWER_TO_MW(batt_pdo.max_power));
	} break;
	case PDO_VARIABLE: {
		union pd_variable_supply_pdo_source var_pdo;

		var_pdo.raw_value = pdo_value;
		LOG_INF("\tType:                VARIABLE");
		LOG_INF("\tMin Voltage:         %d",
			PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(var_pdo.min_voltage));
		LOG_INF("\tMax Voltage:         %d",
			PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(var_pdo.max_voltage));
		LOG_INF("\tMax Current:         %d",
			PD_CONVERT_VARIABLE_PDO_CURRENT_TO_MA(var_pdo.max_current));
	} break;
	case PDO_AUGMENTED: {
		union pd_augmented_supply_pdo_source aug_pdo;

		aug_pdo.raw_value = pdo_value;
		LOG_INF("\tType:                AUGMENTED");
		LOG_INF("\tMin Voltage:         %d",
			PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(aug_pdo.min_voltage));
		LOG_INF("\tMax Voltage:         %d",
			PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(aug_pdo.max_voltage));
		LOG_INF("\tMax Current:         %d",
			PD_CONVERT_AUGMENTED_PDO_CURRENT_TO_MA(aug_pdo.max_current));
		LOG_INF("\tPPS Power Limited:   %d", aug_pdo.pps_power_limited);
	} break;
	}
}

static void display_rdo(const uint32_t rdo_value)
{
	union pd_rdo rdo;

	rdo.raw_value = rdo_value;
	LOG_INF("RDO :0x%08x", rdo.raw_value);
	LOG_INF("\tType:                FIXED");
	LOG_INF("\tObject Position:     %d", rdo.fixed.object_pos);
	LOG_INF("\tGiveback:            %d", rdo.fixed.giveback);
	LOG_INF("\tCapability Mismatch: %d", rdo.fixed.cap_mismatch);
	LOG_INF("\tUSB Comm Capable:    %d", rdo.fixed.usb_comm_capable);
	LOG_INF("\tNo USB Suspend:      %d", rdo.fixed.no_usb_suspend);
	LOG_INF("\tUnchunk Ext MSG Support: %d", rdo.fixed.unchunked_ext_msg_supported);
	LOG_INF("\tOperating Current:   %d mA",
		PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(rdo.fixed.operating_current));
}

static void display_source_caps(const uint32_t *pdos, const int num_pdos)
{
	LOG_INF("Source Caps:");
	for (int i = 0; i < num_pdos; i++) {
		display_pdo(i, pdos[i]);
		k_msleep(50);
	}
}

/* usbc.rst callbacks start */
/**
 * @brief PE calls this function when it needs to set the Rp on CC
 */
int port0_policy_cb_get_src_rp(const struct device *dev, enum tc_rp_value *rp)
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
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	return source_ctrl_set(dpm_data->pwrctrl, en ? SOURCE_5V : SOURCE_0V);
}

/**
 * @brief PE calls this function to Enable or Disable VCONN
 */
int port0_policy_cb_vconn_en(const struct device *tcpc_dev, const struct device *usbc_dev,
			     enum tc_cc_polarity pol, bool en)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(usbc_dev);

	dpm_data->vconn_pol = pol;

	if (en == true) {
		if (pol == TC_POLARITY_CC1) {
			/* set VCONN on CC1 */
			return vconn_ctrl_set(dpm_data->pwrctrl, VCONN1_ON);
		}
		/* set VCONN on CC2 */
		return vconn_ctrl_set(dpm_data->pwrctrl, VCONN2_ON);
	}

	return vconn_ctrl_set(dpm_data->pwrctrl, VCONN_OFF);
}

/**
 * @brief PE calls this function to get the Source Caps that will be sent
 *	  to the Sink
 */
int port0_policy_cb_get_src_caps(const struct device *dev, const uint32_t **pdos,
				 uint32_t *num_pdos)
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

	atomic_set_bit(&dpm_data->ps_flags, PS_SOURCE_REQUEST_BIT);

	/*
	 * Clear PS ready. This will be set to true after PS is ready after
	 * it transitions to the new level.
	 */
	atomic_clear_bit(&dpm_data->ps_flags, PS_SOURCE_READY_BIT);

	return SNK_REQUEST_VALID;
}

/**
 * @brief PE calls this function to check if the Power Supply is at the requested
 *	  level
 */
static bool port0_policy_cb_is_ps_ready(const struct device *dev)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	return atomic_test_bit(&dpm_data->ps_flags, PS_SOURCE_READY_BIT);
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

static int port0_policy_cb_get_snk_cap(const struct device *dev, uint32_t **pdos, int *num_pdos)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	*pdos = dpm_data->snk_caps;
	*num_pdos = dpm_data->snk_cap_cnt;

	return 0;
}

static void port0_policy_cb_set_src_cap(const struct device *dev, const uint32_t *pdos,
					const int num_pdos)
{
	struct port0_data_t *dpm_data;
	int num;
	int i;

	dpm_data = usbc_get_dpm_data(dev);

	num = num_pdos;
	if (num > PDO_MAX_DATA_OBJECTS) {
		num = PDO_MAX_DATA_OBJECTS;
	}

	for (i = 0; i < num; i++) {
		dpm_data->partner_src_caps[i] = *(pdos + i);
	}

	dpm_data->partner_src_cap_cnt = num;
}

static uint32_t port0_policy_cb_get_rdo(const struct device *dev)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	return build_rdo(dpm_data);
}
/* usbc.rst callbacks end */

/* usbc.rst notify start */
static void port0_notify(const struct device *dev, const enum usbc_policy_notify_t policy_notify)
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
	case POWER_ROLE_IS_SOURCE:
		dpm_data->current_power_role = TC_ROLE_SOURCE;
		break;
	case POWER_ROLE_IS_SINK:
		dpm_data->current_power_role = TC_ROLE_SINK;
		break;
	case TRANSITION_PS:
		/* Handle based on current power role */
		if (dpm_data->current_power_role == TC_ROLE_SOURCE) {
			/* Source: Start power supply transition */
			atomic_set_bit(&dpm_data->ps_flags, PS_SOURCE_TRAN_START_BIT);
		} else {
			/* Sink: Source has finished transition */
			atomic_set_bit(&dpm_data->ps_flags, PS_SINK_READY_BIT);
		}
		break;
	case PD_CONNECTED:
		break;
	case NOT_PD_CONNECTED:
		break;
	case POWER_CHANGE_0A0:
		break;
	case POWER_CHANGE_DEF:
		break;
	case POWER_CHANGE_1A5:
		break;
	case POWER_CHANGE_3A0:
		break;
	case DATA_ROLE_IS_UFP:
		dpm_data->current_data_role = TC_ROLE_UFP;
		break;
	case DATA_ROLE_IS_DFP:
		dpm_data->current_data_role = TC_ROLE_DFP;
		break;
	case PORT_PARTNER_NOT_RESPONSIVE:
		LOG_INF("Port Partner not PD Capable");
		break;
	case SNK_TRANSITION_TO_DEFAULT:
		break;
	case HARD_RESET_RECEIVED:
		/*
		 * This notification is sent from the PE_SRC_Transition_to_default
		 * state and requires the following:
		 *	1: Vconn should be turned OFF
		 *	2: Reset of the local hardware
		 */

		/* Power off VCONN */
		vconn_ctrl_set(dpm_data->pwrctrl, VCONN_OFF);
		/* Transition PS to Default level */
		source_ctrl_set(dpm_data->pwrctrl, SOURCE_5V);
		break;
	case SENDER_RESPONSE_TIMEOUT:
		break;
	case SOURCE_CAPABILITIES_RECEIVED:
		break;
	default:
	}
}
/* usbc.rst notify end */

/* usbc.rst check start */
bool port0_policy_check(const struct device *dev, const enum usbc_policy_check_t policy_check)
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
		/* Accept data role swap to UFP */
		return true;
	case CHECK_SNK_AT_DEFAULT_LEVEL:
		/* This device is always at the default power level */
		return true;
	case CHECK_SRC_PS_AT_DEFAULT_LEVEL:
		/*
		 * This check is sent from the PE_SRC_Transition_to_default
		 * state and requires the following:
		 *	1: Vconn should be turned ON
		 *	2: Return TRUE when Power Supply is at default level
		 */

		/* Power on VCONN */
		vconn_ctrl_set(dpm_data->pwrctrl, dpm_data->vconn_pol);

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

	/* Get the power controller device for this port */
	port0_data.pwrctrl = DEVICE_DT_GET(USBC_PORT0_PWRCTRL_NODE);
	if (!device_is_ready(port0_data.pwrctrl)) {
		LOG_ERR("PORT0 power controller not ready");
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
	/* Register Policy Get Sink Capabilities callback */
	usbc_set_policy_cb_get_snk_cap(usbc_port0, port0_policy_cb_get_snk_cap);
	/* Register Policy Set Source Capabilities callback */
	usbc_set_policy_cb_set_src_cap(usbc_port0, port0_policy_cb_set_src_cap);
	/* Register Policy Get Request Data Object callback */
	usbc_set_policy_cb_get_rdo(usbc_port0, port0_policy_cb_get_rdo);
	/* usbc.rst register end */

	/* usbc.rst user data start */
	/* Set Application port data object. This object is passed to the policy callbacks */
	usbc_set_dpm_data(usbc_port0, &port0_data);
	/* usbc.rst user data end */

	/* Initialize atomic ps_flags */
	port0_data.ps_flags = ATOMIC_INIT(0);

	/* usbc.rst usbc start */
	/* Start the USB-C Subsystem */
	usbc_start(usbc_port0);
	/* usbc.rst usbc end */

	while (1) {
		/* Source: Transition PS to new level */
		if (atomic_test_and_clear_bit(&port0_data.ps_flags, PS_SOURCE_TRAN_START_BIT)) {
			/*
			 * Transition Power Supply to new voltage.
			 * Okay if this blocks.
			 */
			source_ctrl_set(port0_data.pwrctrl, port0_data.obj_pos);
			atomic_set_bit(&port0_data.ps_flags, PS_SOURCE_READY_BIT);
		}

		/* Source: Display Request */
		if (atomic_test_and_clear_bit(&port0_data.ps_flags, PS_SOURCE_REQUEST_BIT)) {
			/* Display the Sink request */
			display_rdo(port0_data.sink_request.raw_value);
		}

		/* Sink: Display Source Capabilities */
		if (atomic_test_and_clear_bit(&port0_data.ps_flags, PS_SINK_READY_BIT)) {
			/* Display partner Source Capabilities */
			display_source_caps(port0_data.partner_src_caps,
					    port0_data.partner_src_cap_cnt);
		}

		/* Arbitrary delay */
		k_msleep(10);
	}

	return 0;
}
