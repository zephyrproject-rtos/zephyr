/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/kernel.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/tc_util.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>

#include "../../mocks/mock_tester.h"

#define PORT1_NODE DT_NODELABEL(port1)

static const struct device *usbc_port1;

static struct port1_data_t {
	/* Port Policy checks */

	/* Power Role Swap Policy check */
	atomic_t pp_check_power_role_swap;
	/* Data Role Swap to DFP Policy check */
	atomic_t pp_check_data_role_swap_to_dfp;
	/* Data Role Swap Policy to UFP check */
	atomic_t pp_check_data_role_swap_to_ufp;
	/* Sink at default level Policy check */
	atomic_t pp_check_snk_at_default_level;

	/* Port Notifications from Policy Engine */

	/* Protocol Error */
	atomic_t pn_protocol_error;
	/* Message Discarded */
	atomic_t pn_msg_discarded;
	/* Message Accept Received */
	atomic_t pn_msg_accept_received;
	/* Message Rejected Received */
	atomic_t pn_msg_rejected_received;
	/* Message Not Supported Received */
	atomic_t pn_msg_not_supported_received;
	/* Transition Power Supply */
	atomic_t pn_transition_ps;
	/* PD connected */
	atomic_t pn_pd_connected;
	/* Not PD connected */
	atomic_t pn_not_pd_connected;
	/* Power Changed to off */
	atomic_t pn_power_change_0a0;
	/* Power Changed to Default */
	atomic_t pn_power_change_def;
	/* Power Changed to 5V @ 1.5A */
	atomic_t pn_power_change_1a5;
	/* Power Changed to 5V @ 3A */
	atomic_t pn_power_change_3a0;
	/* Current data role is UFP */
	atomic_t pn_data_role_is_ufp;
	/* Current data role is DFP */
	atomic_t pn_data_role_is_dfp;
	/* Port Partner not responsive */
	atomic_t pn_port_partner_not_responsive;
	/* Sink transition to default */
	atomic_t pn_snk_transition_to_default;
	/* Hard Reset Received */
	atomic_t pn_hard_reset_received;
	/* Source Capabilities Received */
	atomic_t pn_source_capabilities_received;
	/* Sender Response Timeout */
	atomic_t pn_sender_response_timeout;
	/* Power Request */
	atomic_t uut_request;

	/* Sink Capability PDO */
	union pd_fixed_supply_pdo_sink snk_cap_pdo;

	/* Source Capability Number */
	uint32_t uut_received_src_cap_num;
	/* Source Capabilities */
	uint32_t uut_received_src_caps[10];
	/* Received message from UUT */
	struct pd_msg rx_msg;
} port1_data;

/**
 * @brief Called by the USB-C subsystem to get the sink caps
 */
static int uut_policy_cb_get_snk_cap(const struct device *dev, uint32_t **pdos, int *num_pdos)
{
	struct port1_data_t *data = usbc_get_dpm_data(dev);

	*pdos = &data->snk_cap_pdo.raw_value;
	*num_pdos = 1;

	return 0;
}

/**
 * @breif Called by USB-C subsystem to set the source caps
 */
static void uut_policy_cb_set_src_cap(const struct device *dev, const uint32_t *pdos,
				      const int num_pdos)
{
	struct port1_data_t *data = usbc_get_dpm_data(dev);
	int i;

	data->uut_received_src_cap_num = num_pdos;

	for (i = 0; i < num_pdos; i++) {
		data->uut_received_src_caps[i] = *(pdos + i);
	}
}

/**
 * @brief Called by USB-C subsystem to get a Request Data Object
 */
static uint32_t uut_policy_cb_get_rdo(const struct device *dev)
{
	struct port1_data_t *data = usbc_get_dpm_data(dev);

	atomic_set(&data->uut_request, true);

	return FIXED_5V_100MA_RDO;
}

/**
 * @brief Called by USB-C subsystem to set a notification
 */
