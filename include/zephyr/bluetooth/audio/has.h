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

#include <sys/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Preset index definitions */
#define BT_HAS_PRESET_INDEX_NONE 0x00
#define BT_HAS_PRESET_INDEX_FIRST 0x01
#define BT_HAS_PRESET_INDEX_LAST 0xFF

/** Preset name minimum length */
#define BT_HAS_PRESET_NAME_MIN 1
/** Preset name maximum length */
#define BT_HAS_PRESET_NAME_MAX 40

/** Hearing Aid device type */
enum bt_has_hearing_aid_type {
	BT_HAS_HEARING_AID_TYPE_BINAURAL = 0x00,
	BT_HAS_HEARING_AID_TYPE_MONAURAL = 0x01,
	BT_HAS_HEARING_AID_TYPE_BANDED = 0x02,
};

/** Hearing Aid device features */
enum bt_has_features {
	/* Hearing Aid Type least significant bit. */
	BT_HAS_FEAT_HEARING_AID_TYPE_LSB = BIT(0),

	/* Hearing Aid Type most significant bit. */
	BT_HAS_FEAT_HEARING_AID_TYPE_MSB = BIT(1),

	/* Preset Synchronization support. */
	BT_HAS_FEAT_PRESET_SYNC_SUPP = BIT(2),

	/* Preset records independence. The list of presets may be different from the list exposed
	 * by the other Coordinated Set Member.
	 */
	BT_HAS_FEAT_INDEPENDENT_PRESETS = BIT(3),

	/* Dynamic presets support. The list of preset records may change. */
	BT_HAS_FEAT_DYNAMIC_PRESETS = BIT(4),

	/* Writable preset records support. The preset name can be changed by the client. */
	BT_HAS_FEAT_WRITABLE_PRESETS_SUPP = BIT(5),
};

/** Preset Properties values */
enum bt_has_properties {
	/** No properties set */
	BT_HAS_PROP_NONE = 0,

	/** Preset name can be written by the client */
	BT_HAS_PROP_WRITABLE = BIT(0),

	/** Preset availability */
	BT_HAS_PROP_AVAILABLE = BIT(1),
};

/** Hearing Aid device capablilities */
enum bt_has_capabilities {
	BT_HAS_PRESET_SUPPORT = BIT(0),
};

/** @brief Preset record definition */
struct bt_has_preset_record {
	/** Unique preset index. */
	uint8_t index;

	/** Bitfield of preset properties. */
	enum bt_has_properties properties;

	/** Preset name. */
	const char *name;
};

/** @brief Opaque Hearing Access Service Client instance. */
struct bt_has_client;

/** @brief Hearing Access Service Client callback structure. */
struct bt_has_client_cb {
	/**
	 * @brief Service has been connected.
	 *
	 * This callback notifies the application of a new service connection.
	 * In case the err parameter is non-zero it means that the connection establishment failed.
	 *
	 * @param client Client instance.
	 * @param err 0 in case of success or negative errno or ATT Error code in case of error.
	 */
	void (*connected)(struct bt_has_client *client, int err);

	/**
	 * @brief Service has been disconnected.
	 *
	 * This callback notifies the application that the service has been disconnected.
	 *
	 * @param client Client instance.
	 * @param unbound True if service binding has been removed.
	 */
	void (*disconnected)(struct bt_has_client *client);

	/**
	 * @brief Client instance unbound.
	 *
	 * This callback notifies the application that the client instance has been free'd.
	 *
	 * @param client Client instance.
	 * @param err 0 in case of success or negative errno or ATT Error code in case of error.
	 */
	void (*unbound)(struct bt_has_client *client, int err);

	/**
	 * @brief Callback function for Hearing Access Service active preset changes.
	 *
	 * Optional callback called when the active preset is changed by the remote server when the
	 * preset switch procedure is complete. The callback must be set to receive active preset
	 * changes and enable support for switching presets. If the callback is not set, the Active
	 * Index and Control Point characteristics will not be discovered by
	 * @ref bt_has_client_discover.
	 *
	 * @param client Client instance.
	 * @param index Active preset index.
	 */
	void (*preset_switch)(struct bt_has_client *client, uint8_t index);

