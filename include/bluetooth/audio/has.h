/*
 * Copyright (c) 2021 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HAS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HAS_H_

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

#include <zephyr/types.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_HAS_PRESET_NAME_MIN	1
#define BT_HAS_PRESET_NAME_MAX	40

/** HAS Hearing Aid device type */
enum {
	/** Binaural Hearing Aid. */
	BT_HAS_BINAURAL_HEARING_AID,

	/** Monaural Hearing Aid */
	BT_HAS_MONAURAL_HEARING_AID,

	/** Banded Hearing Aid */
	BT_HAS_BANDED_HEARING_AID,
};

/** HAS preset properties bit field values */
enum {
	/** No properties set. */
	BT_HAS_PROP_NONE = 0,

	/** @brief Preset name write permission.
	 *
	 *  If set, the name of the preset can be written by the client.
	 */
	BT_HAS_PROP_WRITABLE = BIT(0),

	/** @brief Preset availibility.
	 *
	 *  If set, the preset is available for the client.
	 */
	BT_HAS_PROP_AVAILABLE = BIT(1),
};

#define BT_HAS_PROP_SET(PROPS, BIT)     ((PROPS) |= (BIT))
#define BT_HAS_PROP_CLR(PROPS, BIT)   	((PROPS) &= ~(BIT))

/** @brief The Preset Record representation. */
struct bt_has_preset {
	/** Preset index */
	uint8_t index;
	/** Preset properties */
	uint8_t properties;
	/** Preset name */
	uint8_t *name;
};

/** Statically define and register preset record internal helper. */
#define _BT_HAS_PRESET_DEFINE(_index, _properties, _name)		\
	const STRUCT_SECTION_ITERABLE(bt_has_preset, bt_has_preset_##_index) =	\
	{									\
		.index = _index,						\
		.properties = _properties,					\
		.name = _name,							\
	}

/** @def BT_HAS_PRESET_DEFINE
 *  @brief Statically define and register preset record.
 *
 *  Helper macro to statically define and register non-writable preset record.
 *
 *  @param _name Name of the preset.
 */
#define BT_HAS_PRESET_DEFINE(_index, _name, _properties)			\
	const char *preset_name_##_index = _name;				\
	_BT_HAS_PRESET_DEFINE(_index, BT_HAS_PROP_CLR(_properties,		\
						      BT_HAS_PROP_WRITABLE),	\
			      bt_has_preset_name_##_index)

/** @def BT_HAS_PRESET_WRITABLE_DEFINE
 *  @brief Statically define and register writable preset record.
 *
 *  Helper macro to statically define and register writable preset record.
 *
 *  @param _name Name of the preset.
 */
#define BT_HAS_PRESET_WRITABLE_DEFINE(_index, _name, _properties)		\
	static char preset_name_##_index[BT_HAS_PRESET_NAME_MAX + 1] = _name;	\
	_BT_HAS_PRESET_DEFINE(_index, BT_HAS_PROP_SET(_properties,		\
						      BT_HAS_PROP_WRITABLE),	\
			      bt_has_preset_name_##_index)

struct bt_has_cb {
	/** @brief Server accept callback
	 *
	 *  This callback is called whenever a new incoming profile connection
	 *  requires authorization.
	 *
	 *  @param conn The connection that is requesting authorization
	 *
	 *  @return 0 in case of success or -EACCES if application did not
	 *  authorize the connection.
	 */
	int (*accept)(struct bt_conn *conn);

	/**
	 * @brief Preset set active callback
	 *
	 * The callback can also be used locally in which case no connection
	 * will be set. Once the preset has changed, to complete the procedure
	 * @ref the bt_has_active_preset_changed shall be called.
	 *
	 * @param conn   The connection that is requesting to read
	 * @param preset The preset that's being set active
	 * @param sync   Whether the server must relay this change to the other member of
	 *		 the Binaural Hearing Aid Set.
	 */
	void (*set_active)(struct bt_conn *conn,
			   const struct bt_has_preset *preset,
			   bool sync);

	/**
	 * @brief Preset name changed callback
	 *
	 * This callback notifies the application that the preset name has been
	 * changed by the user.
	 *
	 * @param preset Preset record object.
	 */
	void (*preset_name_changed)(const struct bt_has_preset *preset);
};

/**
 * @brief Register server callbacks.
 *
 * Register callbacks to handle the Hearing Access Service requests.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
int bt_has_cb_register(struct bt_has_cb *cb);

/**
 * @brief Make preset record available for a client.
 *
 * @param preset The preset that's being enabled
 */
void bt_has_preset_enable(struct bt_has_preset *preset);

/**
 * @brief Make preset record unavailable for a client.
 *
 * @param preset The preset that's being disabled
 */
void bt_has_preset_disable(struct bt_has_preset *preset);

/**
 * @brief Register HAS preset record
 *
 * @param preset The preset to be registered
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_register(struct bt_has_preset *preset);

/**
 * @brief Unregister HAS preset record
 *
 * @param preset The preset to be unregistered
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_preset_unregister(struct bt_has_preset *preset);

/**
 * @brief Indicate the active preset change.
 *
 * @param preset The preset that's become active
 */
void bt_has_active_preset_changed(const struct bt_has_preset *preset);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HAS_H_ */
