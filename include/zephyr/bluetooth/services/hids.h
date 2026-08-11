/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth HID Service (HIDS) API.
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth HID Service (HIDS)
 * @defgroup bt_hids Bluetooth HID Service (HIDS)
 *
 * @since 4.5
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 *
 * Server side implementation of the HID Service, as used by the HID over GATT
 * Profile (HOGP) HID Device role. The GATT attribute table is built at
 * registration time from the reports declared by the application.
 *
 * Not supported yet: multiple HID Service instances (HOGP v1.1, Section 2.5),
 * the «Include» of external services whose characteristics are described in
 * the Report Map together with the External Report Reference descriptor
 * (Section 3.1.1), and the Boot Keyboard and Boot Mouse Report
 * characteristics.
 */

/** Maximum Report Map length of one HID Service (HOGP v1.1, Section 2.5) */
#define BT_HIDS_REPORT_MAP_MAX_LEN 512

/**
 * @brief HID Report Type values.
 *
 * Defined in HID Service Specification (HIDS v1.0), Section 2.2,
 * Report Reference Characteristic Descriptor.
 */
enum bt_hid_report_type {
	/** Input Report */
	BT_HID_REPORT_TYPE_INPUT   = 0x01U,
	/** Output Report */
	BT_HID_REPORT_TYPE_OUTPUT  = 0x02U,
	/** Feature Report */
	BT_HID_REPORT_TYPE_FEATURE = 0x03U,
};

/**
 * @brief HID Protocol Mode values.
 *
 * Defined in HID Service Specification (HIDS v1.0), Section 2.6,
 * Protocol Mode Characteristic.
 */
enum bt_hid_protocol_mode {
	/** Boot Protocol Mode */
	BT_HID_PROTOCOL_BOOT   = 0x00U,
	/** Report Protocol Mode */
	BT_HID_PROTOCOL_REPORT = 0x01U,
};

/**
 * @brief HID Information flags.
 *
 * Defined in HID Service Specification (HIDS v1.0), Section 2.8,
 * HID Information Characteristic.
 */
enum bt_hid_info_flags {
	/** Device supports remote wake */
	BT_HID_INFO_FLAG_REMOTE_WAKE          = BIT(0),
	/** Device is normally connectable */
	BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE = BIT(1),
};

/**
 * @brief HID Control Point commands.
 *
 * Defined in HID Service Specification (HIDS v1.0), Section 2.7,
 * HID Control Point Characteristic.
 */
enum bt_hids_ctrl_point {
	/** The Host is entering Suspend Mode */
	BT_HIDS_CTRL_SUSPEND      = 0x00U,
	/** The Host is exiting Suspend Mode */
	BT_HIDS_CTRL_EXIT_SUSPEND = 0x01U,
};

/** HID Information characteristic value */
struct bt_hids_info {
	/** HID specification version (BCD, e.g. 0x0111 for 1.1.1) */
	uint16_t bcd_hid;
	/** HID country code */
	uint8_t b_country_code;
	/**
	 * HID Information flags, see @ref bt_hid_info_flags.
	 *
	 * Setting @ref BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE tells the Host
	 * that the Device is connectable whenever it is bonded and not
	 * connected, see HOGP v1.1, Section 3.1.7 and Appendix A. The
	 * advertising state is owned by the application, so the application
	 * has to implement that behavior if it sets the flag.
	 */
	uint8_t flags;
};

/** HID Service callbacks */
struct bt_hids_cb {
	/**
	 * Called when a Host reads a Report characteristic.
	 * Fill buf with the current report data and return the number of
	 * bytes written, or a negative errno on error (e.g. -ENOENT if the
	 * report is not known to the application).
	 */
	ssize_t (*get_report)(struct bt_conn *conn, uint8_t report_type,
			      uint8_t report_id, uint8_t *buf,
			      uint16_t buf_size);
	/** Called when a Host writes an Output or Feature Report */
	void (*set_report)(struct bt_conn *conn, uint8_t report_type,
			   uint8_t report_id, const uint8_t *data,
			   uint16_t len);
	/**
	 * Protocol Mode changed notification.
	 * The service has already updated its internal state. Use
	 * bt_hids_get_protocol_mode() to query the current mode.
	 *
	 * Boot Protocol Mode is reported to the application, but the Boot
	 * Keyboard and Boot Mouse Report characteristics are not implemented
	 * yet.
	 */
	void (*protocol_mode_changed)(struct bt_conn *conn, uint8_t protocol);
	/**
	 * Suspend state changed notification.
	 * The service has already updated its internal state. Use
	 * bt_hids_get_suspend_state() to query the current state.
	 */
	void (*suspend_changed)(struct bt_conn *conn, bool suspended);
	/**
	 * Called when a Host writes the CCC descriptor of an Input Report.
	 *
	 * It is not called when the CCC value of a bonded Host is restored on
	 * reconnection, nor when the value is cleared on disconnection. An
	 * application that needs to know whether a Host is subscribed can look
	 * at the return value of bt_hids_send_report(), which fails with
	 * -EINVAL when the Host is not subscribed.
	 */
	void (*ccc_changed)(struct bt_conn *conn, uint8_t report_id,
			    uint8_t report_type, bool enabled);
};

