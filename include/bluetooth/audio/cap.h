/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_

/**
 * @brief Common Audio Profile (CAP)
 *
 * @defgroup bt_cap Common Audio Profile (CAP)
 *
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <bluetooth/audio/csis.h>
#include <bluetooth/audio/tbs.h>
#include <bluetooth/audio/media_proxy.h>
#include <bluetooth/audio/audio.h.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Type of CCID records */
enum bt_cap_ccid_record_type {
	/** CCID belongs to a Telephone Bearer Service instance */
	BT_CAP_CCID_TYPE_TELEPHONE_BEARER,
	/** CCID belongs to a Media Control Service instance */
	BT_CAP_CCID_TYPE_MEDIA_PLAYER
};

/** CCID record structure */
struct bt_cap_ccid_record {
	/** The type of the record */
	enum bt_cap_ccid_record_type type;

	union {
		/** Reference to a local or remote Telephone Bearer instance */
		struct bt_tbs *telephone_bearer;
		/** Reference to a local or remote Media Control instance */
		struct media_player *media_player;
	};
};

/** Callback structure for CAP procedures */
struct bt_cap_cb {
	/**
	 * @brief CCID find callback
	 *
	 * Callback for bt_cap_find_ccid_service().
	 * This is called for the first service instance found for the
	 * specified CCID.
	 *
	 * @param conn        Connection object used in
	 *                    bt_cap_find_ccid_service().
	 * @param err         Error value. 0 on success, GATT error on positive
	 *                    value or errno on negative value.
	 * @param ccid        The CCID value used in bt_cap_find_ccid_service().
	 * @param ccid_record A CCID record structure with a reference to the
	 *                    service instance. May be NULL, if not service
	 *                    instance was found. Only valid if @p err is 0.
	 */
	void (*ccid_found)(struct bt_conn *conn, int err, uint8_t ccid,
			   const struct bt_cap_ccid_record *ccid_record);
}

/**
 * @brief Find the content control service based on CCID
 *
 * This is a local operation, and requires that the content control services
 * already have been discovered using e.g. bt_tbs_client_discover() or
 * media_proxy_ctrl_discover_player().
 *
 * @param conn Connection to a remote server or NULL to get local instance.
 * @param ccid The CCID to find.
 *
 * @return 0 if success, errno on failure.
 */
int bt_cap_find_ccid_service_service(struct bt_conn *conn, uint8_t ccid);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_ */