	/**
	 * @brief Callback function for presets read operation.
	 *
	 * The callback is called when the preset read response is sent by the remote server.
	 * The record object as well as its members are temporary and must be copied to in order
	 * to cache its information.
	 *
	 * @param client Client instance.
	 * @param record Preset record or NULL on errors.
	 * @param is_last True if Read Presets operation can be considered concluded.
	 */
	void (*preset_read_rsp)(struct bt_has_client *client,
				const struct bt_has_preset_record *record, bool is_last);

	/**
	 * @brief Callback function for preset update notifications.
	 *
	 * The callback is called when the preset record update is notified by the remote server.
	 * The record object as well as its objects are temporary and must be copied to in order
	 * to cache its information.
	 *
	 * @param client Client instance.
	 * @param index_prev Index of the previous preset in the list.
	 * @param record Preset record.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_update)(struct bt_has_client *client, uint8_t index_prev,
			      const struct bt_has_preset_record *record, bool is_last);

	/**
	 * @brief Callback function for preset deletion notifications.
	 *
	 * The callback is called when the preset has been deleted by the remote server.
	 *
	 * @param client Client instance.
	 * @param index Preset index.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_deleted)(struct bt_has_client *client, uint8_t index, bool is_last);

	/**
	 * @brief Callback function for preset availability notifications.
	 *
	 * The callback is called when the preset availability change is notified by the remote
	 * server.
	 *
	 * @param client Client instance.
	 * @param index Preset index.
	 * @param available True if available, false otherwise.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_availability)(struct bt_has_client *client, uint8_t index, bool available,
				    bool is_last);

	/**
	 * @brief Callback function for features notifications.
	 *
	 * The callback is called when the Hearing Aid Features change is notified by the remote
	 * server.
	 *
	 * @param client Client instance.
	 * @param features Updated features value.
	 */
	void (*features)(struct bt_has_client *client, enum bt_has_features features);

	/**
	 * @brief Callback function for command operations.
	 *
	 * The callback is called when command status is received from the remote server.
	 *
	 * @param client Client instance.
	 * @param err 0 on success, ATT error otherwise.
	 */
	void (*cmd_status)(struct bt_has_client *client, uint8_t err);
};

/**
 * @brief Initialize Hearing Access Service Client.
 *
 * Register Hearing Access Service Client callbacks.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_has_client_init(const struct bt_has_client_cb *cb);

/**
 * @brief Bind service.
 *
 * Initiate service binding to remote device implementing HAS Server role.
 *
 * @param conn Bluetooth connection object.
 * @param[out] client Valid client instance on success.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_has_client_bind(struct bt_conn *conn, struct bt_has_client **client);

/**
 * @brief Unbind service.
 *
 * Unbind an service service or cancel pending service binding.
 * The
 *
 * @param client Client instance.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_has_client_unbind(struct bt_has_client *client);

/**
 * @brief Read Preset Records command.
 *
 * Client method to read up to @p max_count presets starting from given @p index.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 * The preset records are returned in the @ref bt_has_client_cb.preset_read_rsp callback
 * (called once for each preset).
 *
 * @param client Client instance.
 * @param index The index to start with.
 * @param max_count Maximum number of presets to read.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_presets_read(struct bt_has_client *client, uint8_t index, uint8_t max_count);

/**
 * @brief Set Active Preset command.
 *
 * Client procedure to set preset identified by @p index as active.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 *
 * @param client Client instance.
 * @param index Preset index to activate.
 * @param sync Request active preset synchronization in set.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_preset_set(struct bt_has_client *client, uint8_t index, bool sync);

/**
 * @brief Activate Next Preset command.
 *
 * Client procedure to set next available preset as active.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 *
 * @param client Client instance.
 * @param sync Request active preset synchronization in set.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_preset_next(struct bt_has_client *client, bool sync);

/**
 * @brief Activate Previous Preset command.
 *
 * Client procedure to set previous available preset as active.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 *
 * @param client Client instance.
 * @param sync Request active preset synchronization in set.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_preset_prev(struct bt_has_client *client, bool sync);

/**
 * @brief Write Preset Name command.
 *
 * Client procedure to change the preset name.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 *
 * @param client Client instance.
 * @param index Preset index to change the name of.
 * @param name Preset name to write.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_preset_write(struct bt_has_client *client, uint8_t index, const char *name);

/** Hearing Aid device Info Structure */
struct bt_has_client_info {
	/** Local identity */
	uint8_t id;
	/** Peer device address */
	const bt_addr_le_t *addr;
	/** Hearing Aid Type. */
	enum bt_has_hearing_aid_type type;
	/** Hearing Aid Features. */
	enum bt_has_features features;
	/** Hearing Aid Capabilities. */
	enum bt_has_capabilities caps;
	/** Active Preset Index.
	 *
	 *  Only applicable if @ref BT_HAS_PRESET_SUPPORT is set in @ref caps.
	 */
	uint8_t active_index;
};

