/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VCS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VCS_H_

/**
 * @brief Volume Control Service (VCS)
 *
 * @defgroup bt_gatt_vcs Volume Control Service (VCS)
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

#if defined(CONFIG_BT_VCS)
#define BT_VCS_VOCS_CNT CONFIG_BT_VCS_VOCS_INSTANCE_COUNT
#define BT_VCS_AICS_CNT CONFIG_BT_VCS_AICS_INSTANCE_COUNT
#else
#define BT_VCS_VOCS_CNT 0
#define BT_VCS_AICS_CNT 0
#endif /* CONFIG_BT_VCS */

/** Volume Control Service Error codes */
#define BT_VCS_ERR_INVALID_COUNTER             0x80
#define BT_VCS_ERR_OP_NOT_SUPPORTED            0x81

/** Volume Control Service Mute Values */
#define BT_VCS_STATE_UNMUTED                   0x00
#define BT_VCS_STATE_MUTED                     0x01

/** @brief Opaque Volume Control Service instance. */
struct bt_vcs;

/** Register structure for Volume Control Service */
struct bt_vcs_register_param {
	/** Initial step size (1-255) */
	uint8_t step;

	/** Initial mute state (0-1) */
	uint8_t mute;

	/** Initial volume level (0-255) */
	uint8_t volume;

	/** Register parameters for Volume Offset Control Services */
	struct bt_vocs_register_param vocs_param[BT_VCS_VOCS_CNT];

	/** Register parameters  for Audio Input Control Services */
	struct bt_aics_register_param aics_param[BT_VCS_AICS_CNT];

	/** Volume Control Service callback structure. */
	struct bt_vcs_cb *cb;
};

/**
 * @brief Volume Control Service included services
 *
 * Used for to represent the Volume Control Service included service instances,
 * for either a client or a server. The instance pointers either represent
 * local server instances, or remote service instances.
 */
struct bt_vcs_included {
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
 * @param[out] vcs    Pointer to the registered Volume Control Service.
 *                    This will still be valid if the return value is -EALREADY.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_register(struct bt_vcs_register_param *param, struct bt_vcs **vcs);

/**
 * @brief Get Volume Control Service included services.
 *
 * Returns a pointer to a struct that contains information about the
 * Volume Control Service included service instances, such as pointers to the
 * Volume Offset Control Service (Volume Offset Control Service) or
 * Audio Input Control Service (AICS) instances.
 *
 * @param      vcs      Volume Control Service instance pointer.
 * @param[out] included Pointer to store the result in.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_included_get(struct bt_vcs *vcs, struct bt_vcs_included *included);

/**
 * @brief Get the connection pointer of a client instance
 *
 * Get the Bluetooth connection pointer of a Volume Control Service
 * client instance.
 *
 * @param      vcs     Volume Control Service client instance pointer.
 * @param[out] conn    Connection pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_client_conn_get(const struct bt_vcs *vcs, struct bt_conn **conn);

/**
 * @brief Callback function for bt_vcs_discover.
 *
 * This callback is only used for the client.
 *
 * @param vcs          Volume Control Service instance pointer.
 * @param err          Error value. 0 on success, GATT error on positive value
 *                     or errno on negative value.
 * @param vocs_count   Number of Volume Offset Control Service instances
 *                     on peer device.
 * @param aics_count   Number of Audio Input Control Service instances on
 *                     peer device.
 */
typedef void (*bt_vcs_discover_cb)(struct bt_vcs *vcs, int err,
				   uint8_t vocs_count, uint8_t aics_count);

/**
 * @brief Callback function for Volume Control Service volume state.
 *
 * Called when the value is locally read as the server.
 * Called when the value is remotely read as the client.
 * Called if the value is changed by either the server or client.
 *
 * @param vcs     Volume Control Service instance pointer.
 * @param err     Error value. 0 on success, GATT error on positive value
 *                or errno on negative value.
 * @param volume  The volume of the Volume Control Service server.
 * @param mute    The mute setting of the Volume Control Service server.
 */
typedef void (*bt_vcs_state_cb)(struct bt_vcs *vcs, int err, uint8_t volume,
				uint8_t mute);

/**
 * @brief Callback function for Volume Control Service flags.
 *
 * Called when the value is locally read as the server.
 * Called when the value is remotely read as the client.
 * Called if the value is changed by either the server or client.
 *
 * @param vcs     Volume Control Service instance pointer.
 * @param err     Error value. 0 on success, GATT error on positive value
 *                or errno on negative value.
 * @param flags   The flags of the Volume Control Service server.
 */
typedef void (*bt_vcs_flags_cb)(struct bt_vcs *vcs, int err, uint8_t flags);

/**
 * @brief Callback function for writes.
 *
 * This callback is only used for the client.
 *
 * @param vcs     Volume Control Service instance pointer.
 * @param err     Error value. 0 on success, GATT error on fail.
 */
typedef void (*bt_vcs_write_cb)(struct bt_vcs *vcs, int err);

struct bt_vcs_cb {
	/* Volume Control Service */
	bt_vcs_state_cb               state;
	bt_vcs_flags_cb               flags;
#if defined(CONFIG_BT_VCS_CLIENT)
	bt_vcs_discover_cb            discover;
	bt_vcs_write_cb               vol_down;
	bt_vcs_write_cb               vol_up;
	bt_vcs_write_cb               mute;
	bt_vcs_write_cb               unmute;
	bt_vcs_write_cb               vol_down_unmute;
	bt_vcs_write_cb               vol_up_unmute;
	bt_vcs_write_cb               vol_set;

	/* Volume Offset Control Service */
	struct bt_vocs_cb             vocs_cb;

