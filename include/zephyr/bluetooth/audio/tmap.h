/**
 * @file
 * @brief Header for Bluetooth TMAP.
 *
 * Copyright 2023 NXP
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TMAP_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TMAP_

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

/** Call Gateway (CG) supported */
#define BT_TMAP_CG_SUPPORTED                                                                       \
	(IS_ENABLED(CONFIG_BT_CAP_INITIATOR) && IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT) &&        \
	 IS_ENABLED(CONFIG_BT_TBS) && IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR))

/** Call Terminal (CT) supported */
#define BT_TMAP_CT_SUPPORTED                                                                       \
	(IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR) && IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER) &&         \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT) &&                                                       \
	 (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK) &&                                                    \
	  IS_ENABLED(CONFIG_BT_VCP_VOL_REND) == IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)))

/** Unicast Media Sender (UMS) supported */
#define BT_TMAP_UMS_SUPPORTED                                                                      \
	(IS_ENABLED(CONFIG_BT_CAP_INITIATOR) &&                                                    \
	 IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK) && IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR) && \
	 IS_ENABLED(CONFIG_BT_MCS))

/** Unicast Media Receiver (UMR) supported */
#define BT_TMAP_UMR_SUPPORTED                                                                      \
	(IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR) && IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK) &&               \
	 IS_ENABLED(CONFIG_BT_VCP_VOL_REND))

/** Broadcast Media Sender (BMS) supported */
#define BT_TMAP_BMS_SUPPORTED                                                                      \
	(IS_ENABLED(CONFIG_BT_CAP_INITIATOR) && IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SOURCE))

/** Broadcast Media Receiver (BMR) supported */
#define BT_TMAP_BMR_SUPPORTED                                                                      \
	(IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR) && IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SINK))

/** @brief TMAP Role characteristic */
enum bt_tmap_role {
	/**
	 * @brief TMAP Call Gateway role
	 *
	 * This role is defined to telephone and VoIP applications using the Call Control Profile
	 * to control calls on a remote TMAP Call Terminal.
	 * Audio streams in this role are typically bi-directional.
	 */
	BT_TMAP_ROLE_CG = BIT(0),
	/**
	 * @brief TMAP Call Terminal role
	 *
	 * This role is defined to telephone and VoIP applications using the Call Control Profile
	 * to expose calls to remote TMAP Call Gateways.
	 * Audio streams in this role are typically bi-directional.
	 */
	BT_TMAP_ROLE_CT = BIT(1),
	/**
	 * @brief TMAP Unicast Media Sender role
	 *
	 * This role is defined send media audio to TMAP Unicast Media Receivers.
	 * Audio streams in this role are typically uni-directional.
	 */
	BT_TMAP_ROLE_UMS = BIT(2),
	/**
	 * @brief TMAP Unicast Media Receiver role
	 *
	 * This role is defined receive media audio to TMAP Unicast Media Senders.
	 * Audio streams in this role are typically uni-directional.
	 */
	BT_TMAP_ROLE_UMR = BIT(3),
	/**
	 * @brief TMAP Broadcast Media Sender role
	 *
	 * This role is defined send media audio to TMAP Broadcast Media Receivers.
	 * Audio streams in this role are always uni-directional.
	 */
	BT_TMAP_ROLE_BMS = BIT(4),
	/**
	 * @brief TMAP Broadcast Media Receiver role
	 *
	 * This role is defined send media audio to TMAP Broadcast Media Senders.
	 * Audio streams in this role are always uni-directional.
	 */
	BT_TMAP_ROLE_BMR = BIT(5),
};

/** @brief TMAP callback structure. */
struct bt_tmap_cb {
	/**
	 * @brief TMAP discovery complete callback
	 *
	 * This callback notifies the application about the value of the
	 * TMAP Role characteristic on the peer.
	 *
	 * @param role	   Peer TMAP role(s).
	 * @param conn     Pointer to the connection
	 * @param err      0 if success, ATT error received from server otherwise.
	 */
	void (*discovery_complete)(enum bt_tmap_role role, struct bt_conn *conn, int err);
};

/**
 * @brief Adds TMAS instance to database and sets the received TMAP role(s).
 *
 * @param role TMAP role(s) of the device (one or multiple).
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_tmap_register(enum bt_tmap_role role);

/**
 * @brief Perform service discovery as TMAP Client
 *
 * @param conn     Pointer to the connection.
 * @param tmap_cb  Pointer to struct of TMAP callbacks.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_tmap_discover(struct bt_conn *conn, const struct bt_tmap_cb *tmap_cb);

/**
 * @brief Set one or multiple TMAP roles dynamically.
 *        Previously registered value will be overwritten.
 *
 * @param role     TMAP role(s).
 *
 */
void bt_tmap_set_role(enum bt_tmap_role role);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TMAP_ */
