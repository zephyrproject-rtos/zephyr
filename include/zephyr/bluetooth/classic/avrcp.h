/** @file
 *  @brief Audio Video Remote Control Profile header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief AVRCP structure */
struct bt_avrcp;

struct bt_avrcp_unit_info_rsp {
	uint8_t unit_type;
	uint32_t company_id;
};

struct bt_avrcp_subunit_info_rsp {
	uint8_t subunit_type;
	uint8_t max_subunit_id;
	const uint8_t *extended_subunit_type; /**< contains max_subunit_id items */
	const uint8_t *extended_subunit_id; /**< contains max_subunit_id items */
};

struct bt_avrcp_cb {
	/** @brief An AVRCP connection has been established.
	 *
	 *  This callback notifies the application of an avrcp connection,
	 *  i.e., an AVCTP L2CAP connection.
	 *
	 *  @param avrcp AVRCP connection object.
	 */
	void (*connected)(struct bt_avrcp *avrcp);
	/** @brief An AVRCP connection has been disconnected.
	 *
	 *  This callback notifies the application that an avrcp connection
	 *  has been disconnected.
	 *
	 *  @param avrcp AVRCP connection object.
	 */
	void (*disconnected)(struct bt_avrcp *avrcp);
	/** @brief Callback function for bt_avrcp_get_unit_info().
	 *
	 *  Called when the get unit info process is completed.
	 *
	 *  @param avrcp AVRCP connection object.
	 *  @param rsp The response for UNIT INFO command.
	 */
	void (*unit_info_rsp)(struct bt_avrcp *avrcp, struct bt_avrcp_unit_info_rsp *rsp);
	/** @brief Callback function for bt_avrcp_get_subunit_info().
	 *
	 *  Called when the get subunit info process is completed.
	 *
	 *  @param avrcp AVRCP connection object.
	 *  @param rsp The response for SUBUNIT INFO command.
	 */
	void (*subunit_info_rsp)(struct bt_avrcp *avrcp, struct bt_avrcp_subunit_info_rsp *rsp);
};

/** @brief Connect AVRCP.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish AVRCP
 *  connection between devices.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return pointer to struct bt_avrcp in case of success or NULL in case
 *  of error.
 */
struct bt_avrcp *bt_avrcp_connect(struct bt_conn *conn);

/** @brief Disconnect AVRCP.
 *
 *  This function close AVCTP L2CAP connection.
 *
 *  @param avrcp The AVRCP instance.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_disconnect(struct bt_avrcp *avrcp);

/** @brief Register callback.
 *
 *  Register AVRCP callbacks to monitor the state and interact with the remote device.
 *
 *  @param cb The callback function.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_register_cb(const struct bt_avrcp_cb *cb);

/** @brief Get AVRCP Unit Info.
 *
 *  This function obtains information that pertains to the AV/C unit as a whole.
 *
 *  @param avrcp The AVRCP instance.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_get_unit_info(struct bt_avrcp *avrcp);

/** @brief Get AVRCP Subunit Info.
 *
 *  This function obtains information about the subunit(s) of an AV/C unit. A device with AVRCP
 *  may support other subunits than the panel subunit if other profiles co-exist in the device.
 *
 *  @param avrcp The AVRCP instance.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_get_subunit_info(struct bt_avrcp *avrcp);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_H_ */
