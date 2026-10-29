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
 * @since 4.6
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 *
 * Server side implementation of the HID Service. The GATT attribute table is
 * static; the number of Report characteristics of each type is a build time
 * configuration and the application supplies the Report IDs when it registers
 * the service.
 *
 * The HID Service sets no security requirements of its own. The security
 * required to access the characteristics is a build time configuration, see
 * @kconfig{CONFIG_BT_HIDS_SECURITY_NONE},
 * @kconfig{CONFIG_BT_HIDS_SECURITY_ENCRYPT} and
 * @kconfig{CONFIG_BT_HIDS_SECURITY_AUTHEN}.
 *
 * The API can be called from any thread, but not from an interrupt. The
 * service serializes it with the Host accesses to the characteristics, and
 * calls the callbacks without holding any lock of its own, so a callback can
 * call the API.
 *
 * Not supported yet: Boot Protocol Mode, that is the Protocol Mode
 * characteristic and the Boot Keyboard and Boot Mouse Report characteristics,
 * which the HID Service makes mandatory for a keyboard and for a mouse;
 * multiple HID Service instances; and the "Include" of external services whose
 * characteristics are described in the Report Map together with the External
 * Report Reference descriptor.
 */

/**
 * Maximum Report Map length of one HID Service.
 *
 * A composite HID Device that needs more octets than this to describe its
 * functions uses several HID Service instances, which is not supported.
 */
#define BT_HIDS_REPORT_MAP_MAX_LEN 512U

/**
 * @brief HID Report Type values.
 *
 * Defined by the HID Service, Report Reference characteristic descriptor.
 */
enum bt_hids_report_type {
	/** Input Report */
	BT_HIDS_REPORT_TYPE_INPUT = 0x01U,

	/** Output Report */
	BT_HIDS_REPORT_TYPE_OUTPUT = 0x02U,

	/** Feature Report */
	BT_HIDS_REPORT_TYPE_FEATURE = 0x03U,
};

/**
 * @brief HID Information flags.
 *
 * Defined by the HID Service, HID Information characteristic. The
 * characteristic has two further flags for the HID SCI feature, which this
 * service does not implement, and bt_hids_register() rejects them.
 */
enum bt_hids_info_flags {
	/** Device supports remote wake */
	BT_HIDS_INFO_FLAG_REMOTE_WAKE = BIT(0),

	/** Device is normally connectable */
	BT_HIDS_INFO_FLAG_NORMALLY_CONNECTABLE = BIT(1),
};

/**
 * @brief HID Control Point commands.
 *
 * Defined by the HID Service, HID Control Point characteristic. The
 * characteristic has four further commands for the HID SCI feature, which this
 * service does not implement and rejects.
 */
enum bt_hids_ctrl_point {
	/** The Host is entering Suspend Mode */
	BT_HIDS_CTRL_SUSPEND = 0x00U,

	/** The Host is exiting Suspend Mode */
	BT_HIDS_CTRL_EXIT_SUSPEND = 0x01U,
};

/** HID Service callbacks */
struct bt_hids_cb {
	/**
	 * @brief Called when a Host writes an Output or Feature Report.
	 *
	 * A write longer than @kconfig{CONFIG_BT_HIDS_MAX_REPORT_LEN} is
	 * rejected with an ATT Invalid Attribute Value Length error and this
	 * callback is not called.
	 *
	 * @param conn        Connection of the Host that wrote the Report.
	 * @param report_type Report Type of the Report written, either
	 *                    @ref BT_HIDS_REPORT_TYPE_OUTPUT or
	 *                    @ref BT_HIDS_REPORT_TYPE_FEATURE.
	 * @param report_id   Report ID of the Report written, as supplied to
	 *                    bt_hids_register().
	 * @param data        Report payload, excluding the Report ID byte. Valid
	 *                    for the duration of the callback only.
	 * @param len         Length of @p data in bytes, at most
	 *                    @kconfig{CONFIG_BT_HIDS_MAX_REPORT_LEN}.
	 */
	void (*set_report)(struct bt_conn *conn, enum bt_hids_report_type report_type,
			   uint8_t report_id, const uint8_t *data, uint16_t len);

