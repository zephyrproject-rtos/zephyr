/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MOCK_TESTER_H_
#define ZEPHYR_MOCK_TESTER_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/usb_c/usbc_tc.h>

#include "mock_tcpc.h"

/**
 * @brief Tester performs a disconnect
 */
void tester_disconnected(void);

/**
 * @brief Tester sets the CC lines to a voltage level
 *
 * @param cc1 Voltage to place on cc1
 * @param cc2 Voltage to place on cc2
 */
void tester_apply_cc(enum tc_cc_voltage_state cc1, enum tc_cc_voltage_state cc2);

/**
 * @breif Tester sets the CC lines RP value
 *
 * @param cc1 RP value to place on cc1
 * @param cc2 RP value to place on cc2
 */
void tester_apply_rp(enum tc_rp_value cc1, enum tc_rp_value cc2);

/**
 * @brief saves the pull resistor set by the UUT
 *
 * @param pull UUT's cc pull resistor
 */
void tester_set_uut_cc(enum tc_cc_pull pull);

/**
 * @brief Saves the UUT's CC polarity
 *
 * @param polarity UUT's CC polarity
 */
void tester_set_uut_polarity(enum tc_cc_polarity polarity);

/**
 * @brief Get CC1's voltage level
 *
 * @retval CC1 voltage level
 */
enum tc_cc_voltage_state tester_get_cc1(void);

/**
 * @brief Get CC2's voltage level
 *
 * @retval CC2 voltage level
 */
enum tc_cc_voltage_state tester_get_cc2(void);

/**
 * @breif Tester sets the VBUS voltage level
 *
 * @param level VBUS level to set
 */
void tester_apply_vbus_level(enum tc_vbus_level level);

/**
 * @brief Tester VBUS voltage to set
 *
 * @param mv VBUS voltage to set
 */
void tester_apply_vbus(int mv);

/**
 * @brief Get the VBUS voltage set by the tester
 *
 * @retval VBUS volage
 */
int tester_get_vbus(void);

/**
 * @breif Give tester access to the TCPC's data
 *
 * @param tcpc pointer to TCPC's data
 */
void tester_set_tcpc_device(struct mock_tcpc_data *tcpc);

/**
 * @brief Send a Control Message to the UUT
 *
 * @param type Control message type
 * @param true if the message ID should be incremented
 */
void tester_send_ctrl_msg(enum pd_ctrl_msg_type type, bool inc_msgid);

/**
 * @brief Send a Data Message to the UUT
 *
 * @param type Data message type
 * @param true if the message ID should be incremented
 */
void tester_send_data_msg(enum pd_data_msg_type type, uint32_t *data, uint32_t len, bool inc_msgid);

/**
 * @brief Set tester's PD revision to 2
 */
void tester_set_rev_pd2(void);

/**
 * @brief Set tester's PD revision to 3
 */
void tester_set_rev_pd3(void);

/**
 * @breif Get tester's PD revision
 *
 * @retval PD revision
 */
enum pd_rev_type tester_get_rev(void);

/**
 * @brief Set tester's power role to Source
 */
void tester_set_power_role_source(void);

/**
 * @brief Set tester's data role to UFP
 */
void tester_set_data_role_ufp(void);

/**
 * @brief Set tester's data role to DFP
 */
void tester_set_data_role_dfp(void);

/**
 * @brief Increment the PD message ID
 */
void tester_msgid_inc(void);

/**
 * @brief Get PD message sent from UUT
 *
 * @param msg PD message sent from UUT
 */
void tester_get_uut_tx_data(struct pd_msg *msg);

/**
 * @brief Get the PD message ID
 *
 * @retval message id
 */
int tester_get_msgid(void);

/**
 * @brief Tests if UUT has a pending RX PD message
 *
 * @retval true if UUT has a pending RX PD message
 */
bool tester_is_rx_msg_pending(void);

/**
 * @brief Send a Hard Reset to the UUT
 */
void tester_send_hard_reset(void);

#endif /* ZEPHYR_MOCK_TESTER_H_ */
