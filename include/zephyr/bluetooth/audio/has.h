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
