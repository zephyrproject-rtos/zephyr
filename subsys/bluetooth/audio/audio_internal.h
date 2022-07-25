/* @file
 * @brief Internal APIs for LE Audio
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>

/** @brief LE Audio Attribute User Data. */
struct bt_audio_attr_user_data {
	/** Attribute read callback */
	ssize_t (*read)(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset);

	/** Attribute write callback */
	ssize_t	(*write)(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags);

	/** Attribute user data */
	void *user_data;
};

/** LE Audio Read Characteristic value helper. */
ssize_t bt_audio_read_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset);

/** LE Audio Write Characteristic value helper. */
ssize_t bt_audio_write_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

#define BT_AUDIO_ATTR_USER_DATA_INIT(_read, _write, _user_data) \
{ \
	.read = _read, \
	.write = _write, \
	.user_data = _user_data, \
}

/** Helper to define LE Audio characteristic. */
#define BT_AUDIO_CHRC(_uuid, _props, _perm, _read, _write, _user_data) \
	BT_GATT_CHARACTERISTIC(_uuid, _props, _perm, bt_audio_read_chrc, bt_audio_write_chrc, \
			       ((struct bt_audio_attr_user_data[]) { \
				BT_AUDIO_ATTR_USER_DATA_INIT(_read, _write, _user_data), \
			       }))

#define BT_AUDIO_CHRC_USER_DATA(_attr) \
	(((struct bt_audio_attr_user_data *)(_attr)->user_data)->user_data)

/** LE Audio Write CCCD value helper. */
ssize_t bt_audio_ccc_cfg_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			       uint16_t value);

/** Helper to define LE Audio CCC descriptor. */
#define BT_AUDIO_CCC(_changed)								\
	BT_GATT_CCC_MANAGED(((struct _bt_gatt_ccc[])					\
		{BT_GATT_CCC_INITIALIZER(_changed, bt_audio_ccc_cfg_write, NULL)}),	\
		(BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT))