static void uut_notify(const struct device *dev, const enum usbc_policy_notify_t policy_notify)
{
	struct port1_data_t *data = usbc_get_dpm_data(dev);

	switch (policy_notify) {
	case PROTOCOL_ERROR:
		atomic_set(&data->pn_protocol_error, true);
		break;
	case MSG_DISCARDED:
		atomic_set(&data->pn_msg_discarded, true);
		break;
	case MSG_ACCEPT_RECEIVED:
		atomic_set(&data->pn_msg_accept_received, true);
		break;
	case MSG_REJECTED_RECEIVED:
		atomic_set(&data->pn_msg_rejected_received, true);
		break;
	case MSG_NOT_SUPPORTED_RECEIVED:
		atomic_set(&data->pn_msg_not_supported_received, true);
		break;
	case TRANSITION_PS:
		atomic_set(&data->pn_transition_ps, true);
		break;
	case PD_CONNECTED:
		atomic_set(&data->pn_pd_connected, true);
		break;
	case NOT_PD_CONNECTED:
		atomic_set(&data->pn_not_pd_connected, true);
		break;
	case POWER_CHANGE_0A0:
		printk("0A0\n");
		atomic_set(&data->pn_power_change_0a0, true);
		break;
	case POWER_CHANGE_DEF:
		printk("DEF\n");
		atomic_set(&data->pn_power_change_def, true);
		break;
	case POWER_CHANGE_1A5:
		printk("1A5\n");
		atomic_set(&data->pn_power_change_1a5, true);
		break;
	case POWER_CHANGE_3A0:
		printk("3A0\n");
		atomic_set(&data->pn_power_change_3a0, true);
	case DATA_ROLE_IS_UFP:
		atomic_set(&data->pn_data_role_is_ufp, true);
		break;
	case DATA_ROLE_IS_DFP:
		atomic_set(&data->pn_data_role_is_dfp, true);
		break;
	case PORT_PARTNER_NOT_RESPONSIVE:
		atomic_set(&data->pn_port_partner_not_responsive, true);
		break;
	case SNK_TRANSITION_TO_DEFAULT:
		atomic_set(&data->pn_snk_transition_to_default, true);
		break;
	case HARD_RESET_RECEIVED:
		atomic_set(&data->pn_hard_reset_received, true);
		break;
	case SOURCE_CAPABILITIES_RECEIVED:
		atomic_set(&data->pn_source_capabilities_received, true);
		break;
	case SENDER_RESPONSE_TIMEOUT:
		atomic_set(&data->pn_sender_response_timeout, true);
		break;
	default:
		zassert_true(false, "Unknown Policy Notification");
		break;
	}
}

/**
 * @brief Called by USB-C Subsystem to check a policy
 */
bool uut_policy_check(const struct device *dev, const enum usbc_policy_check_t policy_check)
{
	struct port1_data_t *data = usbc_get_dpm_data(dev);

	switch (policy_check) {
	case CHECK_POWER_ROLE_SWAP:
		return data->pp_check_power_role_swap;
	case CHECK_DATA_ROLE_SWAP_TO_DFP:
		return data->pp_check_data_role_swap_to_dfp;
	case CHECK_DATA_ROLE_SWAP_TO_UFP:
		return data->pp_check_data_role_swap_to_ufp;
	case CHECK_SNK_AT_DEFAULT_LEVEL:
		return data->pp_check_snk_at_default_level;
	default:
		zassert_true(false, "Unknown Policy Check");
		break;
	}

	return false;
}

/**
 * @brief Called by ZTest to perform a setup before any tests are run
 */
static void *test_usbc_setup(void)
{
	/* Get the device for this port */
	usbc_port1 = DEVICE_DT_GET(PORT1_NODE);
	zassert_true(device_is_ready(usbc_port1), "Failed to find USBC PORT1");

	/* Initialize the Sink Cap PDO */
	port1_data.snk_cap_pdo.type = PDO_FIXED;
	port1_data.snk_cap_pdo.dual_role_power = 1;
	port1_data.snk_cap_pdo.higher_capability = 0;
	port1_data.snk_cap_pdo.unconstrained_power = 1;
	port1_data.snk_cap_pdo.usb_comms_capable = 0;
	port1_data.snk_cap_pdo.dual_role_data = 0;
	port1_data.snk_cap_pdo.frs_required = 0;
	port1_data.snk_cap_pdo.reserved0 = 0;
	port1_data.snk_cap_pdo.voltage = PD_CONVERT_MV_TO_FIXED_PDO_VOLTAGE(5000);
	port1_data.snk_cap_pdo.operational_current = PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(100);

	/* Register USB-C Callbacks */

	/* Register Policy Check callback */
	usbc_set_policy_cb_check(usbc_port1, uut_policy_check);
	/* Register Policy Notify callback */
	usbc_set_policy_cb_notify(usbc_port1, uut_notify);
	/* Register Policy Get Sink Capabilities callback */
	usbc_set_policy_cb_get_snk_cap(usbc_port1, uut_policy_cb_get_snk_cap);
	/* Register Policy Set Source Capabilities callback */
	usbc_set_policy_cb_set_src_cap(usbc_port1, uut_policy_cb_set_src_cap);
	/* Register Policy Get Request Data Object callback */
	usbc_set_policy_cb_get_rdo(usbc_port1, uut_policy_cb_get_rdo);
	/*
	 * Set Tester port data object. This object is passed to the
	 * policy callbacks
	 */
	usbc_set_dpm_data(usbc_port1, &port1_data);

	return NULL;
}

