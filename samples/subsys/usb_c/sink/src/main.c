/*
 * Copyright (c) 2022 The Chromium OS Authors.
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

#define USBC_PORT0_NODE		DT_ALIAS(usbc_port0)
#define USBC_PORT0_POWER_ROLE	DT_ENUM_IDX(USBC_PORT0_NODE, power_role)

#if (USBC_PORT0_POWER_ROLE != TC_ROLE_SINK)
#error "Unsupported board: Only Sink device supported"
#endif

#define SINK_PDO(node_id, prop, idx)	(DT_PROP_BY_IDX(node_id, prop, idx)),

/* usbc.rst port data object start */
/**
 * @brief A structure that encapsulates Port data.
 */
static struct port0_data_t {
	/** Sink Capabilities */
	uint32_t snk_caps[DT_PROP_LEN(USBC_PORT0_NODE, sink_pdos)];
	/** Number of Sink Capabilities */
	int snk_cap_cnt;
	/** Source Capabilities */
	uint32_t src_caps[PDO_MAX_DATA_OBJECTS];
	/** Number of Source Capabilities */
	int src_cap_cnt;
	/* Power Supply Ready flag */
	atomic_t ps_ready;
} port0_data = {
	.snk_caps = {DT_FOREACH_PROP_ELEM(USBC_PORT0_NODE, sink_pdos, SINK_PDO)},
	.snk_cap_cnt = DT_PROP_LEN(USBC_PORT0_NODE, sink_pdos),
	.src_caps = {0},
	.src_cap_cnt = 0,
	.ps_ready = 0
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

static void display_pdo(const int idx,
			const uint32_t pdo_value)
{
	union pd_fixed_supply_pdo_source pdo;

	/* Default to fixed supply pdo source until type is detected */
	pdo.raw_value = pdo_value;

	LOG_INF("PDO %d:", idx);
	switch (pdo.type) {
	case PDO_FIXED: {
		LOG_INF("\tType:              FIXED");
		LOG_INF("\tCurrent:           %d",
		       PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(pdo.max_current));
		LOG_INF("\tVoltage:           %d",
		       PD_CONVERT_FIXED_PDO_VOLTAGE_TO_MV(pdo.voltage));
		LOG_INF("\tPeak Current:      %d", pdo.peak_current);
		LOG_INF("\tUchunked Support:  %d",
		       pdo.unchunked_ext_msg_supported);
		LOG_INF("\tDual Role Data:    %d",
		       pdo.dual_role_data);
		LOG_INF("\tUSB Comms:         %d",
		       pdo.usb_comms_capable);
		LOG_INF("\tUnconstrained Pwr: %d",
		       pdo.unconstrained_power);
		LOG_INF("\tUSB Suspend:       %d",
		       pdo.usb_suspend_supported);
		LOG_INF("\tDual Role Power:   %d",
		       pdo.dual_role_power);
	}
	break;
	case PDO_BATTERY: {
		union pd_battery_supply_pdo_source batt_pdo;

		batt_pdo.raw_value = pdo_value;
		LOG_INF("\tType:              BATTERY");
		LOG_INF("\tMin Voltage: %d",
			PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(batt_pdo.min_voltage));
		LOG_INF("\tMax Voltage: %d",
			PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(batt_pdo.max_voltage));
		LOG_INF("\tMax Power:   %d",
			PD_CONVERT_BATTERY_PDO_POWER_TO_MW(batt_pdo.max_power));
	}
	break;
	case PDO_VARIABLE: {
		union pd_variable_supply_pdo_source var_pdo;

		var_pdo.raw_value = pdo_value;
		LOG_INF("\tType:        VARIABLE");
		LOG_INF("\tMin Voltage: %d",
			PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(var_pdo.min_voltage));
		LOG_INF("\tMax Voltage: %d",
			PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(var_pdo.max_voltage));
		LOG_INF("\tMax Current: %d",
			PD_CONVERT_VARIABLE_PDO_CURRENT_TO_MA(var_pdo.max_current));
	}
	break;
	case PDO_AUGMENTED: {
		union pd_augmented_supply_pdo_source aug_pdo;

		aug_pdo.raw_value = pdo_value;
		LOG_INF("\tType:              AUGMENTED");
		LOG_INF("\tMin Voltage:       %d",
			PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(aug_pdo.min_voltage));
		LOG_INF("\tMax Voltage:       %d",
			PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(aug_pdo.max_voltage));
		LOG_INF("\tMax Current:       %d",
			PD_CONVERT_AUGMENTED_PDO_CURRENT_TO_MA(aug_pdo.max_current));
		LOG_INF("\tPPS Power Limited: %d", aug_pdo.pps_power_limited);
	}
	break;
	}
}

static void display_source_caps(const struct device *dev)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	LOG_INF("Source Caps:");
	for (int i = 0; i < dpm_data->src_cap_cnt; i++) {
		display_pdo(i, dpm_data->src_caps[i]);
		k_msleep(50);
	}
}

