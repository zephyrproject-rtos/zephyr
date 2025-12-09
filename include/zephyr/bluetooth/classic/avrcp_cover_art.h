/** @file
 *  @brief Audio Video Remote Control Cover Art Profile header.
 */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_COVER_ART_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_COVER_ART_H_

/**
 * @brief Audio Video Remote Control Cover Art Profile (AVRCP-CA)
 * @defgroup bt_avrcp_cover_art Audio Video Remote Control Cover Art Profile
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/classic/bip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief AVRCP CT structure */
struct bt_avrcp_ct;
/** @brief AVRCP TG structure */
struct bt_avrcp_tg;

/**
 * @brief AVRCP Controller Cover Art structure
 *
 * This structure represents an AVRCP Controller Cover Art instance,
 * which handles cover art retrieval from AVRCP Target devices.
 */
struct bt_avrcp_cover_art_ct {
	/** Basic Image Profile instance */
	struct bt_bip bip;
	/** BIP client instance */
	struct bt_bip_client client;
	/** Associated AVRCP controller */
	struct bt_avrcp_ct *ct;
	/** Bluetooth connection */
	struct bt_conn *conn;
};

/**
 * @brief AVRCP Target Cover Art structure
 *
 * This structure represents an AVRCP Target Cover Art instance,
 * which serves cover art to AVRCP Controller devices.
 */
struct bt_avrcp_cover_art_tg {
	/** Basic Image Profile instance */
	struct bt_bip bip;
	/** BIP server instance */
	struct bt_bip_server server;
	/** Associated AVRCP target */
	struct bt_avrcp_tg *tg;
	/** Bluetooth connection */
	struct bt_conn *conn;
};

/**
 * @brief AVRCP Cover Art Controller callback structure
 *
 * This structure defines callback functions that will be called
 * when various AVRCP Cover Art Controller events occur.
 */
struct bt_avrcp_cover_art_ct_cb {
	/**
	 * @brief L2CAP connected callback
	 *
	 * Called when the underlying transport (L2CAP) is connected
	 *
	 * @param ct AVRCP controller instance
	 * @param cover_art_ct AVRCP Cover Art controller instance
	 */
	void (*l2cap_connected)(struct bt_avrcp_ct *ct, struct bt_avrcp_cover_art_ct *cover_art_ct);

	/**
	 * @brief L2CAP disconnected callback
	 *
	 * Called when the underlying transport is disconnected
	 *
	 * @param ct AVRCP Cover Art controller instance
	 */
	void (*l2cap_disconnected)(struct bt_avrcp_cover_art_ct *ct);

	/**
	 * @brief OBEX connect response callback
	 *
	 * Called when a connect response is received from the target
	 *
	 * @param ct AVRCP Cover Art controller instance
	 * @param rsp_code OBEX response code
	 * @param version OBEX version
	 * @param mopl Maximum OBEX packet length
	 * @param buf Response data buffer
	 */
	void (*connect)(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, uint8_t version,
			uint16_t mopl, struct net_buf *buf);

	/**
	 * @brief OBEX disconnect response callback
	 *
	 * @param ct AVRCP Cover Art controller instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*disconnect)(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief OBEX abort response callback
	 *
	 * @param ct AVRCP Cover Art controller instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*abort)(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Get image properties response callback
	 *
	 * @param ct AVRCP Cover Art controller instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer containing image properties
	 */
	void (*get_image_properties)(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				     struct net_buf *buf);

	/**
	 * @brief Get image response callback
	 *
	 * @param ct AVRCP Cover Art controller instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer containing image data
	 */
	void (*get_image)(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Get linked thumbnail response callback
	 *
	 * @param ct AVRCP Cover Art controller instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer containing thumbnail data
	 */
	void (*get_linked_thumbnail)(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				     struct net_buf *buf);
};

/**
 * @brief AVRCP Cover Art Target callback structure
 *
 * This structure defines callback functions that will be called
 * when various AVRCP Cover Art Target events occur.
 */
struct bt_avrcp_cover_art_tg_cb {
	/**
	 * @brief L2CAP connected callback
	 *
	 * Called when the underlying transport (L2CAP) is connected
	 *
	 * @param tg AVRCP target instance
	 * @param cover_art_tg AVRCP Cover Art target instance
	 */
	void (*l2cap_connected)(struct bt_avrcp_tg *tg, struct bt_avrcp_cover_art_tg *cover_art_tg);

	/**
	 * @brief L2CAP disconnected callback
	 *
	 * Called when the underlying transport is disconnected
	 *
	 * @param tg AVRCP Cover Art target instance
	 */
	void (*l2cap_disconnected)(struct bt_avrcp_cover_art_tg *tg);

