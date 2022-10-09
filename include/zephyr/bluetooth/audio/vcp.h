/*
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VCP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VCP_H_

/**
 * @brief Volume Control Profile (VCP)
 *
 * @defgroup bt_gatt_vcp Volume Control Profile (VCP)
 *
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/vocs.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_VCP)
#define BT_VCP_VOCS_CNT CONFIG_BT_VCP_VOCS_INSTANCE_COUNT
#define BT_VCP_AICS_CNT CONFIG_BT_VCP_AICS_INSTANCE_COUNT
#else
#define BT_VCP_VOCS_CNT 0
#define BT_VCP_AICS_CNT 0
#endif /* CONFIG_BT_VCP */

/** Volume Control Service Error codes */
#define BT_VCP_ERR_INVALID_COUNTER             0x80
#define BT_VCP_ERR_OP_NOT_SUPPORTED            0x81

/** Volume Control Service Mute Values */
#define BT_VCP_STATE_UNMUTED                   0x00
#define BT_VCP_STATE_MUTED                     0x01

/** @brief Opaque Volume Control Service instance. */
struct bt_vcp;

/** Register structure for Volume Control Service */
struct bt_vcp_register_param {
	/** Initial step size (1-255) */
	uint8_t step;

	/** Initial mute state (0-1) */
	uint8_t mute;

	/** Initial volume level (0-255) */
	uint8_t volume;

	/** Register parameters for Volume Offset Control Services */
	struct bt_vocs_register_param vocs_param[BT_VCP_VOCS_CNT];

	/** Register parameters  for Audio Input Control Services */
	struct bt_aics_register_param aics_param[BT_VCP_AICS_CNT];

	/** Volume Control Service callback structure. */
	struct bt_vcp_cb *cb;
};

/**
 * @brief Volume Control Service included services
 *
 * Used for to represent the Volume Control Service included service instances,
 * for either a client or a server. The instance pointers either represent
 * local server instances, or remote service instances.
 */
struct bt_vcp_included {
	/** Number of Volume Offset Control Service instances */
	uint8_t vocs_cnt;
	/** Array of pointers to Volume Offset Control Service instances */
	struct bt_vocs **vocs;

	/** Number of Audio Input Control Service instances */
	uint8_t aics_cnt;
	/** Array of pointers to Audio Input Control Service instances */
	struct bt_aics **aics;
};

/**
 * @brief Register the Volume Control Service.
 *
 * This will register and enable the service and make it discoverable by
 * clients.
 *
 * @param      param  Volume Control Service register parameters.
 * @param[out] vcp    Pointer to the registered Volume Control Service.
 *                    This will still be valid if the return value is -EALREADY.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_register(struct bt_vcp_register_param *param, struct bt_vcp **vcp);

/**
 * @brief Get Volume Control Service included services.
 *
 * Returns a pointer to a struct that contains information about the
 * Volume Control Service included service instances, such as pointers to the
 * Volume Offset Control Service (Volume Offset Control Service) or
 * Audio Input Control Service (AICS) instances.
 *
 * @param      vcp      Volume Control Service instance pointer.
 * @param[out] included Pointer to store the result in.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_included_get(struct bt_vcp *vcp, struct bt_vcp_included *included);

/**
 * @brief Get the connection pointer of a client instance
 *
 * Get the Bluetooth connection pointer of a Volume Control Service
 * client instance.
 *
 * @param      vcp     Volume Control Service client instance pointer.
 * @param[out] conn    Connection pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_client_conn_get(const struct bt_vcp *vcp, struct bt_conn **conn);

/**
 * @brief Callback function for bt_vcp_discover.
 *
 * This callback is only used for the client.
 *
 * @param vcp          Volume Control Service instance pointer.
 * @param err          Error value. 0 on success, GATT error on positive value
 *                     or errno on negative value.
 * @param vocs_count   Number of Volume Offset Control Service instances
 *                     on peer device.
 * @param aics_count   Number of Audio Input Control Service instances on
 *                     peer device.
 */
typedef void (*bt_vcp_discover_cb)(struct bt_vcp *vcp, int err,
				   uint8_t vocs_count, uint8_t aics_count);

/**
 * @brief Callback function for Volume Control Service volume state.
 *
 * Called when the value is locally read as the server.
 * Called when the value is remotely read as the client.
 * Called if the value is changed by either the server or client.
 *
 * @param vcp     Volume Control Service instance pointer.
 * @param err     Error value. 0 on success, GATT error on positive value
 *                or errno on negative value.
 * @param volume  The volume of the Volume Control Service server.
 * @param mute    The mute setting of the Volume Control Service server.
 */
