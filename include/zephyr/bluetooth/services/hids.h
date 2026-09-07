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
 * role of the HID over GATT Profile (HOGP), and usable on its own. The GATT
 * attribute table is static; the number of Report characteristics of each type
 * is a build time configuration and the application supplies the Report IDs
 * when it registers the service.
 *
 * All characteristics require an encrypted link. A profile that uses this
 * service, such as the HID Device role of HOGP, states which other services it
 * composes this one with and what it requires of the link beyond encryption.
 *
 * Not supported yet: multiple HID Service instances and the "Include" of
 * external services whose characteristics are described in the Report Map
 * together with the External Report Reference descriptor.
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
 * @brief HID Protocol Mode values.
 *
 * Defined by the HID Service, Protocol Mode characteristic.
 */
enum bt_hids_protocol_mode {
	/** Boot Protocol Mode */
	BT_HIDS_PROTOCOL_BOOT = 0x00U,
	/** Report Protocol Mode */
	BT_HIDS_PROTOCOL_REPORT = 0x01U,
};

/**
 * @brief Boot Protocol Mode Report characteristics.
 *
 * A Host operating in Boot Protocol Mode exchanges Reports of a fixed format
 * and length, defined by the Boot Interface Descriptors in Appendix B of the
 * USB HID Specification, through these characteristics instead of the Report
 * characteristics. Which of them are part of the service is a build time
 * configuration, because the HID Service makes them mandatory for a keyboard
 * and for a mouse and forbids them otherwise, see
 * @kconfig{CONFIG_BT_HIDS_BOOT_KEYBOARD} and
 * @kconfig{CONFIG_BT_HIDS_BOOT_MOUSE}.
 *
 * The service carries the payloads without interpreting them. The layout of
 * each is given below, so that an application does not have to derive it from
 * the Report Map, which does not describe the Boot Reports.
 *
 * Whether a Host may use the Report characteristics while it has selected Boot
 * Protocol Mode is a requirement on the Host, so the service does not restrict
 * either set of characteristics. An application that supports Boot Protocol
 * Mode picks the Reports to send from bt_hids_get_protocol_mode().
 *
 * The enumerators name a characteristic in this API, and their values identify
 * it there only. Unlike the Report Types of @ref bt_hids_report_type, they are
 * not values the HID Service defines, and none of them reaches a Host.
 */
enum bt_hids_boot_report {
	/**
	 * Boot Keyboard Input Report, @ref BT_HIDS_BOOT_KB_IN_LEN octets: a
	 * modifier key bitmap, one reserved octet that is zero, and six key
	 * codes, each zero when no key occupies it.
	 */
	BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT = 0x00U,
	/**
	 * Boot Keyboard Output Report, @ref BT_HIDS_BOOT_KB_OUT_LEN octets: the
	 * LED bitmap.
	 */
	BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT = 0x01U,
	/**
	 * Boot Mouse Input Report, @ref BT_HIDS_BOOT_MOUSE_IN_LEN octets: a
	 * button bitmap, then the X and the Y displacement since the previous
	 * Report, each a signed octet.
	 */
	BT_HIDS_BOOT_REPORT_MOUSE_INPUT = 0x02U,
};

/** Length of the Boot Keyboard Input Report */
#define BT_HIDS_BOOT_KB_IN_LEN 8U
/** Length of the Boot Keyboard Output Report */
#define BT_HIDS_BOOT_KB_OUT_LEN 1U
/** Length of the Boot Mouse Input Report */
#define BT_HIDS_BOOT_MOUSE_IN_LEN 3U

/**
 * @brief Boot Keyboard Input Report layout (USB HID Specification, Appendix B.1).
 *
 * Cast the payload received in set_boot_report() to this struct, or fill it
 * and pass a pointer cast to @c const @c uint8_t * to
 * bt_hids_boot_report_send().
 *
 * Bit definitions for the @p modifier field (e.g. the modifier key bitmasks
 * from @c <zephyr/usb/class/hid.h>) and for the @p keys array (HID Usage IDs
 * from the USB HID Usage Tables, Keyboard/Keypad page) are intentionally not
 * defined here; they belong in a shared BT/USB HID header.
 */
struct bt_hids_boot_kb_in_report {
	/** Modifier key bitmap */
	uint8_t modifier;
	/** Reserved, must be zero */
	uint8_t reserved;
	/** Key codes; zero means the slot is empty */
	uint8_t keys[6];
} __packed;

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(sizeof(struct bt_hids_boot_kb_in_report) == BT_HIDS_BOOT_KB_IN_LEN,
	     "Boot Keyboard Input Report layout must match the HID spec");
/** @endcond */

/**
 * @brief Boot Keyboard Output Report layout (USB HID Specification, Appendix B.1).
 *
 * Cast the payload received in set_boot_report() to this struct to decode the
 * LED state written by the Host.
 */
