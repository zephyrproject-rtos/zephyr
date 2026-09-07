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
	 * @brief Boot Keyboard Input Report.
	 *
	 * Layout: @ref bt_hids_boot_kb_in_report.
	 */
	BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT = 0x00U,
	/**
	 * @brief Boot Keyboard Output Report.
	 *
	 * Layout: @ref bt_hids_boot_kb_out_report.
	 */
	BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT = 0x01U,
	/**
	 * @brief Boot Mouse Input Report.
	 *
	 * Layout: @ref bt_hids_boot_mouse_in_report.
	 */
	BT_HIDS_BOOT_REPORT_MOUSE_INPUT = 0x02U,
};

/**
 * @brief Boot Keyboard Input Report layout.
 *
 * Defined by the USB HID Specification, Appendix B.1. Fill it and pass it,
 * cast to `const uint8_t *`, to bt_hids_boot_report_set() or
 * bt_hids_boot_report_send().
 *
 * The bit definitions of @p modifier and the key codes of @p keys are those of
 * the USB HID Usage Tables, Keyboard/Keypad page, and are not defined here.
 */
struct bt_hids_boot_kb_in_report {
	/** Modifier key bitmap */
	uint8_t modifier;
	/** Reserved, must be zero */
	uint8_t reserved;
	/** Key codes; zero means the slot is empty */
	uint8_t keys[6];
};

/**
 * @brief Boot Keyboard Output Report layout.
 *
 * Defined by the USB HID Specification, Appendix B.1. Cast the payload of
 * @ref bt_hids_cb.set_boot_report to this struct to decode the LED state.
 */
struct bt_hids_boot_kb_out_report {
	/** LED bitmap */
	uint8_t leds;
};

/**
 * @brief Boot Mouse Input Report layout.
 *
 * Defined by the USB HID Specification, Appendix B.2. Fill it and pass it,
 * cast to `const uint8_t *`, to bt_hids_boot_report_set() or
 * bt_hids_boot_report_send().
 */
struct bt_hids_boot_mouse_in_report {
	/** Button bitmap */
	uint8_t buttons;
	/** X displacement since the previous Report */
	int8_t x;
	/** Y displacement since the previous Report */
	int8_t y;
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
	 * @brief Called when a Host writes the Protocol Mode characteristic.
	 *
	 * Only called when the Protocol Mode characteristic is part of the
	 * service, see @kconfig{CONFIG_BT_HIDS_PROTOCOL_MODE}. The service has
	 * already updated its internal state, so bt_hids_get_protocol_mode()
	 * returns @p protocol for @p conn.
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
	 * @param conn Connection of the Host that wrote the characteristic.
	 * @param cmd  Command the Host wrote. The commands the service does not
	 *             implement are rejected and never reach this callback.
	 */
	void (*ctrl_point)(struct bt_conn *conn, enum bt_hids_ctrl_point cmd);
	/**
	 * @brief Called when a Host writes the Boot Keyboard Output Report.
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
 * @retval -ENOMEM No buffer is available for the notification.
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
 *                       length of @p report, so a
 *                       @ref bt_hids_boot_kb_in_report fits any of them.
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
 *                 @p len does not match the length of @p report, or the Host
 *                 is not subscribed to the Boot Input Report, see
 *                 bt_hids_send_report().
 * @retval -ESRCH Service not registered.
 * @retval -ENOTSUP @p report is not part of the service.
 * @retval -ENOTCONN The Host is not connected, or no Host is subscribed when
 *                   @p conn is NULL.
 * @retval -EPERM The link does not have the security level the
 *                characteristics require.
 * @retval -EAGAIN Bluetooth is not ready, or the Host is not change-aware.
 * @retval -ENOMEM No buffer is available for the notification.
 */
int bt_hids_boot_report_send(struct bt_conn *conn, enum bt_hids_boot_report report,
			     const uint8_t *data, uint16_t len, bt_gatt_complete_func_t func,
			     void *user_data);

/**
 * @brief Get the Protocol Mode of a Host.
 *
 * The Protocol Mode is tracked per connection and reset to
 * @ref BT_HIDS_PROTOCOL_REPORT every time the service starts tracking a Host. It stays at its
 * default value when the Protocol Mode characteristic is not present, see
 * @kconfig{CONFIG_BT_HIDS_PROTOCOL_MODE}.
 *
 * @param[in]  conn Connection to the Host.
 * @param[out] mode Protocol Mode.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid parameters.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTCONN The Host is not connected, or the service does not track
 *                   it. A Host is tracked from the moment its link has the
 *                   security level the characteristics require, if one of the
 *                   tracking slots described for @kconfig{CONFIG_BT_HIDS} is
 *                   free. A Host whose link had that level before
 *                   bt_hids_register() is tracked from its first access to
 *                   the Protocol Mode or the HID Control Point.
 */
int bt_hids_get_protocol_mode(struct bt_conn *conn, enum bt_hids_protocol_mode *mode);

/**
 * @brief Get the Suspend state of a Host.
 *
 * The Suspend state is tracked per connection and reset every time the service
 * starts tracking a Host. It is set when the Host writes Suspend to the HID Control Point,
 * and cleared when the Host writes Exit Suspend.
 *
 * @param[in]  conn      Connection to the Host.
 * @param[out] suspended true if the Host suspended the Device.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid parameters.
 * @retval -ESRCH Service not registered.
 * @retval -ENOTCONN The Host is not connected, or the service does not track
 *                   it. A Host is tracked from the moment its link has the
 *                   security level the characteristics require, if one of the
 *                   tracking slots described for @kconfig{CONFIG_BT_HIDS} is
 *                   free. A Host whose link had that level before
 *                   bt_hids_register() is tracked from its first access to
 *                   the Protocol Mode or the HID Control Point.
 */
int bt_hids_get_suspend_state(struct bt_conn *conn, bool *suspended);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_ */