typedef void (*bt_vcp_state_cb)(struct bt_vcp *vcp, int err, uint8_t volume,
				uint8_t mute);

/**
 * @brief Callback function for Volume Control Service flags.
 *
 * Called when the value is locally read as the server.
 * Called when the value is remotely read as the client.
 * Called if the value is changed by either the server or client.
 *
 * @param vcp     Volume Control Service instance pointer.
 * @param err     Error value. 0 on success, GATT error on positive value
 *                or errno on negative value.
 * @param flags   The flags of the Volume Control Service server.
 */
typedef void (*bt_vcp_flags_cb)(struct bt_vcp *vcp, int err, uint8_t flags);

/**
 * @brief Callback function for writes.
 *
 * This callback is only used for the client.
 *
 * @param vcp     Volume Control Service instance pointer.
 * @param err     Error value. 0 on success, GATT error on fail.
 */
typedef void (*bt_vcp_write_cb)(struct bt_vcp *vcp, int err);

struct bt_vcp_cb {
	/* Volume Control Service */
	bt_vcp_state_cb               state;
	bt_vcp_flags_cb               flags;
#if defined(CONFIG_BT_VCP_CLIENT)
	bt_vcp_discover_cb            discover;
	bt_vcp_write_cb               vol_down;
	bt_vcp_write_cb               vol_up;
	bt_vcp_write_cb               mute;
	bt_vcp_write_cb               unmute;
	bt_vcp_write_cb               vol_down_unmute;
	bt_vcp_write_cb               vol_up_unmute;
	bt_vcp_write_cb               vol_set;

	/* Volume Offset Control Service */
	struct bt_vocs_cb             vocs_cb;

	/* Audio Input Control Service */
	struct bt_aics_cb             aics_cb;
#endif /* CONFIG_BT_VCP_CLIENT */
};

/**
 * @brief Discover Volume Control Service and included services.
 *
 * This will start a GATT discovery and setup handles and subscriptions.
 * This shall be called once before any other actions can be
 * executed for the peer device, and the @ref bt_vcp_discover_cb callback
 * will notify when it is possible to start remote operations.
 *
 * This shall only be done as the client,
 *
 * @param      conn  The connection to discover Volume Control Service for.
 * @param[out] vcp   Valid remote instance object on success.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_discover(struct bt_conn *conn, struct bt_vcp **vcp);

/**
 * @brief Set the Volume Control Service volume step size.
 *
 * Set the value that the volume changes, when changed relatively with e.g.
 * @ref bt_vcp_vol_down or @ref bt_vcp_vol_up.
 *
 * This can only be done as the server.
 *
 * @param volume_step  The volume step size (1-255).
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_vol_step_set(uint8_t volume_step);

/**
 * @brief Read the Volume Control Service volume state.
 *
 * @param vcp  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_vol_get(struct bt_vcp *vcp);

/**
 * @brief Read the Volume Control Service flags.
 *
 * @param vcp  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_flags_get(struct bt_vcp *vcp);

/**
 * @brief Turn the volume down by one step on the server.
 *
 * @param vcp  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_vol_down(struct bt_vcp *vcp);

/**
 * @brief Turn the volume up by one step on the server.
 *
 * @param vcp  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_vol_up(struct bt_vcp *vcp);

/**
 * @brief Turn the volume down and unmute the server.
 *
 * @param vcp  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_unmute_vol_down(struct bt_vcp *vcp);

/**
 * @brief Turn the volume up and unmute the server.
 *
 * @param vcp  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_unmute_vol_up(struct bt_vcp *vcp);

/**
 * @brief Set the volume on the server
 *
 * @param vcp    Volume Control Service instance pointer.
 * @param volume The absolute volume to set.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_vol_set(struct bt_vcp *vcp, uint8_t volume);

/**
 * @brief Unmute the server.
 *
 * @param vcp  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_unmute(struct bt_vcp *vcp);

/**
 * @brief Mute the server.
 *
 * @param vcp  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_mute(struct bt_vcp *vcp);

/**
 * @brief Registers the callbacks used by the Volume Control Service client.
 *
 * @param cb   The callback structure.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcp_client_cb_register(struct bt_vcp_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VCP_H_ */