	/* Audio Input Control Service */
	struct bt_aics_cb             aics_cb;
#endif /* CONFIG_BT_VCS_CLIENT */
};

/**
 * @brief Discover Volume Control Service and included services.
 *
 * This will start a GATT discovery and setup handles and subscriptions.
 * This shall be called once before any other actions can be
 * executed for the peer device, and the @ref bt_vcs_discover_cb callback
 * will notify when it is possible to start remote operations.
 *
 * This shall only be done as the client,
 *
 * @param      conn  The connection to discover Volume Control Service for.
 * @param[out] vcs   Valid remote instance object on success.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_discover(struct bt_conn *conn, struct bt_vcs **vcs);

/**
 * @brief Set the Volume Control Service volume step size.
 *
 * Set the value that the volume changes, when changed relatively with e.g.
 * @ref bt_vcs_vol_down or @ref bt_vcs_vol_up.
 *
 * This can only be done as the server.
 *
 * @param volume_step  The volume step size (1-255).
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vol_step_set(uint8_t volume_step);

/**
 * @brief Read the Volume Control Service volume state.
 *
 * @param vcs  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vol_get(struct bt_vcs *vcs);

/**
 * @brief Read the Volume Control Service flags.
 *
 * @param vcs  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_flags_get(struct bt_vcs *vcs);

/**
 * @brief Turn the volume down by one step on the server.
 *
 * @param vcs  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vol_down(struct bt_vcs *vcs);

/**
 * @brief Turn the volume up by one step on the server.
 *
 * @param vcs  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vol_up(struct bt_vcs *vcs);

/**
 * @brief Turn the volume down and unmute the server.
 *
 * @param vcs  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_unmute_vol_down(struct bt_vcs *vcs);

/**
 * @brief Turn the volume up and unmute the server.
 *
 * @param vcs  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_unmute_vol_up(struct bt_vcs *vcs);

/**
 * @brief Set the volume on the server
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param volume The absolute volume to set.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vol_set(struct bt_vcs *vcs, uint8_t volume);

/**
 * @brief Unmute the server.
 *
 * @param vcs  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_unmute(struct bt_vcs *vcs);

/**
 * @brief Mute the server.
 *
 * @param vcs  Volume Control Service instance pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_mute(struct bt_vcs *vcs);

/**
 * @brief Read the Volume Offset Control Service offset state.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Volume Offset Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vocs_state_get(struct bt_vcs *vcs, struct bt_vocs *inst);

/**
 * @brief Read the Volume Offset Control Service location.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Volume Offset Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vocs_location_get(struct bt_vcs *vcs, struct bt_vocs *inst);

/**
 * @brief Set the Volume Offset Control Service location.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst       Pointer to the Volume Offset Control Service instance.
 * @param location   The location to set.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vocs_location_set(struct bt_vcs *vcs, struct bt_vocs *inst,
			     uint8_t location);

/**
 * @brief Set the Volume Offset Control Service offset state.
 *
 * @param vcs     Volume Control Service instance pointer.
 * @param inst    Pointer to the Volume Offset Control Service instance.
 * @param offset  The offset to set (-255 to 255).
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vocs_state_set(struct bt_vcs *vcs, struct bt_vocs *inst,
			  int16_t offset);

/**
 * @brief Read the Volume Offset Control Service output description.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Volume Offset Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vocs_description_get(struct bt_vcs *vcs, struct bt_vocs *inst);

/**
 * @brief Set the Volume Offset Control Service description.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst          Pointer to the Volume Offset Control Service instance.
 * @param description   The description to set.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_vocs_description_set(struct bt_vcs *vcs, struct bt_vocs *inst,
				const char *description);

/**
 * @brief Deactivates an Audio Input Control Service instance.
 *
 * Audio Input Control Services are activated by default, but this will allow
 * the server to deactivate an Audio Input Control Service.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_deactivate(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Activates an Audio Input Control Service instance.
 *
 * Audio Input Control Services are activated by default, but this will allow
 * the server to reactivate an Audio Input Control Service instance after it has
 * been deactivated with @ref bt_vcs_aics_deactivate.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_activate(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Read the Audio Input Control Service input state.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_state_get(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Read the Audio Input Control Service gain settings.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_gain_setting_get(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Read the Audio Input Control Service input type.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_type_get(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Read the Audio Input Control Service input status.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_status_get(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Mute the Audio Input Control Service input.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_mute(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Unmute the Audio Input Control Service input.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_unmute(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Set input gain to manual.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_manual_gain_set(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Set the input gain to automatic.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_automatic_gain_set(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Set the input gain.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 * @param gain   The gain in dB to set (-128 to 127).
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_gain_set(struct bt_vcs *vcs, struct bt_aics *inst,
			 int8_t gain);

/**
 * @brief Read the Audio Input Control Service description.
 *
 * @param vcs    Volume Control Service instance pointer.
 * @param inst   Pointer to the Audio Input Control Service instance.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_description_get(struct bt_vcs *vcs, struct bt_aics *inst);

/**
 * @brief Set the Audio Input Control Service description.
 *
 * @param vcs           Volume Control Service instance pointer.
 * @param inst          Pointer to the Audio Input Control Service instance.
 * @param description   The description to set.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_aics_description_set(struct bt_vcs *vcs, struct bt_aics *inst,
				const char *description);

/**
 * @brief Registers the callbacks used by the Volume Control Service client.
 *
 * @param cb   The callback structure.
 *
 * @return 0 if success, errno on failure.
 */
int bt_vcs_client_cb_register(struct bt_vcs_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VCS_H_ */
