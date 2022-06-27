/*
 * Copyright (c) 2022 The Chromium OS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/smf.h>
#include <zephyr/drivers/usbc/usbc_tcpc.h>

#include "board.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(snk, LOG_LEVEL_DBG);

#define TCPC_NODE       DT_ALIAS(tcpc)

/**
 * @brief Loop time: 5 ms
 */
#define LOOP_TIMEOUT_5MS        5

/**
 * @brief CC debounce time converted to 5mS counts
 */
#define T_CC_DEBOUNCE      (TC_T_CC_DEBOUNCE_MIN_MS / LOOP_TIMEOUT_5MS)

/**
 * @brief RP value change time converted to 5mS counts
 */
#define T_RP_VALUE_CHANGE  (TC_T_RP_VALUE_CHANGE_MIN_MS / LOOP_TIMEOUT_5MS)

/**
 * @brief Sink wait for source cap time converted to 5mS counts
 */
#define T_SINK_WAIT_CAP    (PD_T_TYPEC_SINK_WAIT_CAP_MAX_MS / LOOP_TIMEOUT_5MS)


/* PD message received flag*/
#define FLAG_PDMSG_RECEIVED        BIT(0)
/* Hard Reset message received flag */
#define FLAG_HARD_RESET_RECEIVED   BIT(1)
/* CC event flag */
#define FLAG_CC_EVENT              BIT(2)
/* PD Message Send Failed */
#define FLAG_PDMSG_FAILED          BIT(3)
/* PD Message Send was Discarded */
#define FLAG_PDMSG_DISCARDED       BIT(4)
/* PD Message Send was successful */
#define FLAG_PDMSG_SENT            BIT(5)

/*
 * USB Type-C Sink
 */

/**
 * @brief List of all TypeC-level states
 */
enum usb_tc_state {
	/** Unattached Sink */
	TC_UNATTACHED_SNK,
	/** Attach Wait Sink */
	TC_ATTACH_WAIT_SNK,
	/** Attached Sink */
	TC_ATTACHED_SNK,
};

/**
 * @brief Power Delivery states
 */
enum pd_states {
	/** Sink wait for source caps */
	SNK_WAIT_FOR_CAPABILITIES,
	/** Sink evaluate source caps */
	SNK_EVALUATE_CAPABILITY,
	/** Sink select source caps */
	SNK_SELECT_CAPABILITY,
	/** SinK transition sink */
	SNK_TRANSITION_SINK,
	/** Sink ready */
	SNK_READY,
};

/**
 * @brief Generic substates
 */
enum substate {
	/** Generic substate 0 */
	SUB_STATE0,
	/** Generic substate 1 */
	SUB_STATE1,
	/** Generic substate 2 */
	SUB_STATE2,
	/** Generic substate 3 */
	SUB_STATE3,
};

/* TypeC Only Power strings (No PD) */
static const char *const pwr2_5_str = "5V/0.5A";
static const char *const pwr7_5_str = "5V/1.5A";
static const char *const pwr15_str = "5V/3A";
static const char *const pwr_open_str = "Removed";

/**
 * @brief TypeC PD object
 */
static struct tc_t {
	/** state machine context */
	struct smf_ctx ctx;
	/** TypeC Port Controller device */
	const struct device *tcpc_dev;
	/** VBUS measurement device */
	const struct device *vbus_dev;
	/** Port polarity */
	enum tc_cc_polarity cc_polarity;
	/** Time a port shall wait before it can determine it is attached */
	uint32_t cc_debounce;
	/** The cc state */
	enum tc_cc_states cc_state;
	/** Voltage on CC pin */
	enum tc_cc_voltage_state cc_voltage;
	/** Current CC1 value */
	enum tc_cc_voltage_state cc1;
	/** Current CC2 value */
	enum tc_cc_voltage_state cc2;
	/** PD message for RX and TX */
	struct pd_msg msg;
	/** PD connection flag. Also used as PD contract in place flag */
	bool pd_connected;
	/** PD state */
	enum pd_states pd_state;
	/** Event flags */
	uint32_t flag;
	/** Transmission message id count */
	uint32_t msg_id_cnt;
	/** Receive message id */
	uint32_t msg_id;
	/** General timer */
	uint32_t timer;
	/** Sub state variable */
	enum substate sub_state;
	/** Source caps */
	uint32_t src_caps[7];
	/** Source caps header */
	union pd_header src_caps_hdr;
} tc;

