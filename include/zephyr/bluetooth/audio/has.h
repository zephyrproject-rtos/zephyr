/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAS_H_

/**
 * @brief Hearing Access Service (HAS)
 *
 * @defgroup bt_has Hearing Access Service (HAS)
 *
 * @ingroup bluetooth
 * @{
 *
 * The Hearing Access Service is used to identify a hearing aid and optionally
 * to control hearing aid presets.
 *
 * [Experimental] Users should note that the APIs can change as a part of
 * ongoing development.
 */

#include <bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque Hearing Access Service object. */
struct bt_has;

/** Hearing Aid device type */
enum bt_has_hearing_aid_type {
	BT_HAS_HEARING_AID_TYPE_BINAURAL,
	BT_HAS_HEARING_AID_TYPE_MONAURAL,
	BT_HAS_HEARING_AID_TYPE_BANDED,
};

/** Hearing Aid device capablilities */
enum bt_has_capabilities {
	BT_HAS_PRESET_SUPPORT = BIT(0),
};

/** @brief Hearing Access Service Client callback structure. */
struct bt_has_client_cb {
	/**
	 * @brief Callback function for bt_has_discover.
	 *
	 * This callback is called when discovery procedure is complete.
	 *
	 * @param conn Bluetooth connection object.
	 * @param err 0 on success, ATT error or negative errno otherwise.
	 * @param has Pointer to the Hearing Access Service object or NULL on errors.
	 * @param type Hearing Aid type.
	 * @param caps Hearing Aid capabilities.
	 */
	void (*discover)(struct bt_conn *conn, int err, struct bt_has *has,
			 enum bt_has_hearing_aid_type type, enum bt_has_capabilities caps);

	/**
	 * @brief Callback function for Hearing Access Service active preset changes.
	 *
	 * Optional callback called when the value is changed by the remote server.
	 * The callback must be set to receive active preset changes and enable support
	 * for switching presets. If the callback is not set, the Active Index and
	 * Control Point characteristics will not be discovered by @ref bt_has_client_discover.
	 *
	 * @param has Pointer to the Hearing Access Service object.
	 * @param index Active preset index.
	 */
	void (*preset_switch)(struct bt_has *has, uint8_t index);
};

/** @brief Registers the callbacks used by the Hearing Access Service client.
 *
 *  @param cb The callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cb_register(const struct bt_has_client_cb *cb);

/**
 * @brief Discover Hearing Access Service on a remote device.
 *
 * Client method to find a Hearing Access Service on a server identified by @p conn.
 * The @ref bt_has_client_cb.discover callback will be called when the discovery procedure
 * is complete to provide user a @ref bt_has object.
 *
 * @param conn Bluetooth connection object.
 *
 * @return 0 if success, errno on failure.
 */
int bt_has_client_discover(struct bt_conn *conn);

/**
 * @brief Get the Bluetooth connection object of the service object.
 *
 * The caller gets a new reference to the connection object which must be
 * released with bt_conn_unref() once done using the object.
 *
 * @param[in] has Pointer to the Hearing Access Service object.
 * @param[out] conn Connection object.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_conn_get(const struct bt_has *has, struct bt_conn **conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAS_H_ */
