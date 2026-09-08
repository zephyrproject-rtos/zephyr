/** @file
 *  @brief Bluetooth HID protocol values shared by the Device and Host roles.
 */

/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_H_

/**
 * @brief Bluetooth HID
 * @defgroup bt_hid Bluetooth HID
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name HID protocol mode values
 *
 * Defined in HID spec v1.1.2 Section 2.1.2, which makes Report Protocol Mode
 * the default. The same values are carried by GET_PROTOCOL and SET_PROTOCOL in
 * both directions, so they are shared by the Device and the Host role.
 * @{
 */
/** Boot Protocol Mode (legacy). */
#define BT_HID_PROTOCOL_BOOT_MODE   0x00U
/** Report Protocol Mode (default). */
#define BT_HID_PROTOCOL_REPORT_MODE 0x01U
/** @} */

/**
 * @name HID report type values
 *
 * The report type carried by GET_REPORT, SET_REPORT and DATA transfers.
 * Defined in HID spec v1.1.2 Table 3.4.
 * @{
 */
/** Reserved report type, used where no type applies. */
#define BT_HID_REPORT_TYPE_OTHER   0x00U
/** Input report (device to host). */
#define BT_HID_REPORT_TYPE_INPUT   0x01U
/** Output report (host to device). */
#define BT_HID_REPORT_TYPE_OUTPUT  0x02U
/** Feature report. */
#define BT_HID_REPORT_TYPE_FEATURE 0x03U
/** @} */

/**
 * @name HID HANDSHAKE result codes
 *
 * Result of a request sent on the control channel. Defined in HID spec v1.1.2
 * Section 3.1.2.1. A Host reports them to the application, a Device answers
 * with them, so they are shared by both roles.
 * @{
 */
/** The request completed successfully. */
#define BT_HID_HS_RSP_SUCCESS               0x00U
/** The peer is too busy to accept the request; the sender may retry. */
#define BT_HID_HS_RSP_NOT_READY             0x01U
/** The request referenced an unsupported Report ID. */
#define BT_HID_HS_RSP_ERR_INVALID_REPORT_ID 0x02U
/** The peer does not support the request. */
#define BT_HID_HS_RSP_ERR_UNSUPPORTED_REQ   0x03U
/** The request carried an invalid parameter. */
#define BT_HID_HS_RSP_ERR_INVALID_PARAM     0x04U
/** The peer could not identify the error condition. */
#define BT_HID_HS_RSP_ERR_UNKNOWN           0x0eU
/** The peer encountered a fatal error and needs to be restarted. */
#define BT_HID_HS_RSP_ERR_FATAL             0x0fU
/** @} */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_H_ */
