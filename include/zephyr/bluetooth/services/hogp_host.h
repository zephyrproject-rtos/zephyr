/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief Bluetooth HOGP Host (HID over GATT Profile Host) API.
 */
/**
 * @file
 * @brief Bluetooth HOGP Host (HID over GATT Profile Host) API.
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HOGP_HOST_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HOGP_HOST_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hid.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_hogp_host HID over GATT Profile Host
 * @ingroup bluetooth
 * @{
 */

/** PnP ID from Device Information Service */
struct bt_hogp_host_pnp_id {
	/** Vendor ID source */
	uint8_t vid_src;
	/** Vendor ID */
	uint16_t vid;
	/** Product ID */
	uint16_t pid;
	/** Product version */
	uint16_t version;
};

/** Mode parameters (Phase 2 extension point) */
struct bt_hogp_host_mode_param {
	/** Operation mode (BT_HOGP_MODE_xxx) */
	uint8_t mode;
	/** SCI policy */
	uint8_t policy;
	/** ISO report interval in microseconds */
	uint16_t iso_interval;
};

/** HOGP Host callbacks */
struct bt_hogp_host_cb {
	/**
	 * @brief Service discovery and setup complete.
	 *
	 * @param conn           Connection.
	 * @param status         0 on success, negative errno on failure.
	 * @param num_instances  Number of HID Service instances found.
	 * @param protocol_mode Current protocol mode.
	 */
	void (*connected)(struct bt_conn *conn, int status,
			  uint8_t num_instances, uint8_t protocol_mode);

	/** Disconnected */
	void (*disconnected)(struct bt_conn *conn, int reason);

	/** Report Map received for a service instance */
	void (*report_map)(struct bt_conn *conn, uint8_t service_index,
			   const uint8_t *data, uint16_t len);

	/** Input Report received (via GATT Notification) */
	void (*input_report)(struct bt_conn *conn, uint8_t service_index,
			     uint8_t report_id, const uint8_t *data,
			     uint16_t len);

	/** Get Report response */
	void (*get_report_result)(struct bt_conn *conn, uint8_t report_id,
				  uint8_t report_type, const uint8_t *data,
				  uint16_t len, int status);

	/** PnP ID read from DIS */
	void (*pnp_id)(struct bt_conn *conn,
		       const struct bt_hogp_host_pnp_id *id);

	/** HID Information from HID Service */
	void (*hid_info)(struct bt_conn *conn, uint8_t service_index,
			 uint16_t bcd_hid, uint8_t b_country_code,
			 uint8_t flags);

	/** Battery level notification or read result */
	void (*battery_level)(struct bt_conn *conn, uint8_t bat_index,
			      uint8_t level);

	/** Mode changed notification (Phase 2 extension point) */
	void (*mode_changed)(struct bt_conn *conn, uint8_t mode, int status);
};

/**
 * @brief Initialize HOGP Host module.
 *
 * Registers internal connection callbacks. Call once at boot.
 */
void bt_hogp_host_init(void);

/**
 * @brief Start HOGP discovery on an established LE connection.
 *
 * Automatically discovers HID Service, DIS, BAS, reads Report Map,
 * reads Report References, and subscribes to all Input Report notifications.
 *
 * @param conn           Established BLE connection.
 * @param protocol_mode  Desired protocol mode (BT_HID_PROTOCOL_REPORT or _BOOT).
 * @param cb             Application callbacks.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_host_connect(struct bt_conn *conn, uint8_t protocol_mode,
			 const struct bt_hogp_host_cb *cb);

/**
 * @brief Release HOGP Host state for a connection.
 *
 * @param conn Connection.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_host_disconnect(struct bt_conn *conn);

/**
 * @brief Read a report from device (async).
 *
 * Result delivered via cb->get_report_result().
 *
 * @param conn        Connection.
 * @param report_id   Report ID to read.
 * @param report_type Report type (BT_HID_REPORT_TYPE_*).
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_host_get_report(struct bt_conn *conn, uint8_t report_id,
			    uint8_t report_type);

/**
 * @brief Write a report to device (Output or Feature report).
 *
 * @param conn        Connection.
 * @param report_id   Report ID to write.
 * @param report_type Report type.
 * @param data        Report data.
 * @param len         Data length.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_host_set_report(struct bt_conn *conn, uint8_t report_id,
			    uint8_t report_type, const uint8_t *data,
			    uint16_t len);

/**
 * @brief Set Protocol Mode on a HID Service instance.
 *
 * @param conn          Connection.
 * @param service_index HID Service instance index.
 * @param mode          BT_HID_PROTOCOL_BOOT or BT_HID_PROTOCOL_REPORT.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_host_set_protocol_mode(struct bt_conn *conn,
				   uint8_t service_index, uint8_t mode);

/**
 * @brief Write HID Control Point: Suspend.
 *
 * @param conn          Connection.
 * @param service_index HID Service instance index.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_host_suspend(struct bt_conn *conn, uint8_t service_index);

/**
 * @brief Write HID Control Point: Exit Suspend.
 *
 * @param conn          Connection.
 * @param service_index HID Service instance index.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_host_exit_suspend(struct bt_conn *conn, uint8_t service_index);

/**
 * @brief Get cached Report Map for a service instance.
 *
 * @param conn          Connection.
 * @param service_index HID Service instance index.
 * @param data          [out] Pointer to cached report map data.
 * @param len           [out] Report map length.
 * @return 0 on success, negative errno if not connected.
 */
int bt_hogp_host_get_report_map(struct bt_conn *conn, uint8_t service_index,
				const uint8_t **data, uint16_t *len);

/**
 * @brief Set operation mode (Phase 2 extension).
 * @return -ENOTSUP in Phase 1.
 */
int bt_hogp_host_set_mode(struct bt_conn *conn,
			  const struct bt_hogp_host_mode_param *param);

/**
 * @brief Get current operation mode (Phase 2 extension).
 * @return -ENOTSUP in Phase 1.
 */
int bt_hogp_host_get_mode(struct bt_conn *conn,
			  struct bt_hogp_host_mode_param *param);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HOGP_HOST_H_ */