static void set_state_tc(const enum usb_tc_state new_state);

/**
 * @brief Sink startup
 */
static void snk_startup(struct tc_t *tc)
{
	tc->flag = 0;
	tc->msg_id_cnt = 0;
	tc->msg_id = 0;
	tc->timer = 0;

	/* Not currently PD connected */
	tc->pd_connected = false;

	/* Initialize PD state */
	tc->pd_state = SNK_WAIT_FOR_CAPABILITIES;
}

/**
 * @brief Sink power sub states. Only called if a PD contract is ot in place
 */
static void sink_power_sub_states(void)
{
	enum tc_cc_voltage_state cc;
	enum tc_cc_voltage_state new_cc_voltage;
	char const *pwr;

	cc = tc.cc_polarity ? tc.cc2 : tc.cc1;

	if (cc == TC_CC_VOLT_RP_DEF) {
		new_cc_voltage = TC_CC_VOLT_RP_DEF;
		pwr = pwr2_5_str;
	} else if (cc == TC_CC_VOLT_RP_1A5) {
		new_cc_voltage = TC_CC_VOLT_RP_1A5;
		pwr = pwr7_5_str;
	} else if (cc == TC_CC_VOLT_RP_3A0) {
		new_cc_voltage = TC_CC_VOLT_RP_3A0;
		pwr = pwr15_str;
	} else {
		new_cc_voltage = TC_CC_VOLT_OPEN;
		pwr = pwr_open_str;
	}

	/* Debounce the cc state */
	if (new_cc_voltage != tc.cc_voltage) {
		tc.cc_voltage = new_cc_voltage;
		tc.cc_debounce = T_RP_VALUE_CHANGE;
	} else if (tc.cc_debounce) {
		/* Update timer */
		tc.cc_debounce--;
	}
}

/*
 * TYPE-C State Implementations
 */

/**
 * @brief Unattached.SNK
 */
static void tc_unattached_snk_entry(void *tc_obj)
{
	struct tc_t *tc = tc_obj;

	LOG_INF("UnAttached.SNK");

	tcpc_set_cc(tc->tcpc_dev, TC_CC_RD);
	tc->cc_voltage = TC_CC_VOLT_OPEN;
}

static void tc_unattached_snk_run(void *tc_obj)
{
	struct tc_t *tc = tc_obj;

	/*
	 * The port shall transition to AttachWait.SNK when a Source
	 * connection is detected, as indicated by the SNK.Rp state
	 * on at least one of its CC pins.
	 */
	if (tcpc_is_cc_rp(tc->cc1) || tcpc_is_cc_rp(tc->cc2)) {
		set_state_tc(TC_ATTACH_WAIT_SNK);
	}
}

/**
 * @brief AttachWait.SNK
 */
static void tc_attach_wait_snk_entry(void *tc_obj)
{
	struct tc_t *tc = tc_obj;

	LOG_INF("AttachedWait.SNK");
	tc->cc_state = TC_CC_NONE;
}

