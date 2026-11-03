/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth HID over GATT Profile (HOGP) API.
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_PROFILES_HOGP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_PROFILES_HOGP_H_

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth HID over GATT Profile (HOGP)
 * @defgroup bt_hogp Bluetooth HID over GATT Profile (HOGP)
 *
 * @since 4.6
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 *
 * HOGP v1.2 defines three roles: HID Device, Boot Host and Report Host. For the HID Device role,
 * register the HID Service of @ref bt_hids. This API implements part of the Report Host role, the
 * functions and types of which are prefixed with @c bt_hogp_report_host. The Boot Host role is not
 * supported.
 *
 * The Report Host is a GATT client that discovers the HID Service of a HID Device, hands its HID
 * Information and Report Map to the application, subscribes to its Input Reports, and reads and
 * writes its Reports.
 *
 * Establish the connection in the application. HOGP requires the Report Host to bond with the HID
 * Device, so enable @kconfig{CONFIG_BT_BONDABLE}, and recommends encrypting the link as early as
 * possible after reconnecting. When the HID Device answers a request with a security error, the
 * GATT client raises the security of the link itself and retries the request, see
 * @kconfig{CONFIG_BT_ATT_RETRY_ON_SEC_ERR}, so the link gets paired even if the HID Device never
 * sends a Security Request. The discovery fails if the link is not encrypted by the time it would
 * succeed, and Input Reports received over a link that is not encrypted are dropped, as HOGP
 * requires an encrypted link for the HID Service. If the application supports an ATT_MTU larger
 * than the default, exchange the MTU before calling bt_hogp_report_host_discover(), as HOGP
 * requires.
 *
 * Every HID Service the discovery finds is represented by a @ref bt_hogp_report_host_hids object.
 * The discovery passes the object to @ref bt_hogp_report_host_cb.hid_info and
 * @ref bt_hogp_report_host_cb.report_map, and it is valid from then until the connection is
 * disconnected or @ref bt_hogp_report_host_cb.discovered reports a failure. Do not use it after
 * that. While the discovery runs, bt_hogp_report_host_conn_get() accepts it, and the functions
 * that read or write the Reports of the HID Service return -EBUSY.
 *
 * The API can be called from any thread, but not from an interrupt. The callbacks are called
 * without holding any lock of the profile, so a callback can call the API.
 *
 * Not supported yet:
 * - the Protocol Mode and HID Control Point characteristics, that is Boot Protocol Mode and
 *   Suspend;
 * - the Write Without Response sub-procedure, which HOGP requires a Report Host to support for
 *   the Report characteristics;
 * - more than one HID Service instance, and the External Report Reference descriptor with the
 *   included services it refers to;
 * - the Battery Service and the Device Information Service of the HID Device, including the PnP ID
 *   characteristic, which HOGP requires a Report Host to discover;
 * - the Scan Client role of the Scan Parameters Profile, which HOGP requires a Report Host to
 *   support;
 * - keeping the discovered handles and the subscriptions of a bonded HID Device across
 *   connections, so an Input Report the HID Device sends after reconnecting, before the discovery
 *   completes, is dropped;
 * - the HID ISO and HID SCI features.
 */

/**
 * @struct bt_hogp_report_host_hids
 * @brief A HID Service discovered by the Report Host on a HID Device.
 */
struct bt_hogp_report_host_hids;

/** Information about a Report characteristic of a HID Service */
struct bt_hogp_report_host_report_info {
	/** Report Type, from the Report Reference descriptor */
	enum bt_hids_report_type type;

	/** Report ID, from the Report Reference descriptor */
	uint8_t id;

	/** Characteristic properties, a combination of @c BT_GATT_CHRC_* values */
	uint8_t properties;
};

/** HID Information characteristic of a HID Service */
struct bt_hogp_report_host_hid_info {
	/**
	 * @brief Version of the USB HID Specification the HID Device implements.
	 *
	 * In binary coded decimal, 0x0111 is version 1.11. The value is the one the HID Device
	 * reports, it is not checked.
	 */
	uint16_t bcd_hid;

