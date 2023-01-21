/*
 * Copyright 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <stddef.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>

#include "mock_tester.h"
#include "mock_tcpc.h"

/**
 * @breif Tester data
 */
struct mock_tester_t {
	/* pointer to mock tcpc */
	struct mock_tcpc_data *tcpc;
	/* CC1 voltage state */
	enum tc_cc_voltage_state cc1;
	/* CC2 voltage state */
	enum tc_cc_voltage_state cc2;
	/* CC Pull set by UUT */
	enum tc_cc_pull uut_cc_pull;
	/* CC Polarity set by UUT */
	enum tc_cc_polarity uut_polarity;

	/* VBUS set by UUT or Tester */
	int vbus;
	/* PD Message to send to the UUT */
	struct pd_msg tx_msg;
	/* PD Message received from the UUT */
	struct pd_msg rx_msg;

	/* PD message ID */
	int msg_id;
	/* PD revision */
	enum pd_rev_type rev;
	/* Tester's Power Role */
	enum tc_power_role power_role;
	/* Tester's Data Role */
	enum tc_data_role data_role;
};

/* Test data object */
static struct mock_tester_t tester;

/**
 * @brief Set the PD message ID
 *
 * @param id The id to set
 */
void tester_msgid_set(int id)
{
	tester.tx_msg.header.message_id = id & 7;
}

/**
 * @brief Increment the PD message ID
 */
void tester_msgid_inc(void)
{
	tester.tx_msg.header.message_id++;
	tester.tx_msg.header.message_id &= 7;
}

/**
 * @brief Get the PD message ID
 *
 * @retval message id
 */
int tester_get_msgid(void)
{
	return tester.tx_msg.header.message_id;
}

/**
 * @brief Send a PD message to the UUT
 *
 * @param msg PD message to send
 */
void tester_transmit_data(struct pd_msg *msg)
{
	if (tester.tcpc->rx_enable) {
		memcpy(&tester.tcpc->rx_msg, msg, sizeof(struct pd_msg));
		tester.tcpc->pending_rx_msg = true;
	}
}

/**
 * @brief Send a Hard Reset to the UUT
 */
void tester_send_hard_reset(void)
{
	memset(&tester.tcpc->rx_msg, 0, sizeof(struct pd_msg));
	tester.tcpc->rx_msg.type = PD_PACKET_TX_HARD_RESET;
	tester.tcpc->pending_rx_msg = true;
}

/**
 * @brief Send a Control Message to the UUT
 *
 * @param type Control message type
 * @param true if the message ID should be incremented
 */
void tester_send_ctrl_msg(enum pd_ctrl_msg_type type, bool inc_msgid)
{
	tester.tx_msg.type = PD_PACKET_SOP;
	tester.tx_msg.header.message_type = type;
	tester.tx_msg.header.port_data_role = tester.data_role;
	tester.tx_msg.header.specification_revision = tester.rev;
	tester.tx_msg.header.port_power_role = tester.power_role;

	if (inc_msgid) {
		tester_msgid_inc();
	}

	tester.tx_msg.header.number_of_data_objects = 0;
	tester.tx_msg.header.extended = 0;
	tester.tx_msg.len = 0;

	tester_transmit_data(&tester.tx_msg);
}

/**
 * @brief Send a Data Message to the UUT
 *
 * @param type Data message type
 * @param true if the message ID should be incremented
 */
void tester_send_data_msg(enum pd_data_msg_type type, uint32_t *data, uint32_t len, bool inc_msgid)
{
	tester.tx_msg.type = PD_PACKET_SOP;
	tester.tx_msg.header.message_type = type;
	tester.tx_msg.header.port_data_role = tester.data_role;
	tester.tx_msg.header.specification_revision = tester.rev;
	tester.tx_msg.header.port_power_role = tester.power_role;

	if (inc_msgid) {
		tester_msgid_inc();
	}

	tester.tx_msg.header.number_of_data_objects = len;
	tester.tx_msg.header.extended = 0;
	tester.tx_msg.len = len * 4;
	memcpy(tester.tx_msg.data, (uint8_t *)data, len * 4);

	tester_transmit_data(&tester.tx_msg);
}

/**
 * @breif Get tester's PD revision
 *
 * @retval PD revision
 */
enum pd_rev_type tester_get_rev(void)
{
	return tester.rev;
}

/**
 * @brief Set tester's PD revision to 2
 */
void tester_set_rev_pd2(void)
{
	tester.rev = PD_REV20;
}

/**
 * @brief Set tester's PD revision to 3
 */
void tester_set_rev_pd3(void)
{
	tester.rev = PD_REV30;
}

/**
 * @brief Set tester's power role to Source
 */
void tester_set_power_role_source(void)
{
	tester.power_role = TC_ROLE_SOURCE;
}

/**
 * @brief Set tester's data role to UFP
 */
void tester_set_data_role_ufp(void)
{
	tester.data_role = TC_ROLE_UFP;
}

/**
 * @brief Set tester's data role to DFP
 */
void tester_set_data_role_dfp(void)
{
	tester.data_role = TC_ROLE_DFP;
}

/**
 * @breif Give tester access to the TCPC's data
 *
 * @param tcpc pointer to TCPC's data
 */