static void tc_attach_wait_snk_run(void *tc_obj)
{

	struct tc_t *tc = tc_obj;
	enum tc_cc_states new_cc_state;

	if (tcpc_is_cc_rp(tc->cc1) && tcpc_is_cc_rp(tc->cc2)) {
		new_cc_state = TC_CC_DFP_DEBUG_ACC;
	} else if (tcpc_is_cc_rp(tc->cc1) || tcpc_is_cc_rp(tc->cc2)) {
		new_cc_state = TC_CC_DFP_ATTACHED;
	} else {
		new_cc_state = TC_CC_NONE;
	}

	/* Debounce the cc state */
	if (new_cc_state != tc->cc_state) {
		tc->cc_debounce = T_CC_DEBOUNCE;
		tc->cc_state = new_cc_state;
	} else if (tc->cc_debounce) {
		/* Update timers */
		tc->cc_debounce--;
	}

	/* Wait for CC debounce */
	if (tc->cc_debounce) {
		return;
	}

	/*
	 * The port shall transition to Attached.SNK after the state of only
	 * one of the CC1 or CC2 pins is SNK.Rp for at least tCCDebounce and
	 * VBUS is detected.
	 */
	if (tcpc_check_vbus_level(tc->tcpc_dev, TC_VBUS_PRESENT) &&
	    (new_cc_state == TC_CC_DFP_ATTACHED)) {
		set_state_tc(TC_ATTACHED_SNK);
	} else {
		set_state_tc(TC_UNATTACHED_SNK);
	}
}
/**
 * @brief Display a single Power Delivery Object
 */
static void display_pdo(int idx, uint32_t pdo_value)
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
		LOG_INF("\tUnchunked Support: %d",
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
		union pd_battery_supply_pdo_source pdo;

		pdo.raw_value = pdo_value;
		LOG_INF("\tType:              BATTERY");
		LOG_INF("\tMin Voltage: %d",
		       PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(pdo.min_voltage));
		LOG_INF("\tMax Voltage: %d",
		       PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(pdo.max_voltage));
		LOG_INF("\tMax Power:   %d",
		       PD_CONVERT_BATTERY_PDO_POWER_TO_MW(pdo.max_power));
	}
	break;
	case PDO_VARIABLE: {
		union pd_variable_supply_pdo_source pdo;

		pdo.raw_value = pdo_value;
		LOG_INF("\tType:        VARIABLE");
		LOG_INF("\tMin Voltage: %d",
		       PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(pdo.min_voltage));
		LOG_INF("\tMax Voltage: %d",
		       PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(pdo.max_voltage));
		LOG_INF("\tMax Current: %d",
		       PD_CONVERT_VARIABLE_PDO_CURRENT_TO_MA(pdo.max_current));
	}
	break;
	case PDO_AUGMENTED: {
		union pd_augmented_supply_pdo_source pdo;

		pdo.raw_value = pdo_value;
		LOG_INF("\tType:              AUGMENTED");
		LOG_INF("\tMin Voltage:       %d",
		       PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(pdo.min_voltage));
		LOG_INF("\tMax Voltage:       %d",
		       PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(pdo.max_voltage));
		LOG_INF("\tMax Current:       %d",
		       PD_CONVERT_AUGMENTED_PDO_CURRENT_TO_MA(pdo.max_current));
		LOG_INF("\tPPS Power Limited: %d", pdo.pps_power_limited);
	}
	break;
	}
}

/**
 * @brief Display source caps
 */
static void display_source_caps(struct tc_t *tc)
{
	int pdo_num;
	union pd_header header = tc->src_caps_hdr;

	/* Make sure the message is a source caps */
	if (header.number_of_data_objects == 0 ||
	    header.message_type != PD_DATA_SOURCE_CAP) {
		return;
	}

	/* Get number of Power Data Objects (PDOs) */
	pdo_num = header.number_of_data_objects;

	LOG_INF("Source Caps:");
	for (int i = 0; i < pdo_num; i++) {
		display_pdo(i, tc->src_caps[i]);
	}
}

/**
 * @brief Attached.SNK
 */
static void tc_attached_snk_entry(void *tc_obj)
{
	struct tc_t *tc = tc_obj;

	LOG_INF("Attached.SNK");

	snk_startup(tc);

	/* Set CC polarity */
	tcpc_set_cc_polarity(tc->tcpc_dev, tc->cc_polarity);

	/* Enable PD */
	tcpc_set_rx_enable(tc->tcpc_dev, true);
}

