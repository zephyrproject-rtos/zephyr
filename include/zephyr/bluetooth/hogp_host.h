/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth HID over GATT Profile (HOGP) Host API.
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOGP_HOST_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOGP_HOST_H_

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/hids.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth HID over GATT Profile (HOGP) Host
 * @defgroup bt_hogp_host Bluetooth HID over GATT Profile (HOGP) Host
 *
 * @since 4.6
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 *
 * Report Host role of the HID over GATT Profile: a GATT client that discovers
 * the HID Service of a HID Device, reads its Report Map and HID Information,
 * subscribes to its Input Reports, and reads and writes its Reports.
 *
 * The profile does not establish the connection or the security of the link.
 * HOGP requires the Host to bond with the Device, and recommends encrypting the
 * link as early as possible after reconnecting, so the application enables
 * @kconfig{CONFIG_BT_SMP} and @kconfig{CONFIG_BT_BONDABLE}. A read or write
 * that fails because the link is not encrypted is retried by the GATT client
 * once the link is, see @kconfig{CONFIG_BT_ATT_RETRY_ON_SEC_ERR}. If the
 * application supports an ATT_MTU larger than the default, HOGP requires it to
 * exchange the MTU before calling bt_hogp_host_discover().
 *
 * Every function that addresses a HID Service takes a @p service_index, the
 * position of the service among the HID Services of the Device in handle
 * order. Only the first HID Service of a Device is used, so the index is
 * always 0.
 *
 * The API can be called from any thread, but not from an interrupt. The
 * callbacks are called without holding any lock of the profile, so a callback
 * can call the API.
 *
 * Not supported yet:
 * - the Protocol Mode and HID Control Point characteristics, that is Boot
 *   Protocol Mode and Suspend;
 * - the Boot Keyboard and Boot Mouse Report characteristics, that is the Boot
 *   Host role;
 * - more than one HID Service instance, and the External Report Reference
 *   descriptor with the included services it refers to;
 * - the Battery Service and the Device Information Service of the Device,
 *   including the PnP ID characteristic, which HOGP requires a Report Host to
 *   discover;
 * - the Scan Client role of the Scan Parameters Profile, which HOGP requires a
 *   Report Host to support;
 * - the HID ISO feature of HOGP v1.1.
 */

/** Information about a Report characteristic of a HID Service */
struct bt_hogp_host_report_info {
	/** Report Type, from the Report Reference descriptor */
	enum bt_hids_report_type type;

	/** Report ID, from the Report Reference descriptor */
	uint8_t id;

	/** Characteristic properties, a combination of @c BT_GATT_CHRC_* values */
	uint8_t properties;
};

/** HID Information characteristic of a HID Service */
struct bt_hogp_host_hid_info {
	/**
	 * @brief Version of the USB HID Specification the Device implements.
	 *
	 * In binary coded decimal, 0x0111 is version 1.11. The value is the
	 * one the Device reports, it is not checked.
	 */
	uint16_t bcd_hid;

	/** Country code of the localized hardware, 0 if not localized */
	uint8_t country_code;

	/** HID Information flags, see @ref bt_hids_info_flags */
	uint8_t flags;
};

/** HOGP Host callbacks */
struct bt_hogp_host_cb {
	/**
	 * @brief Called when the discovery started by bt_hogp_host_discover() ends.
	 *
	 * On success the information getters of this API can be used on
	 * @p conn and the Input Reports are subscribed to.
	 *
	 * @param conn Connection of the Device.
	 * @param err  0 on success, a positive ATT error code if the Device
	 *             answered a request with an error, or a negative errno:
	 *             -ENOENT if the Device has no HID Service or the service
	 *             lacks a characteristic or descriptor this profile needs,
	 *             -EBADMSG if a value has the wrong length, -ENOMEM if the
	 *             Report Map or the Reports do not fit in
	 *             @kconfig{CONFIG_BT_HOGP_HOST_REPORT_MAP_MAX_LEN} or
	 *             @kconfig{CONFIG_BT_HOGP_HOST_MAX_REPORTS}, or the error of
	 *             the GATT client function that failed. After a failure the
	 *             discovery can only be started again on a new connection.
	 */
	void (*discovered)(struct bt_conn *conn, int err);

