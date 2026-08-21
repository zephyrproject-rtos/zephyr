/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth HID over GATT Profile (HOGP) Device role API.
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOGP_DEVICE_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOGP_DEVICE_H_

#include <zephyr/bluetooth/services/hids.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HID over GATT Profile Device
 * @defgroup bt_hogp_device HID over GATT Profile Device
 *
 * @since 4.5
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 *
 * Implementation of the HID Device role of the HID over GATT Profile (HOGP).
 *
 * The profile layer composes the services required by HOGP v1.1, Section 3,
 * Table 3.1 and applies the profile level requirements on top of them:
 *
 * - The HID Service (@ref bt_hids), the Battery Service and the Device
 *   Information Service including the PnP ID characteristic are mandatory and
 *   are selected by @kconfig{CONFIG_BT_HOGP_DEVICE}.
 * - Security requirements of Section 7.1: the Device is bondable and requests
 *   an encrypted link when a Host connects, see
 *   @kconfig{CONFIG_BT_HOGP_DEVICE_SECURITY_REQUEST}.
 *
 * Reports are exchanged through this profile API, for example
 * bt_hogp_device_send_report(). The HID Service API (@ref bt_hids) provides
 * the GATT level primitives the profile is built on.
 *
 * The advertising requirements of Sections 3.1.3 to 3.1.5 and the
 * NormallyConnectable behavior of Section 3.1.7 and Appendix A are the
 * responsibility of the application, as it owns the advertising data and the
 * advertising state.
 *
 * Not implemented: the Scan Parameters Service (Section 3.4, optional) and
 * HID ISO (Sections 5 and 6). Section 7.1 recommends that the Battery Service
 * and the Device Information Service characteristics use the same security
 * level as the HID Service characteristics, which the in-tree implementations
 * of those services do not support.
 */

/** HOGP Device registration parameters */
struct bt_hogp_device_register_param {
	/** HID Service configuration */
	struct bt_hids_register_param hids;
};

/**
 * @brief Register the HOGP Device role.
 *
 * Registers the HID Service with the given parameters and enables the profile
 * level behavior on top of it.
 *
 * The HID Service configuration is passed to bt_hids_register(), so the Report
 * Map and the callback structure must remain valid until
 * bt_hogp_device_unregister() is called.
 *
 * @param[in] param Registration parameters.
 *
 * @retval 0 Success.
 * @retval -EALREADY Already registered.
 * @retval -EINVAL Invalid parameters.
 */
int bt_hogp_device_register(const struct bt_hogp_device_register_param *param);

/**
 * @brief Unregister the HOGP Device role.
 *
 * Unregisters the HID Service and disables the profile level behavior.
 * Connected Hosts stay connected.
 *
 * @retval 0 Success.
 * @retval -EALREADY Not registered.
 * @retval -errno Negative errno from bt_hids_unregister().
 */
int bt_hogp_device_unregister(void);

/**
 * @brief Send an Input Report to a Host.
 *
 * Reports are sent over GATT. HOGP v1.1, Section 5.2 also allows selected
 * reports to be sent over LE Isochronous Channels in Hybrid Operation mode,
 * which is not supported yet.
 *
 * @param[in] conn       Target connection, or NULL for all subscribed Hosts.
 * @param[in] report_id  HID Report ID.
 * @param[in] data       Report payload (excluding the Report ID byte).
 * @param[in] len        Payload length.
 * @param[in] func       Optional completion callback, NULL if not needed.
 * @param[in] user_data  User data passed to the completion callback.
 *
 * @retval 0 Success.
 * @retval -ESRCH Not registered.
 * @retval -ENOENT Report ID not found.
 * @retval -EINVAL The Host is not subscribed to the Input Report.
 * @retval -errno Negative errno from bt_hids_send_report().
 */
int bt_hogp_device_send_report(struct bt_conn *conn, uint8_t report_id,
			       const uint8_t *data, uint16_t len,
			       bt_gatt_complete_func_t func, void *user_data);

/**
 * @brief Get the Protocol Mode of a Host.
 *
 * See bt_hids_get_protocol_mode().
 *
 * @param[in]  conn Connection to the Host.
 * @param[out] mode Protocol Mode.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid parameters.
 * @retval -ESRCH Not registered.
 * @retval -ENOTCONN No HID state tracked for this connection.
 */
int bt_hogp_device_get_protocol_mode(struct bt_conn *conn,
				     enum bt_hid_protocol_mode *mode);

/**
 * @brief Get the Suspend state of a Host.
 *
 * See bt_hids_get_suspend_state().
 *
 * @param[in]  conn      Connection to the Host.
 * @param[out] suspended true if the Host suspended the Device.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid parameters.
 * @retval -ESRCH Not registered.
 * @retval -ENOTCONN No HID state tracked for this connection.
 */
int bt_hogp_device_get_suspend_state(struct bt_conn *conn, bool *suspended);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOGP_DEVICE_H_ */
