/*
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MICP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MICP_H_

/**
 * @brief Microphone Input Control Profile (MICP)
 *
 * @defgroup bt_gatt_micp Microphone Input Control Profile (MICP)
 *
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/aics.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MICP)
#define BT_MICP_AICS_CNT CONFIG_BT_MICP_AICS_INSTANCE_COUNT
#else
#define BT_MICP_AICS_CNT 0
#endif /* CONFIG_BT_MICP */

/** Application error codes */
#define BT_MICP_ERR_MUTE_DISABLED                  0x80
#define BT_MICP_ERR_VAL_OUT_OF_RANGE               0x81

/** Microphone Input Control Profile mute states */
#define BT_MICP_MUTE_UNMUTED                       0x00
#define BT_MICP_MUTE_MUTED                         0x01
#define BT_MICP_MUTE_DISABLED                      0x02

/** @brief Opaque Microphone Input Control Profile instance. */
struct bt_micp;

/** @brief Register parameters structure for Microphone Input Control Profile instance */
struct bt_micp_register_param {
#if defined(CONFIG_BT_MICP_AICS)
	/** Register parameter structure for Audio Input Control Services */
	struct bt_aics_register_param aics_param[BT_MICP_AICS_CNT];
#endif /* CONFIG_BT_MICP_AICS */

	/** Microphone Input Control Profile callback structure. */
	struct bt_micp_cb *cb;
};

/**
 * @brief Microphone Input Control Profile included services
 *
 * Used for to represent the Microphone Input Control Profile included service
 * instances, for either a client or a server instance. The instance pointers
 * either represent local server instances, or remote service instances.
 */
struct bt_micp_included  {
	/** Number of Audio Input Control Service instances */
	uint8_t aics_cnt;
	/** Array of pointers to Audio Input Control Service instances */
	struct bt_aics **aics;
};

/**
 * @brief Initialize the Microphone Input Control Profile
 *
 * This will enable the Microphone Input Control Profile instance and make it
 * discoverable by clients.
 * This can only be done as the server.
 *
 * @param      param Pointer to an initialization structure.
 * @param[out] micp  Pointer to the registered Microphone Input Control Profile
 *                   instance. This will still be valid if the return value is
 *                   -EALREADY.
 *
 * @return 0 if success, errno on failure.
 */
int bt_micp_register(struct bt_micp_register_param *param,
		     struct bt_micp **micp);

/**
 * @brief Get Microphone Input Control Profile included services
 *
 * Returns a pointer to a struct that contains information about the
 * Microphone Input Control Profile included services instances, such as
 * pointers to the Audio Input Control Service instances.
 *
 * Requires that @kconfig{CONFIG_BT_MICP_AICS} or
 * @kconfig{CONFIG_BT_MICP_CLIENT_AICS} is enabled.
 *
 * @param      micp     Microphone Input Control Profile instance pointer.
 * @param[out] included Pointer to store the result in.
 *
 * @return 0 if success, errno on failure.
 */
int bt_micp_included_get(struct bt_micp *micp,
			 struct bt_micp_included *included);

/**
 * @brief Get the connection pointer of a client instance
 *
 * Get the Bluetooth connection pointer of a Microphone Input Control Profile
 * client instance.
 *
 * @param micp    Microphone Input Control Profile client instance pointer.
 * @param conn    Connection pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_micp_client_conn_get(const struct bt_micp *micp, struct bt_conn **conn);

/**
 * @brief Callback function for @ref bt_micp_discover.
 *
 * This callback is only used for the client.
 *
 * @param micp         Microphone Input Control Profile instance pointer.
 * @param err          Error value. 0 on success, GATT error or errno on fail.
 * @param aics_count   Number of Audio Input Control Service instances on
 *                     peer device.
 */
typedef void (*bt_micp_discover_cb)(struct bt_micp *micp, int err,
				    uint8_t aics_count);

/**
 * @brief Callback function for Microphone Input Control Profile mute.
 *
 * Called when the value is read,
 * or if the value is changed by either the server or client.
 *
 * @param micp     Microphone Input Control Profile instance pointer.
 * @param err      Error value. 0 on success, GATT error or errno on fail.
 *                 For notifications, this will always be 0.
 * @param mute     The mute setting of the Microphone Input Control Profile instance.
 */
typedef void (*bt_micp_mute_read_cb)(struct bt_micp *micp, int err,
				     uint8_t mute);

/**
 * @brief Callback function for Microphone Input Control Profile mute/unmute.
 *
 * @param micp      Microphone Input Control Profile instance pointer.
 * @param err       Error value. 0 on success, GATT error or errno on fail.
 */
typedef void (*bt_micp_mute_write_cb)(struct bt_micp *micp, int err);

struct bt_micp_cb {
	bt_micp_mute_read_cb            mute;

#if defined(CONFIG_BT_MICP_CLIENT)
	bt_micp_discover_cb             discover;
	bt_micp_mute_write_cb           mute_write;
	bt_micp_mute_write_cb           unmute_write;

#if defined(CONFIG_BT_MICP_CLIENT_AICS)
	/** Audio Input Control Service client callback */
	struct bt_aics_cb               aics_cb;
#endif /* CONFIG_BT_MICP_CLIENT_AICS */
#endif /* CONFIG_BT_MICP_CLIENT */
};

/**
 * @brief Discover Microphone Input Control Profile instance
 *
 * This will start a GATT discovery and setup handles and subscriptions.
 * This shall be called once before any other actions can be executed for the
 * peer device, and the @ref bt_micp_discover_cb callback will notify when it
 * is possible to start remote operations.
 *
 * This shall only be done as the client.
 *
 * @param conn          The connection to initialize the profile for.
 * @param[out] micp     Valid remote instance object on success.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_micp_discover(struct bt_conn *conn, struct bt_micp **micp);

/**
 * @brief Unmute the server.
 *
 * @param micp  Microphone Input Control Profile instance pointer.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_micp_unmute(struct bt_micp *micp);

/**
 * @brief Mute the server.
 *
 * @param micp  Microphone Input Control Profile instance pointer.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_micp_mute(struct bt_micp *micp);

/**
 * @brief Disable the mute functionality.
 *
 * Can be reenabled by called @ref bt_micp_mute or @ref bt_micp_unmute.
 * This can only be done as the server.
 *
 * @param micp  Microphone Input Control Profile instance pointer.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_micp_mute_disable(struct bt_micp *micp);

/**
 * @brief Read the mute state of a Microphone Input Control Profile instance.
 *
 * @param micp  Microphone Input Control Profile instance pointer.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_micp_mute_get(struct bt_micp *micp);

/**
 * @brief Registers the callbacks used by Microphone Input Control Profile client.
 *
 * This can only be done as the client.
 *
 * @param cb    The callback structure.
 *
 * @return 0 if success, errno on failure.
 */
int bt_micp_client_cb_register(struct bt_micp_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MICP_H_ */
