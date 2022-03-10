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
 * to control hearing aid presets. This API implements the server functionality.
 *
 * [Experimental] Users should note that the APIs can change as a part of
 * ongoing development.
 */

#include <stdint.h>
#include <sys/util.h>
#include <bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_HAS)
#define BT_HAS_PRESET_CNT CONFIG_BT_HAS_PRESET_CNT
#else
#define BT_HAS_PRESET_CNT 0
#endif /* CONFIG_BT_HAS */

#define BT_HAS_PRESET_NAME_MIN 1
#define BT_HAS_PRESET_NAME_MAX 40

/** @brief Opaque Hearing Access Service object. */
struct bt_has;

/** Preset Properties values */
enum bt_has_prop {
	/** No properties set */
	BT_HAS_PROP_NONE        = 0,

	/** Preset name can be written by the client */
	BT_HAS_PROP_WRITABLE    = BIT(0),

	/** Preset availability */
	BT_HAS_PROP_AVAILABLE   = BIT(1),
};

/** @brief Register structure for preset. */
struct bt_has_preset_register_param {
	/** Preset index */
	uint8_t index;
	/** Preset properties */
	enum bt_has_prop properties;
	/** Preset name */
	const char *name;
};

/** @brief Preset operations structure. */
struct bt_has_ops {
	/** @brief Preset set active callback
	 *
	 *  Called if the client requests to activate preset identified by index.
	 *
	 *  @param has Pointer to the Hearing Access Service object.
	 *  @param index Preset index requested to activate,
	 *  @param sync Whether the server must relay this change to the
	 *             other member of the Binaural Hearing Aid Set.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 *  @return -EBUSY if operation cannot be executed at the time.
	 *  @return -EINPROGRESS in case where user has to confirm once the requested preset
	 *                       becomes active by calling @ref bt_has_preset_active_set.
	 */
	int (*active_set)(struct bt_has *has, uint8_t index, bool sync);

	/** @brief Preset name changed callback
	 *
	 *  Called if the preset name is changed by either the server or client.
	 *
	 *  @param has Pointer to the Hearing Access Service object.
	 *  @param index Preset index that name changed.
	 *  @param name Preset name after change.
	 */
	void (*name_changed)(struct bt_has *has, uint8_t index, const char *name);
};

/** Register structure for Hearing Access Service. */
struct bt_has_register_param {
	/** Preset records with the initial parameters. */
	struct bt_has_preset_register_param preset_param[BT_HAS_PRESET_CNT];

	/** Preset operations structure. */
	struct bt_has_ops *ops;
};

/** @brief Register Hearing Access Service.
 *
 *  @param param Hearing Access Service register parameters.
 *  @param[out] has Pointer to the local Hearing Access Service object.
 *                  This will still be valid if the return value is -EALREADY.
 *
 *  @return 0 if success, errno on failure.
 */
int bt_has_register(const struct bt_has_register_param *param, struct bt_has **has);

/** Hearing Aid device type */
enum bt_has_hearing_aid_type {
	BT_HAS_HEARING_AID_TYPE_BINAURAL,
	BT_HAS_HEARING_AID_TYPE_MONAURAL,
	BT_HAS_HEARING_AID_TYPE_BANDED,
};

/** @brief Hearing Access Service Client callback structure. */
struct bt_has_client_cb {
	/** @brief Callback function for bt_has_discover.
	 *
	 *  This callback is called when discovery procedure is complete.
	 *
	 *  @param conn Bluetooth connection object.
	 *  @param has Pointer to the Hearing Access Service object.
	 *  @param type Hearing Aid type.
	 */
	void (*discover)(struct bt_conn *conn, struct bt_has *has,
                         enum bt_has_hearing_aid_type type);

	/** @brief Callback function for Hearing Access Service active preset.
	 *
	 *  Called when the value is read by client or changed by the remote server.
	 *
	 *  @param has Pointer to the Hearing Access Service object.
	 *  @param err 0 in case of success or negative value in case of error.
	 *  @param index Active preset index.
	 */
	void (*active_preset)(struct bt_has *has, int err, uint8_t index);

	/** @brief Callback function for Hearing Access Service preset.
	 *
	 *  Called when the value is read by client, added or changed by the remote server.
	 *
	 *  @param has Pointer to the Hearing Access Service object.
	 *  @param err 0 in case of success or negative value in case of error.
	 *  @param index Preset index.
	 *  @param properties Preset properties.
	 *  @param name Preset name.
	 */
	void (*preset)(struct bt_has *has, int err, uint8_t index, uint8_t properties,
		       const char *name);