/* usbc.rst callbacks start */
static int port0_policy_cb_get_snk_cap(const struct device *dev,
					    uint32_t **pdos,
					    int *num_pdos)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	*pdos = dpm_data->snk_caps;
	*num_pdos = dpm_data->snk_cap_cnt;

	return 0;
}

static void port0_policy_cb_set_src_cap(const struct device *dev,
					     const uint32_t *pdos,
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
		dpm_data->src_caps[i] = *(pdos + i);
	}

	dpm_data->src_cap_cnt = num;
}

static uint32_t port0_policy_cb_get_rdo(const struct device *dev)
{
	struct port0_data_t *dpm_data = usbc_get_dpm_data(dev);

	return build_rdo(dpm_data);
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
		atomic_set_bit(&dpm_data->ps_ready, 0);
		break;
	case PD_CONNECTED:
		break;
	case NOT_PD_CONNECTED:
		break;
	case POWER_CHANGE_0A0:
		LOG_INF("PWR 0A");
		break;
	case POWER_CHANGE_DEF:
		LOG_INF("PWR DEF");
		break;
	case POWER_CHANGE_1A5:
		LOG_INF("PWR 1A5");
		break;
	case POWER_CHANGE_3A0:
		LOG_INF("PWR 3A0");
		break;
	case DATA_ROLE_IS_UFP:
		break;
	case DATA_ROLE_IS_DFP:
		break;
	case PORT_PARTNER_NOT_RESPONSIVE:
		LOG_INF("Port Partner not PD Capable");
		break;
	case SNK_TRANSITION_TO_DEFAULT:
		break;
	case HARD_RESET_RECEIVED:
		break;
	case SENDER_RESPONSE_TIMEOUT:
		break;
	case SOURCE_CAPABILITIES_RECEIVED:
		break;
	}
}
/* usbc.rst notify end */

/* usbc.rst check start */
bool port0_policy_check(const struct device *dev,
			const enum usbc_policy_check_t policy_check)
{
	switch (policy_check) {
	case CHECK_POWER_ROLE_SWAP:
		/* Reject power role swaps */
		return false;
	case CHECK_DATA_ROLE_SWAP_TO_DFP:
		/* Reject data role swap to DFP */
		return false;
	case CHECK_DATA_ROLE_SWAP_TO_UFP:
		/* Accept data role swap to UFP */
		return true;
	case CHECK_SNK_AT_DEFAULT_LEVEL:
		/* This device is always at the default power level */
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
	/* Register Policy Get Sink Capabilities callback */
	usbc_set_policy_cb_get_snk_cap(usbc_port0, port0_policy_cb_get_snk_cap);
	/* Register Policy Set Source Capabilities callback */
	usbc_set_policy_cb_set_src_cap(usbc_port0, port0_policy_cb_set_src_cap);
	/* Register Policy Get Request Data Object callback */
	usbc_set_policy_cb_get_rdo(usbc_port0, port0_policy_cb_get_rdo);
	/* usbc.rst register end */

	/* usbc.rst user data start */
	/* Set Application port data object. This object is passed to the policy callbacks */
	port0_data.ps_ready = ATOMIC_INIT(0);
	usbc_set_dpm_data(usbc_port0, &port0_data);
	/* usbc.rst user data end */

	/* usbc.rst usbc start */
	/* Start the USB-C Subsystem */
	usbc_start(usbc_port0);
	/* usbc.rst usbc end */

	while (1) {
		/* Perform Application Specific functions */
		if (atomic_test_and_clear_bit(&port0_data.ps_ready, 0)) {
			/* Display the Source Capabilities */
			display_source_caps(usbc_port0);
		}

		/* Arbitrary delay */
		k_msleep(1000);
	}
	return 0;
}