/** HID Service registration parameters */
struct bt_hids_register_param {
	/** HID Information characteristic value */
	struct bt_hids_info info;
	/**
	 * HID Report Map descriptor data.
	 *
	 * At most @ref BT_HIDS_REPORT_MAP_MAX_LEN octets.
	 *
	 * The Report Map is not copied. It is read directly whenever a Host
	 * reads the Report Map characteristic, so it must remain valid until
	 * bt_hids_unregister() is called.
	 */
	const uint8_t *report_map;
	/** Length of report_map in bytes */
	uint16_t report_map_len;
#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
	/**
	 * Report IDs of the Input Report characteristics.
	 *
	 * One Report ID per Input Report characteristic, in the order the
	 * characteristics appear in the service. The number of Input Report
	 * characteristics is set with
	 * @kconfig{CONFIG_BT_HIDS_INPUT_REPORT_COUNT}.
	 */
	uint8_t input_report_ids[CONFIG_BT_HIDS_INPUT_REPORT_COUNT];
#endif
#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0
	/**
	 * Report IDs of the Output Report characteristics.
	 *
	 * See @kconfig{CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT}.
	 */
	uint8_t output_report_ids[CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT];
#endif
#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0
	/**
	 * Report IDs of the Feature Report characteristics.
	 *
	 * See @kconfig{CONFIG_BT_HIDS_FEATURE_REPORT_COUNT}.
	 */
	uint8_t feature_report_ids[CONFIG_BT_HIDS_FEATURE_REPORT_COUNT];
#endif
	/**
	 * Application callbacks.
	 *
	 * The callback structure is not copied and must remain valid until
	 * bt_hids_unregister() is called.
	 */
	const struct bt_hids_cb *cb;
};

/**
 * @brief Register the HID Service.
 *
 * Builds and registers the GATT HID Service attributes dynamically. Only one
 * service instance is supported.
 *
 * All HID Service characteristics require an encrypted link, as required by
 * the HID over GATT Profile, Section 7.1.
 *
 * @param[in] param Registration parameters.
 *
 * @retval 0 Success.
 * @retval -EALREADY Already registered.
 * @retval -EINVAL Invalid parameters, including a Report Map longer than
 *                 @ref BT_HIDS_REPORT_MAP_MAX_LEN and duplicate Report IDs
 *                 within one Report Type.
 */
int bt_hids_register(const struct bt_hids_register_param *param);

/**
 * @brief Unregister the HID Service.
 *
 * Unregisters the GATT service and drops the HID state of every Host.
 * Connected Hosts stay connected and are notified through the Service Changed
 * characteristic.
 *
 * @retval 0 Success.
 * @retval -EALREADY Not registered.
 * @retval -errno Negative errno from bt_gatt_service_unregister().
 */
int bt_hids_unregister(void);

/**
 * @brief Send an Input Report to a Host.
 *
 * Uses a GATT notification. If conn is NULL, all subscribed Hosts are
 * notified.
 *
 * @param[in] conn       Target connection, or NULL for all subscribed Hosts.
 * @param[in] report_id  HID Report ID.
 * @param[in] data       Report payload (excluding the Report ID byte).
 * @param[in] len        Payload length.
 * @param[in] func       Optional completion callback, NULL if not needed.
 * @param[in] user_data  User data passed to the completion callback.
 *
 * @retval 0 Success.
 * @retval -ESRCH Service not registered.
 * @retval -ENOENT Report ID not found.
 * @retval -EINVAL The Host is not subscribed to the Input Report.
 * @retval -errno Negative errno from bt_gatt_notify_cb().
 */
int bt_hids_send_report(struct bt_conn *conn, uint8_t report_id,
			const uint8_t *data, uint16_t len,
			bt_gatt_complete_func_t func, void *user_data);

/**
 * @brief Get the Protocol Mode of a Host.
 *
 * The Protocol Mode is tracked per connection and reset to
 * @ref BT_HID_PROTOCOL_REPORT every time a Host connects. It stays at its
 * default value when the Protocol Mode characteristic is not present, see
 * @kconfig{CONFIG_BT_HIDS_PROTOCOL_MODE}.
 *
 * @param[in]  conn Connection to the Host.
 * @param[out] mode Protocol Mode.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid parameters.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTCONN No HID state tracked for this connection.
 */
int bt_hids_get_protocol_mode(struct bt_conn *conn,
			      enum bt_hid_protocol_mode *mode);

/**
 * @brief Get the Suspend state of a Host.
 *
 * The Suspend state is tracked per connection and reset every time a Host
 * connects. It is set when the Host writes Suspend to the HID Control Point,
 * and cleared when the Host writes Exit Suspend.
 *
 * @param[in]  conn      Connection to the Host.
 * @param[out] suspended true if the Host suspended the Device.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid parameters.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTCONN No HID state tracked for this connection.
 */
int bt_hids_get_suspend_state(struct bt_conn *conn, bool *suspended);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_ */
