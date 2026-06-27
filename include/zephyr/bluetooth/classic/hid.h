/** @file
 *  @brief Bluetooth Human Interface Device Profile (HID)
 */

/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_H_

/**
 * @brief Human Interface Device Profile (HID)
 * @defgroup bt_hid Human Interface Device Profile (HID)
 * @ingroup bluetooth
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/** HID L2CAP PSM for the control channel */
#define BT_HID_PSM_CTRL 0x0011U

/** HID L2CAP PSM for the interrupt channel */
#define BT_HID_PSM_INTR 0x0013U

/** HID profile version 1.1 */
#define BT_HID_PROFILE_VERSION 0x0101U

/** HID parser version 1.1.1 */
#define BT_HID_PARSER_VERSION 0x0111U

/** @brief HID device subclass values (Device Class: Major 0x05) */
enum bt_hid_device_subclass {
	/** Miscellaneous device */
	BT_HID_DEVICE_SUBCLASS_MISC = 0x00,
	/** Keyboard */
	BT_HID_DEVICE_SUBCLASS_KEYBOARD = 0x40,
	/** Pointing device (mouse) */
	BT_HID_DEVICE_SUBCLASS_MOUSE = 0x80,
	/** Combo keyboard/pointing device */
	BT_HID_DEVICE_SUBCLASS_COMBO = 0xC0,
};

/** @brief HID protocol mode */
enum bt_hid_protocol {
	/** Boot protocol mode */
	BT_HID_PROTOCOL_BOOT = 0x00,
	/** Report protocol mode */
	BT_HID_PROTOCOL_REPORT = 0x01,
};

/** @brief HID report type */
enum bt_hid_report_type {
	/** Input report */
	BT_HID_REPORT_INPUT = 0x01,
	/** Output report */
	BT_HID_REPORT_OUTPUT = 0x02,
	/** Feature report */
	BT_HID_REPORT_FEATURE = 0x03,
};

/** @brief HID transaction header types */
enum bt_hid_msg_type {
	/** Handshake message */
	BT_HID_MSG_HANDSHAKE = 0x00,
	/** HID control message */
	BT_HID_MSG_HID_CONTROL = 0x01,
	/** GET_REPORT transaction */
	BT_HID_MSG_GET_REPORT = 0x04,
	/** SET_REPORT transaction */
	BT_HID_MSG_SET_REPORT = 0x05,
	/** GET_PROTOCOL transaction */
	BT_HID_MSG_GET_PROTOCOL = 0x06,
	/** SET_PROTOCOL transaction */
	BT_HID_MSG_SET_PROTOCOL = 0x07,
	/** GET_IDLE transaction */
	BT_HID_MSG_GET_IDLE = 0x08,
	/** SET_IDLE transaction */
	BT_HID_MSG_SET_IDLE = 0x09,
	/** DATA transaction */
	BT_HID_MSG_DATA = 0x0A,
	/** DATA continuation transaction */
	BT_HID_MSG_DATC = 0x0B,
};

/** @brief HID handshake result codes */
enum bt_hid_handshake_result {
	/** Successful handshake */
	BT_HID_HANDSHAKE_SUCCESS = 0x00,
	/** Device not ready */
	BT_HID_HANDSHAKE_NOT_READY = 0x01,
	/** Invalid report ID */
	BT_HID_HANDSHAKE_ERR_INVALID_REPORT_ID = 0x02,
	/** Unsupported request */
	BT_HID_HANDSHAKE_ERR_UNSUPPORTED_REQUEST = 0x03,
	/** Invalid parameter */
	BT_HID_HANDSHAKE_ERR_INVALID_PARAMETER = 0x04,
	/** Unknown error */
	BT_HID_HANDSHAKE_ERR_UNKNOWN = 0x0E,
	/** Fatal error */
	BT_HID_HANDSHAKE_ERR_FATAL = 0x0F,
};

/** @brief HID control operation parameters */
enum bt_hid_control_op {
	/** No operation */
	BT_HID_CONTROL_NOP = 0x00,
	/** Hard reset */
	BT_HID_CONTROL_HARD_RESET = 0x01,
	/** Soft reset */
	BT_HID_CONTROL_SOFT_RESET = 0x02,
	/** Suspend */
	BT_HID_CONTROL_SUSPEND = 0x03,
	/** Exit suspend */
	BT_HID_CONTROL_EXIT_SUSPEND = 0x04,
	/** Virtual cable unplug */
	BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG = 0x05,
};