	/** Country code of the localized hardware, 0 if not localized */
	uint8_t country_code;

	/** HID Information flags, see @ref bt_hids_info_flags */
	uint8_t flags;
};

/** HID Services discovered on a HID Device */
struct bt_hogp_report_host_services {
	/** Number of entries in @ref bt_hogp_report_host_services.hids */
	uint8_t count;

	/** The HID Services, in handle order */
	struct bt_hogp_report_host_hids *const *hids;
};

/** Report Host callbacks */
struct bt_hogp_report_host_cb {
	/**
	 * @brief Called with the HID Information of a HID Service during the discovery.
	 *
	 * Store the value if it is needed after the callback. If the discovery fails afterwards,
	 * discard it.
	 *
	 * @param hids HID Service.
	 * @param info HID Information. Valid for the duration of the callback only.
	 */
	void (*hid_info)(struct bt_hogp_report_host_hids *hids,
			 const struct bt_hogp_report_host_hid_info *info);

	/**
	 * @brief Called with a fragment of the Report Map of a HID Service during the discovery.
	 *
	 * The Report Map is read with the Read Long Characteristic Value procedure, and this
	 * callback is called once per fragment, in order. The Report Map is complete when
	 * @ref bt_hogp_report_host_cb.discovered reports success. To parse it, concatenate the
	 * fragments. If the discovery fails, discard them.
	 *
	 * @param hids   HID Service.
	 * @param offset Offset of @p data in the Report Map.
	 * @param data   Fragment. Valid for the duration of the callback only.
	 * @param len    Length of @p data in octets.
	 */
	void (*report_map)(struct bt_hogp_report_host_hids *hids, uint16_t offset,
			   const uint8_t *data, uint16_t len);

	/**
	 * @brief Called when the discovery started by bt_hogp_report_host_discover() ends.
	 *
	 * On success the Input Reports are subscribed to and the HID Services in @p services can
	 * be used. After a failure the Input Reports the discovery subscribed to stay subscribed
	 * until the connection goes away, and their notifications are dropped. The discovery can
	 * be started again on the same connection, also from this callback, and keeps them.
	 *
	 * @param conn     Connection of the HID Device.
	 * @param err      0 on success, a positive ATT error code if the HID Device answered a
	 *                 request with an error, or a negative errno: -ENOTCONN if the connection
	 *                 went away during the discovery, -ENOENT if the HID Device has no HID
	 *                 Service or the service lacks a characteristic or descriptor this profile
	 *                 needs, -EBADMSG if a value has the wrong length, or two Report
	 *                 characteristics have the same value handle or the same Report Type and
	 *                 Report ID, -EPERM if the link is not encrypted, -ENOMEM if the HID
	 *                 Service has more Report characteristics than
	 *                 @kconfig{CONFIG_BT_HOGP_REPORT_HOST_MAX_REPORTS}, or the error of the
	 *                 GATT client function that failed.
	 * @param services The discovered HID Services on success, NULL on failure. Valid for the
	 *                 duration of the callback only; bt_hogp_report_host_services_get() returns
	 *                 the same HID Services later.
	 */
	void (*discovered)(struct bt_conn *conn, int err,
			   const struct bt_hogp_report_host_services *services);

	/**
	 * @brief Called when the HID Device notifies an Input Report.
	 *
	 * @param hids      HID Service.
	 * @param report_id Report ID of the Input Report.
	 * @param data      Report payload, excluding the Report ID. Valid for the duration of the
	 *                  callback only.
	 * @param len       Length of @p data in octets.
	 */
	void (*input_report)(struct bt_hogp_report_host_hids *hids, uint8_t report_id,
			     const uint8_t *data, uint16_t len);