	/**
	 * @brief OBEX connect request callback
	 *
	 * @param tg AVRCP Cover Art target instance
	 * @param version OBEX version
	 * @param mopl Maximum OBEX packet length
	 * @param buf Request data buffer
	 */
	void (*connect)(struct bt_avrcp_cover_art_tg *tg, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/**
	 * @brief OBEX disconnect request callback
	 *
	 * @param tg AVRCP Cover Art target instance
	 * @param buf Request data buffer
	 */
	void (*disconnect)(struct bt_avrcp_cover_art_tg *tg, struct net_buf *buf);

	/**
	 * @brief OBEX abort request callback
	 *
	 * @param tg AVRCP Cover Art target instance
	 * @param buf Request data buffer
	 */
	void (*abort)(struct bt_avrcp_cover_art_tg *tg, struct net_buf *buf);

	/**
	 * @brief Get image properties request callback
	 *
	 * @param tg AVRCP Cover Art target instance
	 * @param final True if this is the final packet in the request
	 * @param buf Request data buffer
	 */
	void (*get_image_properties)(struct bt_avrcp_cover_art_tg *tg, bool final,
				     struct net_buf *buf);

	/**
	 * @brief Get image request callback
	 *
	 * @param tg AVRCP Cover Art target instance
	 * @param final True if this is the final packet in the request
	 * @param buf Request data buffer
	 */
	void (*get_image)(struct bt_avrcp_cover_art_tg *tg, bool final, struct net_buf *buf);

	/**
	 * @brief Get linked thumbnail request callback
	 *
	 * @param tg AVRCP Cover Art target instance
	 * @param final True if this is the final packet in the request
	 * @param buf Request data buffer
	 */
	void (*get_linked_thumbnail)(struct bt_avrcp_cover_art_tg *tg, bool final,
				     struct net_buf *buf);
};

/**
 * @brief Register AVRCP Cover Art Controller callbacks
 *
 * @param cb Pointer to callback structure
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_cb_register(struct bt_avrcp_cover_art_ct_cb *cb);

/**
 * @brief Register AVRCP Cover Art target callbacks
 *
 * @param cb Pointer to callback structure
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_tg_cb_register(struct bt_avrcp_cover_art_tg_cb *cb);

/**
 * @brief Connect L2CAP channel for AVRCP Cover Art Controller
 *
 * @param ct AVRCP controller instance
 * @param cover_art_ct Pointer to store the created Cover Art controller instance
 * @param psm Protocol Service Multiplexer for the L2CAP connection
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_l2cap_connect(struct bt_avrcp_ct *ct,
					struct bt_avrcp_cover_art_ct **cover_art_ct, uint16_t psm);

/**
 * @brief Disconnect L2CAP channel for AVRCP Cover Art Controller
 *
 * @param ct AVRCP Cover Art controller instance
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_l2cap_disconnect(struct bt_avrcp_cover_art_ct *ct);

/**
 * @brief Disconnect L2CAP channel for AVRCP Cover Art Target
 *
 * @param tg AVRCP Cover Art target instance
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_tg_l2cap_disconnect(struct bt_avrcp_cover_art_tg *tg);

/**
 * @brief Create PDU buffer for AVRCP Cover Art Controller
 *
 * @param ct AVRCP Cover Art controller instance
 * @param pool Buffer pool to allocate from
 * @return Allocated buffer or NULL on failure
 */
struct net_buf *bt_avrcp_cover_art_ct_create_pdu(struct bt_avrcp_cover_art_ct *ct,
						 struct net_buf_pool *pool);

/**
 * @brief Create PDU buffer for AVRCP Cover Art Target
 *
 * @param tg AVRCP Cover Art target instance
 * @param pool Buffer pool to allocate from
 * @return Allocated buffer or NULL on failure
 */
struct net_buf *bt_avrcp_cover_art_tg_create_pdu(struct bt_avrcp_cover_art_tg *tg,
						 struct net_buf_pool *pool);

/**
 * @brief Send OBEX connect request
 *
 * @param ct AVRCP Cover Art controller instance
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_connect(struct bt_avrcp_cover_art_ct *ct);

/**
 * @brief Send OBEX connect response
 *
 * @param tg AVRCP Cover Art target instance
 * @param rsp_code OBEX response code
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_tg_connect(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code);

/**
 * @brief Send OBEX disconnect request
 *
 * @param ct AVRCP Cover Art controller instance
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_disconnect(struct bt_avrcp_cover_art_ct *ct);

/**
 * @brief Send OBEX disconnect response
 *
 * @param tg AVRCP Cover Art target instance
 * @param rsp_code OBEX response code
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_tg_disconnect(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code);

/**
 * @brief Send OBEX abort request
 *
 * @param ct AVRCP Cover Art controller instance
 * @param buf Request data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_abort(struct bt_avrcp_cover_art_ct *ct, struct net_buf *buf);

/**
 * @brief Send OBEX abort response
 *
 * @param tg AVRCP Cover Art target instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_tg_abort(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code,
				struct net_buf *buf);

/**
 * @brief Send get image properties request
 *
 * @param ct AVRCP Cover Art controller instance
 * @param final True if this is the final packet in the request
 * @param buf Request data buffer containing image handle
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_get_image_properties(struct bt_avrcp_cover_art_ct *ct, bool final,
					       struct net_buf *buf);

/**
 * @brief Send get image properties response
 *
 * @param tg AVRCP Cover Art target instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer containing image properties
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_tg_get_image_properties(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code,
					       struct net_buf *buf);

/**
 * @brief Send get image request
 *
 * @param ct AVRCP Cover Art controller instance
 * @param final True if this is the final packet in the request
 * @param buf Request data buffer containing image descriptor
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_get_image(struct bt_avrcp_cover_art_ct *ct, bool final,
				    struct net_buf *buf);

/**
 * @brief Send get image response
 *
 * @param tg AVRCP Cover Art target instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer containing image data
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_tg_get_image(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code,
				    struct net_buf *buf);

/**
 * @brief Send get linked thumbnail request
 *
 * @param ct AVRCP Cover Art controller instance
 * @param final True if this is the final packet in the request
 * @param buf Request data buffer containing image handle
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_ct_get_linked_thumbnail(struct bt_avrcp_cover_art_ct *ct, bool final,
					       struct net_buf *buf);

/**
 * @brief Send get linked thumbnail response
 *
 * @param tg AVRCP Cover Art target instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer containing thumbnail data
 * @return 0 on success, negative error code on failure
 */
int bt_avrcp_cover_art_tg_get_linked_thumbnail(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code,
					       struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_COVER_ART_H_ */