/**
 * @brief HID device session.
 *
 * Represents one active HID connection to a remote host.
 */
struct bt_hid_device;

/**
 * @brief HID device registration parameters.
 */
struct bt_hid_device_register_param {
	/** HID report descriptor */
	const uint8_t *report_desc;
	/** Length of the report descriptor in bytes */
	size_t report_desc_len;
	/** Device subclass advertised in SDP */
	enum bt_hid_device_subclass subclass;
	/** Country code (0 = not localized) */
	uint8_t country_code;
	/** Device release number (BCD) */
	uint16_t device_release_number;
	/** Service name advertised in SDP */
	const char *name;
	/** True if the device supports boot protocol mode */
	bool boot_device;
	/** True if normally connectable */
	bool normally_connectable;
	/** Supervision timeout in milliseconds */
	uint16_t supervision_timeout;
};

/**
 * @brief HID device application callbacks.
 */
struct bt_hid_device_cb {
	/**
	 * @brief HID control channel connected.
	 *
	 * @param conn ACL connection.
	 * @param hid  HID session.
	 */
	void (*connected)(struct bt_conn *conn, struct bt_hid_device *hid);

	/**
	 * @brief HID session disconnected.
	 *
	 * @param hid HID session.
	 */
	void (*disconnected)(struct bt_hid_device *hid);

	/**
	 * @brief HID interrupt channel connected.
	 *
	 * Input reports may be sent once this callback is received.
	 *
	 * @param hid HID session.
	 */
	void (*intr_connected)(struct bt_hid_device *hid);

	/**
	 * @brief Output report received from the host.
	 *
	 * @param hid HID session.
	 * @param type Report type (typically @ref BT_HID_REPORT_OUTPUT).
	 * @param report_id Report ID, or 0 if none.
	 * @param data Report payload.
	 * @param len Length of @p data.
	 *
	 * @return 0 on success, negative errno on error.
	 */
	int (*set_report)(struct bt_hid_device *hid, enum bt_hid_report_type type,
			  uint8_t report_id, const uint8_t *data, size_t len);

	/**
	 * @brief Host requested a report.
	 *
	 * The application must write the report into @p data and set @p len.
	 *
	 * @param hid HID session.
	 * @param type Report type.
	 * @param report_id Report ID, or 0 if none.
	 * @param data Buffer for the report.
	 * @param len On input, size of @p data. On output, report length.
	 *
	 * @return 0 on success, negative errno on error.
	 */
	int (*get_report)(struct bt_hid_device *hid, enum bt_hid_report_type type,
			  uint8_t report_id, uint8_t *data, size_t *len);

	/**
	 * @brief Protocol mode changed by the host.
	 *
	 * @param hid HID session.
	 * @param protocol New protocol mode.
	 */
	void (*protocol_changed)(struct bt_hid_device *hid, enum bt_hid_protocol protocol);
};

/**
 * @brief Register a Bluetooth HID device.
 *
 * Registers the SDP record and L2CAP servers for the HID control and interrupt
 * channels. Only one HID device registration is supported at a time.
 *
 * @param param Registration parameters.
 * @param cb Application callbacks.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_device_register(const struct bt_hid_device_register_param *param,
			   const struct bt_hid_device_cb *cb);

/**
 * @brief Send an input report on the interrupt channel.
 *
 * @param hid HID session.
 * @param report_id Report ID, or 0 if none.
 * @param data Report payload.
 * @param len Length of @p data.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_device_input_report_send(struct bt_hid_device *hid, uint8_t report_id,
				    const uint8_t *data, size_t len);

/**
 * @brief Get the ACL connection for a HID session.
 *
 * @param hid HID session.
 *
 * @return ACL connection, or NULL.
 */
struct bt_conn *bt_hid_device_conn_get(const struct bt_hid_device *hid);

/**
 * @brief Get the current protocol mode.
 *
 * @param hid HID session.
 *
 * @return Current protocol mode.
 */
enum bt_hid_protocol bt_hid_device_protocol_get(const struct bt_hid_device *hid);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_H_ */