	/**
	 * @brief Called when the Device notifies an Input Report.
	 *
	 * @param conn          Connection of the Device.
	 * @param service_index Index of the HID Service.
	 * @param report_id     Report ID of the Input Report.
	 * @param data          Report payload, excluding the Report ID. Valid
	 *                      for the duration of the callback only.
	 * @param len           Length of @p data in bytes.
	 */
	void (*input_report)(struct bt_conn *conn, uint8_t service_index, uint8_t report_id,
			     const uint8_t *data, uint16_t len);

	/**
	 * @brief Called when a read started by bt_hogp_host_report_read() ends.
	 *
	 * @param conn          Connection of the Device.
	 * @param service_index Index of the HID Service.
	 * @param type          Report Type of the Report read.
	 * @param report_id     Report ID of the Report read.
	 * @param err           0 on success, a positive ATT error code, or
	 *                      -ENOMEM if the value is longer than
	 *                      @kconfig{CONFIG_BT_HOGP_HOST_MAX_REPORT_LEN}.
	 * @param data          Report payload, excluding the Report ID, or NULL
	 *                      on failure. Valid for the duration of the
	 *                      callback only.
	 * @param len           Length of @p data in bytes, 0 on failure.
	 */
	void (*report_read)(struct bt_conn *conn, uint8_t service_index,
			    enum bt_hids_report_type type, uint8_t report_id, int err,
			    const uint8_t *data, uint16_t len);

	/**
	 * @brief Called when a write started by bt_hogp_host_report_write() ends.
	 *
	 * @param conn          Connection of the Device.
	 * @param service_index Index of the HID Service.
	 * @param type          Report Type of the Report written.
	 * @param report_id     Report ID of the Report written.
	 * @param err           0 on success or a positive ATT error code.
	 */
	void (*report_written)(struct bt_conn *conn, uint8_t service_index,
			       enum bt_hids_report_type type, uint8_t report_id, int err);
};

/**
 * @brief Register the HOGP Host callbacks.
 *
 * @param cb Callbacks. Not copied, so it must stay valid. Any callback can be
 *           NULL.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p cb is NULL.
 * @retval -EALREADY Callbacks are already registered.
 */
int bt_hogp_host_cb_register(const struct bt_hogp_host_cb *cb);

/**
 * @brief Discover the HID Service of a Device.
 *
 * Discovers the first HID Service of the Device and its Report
 * characteristics, reads the Report Reference descriptors, the HID
 * Information and the Report Map, and subscribes to every Input Report. The
 * outcome is reported by @ref bt_hogp_host_cb.discovered.
 *
 * The subscriptions are not kept across connections, so the discovery is
 * repeated on every connection. The state of the connection is dropped when
 * it disconnects. The application does not subscribe to the Input Reports
 * itself: the GATT client then skips the CCC write of the profile, and the
 * discovery does not end.
 *
 * @param conn Connection of the Device, an LE connection.
 *
 * @retval 0 The discovery started.
 * @retval -EINVAL @p conn is NULL or not an LE connection.
 * @retval -ENOTCONN @p conn is not connected.
 * @retval -EALREADY The discovery already ran or is running on @p conn.
 * @retval -ENOMEM All @kconfig{CONFIG_BT_HOGP_HOST_MAX_CONNS} connections are
 *                 in use, or no buffer is available for the request.
 */
int bt_hogp_host_discover(struct bt_conn *conn);

/**
 * @brief Get the number of HID Services discovered on a Device.
 *
 * @param[in]  conn  Connection of the Device.
 * @param[out] count Number of HID Services.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p conn or @p count is NULL.
 * @retval -ENOENT The discovery did not succeed on @p conn.
 */
int bt_hogp_host_service_count_get(struct bt_conn *conn, uint8_t *count);

/**
 * @brief Get the HID Information of a HID Service.
 *
 * @param[in]  conn          Connection of the Device.
 * @param[in]  service_index Index of the HID Service.
 * @param[out] info          HID Information.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p conn or @p info is NULL, or @p service_index is out of
 *                 range.
 * @retval -ENOENT The discovery did not succeed on @p conn.
 */