/**
 * @brief Called by ZTest before test is run
 */
static void test_usbc_before(void *f)
{
	/* Tester is source */
	tester_set_power_role_source();

	/* Tester is UFP */
	tester_set_data_role_ufp();

	/* Start the USB-C Subsystem */
	usbc_start(usbc_port1);
}

/**
 * @brief Called by ZTest after test has run
 */
static void test_usbc_after(void *f)
{
	/* Stop the USB-C Subsystem */
	usbc_suspend(usbc_port1);
}

/**
 * @brief Check Request Message
 *
 * The Tester performs additional protocol checks to all Request messages
 * sent by the UUT.
 */
static void check_request_message(const struct device *dev)
{
	struct port1_data_t *data = usbc_get_dpm_data(dev);
	union pd_rdo rdo;
	uint32_t object_pos;

	/* 1) Field check for all types of Request Data Object */
	rdo.raw_value = sys_get_le32(data->rx_msg.data);
	object_pos = rdo.fixed.object_pos;

	/*
	 * a) B31…28 (Object Position) is not 000b, and the value is not
	 *    greater than the number of PDOs in the last Source Capabilities
	 *    message
	 */
	zassert_not_equal(object_pos, 0, "RDO object position can't be zero");
	zassert_true(object_pos <= data->uut_received_src_cap_num,
		     "RDO object position out of range");
}

/**
 * @brief UUT Sent Request
 *
 * Tester runs this procedure whenever receiving
 * Request message from the UUT.
 */
static void uut_sent_request(const struct device *dev)
{
	struct port1_data_t *data = usbc_get_dpm_data(dev);
	int i;

	/* Send Accept message to UUT */
	tester_send_ctrl_msg(PD_CTRL_ACCEPT, true);
	k_msleep(100);
	/* Send PS Ready message to UUT */
	tester_send_ctrl_msg(PD_CTRL_PS_RDY, true);

	for (i = 0; i < 1000; i++) {
		if (atomic_get(&data->pn_transition_ps)) {
			break;
		}
		k_msleep(1);
	}

	/* UUT should signal that the Power Supply should be transitioned */
	zassert_true(atomic_get(&data->pn_transition_ps),
		     "UUT failed to respond to PS_RDY message");
}

/**
 * @brief Bring-up Sink UUT
 */
