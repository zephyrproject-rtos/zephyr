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
 * Server side implementation of the HID Service, as used by the HID Device
 * role of the HID over GATT Profile (HOGP). The GATT attribute table is
 * static; the number of Report characteristics of each type is a build time
 * configuration and the application supplies the Report IDs when it registers
 * the service.
 *
 * The HID Device role of HOGP is a composition of this service, the Battery
 * Service and the Device Information Service including the PnP ID
 * characteristic, on a bondable and encrypted link. The composition is the
 * responsibility of the application, see the peripheral_hogp sample.
 *
 * Not supported yet: multiple HID Service instances and the «Include» of
 * external services whose characteristics are described in the Report Map
 * together with the External Report Reference descriptor.
 */

/**
 * Maximum Report Map length of one HID Service.
 *
 * A composite HID Device that needs more octets than this to describe its
 * functions uses several HID Service instances, which is not supported.
 */
#define BT_HIDS_REPORT_MAP_MAX_LEN 512

/**
 * @brief HID Report Type values.
 *
 * Defined by the HID Service, Report Reference characteristic descriptor.
 */
enum bt_hid_report_type {
	/** Input Report */
	BT_HID_REPORT_TYPE_INPUT = 0x01U,
	/** Output Report */
	BT_HID_REPORT_TYPE_OUTPUT = 0x02U,
	/** Feature Report */
	BT_HID_REPORT_TYPE_FEATURE = 0x03U,
};

/**
 * @brief HID Protocol Mode values.
 *
 * Defined by the HID Service, Protocol Mode characteristic.
 */
enum bt_hid_protocol_mode {
	/** Boot Protocol Mode */
	BT_HID_PROTOCOL_BOOT = 0x00U,
	/** Report Protocol Mode */
	BT_HID_PROTOCOL_REPORT = 0x01U,
};

/**
 * @brief Boot Protocol Mode Report characteristics.
 *
 * A Host operating in Boot Protocol Mode exchanges Reports of a fixed format
 * and length, defined by the USB HID Specification, through these
 * characteristics instead of the Report characteristics. Which of them are part
 * of the service is a build time configuration, because the HID Service makes
 * them mandatory for a keyboard and for a mouse and forbids them otherwise, see
 * @kconfig{CONFIG_BT_HIDS_BOOT_KEYBOARD} and
 * @kconfig{CONFIG_BT_HIDS_BOOT_MOUSE}.
 *
 * Whether a Host may use the Report characteristics while it has selected Boot
 * Protocol Mode is a requirement on the Host, so the service does not restrict
 * either set of characteristics. An application that supports Boot Protocol
 * Mode picks the Reports to send from bt_hids_get_protocol_mode().
 */
enum bt_hids_boot_report {
	/** Boot Keyboard Input Report, @ref BT_HIDS_BOOT_KB_IN_LEN octets */
	BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT,
	/** Boot Keyboard Output Report, @ref BT_HIDS_BOOT_KB_OUT_LEN octets */
	BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT,
	/** Boot Mouse Input Report, @ref BT_HIDS_BOOT_MOUSE_IN_LEN octets */
	BT_HIDS_BOOT_REPORT_MOUSE_INPUT,
};

/** Length of the Boot Keyboard Input Report */
#define BT_HIDS_BOOT_KB_IN_LEN 8
/** Length of the Boot Keyboard Output Report */
#define BT_HIDS_BOOT_KB_OUT_LEN 1
/** Length of the Boot Mouse Input Report */
#define BT_HIDS_BOOT_MOUSE_IN_LEN 3

/**
 * @brief HID Information flags.
 *
 * Defined by the HID Service, HID Information characteristic. The
 * characteristic has two further flags for the HID SCI feature, which this
 * service does not implement, and bt_hids_register() rejects them.
 */
enum bt_hid_info_flags {
	/** Device supports remote wake */
	BT_HID_INFO_FLAG_REMOTE_WAKE = BIT(0),
	/** Device is normally connectable */
	BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE = BIT(1),
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
	 * connected, as described by HOGP. The advertising state is owned by
	 * the application, so the application has to implement that behavior
	 * if it sets the flag.
	 *
	 * Only the flags in @ref bt_hid_info_flags may be set. Any other bit
	 * makes bt_hids_register() fail.
	 */
	uint8_t flags;
};