int bt_hogp_host_hid_info_get(struct bt_conn *conn, uint8_t service_index,
			      struct bt_hogp_host_hid_info *info);

/**
 * @brief Copy the Report Map of a HID Service.
 *
 * @param[in]     conn          Connection of the Device.
 * @param[in]     service_index Index of the HID Service.
 * @param[out]    data          Destination for the Report Map.
 * @param[in,out] len           Size of @p data on entry, the Report Map length
 *                              on return.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p conn, @p data or @p len is NULL, or @p service_index is
 *                 out of range.
 * @retval -ENOENT The discovery did not succeed on @p conn.
 * @retval -ENOMEM @p data is too small. @p len is set to the length needed.
 */
int bt_hogp_host_report_map_get(struct bt_conn *conn, uint8_t service_index, uint8_t *data,
				uint16_t *len);

/**
 * @brief Get the Report characteristics of a HID Service.
 *
 * @param[in]     conn          Connection of the Device.
 * @param[in]     service_index Index of the HID Service.
 * @param[out]    reports       Destination for the Report information, in
 *                              handle order.
 * @param[in,out] count         Number of entries of @p reports on entry, the
 *                              number of Reports on return.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p conn, @p reports or @p count is NULL, or
 *                 @p service_index is out of range.
 * @retval -ENOENT The discovery did not succeed on @p conn.
 * @retval -ENOMEM @p reports is too small. @p count is set to the number
 *                 needed.
 */
int bt_hogp_host_reports_get(struct bt_conn *conn, uint8_t service_index,
			     struct bt_hogp_host_report_info *reports, uint8_t *count);

/**
 * @brief Read a Report, the GET_REPORT operation of HID.
 *
 * The outcome is reported by @ref bt_hogp_host_cb.report_read. A Report longer
 * than ATT_MTU - 1 octets is read with the Read Long Characteristic Value
 * procedure.
 *
 * One read or write is in progress per connection at a time.
 *
 * @param conn          Connection of the Device.
 * @param service_index Index of the HID Service.
 * @param type          Report Type.
 * @param report_id     Report ID.
 *
 * @retval 0 The read started.
 * @retval -EINVAL @p conn is NULL or @p service_index is out of range.
 * @retval -ENOENT The discovery did not succeed on @p conn, or the HID Service
 *                 has no Report with this Report Type and Report ID.
 * @retval -EBUSY A read or write is in progress on @p conn.
 * @retval -ENOTCONN @p conn is not connected.
 * @retval -ENOMEM No buffer is available for the request.
 */
int bt_hogp_host_report_read(struct bt_conn *conn, uint8_t service_index,
			     enum bt_hids_report_type type, uint8_t report_id);

/**
 * @brief Write a Report, the SET_REPORT operation of HID.
 *
 * Uses the Write Characteristic Value procedure, or the Write Long
 * Characteristic Values procedure when @p len is greater than ATT_MTU - 3.
 * The outcome is reported by
 * @ref bt_hogp_host_cb.report_written.
 *
 * One read or write is in progress per connection at a time.
 *
 * @param conn          Connection of the Device.
 * @param service_index Index of the HID Service.
 * @param type          Report Type.
 * @param report_id     Report ID.
 * @param data          Report payload, excluding the Report ID. Not copied,
 *                      so it must stay valid until the callback is called.
 * @param len           Length of @p data in bytes.
 *
 * @retval 0 The write started.
 * @retval -EINVAL @p conn or @p data is NULL, @p len is 0 or greater than 512,
 *                 the maximum length of an attribute value, or
 *                 @p service_index is out of range.
 * @retval -ENOENT The discovery did not succeed on @p conn, or the HID Service
 *                 has no Report with this Report Type and Report ID.
 * @retval -ENOTSUP The Report characteristic is not writable.
 * @retval -EBUSY A read or write is in progress on @p conn.
 * @retval -ENOTCONN @p conn is not connected.
 * @retval -ENOMEM No buffer is available for the request.
 */
int bt_hogp_host_report_write(struct bt_conn *conn, uint8_t service_index,
			      enum bt_hids_report_type type, uint8_t report_id, const uint8_t *data,
			      uint16_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOGP_HOST_H_ */