/**
 * @brief Get information for the remote Hearing Aid device.
 *
 * @param client Client instance.
 * @param[out] info Info object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_has_client_info_get(const struct bt_has_client *client, struct bt_has_client_info *info);

/**
 * @brief Get the connection pointer of a HAS Client instance
 *
 * Get the Bluetooth connection pointer of a HAS Client instance.
 *
 * @param client Client instance.
 * @param[out] conn Connection pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_has_client_conn_get(const struct bt_has_client *client, struct bt_conn **conn);

/** @brief Preset operations structure. */
struct bt_has_preset_ops {
	/**
	 * @brief Preset select callback.
	 *
	 * This callback is called when the client requests to select preset identified by
	 * @p index.
	 *
	 * @param index Preset index requested to activate.
	 * @param sync Whether the server must relay this change to the other member of the
	 *             Binaural Hearing Aid Set.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 * @return -EBUSY if operation cannot be performed at the time.
	 * @return -EINPROGRESS in case where user has to confirm once the requested preset
	 *                      becomes active by calling @ref bt_has_preset_active_set.
	 */
	int (*select)(uint8_t index, bool sync);

	/**
	 * @brief Preset name changed callback
	 *
	 * This callback is called when the name of the preset identified by @p index has changed.
	 *
	 * @param index Preset index that name has been changed.
	 * @param name Preset current name.
	 */
	void (*name_changed)(uint8_t index, const char *name);
};

/** @brief Register structure for preset. */
struct bt_has_preset_register_param {
	/**
	 * @brief Preset index.
	 *
	 * Unique preset identifier. The value shall be other than @ref BT_HAS_PRESET_INDEX_NONE.
	 */
	uint8_t index;

	/**
	 * @brief Preset properties.
	 *
	 * Bitfield of preset properties.
	 */
	enum bt_has_properties properties;

	/**
	 * @brief Preset name.
	 *
	 * Preset name that further can be changed by either server or client if
	 * @kconfig{CONFIG_BT_HAS_PRESET_NAME_DYNAMIC} configuration option has been enabled.
	 * It's length shall be greater than @ref BT_HAS_PRESET_NAME_MIN. If the length exceeds
	 * @ref BT_HAS_PRESET_NAME_MAX, the name will be truncated.
	 */
	const char *name;

	/** Preset operations structure. */
	const struct bt_has_preset_ops *ops;
};

/** @brief Structure for registering features of a Hearing Access Service instance. */
struct bt_has_features_param {
	/** Hearing Aid Type value */
	enum bt_has_hearing_aid_type type;

	/**
	 * @brief Preset Synchronization Support.
	 *
	 * Only applicable if @p type is @ref BT_HAS_HEARING_AID_TYPE_BINAURAL
	 * and @kconfig{CONFIG_BT_HAS_PRESET_COUNT} is non-zero.
	 */
	bool preset_sync_support;