	/** @brief Callback function for Hearing Access Service preset deleted.
	 *
	 *  Called when the preset has been deleted by the remote server.
	 *
	 *  @param has Pointer to the Hearing Access Service object.
	 *  @param index Preset index.
	 */
	void (*preset_deleted)(struct bt_has *has, uint8_t index);

	/** @brief Callback function for Hearing Access Service preset availability.
	 *
	 *  Called when the preset availability is changed by the remote server.
	 *
	 *  @param has Pointer to the Hearing Access Service object.
	 *  @param index Preset index.
	 *  @param available True if available, false otherwise.
	 */
	void (*preset_availibility)(struct bt_has *has, uint8_t index, bool available);

	/** @brief Callback function for bt_has_preset_read.
	 *
	 *  This callback is called when preset read procedure is complete.
	 *
	 *  @param has Pointer to the Hearing Access Service object.
	 *  @param err 0 in case of success or negative value in case of error.
	 */
	void (*preset_read_complete)(struct bt_has *has, int err);
};

/** @brief Registers the callbacks used by the Hearing Access Service client.
 *
 *  @param cb The callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cb_register(struct bt_has_client_cb *cb);

/** @brief Get a Hearing Access Service.
 *
 *  Client method to find a Hearing Access Service on a server identified by conn.
 *
 *  @param conn Bluetooth connection object.
 *
 *  @return 0 if success, errno on failure.
 */
int bt_has_discover(struct bt_conn *conn);

/** @brief Set Preset Name.
 *
 *  Used by client and server to set the preset name identified by the index.
 *  Once used by the server to change the name of local preset, @ref bt_has_ops->name_changed
 *  callback will be called.
 *
 *  @note For server, this API is only available when the
 *        @kconfig{CONFIG_BT_HAS_PRESET_NAME_DYNAMIC} configuration option has been enabled.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *  @param index Preset index.
 *  @param name Preset name to be written.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_name_set(struct bt_has *has, uint8_t index, const char *name);

/** @brief Get Active Preset.
 *
 *  Used by client to get the index of currently active preset on remote server.
 *
 *  The value is returned in the bt_has_client_cb.active_preset callback.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_active_get(struct bt_has *has);

/** @brief Set Active Preset.
 *
 *  Used by client and server to set the preset identified by the ID as the active preset.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *  @param index Preset index.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_active_set(struct bt_has *has, uint8_t index);

/** @brief Set Next Preset.
 *
 *  Used by client to set the next preset record on the server list as the active preset.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_active_set_next(struct bt_has *has);

/** @brief Set Previous Preset.
 *
 *  Used by client to set the previous preset record on the server list as the active preset.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_active_set_prev(struct bt_has *has);

/** @brief Clear out Active Preset.
 *
 *  Used by server to deactivate currently active preset.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_active_clear(struct bt_has *has);

/** @brief Set visibility of preset record.
 *
 *  Server method to set visibility of preset record. Invisible presets are not removed
 *  but are not visible by peer devices.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *  @param index Preset index.
 *  @param visible true if preset should be visible
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_visibility_set(struct bt_has *has, uint8_t index, bool visible);

/** @brief Set availability of preset record.
 *
 *  Server method to set availability of preset record. Available preset can be set by peer
 *  device as active preset.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *  @param index Preset index.
 *  @param available true if preset should be available
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_availability_set(struct bt_has *has, uint8_t index, bool available);

/** @brief Read Preset Record.
 *
 *  Client method to read the preset identified by the index.
 *
 *  The value is returned in the bt_has_client_cb.preset callback.
 *  The bt_has_client_cb.preset_read_complete callback indicates the procedure is done.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *  @param index Index of preset to read.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_read(struct bt_has *has, uint8_t index);

/** @brief Read Multiple Preset Records.
 *
 *  Client method to read the number of presets starting from specified index.
 *
 *  The value is returned in the bt_has_client_cb.preset callback.
 *  The bt_has_client_cb.preset_read_complete callback indicates the procedure is done.
 *
 *  @param has Pointer to the Hearing Access Service object.
 *  @param start_index The index to start with.
 *  @param count Number of presets to read.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_read_multiple(struct bt_has *has, uint8_t start_index, uint8_t count);

/** @brief Get the Bluetooth connection object of the service object.
 *
 *  The caller gets a new reference to the connection object which must be
 *  released with bt_conn_unref() once done using the object.
 *
 *  @param has Service object.
 *  @param[out] conn Connection object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_conn_get(const struct bt_has *has, struct bt_conn **conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAS_H_ */