struct bt_hids_boot_kb_out_report {
	/** LED bitmap */
	uint8_t leds;
} __packed;

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(sizeof(struct bt_hids_boot_kb_out_report) == BT_HIDS_BOOT_KB_OUT_LEN,
	     "Boot Keyboard Output Report layout must match the HID spec");
/** @endcond */

/**
 * @brief Boot Mouse Input Report layout (USB HID Specification, Appendix B.2).
 *
 * Cast the payload received in set_boot_report() to this struct, or fill it
 * and pass a pointer cast to @c const @c uint8_t * to
 * bt_hids_boot_report_send().
 *
 * @p x and @p y are signed displacements since the previous Report.
 */
struct bt_hids_boot_mouse_in_report {
	/** Button bitmap */
	uint8_t buttons;
	/** X displacement */
	int8_t x;
	/** Y displacement */
	int8_t y;
} __packed;

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(sizeof(struct bt_hids_boot_mouse_in_report) == BT_HIDS_BOOT_MOUSE_IN_LEN,
	     "Boot Mouse Input Report layout must match the HID spec");
/** @endcond */

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

/** HID Information characteristic value */
struct bt_hids_info {
	/** HID specification version (BCD, e.g. 0x0111 for 1.1.1) */
	uint16_t bcd_hid;
	/** HID country code */
	uint8_t b_country_code;
	/**
	 * HID Information flags, see @ref bt_hids_info_flags.
	 *
	 * Setting @ref BT_HIDS_INFO_FLAG_NORMALLY_CONNECTABLE tells the Host
	 * that the Device is connectable whenever it is bonded and not
	 * connected, as described by HOGP. The advertising state is owned by
	 * the application, so the application has to implement that behavior
	 * if it sets the flag.
	 *
	 * Only the flags in @ref bt_hids_info_flags may be set. Any other bit
	 * makes bt_hids_register() fail.
	 */
	uint8_t flags;
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
	 * Runs on a thread context chosen by the Bluetooth stack, never from an
	 * ISR, and must not block.
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
	 * @brief Called when a Host writes the Protocol Mode characteristic.
	 *
	 * Only called when the Protocol Mode characteristic is part of the
	 * service, see @kconfig{CONFIG_BT_HIDS_PROTOCOL_MODE}. The service has
	 * already updated its internal state, so bt_hids_get_protocol_mode()
	 * returns @p protocol for @p conn.
	 *
	 * Runs on a thread context chosen by the Bluetooth stack, never from an
	 * ISR, and must not block.
	 *
	 * @param conn     Connection of the Host that wrote the characteristic.
	 * @param protocol Protocol Mode the Host selected.
	 */
	void (*protocol_mode_changed)(struct bt_conn *conn, enum bt_hids_protocol_mode protocol);
	/**
	 * @brief Called when a Host writes the HID Control Point.
	 *
	 * The service has already applied what the command implies, so the
	 * Suspend state of @ref BT_HIDS_CTRL_SUSPEND and
	 * @ref BT_HIDS_CTRL_EXIT_SUSPEND is already visible through
	 * bt_hids_get_suspend_state().
	 *
	 * Runs on a thread context chosen by the Bluetooth stack, never from an
	 * ISR, and must not block.
	 *
	 * @param conn Connection of the Host that wrote the characteristic.
	 * @param cmd  Command the Host wrote. The commands the service does not
	 *             implement are rejected and never reach this callback.
	 */
	void (*ctrl_point)(struct bt_conn *conn, enum bt_hids_ctrl_point cmd);
	/**
	 * @brief Called when a Host writes the CCC descriptor of an Input
	 *        Report.
	 *
	 * It is not called when the CCC value of a bonded Host is restored on
	 * reconnection, nor when the value is cleared on disconnection or on
	 * unpair. An application that needs to know whether a Host is subscribed
	 * can look at the return value of bt_hids_send_report(), which fails
	 * with -EINVAL when the Host is not subscribed.
	 *
	 * The GATT core has not committed the new CCC value yet when this runs,
	 * so a bt_hids_send_report() call from this callback still sees the
	 * previous subscription state and fails with -EINVAL for a Host that
	 * has just subscribed. Send the first Report after the callback has
	 * returned.
	 *
	 * Runs on a thread context chosen by the Bluetooth stack, never from an
	 * ISR, and must not block.
	 *
	 * @param conn        Connection of the Host that wrote the descriptor.
	 * @param report_id   Report ID of the Input Report the descriptor
	 *                    belongs to.
	 * @param report_type Report Type of that Report, always
	 *                    @ref BT_HIDS_REPORT_TYPE_INPUT.
	 * @param enabled     True when the Host enabled notifications, false
	 *                    when it disabled them.
	 */
	void (*ccc_changed)(struct bt_conn *conn, uint8_t report_id,
			    enum bt_hids_report_type report_type, bool enabled);
	/**
	 * @brief Called when a Host writes the Boot Keyboard Output Report.
	 *
	 * Runs on a thread context chosen by the Bluetooth stack, never from an
	 * ISR, and must not block.
	 *
	 * @param conn   Connection of the Host that wrote the Report.
	 * @param report Boot Report written. It is the only writable Boot
	 *               Report, so always
	 *               @ref BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT.
	 * @param data   Report payload. Valid for the duration of the callback
	 *               only.
	 * @param len    Length of @p data in bytes, always the fixed length of
	 *               @p report. A write of any other length is rejected and
	 *               never reaches this callback.
	 */
	void (*set_boot_report)(struct bt_conn *conn, enum bt_hids_boot_report report,
				const uint8_t *data, uint16_t len);
	/**
	 * @brief Called when a Host writes the CCC descriptor of a Boot Input
	 *        Report.
	 *
	 * Same restrictions as ccc_changed().
	 *
	 * @param conn    Connection of the Host that wrote the descriptor.
	 * @param report  Boot Input Report the descriptor belongs to, either
	 *                @ref BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT or
	 *                @ref BT_HIDS_BOOT_REPORT_MOUSE_INPUT.
	 * @param enabled True when the Host enabled notifications, false when it
	 *                disabled them.
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
 * All HID Service characteristics require an encrypted link, as required by
 * the HID over GATT Profile.
 *
 * @param[in] param Registration parameters.
 *
 * @retval 0 Success.
 * @retval -EALREADY Already registered.
 * @retval -EINVAL Invalid parameters, including a Report Map longer than
 *                 @ref BT_HIDS_REPORT_MAP_MAX_LEN, a HID Information flag
 *                 outside @ref bt_hids_info_flags, duplicate Report IDs
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
 * @note The caller is responsible for synchronising concurrent calls to this
 *       function with the @ref bt_hids_cb.set_report callback, which the
 *       Bluetooth stack may invoke from a different thread context.
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
 * @note The caller is responsible for synchronising concurrent calls to this
 *       function with the @ref bt_hids_cb.set_report callback, which the
 *       Bluetooth stack may invoke from a different thread context.
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
 * Uses a GATT notification. If conn is NULL, all subscribed Hosts are
 * notified.
 *
 * The broadcast reaches the Hosts whose stored CCC value is exactly
 * BT_GATT_CCC_NOTIFY, which is what the GATT core matches on. A Host that set a
 * Reserved for Future Use bit together with the notify bit counts as subscribed
 * everywhere else, including the ccc_changed() callback and a notification
 * addressed to its connection, but a broadcast skips it. Pass the connection to
 * reach such a Host.
 *
 * @param[in] conn       Target connection, or NULL for all subscribed Hosts.
 * @param[in] report_id  HID Report ID.
 * @param[in] data       Report payload (excluding the Report ID byte).
 * @param[in] len        Payload length.
 * @param[in] func       Optional completion callback, NULL if not needed.
 * @param[in] user_data  User data passed to the completion callback.
 *
 * @return 0 on success, or a negative errno from bt_gatt_notify_cb() on
 *         failure.
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
 * @param[out]    data   Destination for the payload. Its length is the fixed
 *                       length of @p report, so a buffer of
 *                       @ref BT_HIDS_BOOT_KB_IN_LEN octets fits any of them.
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
 * @return 0 on success, or a negative errno from bt_gatt_notify_cb() on
 *         failure.
 * @retval -EINVAL @p report is not a notifiable Boot Report, @p data is NULL,
 *                 or @p len does not match the length of @p report.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTSUP @p report is not part of the service.
 * @retval -ENOTCONN The Host is not connected, or no Host is subscribed when
 *                   @p conn is NULL.
 * @retval -EPERM The link is not encrypted.
 */
int bt_hids_boot_report_send(struct bt_conn *conn, enum bt_hids_boot_report report,
			     const uint8_t *data, uint16_t len, bt_gatt_complete_func_t func,
			     void *user_data);

/**
 * @brief Get the Protocol Mode of a Host.
 *
 * The Protocol Mode is tracked per connection and reset to
 * @ref BT_HIDS_PROTOCOL_REPORT every time a Host connects. It stays at its
 * default value when the Protocol Mode characteristic is not present, see
 * @kconfig{CONFIG_BT_HIDS_PROTOCOL_MODE}.
 *
 * @param[in]  conn Connection to the Host.
 * @param[out] mode Protocol Mode.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid parameters.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTCONN The Host is not connected.
 */
int bt_hids_get_protocol_mode(struct bt_conn *conn, enum bt_hids_protocol_mode *mode);

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
 * @retval -ENOTCONN The Host is not connected.
 */
int bt_hids_get_suspend_state(struct bt_conn *conn, bool *suspended);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_ */