	/**
	 * @brief Called when a Host writes the HID Control Point.
	 *
	 * The Suspend state is per Host, and the service does not keep it, so
	 * an application that acts on it tracks it per @p conn.
	 *
	 * @param conn Connection of the Host that wrote the characteristic.
	 * @param cmd  Command the Host wrote. The commands the service does not
	 *             implement are rejected and never reach this callback.
	 */
	void (*ctrl_point)(struct bt_conn *conn, enum bt_hids_ctrl_point cmd);
};

/** HID Service registration parameters */
struct bt_hids_register_param {
	/**
	 * @brief Version of the USB HID Specification the Device implements.
	 *
	 * The bcdHID field of the HID Information characteristic, in binary
	 * coded decimal: 0xJJMN is version JJ.M.N, so 0x0111 is version 1.11.
	 * Every nibble is a decimal digit, so a value such as 0x0abc makes
	 * bt_hids_register() fail.
	 */
	uint16_t bcd_hid;

	/**
	 * @brief Country code of the localized hardware.
	 *
	 * The bCountryCode field of the HID Information characteristic, defined
	 * by the USB HID Specification, section 6.2.1. 0 if the hardware is not
	 * localized.
	 */
	uint8_t country_code;

	/**
	 * @brief HID Information flags, see @ref bt_hids_info_flags.
	 *
	 * Setting @ref BT_HIDS_INFO_FLAG_NORMALLY_CONNECTABLE tells the Host
	 * that the Device is connectable whenever it is bonded and not
	 * connected. The advertising state is owned by the application, so the
	 * application has to implement that behavior if it sets the flag.
	 *
	 * Only the flags in @ref bt_hids_info_flags may be set. Any other bit
	 * makes bt_hids_register() fail.
	 */
	uint8_t flags;

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

#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0 || defined(__DOXYGEN__)
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

#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0 || defined(__DOXYGEN__)
	/**
	 * Report IDs of the Output Report characteristics.
	 *
	 * See @kconfig{CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT}.
	 */
	uint8_t output_report_ids[CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT];
#endif

#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0 || defined(__DOXYGEN__)
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
 * Registers the GATT HID Service and assigns the given Report IDs to its
 * Report characteristics. Only one service instance is supported.
 *
 * @param[in] param Registration parameters.
 *
 * @retval 0 Success.
 * @retval -EALREADY Already registered.
 * @retval -EINVAL Invalid parameters, including a Report Map longer than
 *                 @ref BT_HIDS_REPORT_MAP_MAX_LEN, a @p bcd_hid that is not
 *                 binary coded decimal, a HID Information flag outside
 *                 @ref bt_hids_info_flags, duplicate Report IDs
 *                 within one Report Type, and a Report ID of 0 when the
 *                 Report Type has more than one Report characteristic.
 */
int bt_hids_register(const struct bt_hids_register_param *param);

/**
 * @brief Unregister the HID Service.
 *
 * Unregisters the GATT service and drops the HID state of every Host.
 * Connected Hosts stay connected and are notified through the Service Changed
 * characteristic.
 *
 * Must not be called while another thread is in bt_hids_send_report(): the
 * notification is sent without the service lock, so the application
 * serializes unregistering with sending Input Reports.
 *
 * Must not be called while settings_load() runs if a settings handler of the
 * application calls a bt_hids_*() function: for a bonded Host that is not
 * connected, unregistering stores the CCC values with the service lock held,
 * which takes the settings lock that settings_load() holds while it runs the
 * handlers.
 *
 * @return 0 on success, or a negative errno from bt_gatt_service_unregister()
 *         on failure.
 * @retval -EALREADY Not registered.
 */
int bt_hids_unregister(void);

/**
 * @brief Set the value a Host reads from a Report characteristic.
 *
 * The HID Service holds the value of every Report and answers a Host read from
 * it, so the application sets the value whenever the state it describes
 * changes. This is the arrangement the Report Map already uses, and the one the
 * other GATT services use for values a Host can read: every fragment of a read
 * comes from that one value, rather than from a per-read copy that could go
 * stale, so an ATT Read Blob Request is consistent with the read it continues.
 *
 * The fragments are read from the value as it is at that moment, so an
 * application that calls this function while a Host is reading a Report longer
 * than the ATT MTU can have the Host assemble octets of two different values.
 * Set the value between reads, or keep Reports that a Host reads within one
 * ATT_MTU-3 octets.
 *
 * A Host write to a writable Report replaces the value as well, so a read that
 * follows a SET_REPORT returns what the Host wrote.
 *
 * The value survives disconnection; bt_hids_unregister() clears it. Until it is
 * set for the first time a Host reads an empty value.
 *
 * @param[in] report_type HID Report Type, see @ref bt_hids_report_type.
 * @param[in] report_id   HID Report ID.
 * @param[in] data        Report payload, excluding the Report ID byte.
 * @param[in] len         Payload length.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p data is NULL, @p len is 0, or @p len is greater than
 *                 @kconfig{CONFIG_BT_HIDS_MAX_REPORT_LEN}.
 * @retval -ESRCH Service not registered.
 * @retval -ENOENT No Report characteristic with this Report ID and Report Type.
 */
int bt_hids_report_set(enum bt_hids_report_type report_type, uint8_t report_id,
		       const uint8_t *data, uint16_t len);

/**
 * @brief Read back the value of a Report characteristic.
 *
 * Returns what a Host reads from the Report, which is the value the application
 * last set with bt_hids_report_set() or a Host last wrote with SET_REPORT.
 *
 * @param[in]     report_type HID Report Type, see @ref bt_hids_report_type.
 * @param[in]     report_id   HID Report ID.
 * @param[out]    data        Destination for the payload. A buffer of
 *                            @kconfig{CONFIG_BT_HIDS_MAX_REPORT_LEN} octets
 *                            always fits, since neither the application nor a
 *                            Host can store a longer value.
 * @param[in,out] len         Size of @p data on entry, the payload length on
 *                            return.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p data or @p len is NULL.
 * @retval -ENOMEM @p data is too small for the value.
 * @retval -ESRCH Service not registered.
 * @retval -ENOENT No Report characteristic with this Report ID and Report Type.
 */
int bt_hids_report_get(enum bt_hids_report_type report_type, uint8_t report_id, uint8_t *data,
		       uint16_t *len);

/**
 * @brief Send an Input Report to a Host.
 *
 * Uses a GATT notification. If @p conn is NULL, all subscribed Hosts are
 * notified, and the return value is that of the last Host notified.
 *
 * Every error value is listed below, whether the HID Service or the GATT
 * notification returns it.
 *
 * @param[in] conn       Target connection, or NULL for all subscribed Hosts.
 * @param[in] report_id  HID Report ID.
 * @param[in] data       Report payload (excluding the Report ID byte).
 * @param[in] len        Payload length.
 * @param[in] func       Optional completion callback, NULL if not needed.
 * @param[in] user_data  User data passed to the completion callback.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p data is NULL, @p len is 0 or greater than
 *                 @kconfig{CONFIG_BT_HIDS_MAX_REPORT_LEN}, or the Host is not
 *                 subscribed to the Input Report. The subscription is only
 *                 enforced when @kconfig{CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION}
 *                 is enabled, which it is by default.
 * @retval -ESRCH Service not registered.
 * @retval -ENOENT Report ID not found.
 * @retval -ENOTCONN The Host is not connected, or no Host is subscribed when
 *                   @p conn is NULL.
 * @retval -EPERM The link does not have the security level the
 *                characteristics require.
 * @retval -EAGAIN Bluetooth is not ready, or the Host is not change-aware.
 * @retval -ENOMEM No buffer is available for the notification, or @p len is
 *                 greater than the ATT_MTU of the Host minus 3.
 */
int bt_hids_send_report(struct bt_conn *conn, uint8_t report_id, const uint8_t *data, uint16_t len,
			bt_gatt_complete_func_t func, void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_ */