	/**
	 * @brief Called when a read started by bt_hogp_report_host_report_read() ends.
	 *
	 * @param hids      HID Service.
	 * @param type      Report Type of the Report read.
	 * @param report_id Report ID of the Report read.
	 * @param err       0 on success, a positive ATT error code, -ENOTCONN if the connection
	 *                  went away during the read, or -ENOMEM if the value is longer than
	 *                  @kconfig{CONFIG_BT_HOGP_REPORT_HOST_MAX_REPORT_LEN}.
	 * @param data      Report payload, excluding the Report ID, or NULL on failure. Valid for
	 *                  the duration of the callback only.
	 * @param len       Length of @p data in octets, 0 on failure.
	 */
	void (*report_read)(struct bt_hogp_report_host_hids *hids, enum bt_hids_report_type type,
			    uint8_t report_id, int err, const uint8_t *data, uint16_t len);

	/**
	 * @brief Called when a write started by bt_hogp_report_host_report_write() ends.
	 *
	 * @param hids      HID Service.
	 * @param type      Report Type of the Report written.
	 * @param report_id Report ID of the Report written.
	 * @param err       0 on success, a positive ATT error code, or -ENOTCONN if the connection
	 *                  went away during the write.
	 */
	void (*report_written)(struct bt_hogp_report_host_hids *hids, enum bt_hids_report_type type,
			       uint8_t report_id, int err);

	/** @cond INTERNAL_HIDDEN */
	/** Internally used field for list handling */
	sys_snode_t _node;
	/** @endcond */
};

/**
 * @brief Register Report Host callbacks.
 *
 * Several callback structures can be registered, and each is called.
 *
 * @param cb Callbacks. Not copied, so keep it valid until it is unregistered. Any callback can be
 *           NULL.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p cb is NULL.
 * @retval -EALREADY @p cb is already registered.
 */
int bt_hogp_report_host_cb_register(struct bt_hogp_report_host_cb *cb);

/**
 * @brief Unregister Report Host callbacks.
 *
 * Do not call this from a callback, or while another thread may be in a callback of @p cb.
 *
 * @param cb Callbacks registered with bt_hogp_report_host_cb_register().
 *
 * @retval 0 Success.
 * @retval -EINVAL @p cb is NULL.
 * @retval -EALREADY @p cb is not registered.
 */
int bt_hogp_report_host_cb_unregister(struct bt_hogp_report_host_cb *cb);

/**
 * @brief Discover the HID Service of a HID Device.
 *
 * Discovers the first HID Service of the HID Device and its Report characteristics, reads the
 * Report Reference descriptors, the HID Information and the Report Map, and subscribes to every
 * Input Report. The HID Information and the Report Map are passed to
 * @ref bt_hogp_report_host_cb.hid_info and @ref bt_hogp_report_host_cb.report_map, and the
 * outcome to @ref bt_hogp_report_host_cb.discovered.
 *
 * Call this on every connection: the profile keeps nothing across connections, and drops the
 * state of a connection when it disconnects. Do not subscribe to the Input Reports with
 * bt_gatt_subscribe() yourself, otherwise the GATT client skips the CCC write of the profile and
 * the discovery does not complete.
 *
 * @param conn Connection of the HID Device, an LE connection.
 *
 * @retval 0 The discovery started.
 * @retval -EINVAL @p conn is NULL or not an LE connection.
 * @retval -ENOTCONN @p conn is not connected.
 * @retval -EALREADY The discovery already succeeded on @p conn.
 * @retval -EBUSY The discovery is running on @p conn.
 * @retval -ENOMEM No buffer is available for the request.
 */
int bt_hogp_report_host_discover(struct bt_conn *conn);

/**
 * @brief Get the HID Services discovered on a HID Device.
 *
 * @param[in]  conn     Connection of the HID Device.
 * @param[out] services The HID Services. The array it points to stays valid until @p conn is
 *                      disconnected.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p conn or @p services is NULL, or @p conn is not an LE connection.
 * @retval -ENOENT The discovery did not succeed on @p conn.
 */
int bt_hogp_report_host_services_get(struct bt_conn *conn,
				     struct bt_hogp_report_host_services *services);