static void tc_attached_snk_run(void *tc_obj)
{
	struct tc_t *tc = tc_obj;
	union pd_rdo rdo;

	/* Detach detection */
	if (!tcpc_check_vbus_level(tc->tcpc_dev, TC_VBUS_PRESENT)) {
		set_state_tc(TC_UNATTACHED_SNK);
		return;
	}

	/* Check if Hard Reset was received */
	if (tc->flag & FLAG_HARD_RESET_RECEIVED) {
		/* Flag is cleared in snk_startup function */
		snk_startup(tc);
	}

	switch (tc->pd_state) {
	case SNK_WAIT_FOR_CAPABILITIES:
		if (tc->timer == 0) {
			tc->timer = T_SINK_WAIT_CAP;
		} else {
			/* Update SinkWaitCapTimer */
			tc->timer--;
			if (tc->timer == 0) {
				LOG_INF("Source Caps not received. Continuing to wait!");
				tc->timer = T_SINK_WAIT_CAP;
				break;
			}
		}

		/* Check if a PD message was received */
		if (tc->flag & FLAG_PDMSG_RECEIVED) {
			/* Clear flag */
			tc->flag &= ~FLAG_PDMSG_RECEIVED;
			/* Get the message */
			if (!tcpc_receive_data(tc->tcpc_dev, &tc->msg)) {
				/* Problem getting the message */
				break;
			}
			/* Check if the message is a source caps message */
			if (tc->msg.header.number_of_data_objects == 0 ||
			    tc->msg.header.message_type != PD_DATA_SOURCE_CAP) {
				/* Not a source caps message */
				break;
			}
		} else {
			/* Message not received */
			break;
		}
		/* We got the source caps message */
		tc->pd_state = SNK_EVALUATE_CAPABILITY;
		__fallthrough;
	case SNK_EVALUATE_CAPABILITY:
		/* Save source caps header and payload */
		tc->src_caps_hdr = tc->msg.header;
		memcpy(tc->src_caps, tc->msg.data,
		       PD_CONVERT_PD_HEADER_COUNT_TO_BYTES(tc->msg.header.number_of_data_objects));

		/* Move on to the select capabilities */
		tc->pd_state = SNK_SELECT_CAPABILITY;
		tc->sub_state = SUB_STATE0;
		__fallthrough;
	case SNK_SELECT_CAPABILITY:
		switch (tc->sub_state) {
		case SUB_STATE0:
			/* Select the 5V PDO at index 1 */

			/* Create the PD header */

			/* Request message */
			tc->msg.header.message_type = PD_DATA_REQUEST;
			/* Always sink */
			tc->msg.header.port_power_role = TC_ROLE_SINK;
			/* Always upstream facing port */
			tc->msg.header.port_data_role = TC_ROLE_UFP;
			/* Message ID */
			tc->msg.header.message_id = tc->msg_id_cnt;
			/* one 4-byte Request Data Object (RDO) */
			tc->msg.header.number_of_data_objects = 1;
			/* PD Revision 2.0 */
			tc->msg.header.specification_revision = PD_REV20;
			/* Not an extended message */
			tc->msg.header.extended = 0;

			/* Create the Request Data Object */

			/* Maximum operating current 100mA (GIVEBACK = 0) */
			rdo.fixed.min_or_max_operating_current =
				PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(100);
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

			/* Set message payload. Select the 5V PDO */
			*(uint32_t *)tc->msg.data = rdo.raw_value;

			/* Set packet type */
			tc->msg.type = PD_PACKET_SOP;

			/* Set message length */
			tc->msg.len = 4;

			/* Send the message */
			tcpc_transmit_data(tc->tcpc_dev, &tc->msg);
			tc->sub_state = SUB_STATE1;
			break;
		case SUB_STATE1:
			if (tc->flag & FLAG_PDMSG_SENT) {
				tc->msg_id_cnt++;
				tc->sub_state = SUB_STATE2;
			} else if (tc->flag &
				   (FLAG_PDMSG_DISCARDED | FLAG_PDMSG_FAILED)) {
				LOG_INF("Failed to send Request message");
				set_state_tc(TC_UNATTACHED_SNK);
			} else {
				/* Waiting for message to send */
				break;
			}
			__fallthrough;
		case SUB_STATE2:
			/* Check if a PD message was received */
			if (tc->flag & FLAG_PDMSG_RECEIVED) {
				/* Clear flag */
				tc->flag &= ~FLAG_PDMSG_RECEIVED;
				/* Get the message */
				if (!tcpc_receive_data(tc->tcpc_dev,
						       &tc->msg)) {
					/* Problem getting the message */
					break;
				}
				/* Check if the message is an Accept message */
				if (tc->msg.header.number_of_data_objects != 0 ||
				    tc->msg.header.message_type != PD_CTRL_ACCEPT) {
					/* Not an Accept message */
					break;
				}
			} else {
				/* Message not received */
				break;
			}
			LOG_INF("Received PD_CTRL_ACCEPT");
			tc->sub_state = SUB_STATE3;
			break;
		case SUB_STATE3:
			/* Check if a PD message was received */
			if (tc->flag & FLAG_PDMSG_RECEIVED) {
				/* Clear flag */
				tc->flag &= ~FLAG_PDMSG_RECEIVED;
				/* Get the message */
				if (!tcpc_receive_data(tc->tcpc_dev, &tc->msg)) {
					/* Problem getting the message */
					break;
				}
				/* Check if the message is a PS Ready message */
				if (tc->msg.header.number_of_data_objects != 0 ||
				    tc->msg.header.message_type != PD_CTRL_PS_RDY) {
					break;
				}
			} else {
				break;
			}
			LOG_INF("Received PD_CTRL_PS_RDY");
			/* PD contract in place and PD connected */
			tc->pd_connected = true;
			tc->pd_state = SNK_TRANSITION_SINK;
		}
		break;
	case SNK_TRANSITION_SINK:
		/* Display Source Caps Received from Source */
		display_source_caps(tc);
		tc->sub_state = SUB_STATE0;
		tc->pd_state = SNK_READY;
		__fallthrough;
	case SNK_READY:
		switch (tc->sub_state) {
		case SUB_STATE0:
			LOG_INF("SNK_READY");
			tc->sub_state++;
			__fallthrough;
		case SUB_STATE1:
			/* STAY HERE FOREVER AND REJECT ALL RECEIVED MESSAGES */

			/* Check if a PD message was received */
			if (tc->flag & FLAG_PDMSG_RECEIVED) {
				/* Clear flag */
				tc->flag &= ~FLAG_PDMSG_RECEIVED;

				/* Create the PD header */

				/* Reject message */
				tc->msg.header.message_type = PD_CTRL_REJECT;
				/* Always sink */
				tc->msg.header.port_power_role = TC_ROLE_SINK;
				/* Always upstream facing port */
				tc->msg.header.port_data_role = TC_ROLE_UFP;
				/* Message ID */
				tc->msg.header.message_id = tc->msg_id_cnt;
				/* one 4-byte Request Data Object (RDO) */
				tc->msg.header.number_of_data_objects = 0;
				/* PD Revision 2.0 */
				tc->msg.header.specification_revision = PD_REV20;
				/* Not an extended message */
				tc->msg.header.extended = 0;

				/* Set packet type */
				tc->msg.type = PD_PACKET_SOP;

				/* Set message length */
				tc->msg.len = 0;

				/* Send the message */
				tcpc_transmit_data(tc->tcpc_dev, &tc->msg);
			}
			break;
		default:
			break;
		}
		break;
	}

	/* Only the sink power sub states when we are not in a pd contract */
	if (!tc->pd_connected) {
		/* Run Sink Power Sub-State */
		sink_power_sub_states();
	}
}

