/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_MICS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_MICS_H_

/**
 * @brief Microphone Input Control Service (Microphone Input Control Service)
 *
 * @defgroup bt_gatt_mics Microphone Input Control Service (Microphone Input Control Service)
 *
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <bluetooth/audio/aics.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MICS)
#define BT_MICS_AICS_CNT CONFIG_BT_MICS_AICS_INSTANCE_COUNT
#else
#define BT_MICS_AICS_CNT 0
#endif /* CONFIG_BT_MICS */

/** Application error codes */
#define BT_MICS_ERR_MUTE_DISABLED                  0x80
#define BT_MICS_ERR_VAL_OUT_OF_RANGE               0x81

/** Microphone Input Control Service mute states */
#define BT_MICS_MUTE_UNMUTED                       0x00
#define BT_MICS_MUTE_MUTED                         0x01
#define BT_MICS_MUTE_DISABLED                      0x02

/** @brief Register parameters structure for Microphone Input Control Service */
struct bt_mics_register_param {
	/** Register parameter structure for Audio Input Control Services */
	struct bt_aics_register_param aics_param[BT_MICS_AICS_CNT];

	/** Microphone Input Control Service callback structure. */
	struct bt_mics_cb *cb;
};

/**
 * @brief Microphone Input Control Service included services
 *
 * Used for to represent the Microphone Input Control Service included service
 * instances, for either a client or a server instance. The instance pointers
 * either represent local server instances, or remote service instances.
 */
struct bt_mics_included  {
	/** Number of Audio Input Control Service instances */
	uint8_t aics_cnt;
	/** Array of pointers to Audio Input Control Service instances */
	struct bt_aics **aics;
};

/**
 * @brief Initialize the Microphone Input Control Service
 *
 * This will enable the service and make it discoverable by clients.
 * This can only be done as the server.
 *
 * @param param  Pointer to an initialization structure.
 *
 * @return 0 if success, errno on failure.
 */
int bt_mics_register(struct bt_mics_register_param *param);

/**
 * @brief Get Microphone Input Control Service included services
 *
 * Returns a pointer to a struct that contains information about the
 * Microphone Input Control Service included services instances, such as
 * pointers to the Audio Input Control Service instances.
 *
 * @param conn          Connection to peer device, or NULL to get server value.
 * @param[out] included Pointer to store the result in.
 *
 * @return 0 if success, errno on failure.
 */
int bt_mics_included_get(struct bt_conn *conn,
			 struct bt_mics_included *included);

/**
 * @brief Callback function for @ref bt_mics_discover.
 *
 * This callback is only used for the client.
 *
 * @param conn         The connection that was used to discover
 *                     Microphone Input Control Service.
 * @param err          Error value. 0 on success, GATT error or errno on fail.
 * @param aics_count   Number of Audio Input Control Service instances on
 *                     peer device.
 */
typedef void (*bt_mics_discover_cb)(struct bt_conn *conn, int err,
				    uint8_t aics_count);

/**
 * @brief Callback function for Microphone Input Control Service mute.
 *
 * Called when the value is read,
 * or if the value is changed by either the server or client.
 *
 * @param conn     Connection to peer device, or NULL if local server read.
 * @param err      Error value. 0 on success, GATT error or errno on fail.
 *                 For notifications, this will always be 0.
 * @param mute     The mute setting of the Microphone Input Control Service.
 */
typedef void (*bt_mics_mute_read_cb)(struct bt_conn *conn, int err,
				     uint8_t mute);

/**
 * @brief Callback function for Microphone Input Control Service mute/unmute.
 *
 * @param conn      Connection to peer device, or NULL if local server read.
 * @param err       Error value. 0 on success, GATT error or errno on fail.
 * @param req_val   The requested mute value.
 */
typedef void (*bt_mics_mute_write_cb)(struct bt_conn *conn, int err);

struct bt_mics_cb {
	bt_mics_mute_read_cb            mute;

#if defined(CONFIG_BT_MICS_CLIENT)
	bt_mics_discover_cb             discover;
	bt_mics_mute_write_cb           mute_write;
	bt_mics_mute_write_cb           unmute_write;

	/** Audio Input Control Service client callback */
	struct bt_aics_cb               aics_cb;
#endif /* CONFIG_BT_MICS_CLIENT */
};

/**
 * @brief Discover Microphone Input Control Service
 *
 * This will start a GATT discovery and setup handles and subscriptions.
 * This shall be called once before any other actions can be executed for
 * the peer device.
 *
 * This shall only be done as the client.
 *
 * @param conn          The connection to initialize the profile for.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_discover(struct bt_conn *conn);

/**
 * @brief Unmute the server.
 *
 * @param conn   Connection to peer device, or NULL to set local server value.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_unmute(struct bt_conn *conn);

/**
 * @brief Mute the server.
 *
 * @param conn   Connection to peer device, or NULL to set local server value.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_mute(struct bt_conn *conn);

/**
 * @brief Disable the mute functionality.
 *
 * Can be reenabled by called @ref bt_mics_mute or @ref bt_mics_unmute.
 * This can only be done as the server.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_mute_disable(void);

/**
 * @brief Read the mute state of a Microphone Input Control Service.
 *
 * @param conn   Connection to peer device, or NULL to read local server value.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_mute_get(struct bt_conn *conn);

/**
 * @brief Read the Audio Input Control Service input state.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to read local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_state_get(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Read the Audio Input Control Service gain settings.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to read local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_gain_setting_get(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Read the Audio Input Control Service input type.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to read local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_type_get(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Read the Audio Input Control Service input status.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to read local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_status_get(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Unmute the Audio Input Control Service input.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to set local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_unmute(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Mute the Audio Input Control Service input.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to set local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_mute(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Set Audio Input Control Service gain mode to manual.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to set local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_manual_gain_set(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Set Audio Input Control Service gain mode to automatic.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to set local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_automatic_gain_set(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Set Audio Input Control Service input gain.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to set local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 * @param gain          The gain in dB to set (-128 to 127).
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_gain_set(struct bt_conn *conn, struct bt_aics *inst,
			  int8_t gain);

/**
 * @brief Read the Audio Input Control Service description.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to read local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_description_get(struct bt_conn *conn, struct bt_aics *inst);

/**
 * @brief Set the Audio Input Control Service description.
 *
 * @param conn          Connection to peer device,
 *                      or NULL to set local server value.
 * @param inst          Pointer to the Audio Input Control Service instance.
 * @param description	 The description to set.
 *
 * @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_description_set(struct bt_conn *conn, struct bt_aics *inst,
				 const char *description);

/**
 * @brief Deactivates a Audio Input Control Service instance.
 *
 * Audio Input Control Services are activated by default, but this will allow
 * the server to deactivate a Audio Input Control Service.
 * This can only be done as the server.
 *
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_mics_aics_deactivate(struct bt_aics *inst);

/**
 * @brief Activates a Audio Input Control Service instance.
 *
 * Audio Input Control Services are activated by default, but this will allow
 * the server to reactivate a Audio Input Control Service instance after it has
 * been deactivated with @ref bt_mics_aics_deactivate.
 * This can only be done as the server.
 *
 * @param inst          Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_mics_aics_activate(struct bt_aics *inst);

/**
 * @brief Registers the callbacks used by Microphone Input Control Service client.
 *
 * This can only be done as the client.
 *
 * @param cb    The callback structure.
 *
 * @return 0 if success, errno on failure.
 */
int bt_mics_client_cb_register(struct bt_mics_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_MICS_H_ */