/**
 * @brief Get the connection of a HID Service.
 *
 * The caller gets a new reference to the connection object, which must be released with
 * bt_conn_unref() once done using the object.
 *
 * @param[in]  hids HID Service.
 * @param[out] conn Connection of the HID Device.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p hids or @p conn is NULL, or @p hids is not a HID Service of this profile.
 * @retval -ENOTCONN The connection of @p hids is gone, or the discovery that found @p hids
 *                   failed.
 */
int bt_hogp_report_host_conn_get(const struct bt_hogp_report_host_hids *hids,
				 struct bt_conn **conn);

/**
 * @brief Get the Report characteristics of a HID Service.
 *
 * @param[in]     hids    HID Service.
 * @param[out]    reports Destination for the Report information, in handle order.
 * @param[in,out] count   Number of entries of @p reports on entry, the number of Reports on
 *                        return.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p hids, @p reports or @p count is NULL, or @p hids is not a HID Service of
 *                 this profile.
 * @retval -ENOTCONN The connection of @p hids is gone, or the discovery that found @p hids
 *                   failed.
 * @retval -EBUSY The discovery is in progress on the connection.
 * @retval -ENOMEM @p reports is too small. @p count is set to the number needed.
 */
int bt_hogp_report_host_reports_get(const struct bt_hogp_report_host_hids *hids,
				    struct bt_hogp_report_host_report_info *reports,
				    uint8_t *count);

/**
 * @brief Read a Report, the GET_REPORT operation of HID.
 *
 * The outcome is passed to @ref bt_hogp_report_host_cb.report_read. A Report longer than
 * ATT_MTU - 1 octets is read with the Read Long Characteristic Value procedure.
 *
 * Wait for the outcome before starting another read or write on the same connection.
 *
 * @param hids      HID Service.
 * @param type      Report Type.
 * @param report_id Report ID.
 *
 * @retval 0 The read started.
 * @retval -EINVAL @p hids is NULL or not a HID Service of this profile.
 * @retval -ENOTCONN The connection of @p hids is gone, or the discovery that found @p hids
 *                   failed.
 * @retval -ENOENT The HID Service has no Report with this Report Type and Report ID.
 * @retval -ENOTSUP The Report characteristic is not readable.
 * @retval -EBUSY The discovery or another read or write is in progress on the connection.
 * @retval -ENOMEM No buffer is available for the request.
 */
int bt_hogp_report_host_report_read(struct bt_hogp_report_host_hids *hids,
				    enum bt_hids_report_type type, uint8_t report_id);

/**
 * @brief Write a Report, the SET_REPORT operation of HID.
 *
 * Uses the Write Characteristic Value procedure, or the Write Long Characteristic Values
 * procedure when @p len is greater than ATT_MTU - 3. The outcome is passed to
 * @ref bt_hogp_report_host_cb.report_written.
 *
 * Wait for the outcome before starting another read or write on the same connection.
 *
 * @param hids      HID Service.
 * @param type      Report Type.
 * @param report_id Report ID.
 * @param data      Report payload, excluding the Report ID. Not copied, so keep it valid until
 *                  the callback is called.
 * @param len       Length of @p data in octets.
 *
 * @retval 0 The write started.
 * @retval -EINVAL @p hids or @p data is NULL, @p hids is not a HID Service of this profile, or
 *                 @p len is 0 or greater than 512, the maximum length of an attribute value.
 * @retval -ENOTCONN The connection of @p hids is gone, or the discovery that found @p hids
 *                   failed.
 * @retval -ENOENT The HID Service has no Report with this Report Type and Report ID.
 * @retval -ENOTSUP The Report characteristic is not writable.
 * @retval -EBUSY The discovery or another read or write is in progress on the connection.
 * @retval -ENOMEM No buffer is available for the request.
 */
int bt_hogp_report_host_report_write(struct bt_hogp_report_host_hids *hids,
				     enum bt_hids_report_type type, uint8_t report_id,
				     const uint8_t *data, uint16_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_PROFILES_HOGP_H_ */