/** HID Service callbacks */
struct bt_hids_cb {
	/**
	 * Called when a Host writes an Output or Feature Report.
	 *
	 * @p len is at most @kconfig{CONFIG_BT_HIDS_MAX_REPORT_LEN}. A longer
	 * write is rejected with an ATT Invalid Attribute Value Length error
	 * and this callback is not called.
	 *
	 * Runs on the Bluetooth RX thread and must not block.
	 */
	void (*set_report)(struct bt_conn *conn, uint8_t report_type, uint8_t report_id,
			   const uint8_t *data, uint16_t len);
	/**
	 * Protocol Mode changed notification.
	 *
	 * Only called when the Protocol Mode characteristic is part of the
	 * service, see @kconfig{CONFIG_BT_HIDS_PROTOCOL_MODE}. The service has
	 * already updated its internal state; use bt_hids_get_protocol_mode()
	 * to query the current mode.
	 *
	 * Runs on the Bluetooth RX thread and must not block.
	 */
	void (*protocol_mode_changed)(struct bt_conn *conn, uint8_t protocol);
	/**
	 * Called when a Host writes the HID Control Point.
	 *
	 * The service has already applied what the command implies, so the
	 * Suspend state of @ref BT_HIDS_CTRL_SUSPEND and
	 * @ref BT_HIDS_CTRL_EXIT_SUSPEND is already visible through
	 * bt_hids_get_suspend_state().
	 *
	 * Runs on the Bluetooth RX thread and must not block.
	 */
	void (*ctrl_point)(struct bt_conn *conn, enum bt_hids_ctrl_point cmd);
	/**
	 * Called when a Host writes the CCC descriptor of an Input Report.
	 *
	 * It is not called when the CCC value of a bonded Host is restored on
	 * reconnection, nor when the value is cleared on disconnection. An
	 * application that needs to know whether a Host is subscribed can look
	 * at the return value of bt_hids_send_report(), which fails with
	 * -EINVAL when the Host is not subscribed.
	 *
	 * Runs on the Bluetooth RX thread and must not block.
	 */
	void (*ccc_changed)(struct bt_conn *conn, uint8_t report_id, uint8_t report_type,
			    bool enabled);
	/**
	 * Called when a Host writes the Boot Keyboard Output Report.
	 *
	 * @p len is the fixed length of the Boot Report. It is the only
	 * writable Boot Report, so @p report is always
	 * @ref BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT.
	 *
	 * Runs on the Bluetooth RX thread and must not block.
	 */
	void (*set_boot_report)(struct bt_conn *conn, enum bt_hids_boot_report report,
				const uint8_t *data, uint16_t len);
	/**
	 * Called when a Host writes the CCC descriptor of a Boot Input Report.
	 *
	 * Same restrictions as ccc_changed().
	 */
	void (*boot_ccc_changed)(struct bt_conn *conn, enum bt_hids_boot_report report,
				 bool enabled);
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
 * Registers the GATT HID Service and assigns the given Report IDs to its
 * Report characteristics. Only one service instance is supported.
 *
 * All HID Service characteristics require an encrypted link, as required by
 * the HID over GATT Profile.
 *
 * @param[in] param Registration parameters.
 *
 * @retval 0 Success.
 * @retval -EALREADY Already registered.
 * @retval -EINVAL Invalid parameters, including a Report Map longer than
 *                 @ref BT_HIDS_REPORT_MAP_MAX_LEN, a HID Information flag
 *                 outside @ref bt_hid_info_flags, duplicate Report IDs
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
 * @retval 0 Success.
 * @retval -EALREADY Not registered.
 * @retval -errno Negative errno from bt_gatt_service_unregister().
 */
int bt_hids_unregister(void);

/**
 * @brief Set the value a Host reads from a Report characteristic.
 *
 * The HID Service holds the value of every Report and answers a Host read from
 * it, so the application sets the value whenever the state it describes
 * changes. This is the arrangement the Report Map already uses, and the one the
 * other GATT services use for values a Host can read: the value a Host
 * assembles with several ATT Read Blob Requests then comes from one place and
 * cannot change halfway through the read.
 *
 * A Host write to a writable Report replaces the value as well, so a read that
 * follows a SET_REPORT returns what the Host wrote.
 *
 * The value survives disconnection; bt_hids_unregister() clears it. Until it is
 * set for the first time a Host reads an empty value.
 *
 * @param[in] report_type HID Report Type, see @ref bt_hid_report_type.
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
int bt_hids_report_set(uint8_t report_type, uint8_t report_id, const uint8_t *data, uint16_t len);

/**
 * @brief Read back the value of a Report characteristic.
 *
 * Returns what a Host reads from the Report, which is the value the application
 * last set with bt_hids_report_set() or a Host last wrote with SET_REPORT.
 *
 * @param[in]     report_type HID Report Type, see @ref bt_hid_report_type.
 * @param[in]     report_id   HID Report ID.
 * @param[out]    data        Destination for the payload.
 * @param[in,out] len         Size of @p data on entry, the payload length on
 *                            return.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p data or @p len is NULL.
 * @retval -ENOMEM @p data is too small for the value.
 * @retval -ESRCH Service not registered.
 * @retval -ENOENT No Report characteristic with this Report ID and Report Type.
 */
int bt_hids_report_get(uint8_t report_type, uint8_t report_id, uint8_t *data, uint16_t *len);

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
 * @retval -EINVAL @p data is NULL, @p len is 0 or greater than
 *                 @kconfig{CONFIG_BT_HIDS_MAX_REPORT_LEN}, or the Host is not
 *                 subscribed to the Input Report. The subscription is only
 *                 enforced when @kconfig{CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION}
 *                 is enabled, which it is by default.
 * @retval -ESRCH Service not registered.
 * @retval -ENOENT Report ID not found.
 * @retval -ENOTCONN The Host is not connected, or no Host is subscribed when
 *                   @p conn is NULL.
 * @retval -EPERM The link is not encrypted.
 * @retval -errno Negative errno from bt_gatt_notify_cb().
 */
int bt_hids_send_report(struct bt_conn *conn, uint8_t report_id, const uint8_t *data, uint16_t len,
			bt_gatt_complete_func_t func, void *user_data);

/**
 * @brief Set the value a Host reads from a Boot Report characteristic.
 *
 * The Boot Report equivalent of bt_hids_report_set(). A Boot Report has a fixed
 * length, so @p len has to match it exactly.
 *
 * @param[in] report Boot Report characteristic.
 * @param[in] data   Report payload.
 * @param[in] len    Payload length, the fixed length of @p report.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p report is not a valid Boot Report, @p data is NULL, or
 *                 @p len does not match the length of @p report.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTSUP @p report is not part of the service.
 */
int bt_hids_boot_report_set(enum bt_hids_boot_report report, const uint8_t *data, uint16_t len);

/**
 * @brief Read back the value of a Boot Report characteristic.
 *
 * The Boot Report equivalent of bt_hids_report_get().
 *
 * @param[in]     report Boot Report characteristic.
 * @param[out]    data   Destination for the payload.
 * @param[in,out] len    Size of @p data on entry, the payload length on return.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p report is not a valid Boot Report, or @p data or @p len is
 *                 NULL.
 * @retval -ENOMEM @p data is too small for the value.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTSUP @p report is not part of the service.
 */
int bt_hids_boot_report_get(enum bt_hids_boot_report report, uint8_t *data, uint16_t *len);

/**
 * @brief Send a Boot Input Report to a Host.
 *
 * The Boot Report equivalent of bt_hids_send_report(). Only the Boot Input
 * Reports can be notified.
 *
 * @param[in] conn      Target connection, or NULL for all subscribed Hosts.
 * @param[in] report    Boot Report characteristic.
 * @param[in] data      Report payload.
 * @param[in] len       Payload length, the fixed length of @p report.
 * @param[in] func      Optional completion callback, NULL if not needed.
 * @param[in] user_data User data passed to the completion callback.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p report is not a notifiable Boot Report, @p data is NULL,
 *                 or @p len does not match the length of @p report.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTSUP @p report is not part of the service.
 * @retval -ENOTCONN The Host is not connected, or no Host is subscribed when
 *                   @p conn is NULL.
 * @retval -EPERM The link is not encrypted.
 * @retval -errno Negative errno from bt_gatt_notify_cb().
 */
int bt_hids_boot_report_send(struct bt_conn *conn, enum bt_hids_boot_report report,
			     const uint8_t *data, uint16_t len, bt_gatt_complete_func_t func,
			     void *user_data);

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
int bt_hids_get_protocol_mode(struct bt_conn *conn, enum bt_hid_protocol_mode *mode);

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