void tester_set_tcpc_device(struct mock_tcpc_data *tcpc)
{
	tester.tcpc = tcpc;
}

/**
 * @brief Tester performs a disconnect
 */
void tester_disconnected(void)
{
	tester_apply_vbus(0);
	tester_apply_cc(TC_CC_VOLT_OPEN, TC_CC_VOLT_OPEN);
	tester_msgid_set(0);
}

/**
 * @brief Tester sets the CC lines to a voltage level
 *
 * @param cc1 Voltage to place on cc1
 * @param cc2 Voltage to place on cc2
 */
void tester_apply_cc(enum tc_cc_voltage_state cc1, enum tc_cc_voltage_state cc2)
{
	tester.cc1 = cc1;
	tester.cc2 = cc2;
}

/**
 * @brief saves the pull resistor set by the UUT
 *
 * @param pull UUT's cc pull resistor
 */
void tester_set_uut_cc(enum tc_cc_pull pull)
{
	tester.uut_cc_pull = pull;
}

/**
 * @breif Tester sets the CC lines RP value
 *
 * @param cc1 RP value to place on cc1
 * @param cc2 RP value to place on cc2
 */
void tester_apply_rp(enum tc_rp_value cc1, enum tc_rp_value cc2)
{
	switch (cc1) {
	case TC_RP_USB:
		tester.cc1 = TC_CC_VOLT_RP_DEF;
		break;
	case TC_RP_1A5:
		tester.cc1 = TC_CC_VOLT_RP_1A5;
		break;
	case TC_RP_3A0:
		tester.cc1 = TC_CC_VOLT_RP_3A0;
		break;
	case TC_RP_RESERVED:
		tester.cc1 = TC_CC_VOLT_OPEN;
		break;
	}

	switch (cc2) {
	case TC_RP_USB:
		tester.cc2 = TC_CC_VOLT_RP_DEF;
		break;
	case TC_RP_1A5:
		tester.cc2 = TC_CC_VOLT_RP_1A5;
		break;
	case TC_RP_3A0:
		tester.cc2 = TC_CC_VOLT_RP_3A0;
		break;
	case TC_RP_RESERVED:
		tester.cc2 = TC_CC_VOLT_OPEN;
		break;
	}
}

/**
 * @brief Get CC1's voltage level
 *
 * @retval CC1 voltage level
 */
enum tc_cc_voltage_state tester_get_cc1(void)
{
	if (tester.power_role == TC_ROLE_SOURCE) {
		if (tester.uut_cc_pull == TC_CC_RP || tester.uut_cc_pull == TC_CC_OPEN) {
			return TC_CC_VOLT_OPEN;
		}
	} else {
		if (tester.uut_cc_pull != TC_CC_RP) {
			return TC_CC_VOLT_OPEN;
		}
	}

	return tester.cc1;
}

/**
 * @brief Get CC2's voltage level
 *
 * @retval CC2 voltage level
 */
enum tc_cc_voltage_state tester_get_cc2(void)
{
	if (tester.power_role == TC_ROLE_SOURCE) {
		if (tester.uut_cc_pull == TC_CC_RP || tester.uut_cc_pull == TC_CC_OPEN) {
			return TC_CC_VOLT_OPEN;
		}
	} else {
		if (tester.uut_cc_pull != TC_CC_RP) {
			return TC_CC_VOLT_OPEN;
		}
	}

	return tester.cc2;
}

/**
 * @brief Saves the UUT's CC polarity
 *
 * @param polarity UUT's CC polarity
 */
void tester_set_uut_polarity(enum tc_cc_polarity polarity)
{
	tester.uut_polarity = polarity;
}

/**
 * @breif Tester sets the VBUS voltage level
 *
 * @param level VBUS level to set
 */
void tester_apply_vbus_level(enum tc_vbus_level level)
{
	switch (level) {
	case TC_VBUS_SAFE0V:
		tester.vbus = PD_V_SAFE_0V_MAX_MV - 1;
		break;
	case TC_VBUS_PRESENT:
		tester.vbus = PD_V_SAFE_5V_MIN_MV;
		break;
	case TC_VBUS_REMOVED:
		tester.vbus = TC_V_SINK_DISCONNECT_MIN_MV - 1;
		break;
	}
}

/**
 * @brief Tester VBUS voltage to set
 *
 * @param mv VBUS voltage to set
 */
void tester_apply_vbus(int mv)
{
	tester.vbus = mv;
}

/**
 * @brief Get the VBUS voltage set by the tester
 *
 * @retval VBUS volage
 */
int tester_get_vbus(void)
{
	return tester.vbus;
}

/**
 * @brief Get PD message sent from UUT
 *
 * @param msg PD message sent from UUT
 */
void tester_get_uut_tx_data(struct pd_msg *msg)
{
	tester.tcpc->pending_tx_msg = false;
	memcpy(msg, &tester.tcpc->tx_msg, sizeof(struct pd_msg));
}

/**
 * @brief Tests if UUT has a pending RX PD message
 *
 * @retval true if UUT has a pending RX PD message
 */
bool tester_is_rx_msg_pending(void)
{
	return tester.tcpc->pending_rx_msg;
}