	/**
	 * @brief Independent Presets.
	 *
	 * Only applicable if @p type is @ref BT_HAS_HEARING_AID_TYPE_BINAURAL
	 * and @kconfig{CONFIG_BT_HAS_PRESET_COUNT} is non-zero.
	 */
	bool independent_presets;
};

/**
 * @brief Register the Hearing Access Service instance.
 *
 * @param features     Hearing Access Service register parameters.
 *
 * @return 0 if success, errno on failure.
 */
int bt_has_register(const struct bt_has_features_param *features);

/**
 * @brief Register preset.
 *
 * Register preset. The preset will be a added to the list of exposed preset records.
 * This symbol is linkable if @kconfig{CONFIG_BT_HAS_PRESET_COUNT} is non-zero.
 *
 * @param param Preset registration parameter.
 *
 * @return 0 if success, errno on failure.
 */
int bt_has_preset_register(const struct bt_has_preset_register_param *param);

/**
 * @brief Unregister Preset.
 *
 * Unregister preset. The preset will be removed from the list of preset records.
 *
 * @param index The index of preset that's being requested to unregister.
 *
 * @return 0 if success, errno on failure.
 */
int bt_has_preset_unregister(uint8_t index);

/**
 * @brief Set the preset as available.
 *
 * Set the @ref BT_HAS_PROP_AVAILABLE property bit. This will notify preset availability
 * to peer devices. Only available preset can be selected as active preset.
 *
 * @param index The index of preset that's became available.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_available(uint8_t index);

/**
 * @brief Set the preset as unavailable.
 *
 * Clear the @ref BT_HAS_PROP_AVAILABLE property bit. This will notify preset availability
 * to peer devices. Unavailable preset cannot be selected as active preset.
 *
 * @param index The index of preset that's became unavailable.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_unavailable(uint8_t index);

enum {
	BT_HAS_PRESET_ITER_STOP = 0,
	BT_HAS_PRESET_ITER_CONTINUE,
};

/**
 * @typedef bt_has_preset_func_t
 * @brief Preset iterator callback.
 *
 * @param index The index of preset found.
 * @param properties Preset properties.
 * @param name Preset name.
 * @param user_data Data given.
 *
 * @return BT_HAS_PRESET_ITER_CONTINUE if should continue to the next preset.
 * @return BT_HAS_PRESET_ITER_STOP to stop.
 */
typedef uint8_t (*bt_has_preset_func_t)(uint8_t index, enum bt_has_properties properties,
					const char *name, void *user_data);

/**
 * @brief Preset iterator.
 *
 * Iterate presets. Optionally, match non-zero index if given.
 *
 * @param index Preset index, passing @ref BT_HAS_PRESET_INDEX_NONE skips index matching.
 * @param func Callback function.
 * @param user_data Data to pass to the callback.
 */
void bt_has_preset_foreach(uint8_t index, bt_has_preset_func_t func, void *user_data);

/**
 * @brief Set active preset.
 *
 * Function used to set the preset identified by the @p index as the active preset.
 * The preset index will be notified to peer devices.
 *
 * @param index Preset index.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_active_set(uint8_t index);

/**
 * @brief Get active preset.
 *
 * Function used to get the currently active preset index.
 *
 * @return Active preset index.
 */
uint8_t bt_has_preset_active_get(void);

/**
 * @brief Clear out active preset.
 *
 * Used by server to deactivate currently active preset.
 *
 * @return 0 in case of success or negative value in case of error.
 */
static inline int bt_has_preset_active_clear(void)
{
	return bt_has_preset_active_set(BT_HAS_PRESET_INDEX_NONE);
}

/**
 * @brief Change the Preset Name.
 *
 * Change the name of the preset identified by @p index.
 *
 * @param index The index of the preset to change the name of.
 * @param name Name to write.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_name_change(uint8_t index, const char *name);

/**
 * @brief Change the Hearing Aid Features.
 *
 * Change the hearing aid features.
 *
 * @param features The features to be set.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_features_set(const struct bt_has_features_param *features);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAS_H_ */
