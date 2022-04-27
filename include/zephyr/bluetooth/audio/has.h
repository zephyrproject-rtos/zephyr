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

#include <zephyr/bluetooth/bluetooth.h>
#include <sys/types.h>
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

/** @brief Opaque Hearing Access Service object. */
struct bt_has;

/** Hearing Aid device type */
enum bt_has_hearing_aid_type {
	BT_HAS_HEARING_AID_TYPE_BINAURAL,
	BT_HAS_HEARING_AID_TYPE_MONAURAL,
	BT_HAS_HEARING_AID_TYPE_BANDED,
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

	/**
	 * @brief Callback function for presets read operation.
	 *
	 * The callback is called when the preset read response is sent by the remote server.
	 * The record object as well as its members are temporary and must be copied to in order
	 * to cache its information.
	 *
	 * @param has Pointer to the Hearing Access Service object.
	 * @param err 0 on success, ATT error or negative errno otherwise.
	 * @param record Preset record or NULL on errors.
	 * @param is_last True if Read Presets operation can be considered concluded.
	 */
	void (*preset_read_rsp)(struct bt_has *has, int err,
				const struct bt_has_preset_record *record, bool is_last);

	/**
	 * @brief Callback function for preset update notifications.
	 *
	 * The callback is called when the preset record update is notified by the remote server.
	 * The record object as well as its objects are temporary and must be copied to in order
	 * to cache its information.
	 *
	 * @param has Pointer to the Hearing Access Service object.
	 * @param index_prev Index of the previous preset in the list.
	 * @param record Preset record.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_update)(struct bt_has *has, uint8_t index_prev,
			      const struct bt_has_preset_record *record, bool is_last);

	/**
	 * @brief Callback function for preset deletion notifications.
	 *
	 * The callback is called when the preset has been deleted by the remote server.
	 *
	 * @param has Pointer to the Hearing Access Service object.
	 * @param index Preset index.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_deleted)(struct bt_has *has, uint8_t index, bool is_last);

	/**
	 * @brief Callback function for preset availability notifications.
	 *
	 * The callback is called when the preset availability change is notified by the remote
	 * server.
	 *
	 * @param has Pointer to the Hearing Access Service object.
	 * @param index Preset index.
	 * @param available True if available, false otherwise.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_availability)(struct bt_has *has, uint8_t index, bool available,
				    bool is_last);
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

/**
 * @brief Read Preset Records.
 *
 * Client method to read up to @p max_count presets starting from given @p index.
 * The preset records are returned in the @ref bt_has_client_cb.preset_read_rsp callback
 * (called once for each preset).
 *
 * @param has Pointer to the Hearing Access Service object.
 * @param index The index to start with.
 * @param max_count Maximum number of presets to read.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_presets_read(struct bt_has *has, uint8_t index, uint8_t max_count);

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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAS_H_ */