static void tc_attached_snk_exit(void *tc_obj)
{
	struct tc_t *tc = tc_obj;

	tc->pd_connected = false;
	/* Disable PD */
	tcpc_set_rx_enable(tc->tcpc_dev, false);
}

/**
 * @brief Type-C State Table
 */
static struct smf_state tc_states[] = {
	/** Unattached Sink */
	[TC_UNATTACHED_SNK] = {
		.entry = tc_unattached_snk_entry,
		.run = tc_unattached_snk_run,
	},
	/** Attach Wait Sink */
	[TC_ATTACH_WAIT_SNK] = {
		.entry = tc_attach_wait_snk_entry,
		.run = tc_attach_wait_snk_run,
	},
	/** Attached Sink */
	[TC_ATTACHED_SNK] = {
		.entry = tc_attached_snk_entry,
		.run = tc_attached_snk_run,
		.exit = tc_attached_snk_exit
	},
};

/**
 * @brief Set the TypeC state machine to a new state
 */
static void set_state_tc(const enum usb_tc_state new_state)
{
	smf_set_state(SMF_CTX(&tc), &tc_states[new_state]);
}

/**
 * @brief Alert Handler called by the TCPC driver
 */
static void alert_handler(const struct device *dev,
			  void *data, enum tcpc_alert alert)
{
	struct tc_t *tc = data;

	switch (alert) {
	case TCPC_ALERT_CC_STATUS:
		tc->flag |= FLAG_CC_EVENT;
		break;
	case TCPC_ALERT_POWER_STATUS:
		break;
	case TCPC_ALERT_MSG_STATUS:
		tc->flag |= FLAG_PDMSG_RECEIVED;
		break;
	case TCPC_ALERT_HARD_RESET_RECEIVED:
		tc->flag |= FLAG_HARD_RESET_RECEIVED;
		break;
	case TCPC_ALERT_TRANSMIT_MSG_FAILED:
		tc->flag |= FLAG_PDMSG_FAILED;
		break;
	case TCPC_ALERT_TRANSMIT_MSG_DISCARDED:
		tc->flag |= FLAG_PDMSG_DISCARDED;
		break;
	case TCPC_ALERT_TRANSMIT_MSG_SUCCESS:
		tc->flag |= FLAG_PDMSG_SENT;
		break;
	/* These alerts are ignored */
	default:
		break;
	}
}

