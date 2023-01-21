/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MOCK_TCPC_PRIV_H_
#define ZEPHYR_MOCK_TCPC_PRIV_H_

#include <zephyr/sys/util.h>
#include <zephyr/drivers/usb_c/usbc_tcpc.h>

/**
 * @brief GoodCRC message header roles
 */
struct msg_header_info {
	/* Power Role */
	enum tc_power_role pr;
	/* Data Role */
	enum tc_data_role dr;
};

/**
 * @brief Mock TCPC Driver Config Data
 */
struct mock_tcpc_config {
	int x;
};

/**
 * @brief Mock TCPC Driver data
 */
struct mock_tcpc_data {
	/* True if the TCPC can receive PD messages */
	bool rx_enable;
	/* True if the TCPC has a pending RX PD message */
	bool pending_rx_msg;
	/* TCPC RX PD message */
	struct pd_msg rx_msg;
	/* True if the TCPC has a pending TX PD message */
	bool pending_tx_msg;
	/* TCPC TX PD message */
	struct pd_msg tx_msg;

	/* GoodCRC message Header */
	struct msg_header_info msg_header;

	/* Alert data passed to the alert handler callback */
	void *alert_data;
	/* Application's alert handler callback */
	tcpc_alert_handler_cb_t alert_handler;
};

#endif /* ZEPHYR_MOCK_TCPC_PRIV_H_ */