static void bring_up_sink_uut(const struct device *dev)
{
	struct port1_data_t *data = usbc_get_dpm_data(dev);
	union pd_fixed_supply_pdo_source pdo;
	int i;
	int j;
	int k;

	/* Initialize test variables */

	data->pp_check_snk_at_default_level = ATOMIC_INIT(false);

	data->pn_protocol_error = ATOMIC_INIT(false);
	data->pn_msg_discarded = ATOMIC_INIT(false);
	data->pn_msg_accept_received = ATOMIC_INIT(false);
	data->pn_msg_rejected_received = ATOMIC_INIT(false);
	data->pn_msg_not_supported_received = ATOMIC_INIT(false);
	data->pn_transition_ps = ATOMIC_INIT(false);
	data->pn_pd_connected = ATOMIC_INIT(false);
	data->pn_not_pd_connected = ATOMIC_INIT(false);
	data->pn_power_change_0a0 = ATOMIC_INIT(false);
	data->pn_power_change_def = ATOMIC_INIT(false);
	data->pn_power_change_1a5 = ATOMIC_INIT(false);
	data->pn_power_change_3a0 = ATOMIC_INIT(false);
	data->pn_data_role_is_ufp = ATOMIC_INIT(false);
	data->pn_data_role_is_dfp = ATOMIC_INIT(false);
	data->pn_port_partner_not_responsive = ATOMIC_INIT(false);
	data->pn_snk_transition_to_default = ATOMIC_INIT(false);
	data->pn_hard_reset_received = ATOMIC_INIT(false);
	data->pn_source_capabilities_received = ATOMIC_INIT(false);
	data->pn_sender_response_timeout = ATOMIC_INIT(false);

	data->uut_received_src_cap_num = 0;
	data->snk_cap_pdo.raw_value = 0;
	data->rx_msg.len = 0;

	/* Initialize PDO for sending in step 5 */

	/* a) B31…30 (Fixed Supply) set to 00b */
	pdo.type = PDO_FIXED;
	/* b) B29 (Dual-Role Power) set to 1b */
	pdo.dual_role_power = 1;
	/* c) B28 (USB Suspend Supported) set to 0b */
	pdo.usb_suspend_supported = 0;
	/* d) B27 (Unconstrained Power) set to 1b */
	pdo.unconstrained_power = 1;
	/* e) B26 (USB Communications Capable) set to 0b */
	pdo.usb_comms_capable = 0;
	/* f) B25 (Dual-Role Data) set to 0b */
	pdo.dual_role_data = 0;
	/* g) B24 (PD3, Unchunked Extended Messages Supported) set to 0b */
	pdo.unchunked_ext_msg_supported = 0;
	/*
	 * h) B23 (EPR Mode Capable) to 0b, unless it is mentioned in the
	 *    test procedure. NOTE: NOT CURRENTLY SUPPORTED IN THE SUBSYSTEM.
	 */
	pdo.reserved0 = 0;
	/* i) B21…20 (Peak Current) set to 00b */
	pdo.peak_current = 0;
	/* j) B19…10 (Voltage) set to 5V */
	pdo.voltage = PD_CONVERT_MV_TO_FIXED_PDO_VOLTAGE(5000);
	/* k) B9…0 (Maximum Current) set to 100mA */
	pdo.max_current = PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(100);

	/* 1) The test starts in a disconnected state. */
	tester_disconnected();
	/* Give the Sink state machine time to transition */
	k_msleep(20);

	/* 2) Apply Rp */
	tester_apply_cc(TC_CC_VOLT_RP_3A0, TC_CC_VOLT_OPEN);

	/* 3) Apply vSafe5V on VBUS. */
	tester_apply_vbus(PD_V_SAFE_5V_MIN_MV);

	/* 4) The Tester waits until TC_ATTACHED_SNK state is reached. */
	for (i = 0; i < 500; i++) {
		/* NOT_PD_CONNECTED notification is sent when the PE starts up */
		if (atomic_get(&data->pn_not_pd_connected)) {
			break;
		}
		k_usleep(500);
	}

	/* 5) Send up to 50 source capability messages to UUT */
	for (i = 0, j = 0; i < 50; i++) {
		switch (j) {
		case 0:
			/* Send Source Cap message */
			tester_send_data_msg(PD_DATA_SOURCE_CAP, &pdo.raw_value, 1, true);
			j++;
			k = 0;
			break;
		case 1:
			/* Wait up to 50 mS for the UUT to receive the Source Cap message */
			if (atomic_get(&data->pn_source_capabilities_received)) {
				/* The UUT received the Source Cap Message, so exit loop */
				i = 50;
			} else if (++k == 50) {
				/* The UUT didn't detect the Source Cap message. Send it again */
				j = 0;
			}
			k_usleep(500);
			break;
		}
	}
	zassert_true(atomic_get(&data->pn_source_capabilities_received),
		     "UUT didn't receive Source Caps message");

	/* 6) Wait until UUT processes the Source Cap message */
	for (i = 0; i < 500; i++) {
		if (atomic_get(&data->pn_pd_connected)) {
			break;
		}
		k_usleep(500);
	}

	/* 7) The check fails if the UUT does not respond with a Request message. */
	zassert_true(atomic_get(&data->uut_request), "UUT didn't send request message.");
	atomic_set(&data->uut_request, false);

	/* Get the request message */
	tester_get_uut_tx_data(&data->rx_msg);
	zassert_equal(data->rx_msg.type, PD_PACKET_SOP, "UUT message not sent to SOP");
	zassert_equal(data->rx_msg.header.message_type, PD_DATA_REQUEST,
		      "UUT did not send request msg");

	/* Check the request message */
	check_request_message(dev);
	/* Send Accept and PS Ready messages to UUT */
	uut_sent_request(dev);

	zassert_equal(data->uut_received_src_cap_num, 1,
		      "UUT failed to respond to Source Capabilities message");
	zassert_equal(pdo.raw_value, data->uut_received_src_caps[0],
		      "Sent PDO does not match UUT's received PDO");
	zassert_true(atomic_get(&data->pn_pd_connected), "UUT not PD connected");

	/* An explicit contract is now established. */

	/*
	 * 8) The Tester presents SinkTxOK if the test is in PD3 mode.
	 *    The Tester waits 500ms to respond to messages from the UUT.
	 */
	k_msleep(500);

	data->pn_transition_ps = ATOMIC_INIT(false);
	data->uut_request = ATOMIC_INIT(false);

	printk("UUT Sink is up in PD%d mode\n", tester_get_rev() + 1);
}

ZTEST(test_usbc, test_sink_bringup)
{
	/** Test in PD2.0 mode */
	tester_set_rev_pd2();
	bring_up_sink_uut(usbc_port1);

	/** Test in PD3.0 mode */
	tester_set_rev_pd3();
	bring_up_sink_uut(usbc_port1);
}

ZTEST_SUITE(test_usbc, NULL, test_usbc_setup, test_usbc_before, test_usbc_after, NULL);