int discharge_vbus(const struct device *dev, bool en)
{
	return board_discharge_vbus(en);
}

int vbus_meas(const struct device *dev, int *vbus_meas)
{
	int ret;

	ret = board_vbus_meas(vbus_meas);
	if (ret != 0) {
		*vbus_meas = 0;
	}

	return 0;
}

int sink_init(void)
{
	tc.tcpc_dev = DEVICE_DT_GET(TCPC_NODE);
	if (!device_is_ready(tc.tcpc_dev)) {
		LOG_ERR("TCPC device not ready");
		return -ENODEV;
	}

	/* Set the Hard Reset Received callback */
	tcpc_set_alert_handler_cb(tc.tcpc_dev, alert_handler, &tc);

	/* Set the VBUS measurement callback */
	tcpc_set_vbus_measure_cb(tc.tcpc_dev, vbus_meas);

	/* Set the VBUS discharge callback */
	tcpc_set_discharge_vbus_cb(tc.tcpc_dev, discharge_vbus);

	/* Remove resistors from CC1 */
	tcpc_set_cc(tc.tcpc_dev, TC_CC_OPEN);

	/* Allow time for disconnection to be detected */
	k_msleep(100);

	/* Unattached.SNK is the default starting state. */
	smf_set_initial(SMF_CTX(&tc), &tc_states[TC_UNATTACHED_SNK]);

	return 0;
}

int sink_sm(void)
{
	/* Sample CC lines */
	tcpc_get_cc(tc.tcpc_dev, &tc.cc1, &tc.cc2);

	/* Detect polarity */
	tc.cc_polarity = (tc.cc1 > tc.cc2) ?
			 TC_POLARITY_CC1 : TC_POLARITY_CC2;

	/* Run TypeC state machine */
	smf_run_state(SMF_CTX(&tc));

	return LOOP_TIMEOUT_5MS;
}
